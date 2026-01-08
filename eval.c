/**
 * @file eval.c
 * @brief AST evaluation implementation.
 */
#include "ast.h"
#include "eval.h"
#include "gc.h"
#include "env.h"
#include "natives.h"
#include "array.h"
#include "lx_error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

typedef struct FunctionDef {
    char *name;
    char **params;
    int param_count;
    AstNode *body;
    struct FunctionDef *next;
} FunctionDef;

static FunctionDef *g_user_fns = NULL;

static int array_contains_inner(Array *hay, Array *needle, Array ***visited, int *count, int *cap) {
    if (!hay || !needle) return 0;
    if (hay == needle) return 1;
    for (int i = 0; i < *count; i++) {
        if ((*visited)[i] == hay) return 0;
    }
    if (*count >= *cap) {
        int ncap = (*cap == 0) ? 8 : (*cap * 2);
        Array **nv = (Array **)realloc(*visited, (size_t)ncap * sizeof(Array *));
        if (!nv) return 0;
        *visited = nv;
        *cap = ncap;
    }
    (*visited)[(*count)++] = hay;
    for (int i = 0; i < hay->size; i++) {
        Value v = hay->entries[i].value;
        if (v.type == VAL_ARRAY && v.a) {
            if (array_contains_inner(v.a, needle, visited, count, cap)) return 1;
        }
    }
    return 0;
}

static int array_contains(Array *hay, Array *needle) {
    Array **visited = NULL;
    int count = 0;
    int cap = 0;
    int found = array_contains_inner(hay, needle, &visited, &count, &cap);
    free(visited);
    return found;
}

static FunctionDef *find_user_fn(const char *name) {
    for (FunctionDef *f=g_user_fns; f; f=f->next)
        if (strcmp(f->name,name)==0) return f;
    return NULL;
}

static void register_user_fn(AstNode *func_node) {
    FunctionDef *f = find_user_fn(func_node->func.name);
    if (!f) {
        f = (FunctionDef*)calloc(1,sizeof(FunctionDef));
        f->next = g_user_fns;
        g_user_fns = f;
        f->name = strdup(func_node->func.name);
    } else {
        /* overwrite params/body pointers (owned by AST lifetime) */
    }
    f->params = func_node->func.params;
    f->param_count = func_node->func.param_count;
    f->body = func_node->func.body;
}

static EvalResult ok(Value v) { EvalResult r; r.flow=FLOW_NORMAL; r.value=v; return r; }
static EvalResult ret(Value v) { EvalResult r; r.flow=FLOW_RETURN; r.value=v; return r; }
static EvalResult brk(void) { EvalResult r; r.flow=FLOW_BREAK; r.value=value_void(); return r; }
static EvalResult cont(void) { EvalResult r; r.flow=FLOW_CONTINUE; r.value=value_void(); return r; }

static Value eval_expr(AstNode *n, Env *env, int *ok_flag);

static void runtime_error(AstNode *n, LxErrorCode code, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char msg[256];
    vsnprintf(msg, sizeof(msg), fmt, ap);
    lx_set_error(code, n ? n->line : 0, n ? n->col : 0, "%s", msg);
    va_end(ap);
}

static Value literal_to_value(Token t) {
    switch (t.type) {
        case TOK_ARRAY: return value_array();
        case TOK_INT:    return value_int(t.int_val);
        case TOK_FLOAT:  return value_float(t.float_val);
        case TOK_STRING: return value_string(t.string_val);
        case TOK_NULL:   return value_null();
        case TOK_UNDEFINED: return value_undefined();
        case TOK_VOID:  return value_void();
        case TOK_TRUE:   return value_bool(1);
        case TOK_FALSE:  return value_bool(0);
        default:         return value_null();
    }
}

static int strict_equal(Value a, Value b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case VAL_UNDEFINED: return 1;
        case VAL_NULL: return 1;
        case VAL_BOOL: return a.b == b.b;
        case VAL_INT: return a.i == b.i;
        case VAL_FLOAT: return a.f == b.f;
        case VAL_STRING: return strcmp(a.s?a.s:"", b.s?b.s:"")==0;
        case VAL_ARRAY: return a.a == b.a; /* identity only in V1 */
        default: return 0;
    }
}

int weak_equal(Value a, Value b) {
    /* number == number */
    if (value_is_number(a) && value_is_number(b)) {
        return value_as_double(a) == value_as_double(b);
    }

    /* number == string */
    if (value_is_number(a) && b.type == VAL_STRING) {
        char *end;
        double v = strtod(b.s, &end);
        if (*end == '\0')
            return value_as_double(a) == v;
        return 0;
    }

    /* string == number */
    if (a.type == VAL_STRING && value_is_number(b)) {
        char *end;
        double v = strtod(a.s, &end);
        if (*end == '\0')
            return v == value_as_double(b);
        return 0;
    }

    /* string == string */
    if (a.type == VAL_STRING && b.type == VAL_STRING) {
        return strcmp(a.s, b.s) == 0;
    }

    /* bool == bool */
    if (a.type == VAL_BOOL && b.type == VAL_BOOL) {
        return a.b == b.b;
    }

    /* null */
    if (a.type == VAL_NULL && b.type == VAL_NULL) {
        return 1;
    }

    return 0;
}

static Value do_concat(Value a, Value b) {
    Value sa = value_to_string(a);
    Value sb = value_to_string(b);
    size_t la = strlen(sa.s), lb = strlen(sb.s);
    Value out = value_string_n("", la+lb);
    memcpy(out.s, sa.s, la);
    memcpy(out.s + la, sb.s, lb);
    out.s[la+lb] = 0;
    value_free(sa); value_free(sb);
    return out;
}

static Value apply_assign_op(AstNode *n, Operator op, Value lhs, Value rhs) {
    if (op == OP_CONCAT) {
        Value out = do_concat(lhs, rhs);
        value_free(lhs);
        value_free(rhs);
        return out;
    }

    int lhs_isf = (lhs.type == VAL_FLOAT);
    int rhs_isf = (rhs.type == VAL_FLOAT);
    if (lhs.type == VAL_STRING) { Value t = value_to_float(lhs); value_free(lhs); lhs = t; lhs_isf = 1; }
    if (rhs.type == VAL_STRING) { Value t = value_to_float(rhs); value_free(rhs); rhs = t; rhs_isf = 1; }

    if (lhs_isf || rhs_isf) {
        Value lf = value_to_float(lhs);
        Value rf = value_to_float(rhs);
        double res = 0.0;
        if (op == OP_ADD) res = lf.f + rf.f;
        if (op == OP_SUB) res = lf.f - rf.f;
        if (op == OP_MUL) res = lf.f * rf.f;
        if (op == OP_DIV) {
            if (rf.f == 0.0) {
                value_free(lhs);
                value_free(rhs);
                value_free(lf);
                value_free(rf);
                runtime_error(n, LX_ERR_DIV_ZERO, "division by zero");
                return value_null();
            }
            res = lf.f / rf.f;
        }
        Value out = value_float(res);
        value_free(lhs);
        value_free(rhs);
        value_free(lf);
        value_free(rf);
        return out;
    } else {
        Value li = value_to_int(lhs);
        Value ri = value_to_int(rhs);
        int res = 0;
        if (op == OP_ADD) res = li.i + ri.i;
        if (op == OP_SUB) res = li.i - ri.i;
        if (op == OP_MUL) res = li.i * ri.i;
        if (op == OP_DIV) {
            if (ri.i == 0) {
                value_free(lhs);
                value_free(rhs);
                value_free(li);
                value_free(ri);
                runtime_error(n, LX_ERR_DIV_ZERO, "division by zero");
                return value_null();
            }
            res = li.i / ri.i;
        }
        Value out = value_int(res);
        value_free(lhs);
        value_free(rhs);
        value_free(li);
        value_free(ri);
        return out;
    }
}

static Value incdec_value(Value cur, int delta) {
    if (cur.type == VAL_UNDEFINED || cur.type == VAL_NULL || cur.type == VAL_VOID) {
        cur = value_int(0);
    }
    if (cur.type == VAL_FLOAT) {
        Value f = value_to_float(cur);
        Value out = value_float(f.f + (double)delta);
        value_free(f);
        value_free(cur);
        return out;
    }
    Value i = value_to_int(cur);
    Value out = value_int(i.i + delta);
    value_free(i);
    value_free(cur);
    return out;
}

static Value *get_lvalue_ref(AstNode *target, Env *env, int *ok_flag, Value *out_base) {
    if (target->type == AST_VAR) {
        return env_get_ref(env, target->var.name);
    }
    if (target->type != AST_INDEX) {
        *ok_flag = 0;
        return NULL;
    }

    AstNode *cur = target;
    AstNode **indices = NULL;
    int index_count = 0;
    while (cur && cur->type == AST_INDEX) {
        indices = realloc(indices, sizeof(AstNode *) * (index_count + 1));
        indices[index_count++] = cur->index.index;
        cur = cur->index.target;
    }
    if (!cur || cur->type != AST_VAR) {
        *ok_flag = 0;
        free(indices);
        return NULL;
    }

    const char *varname = cur->var.name;
    Value arrv = env_get(env, varname);
    if (arrv.type == VAL_UNDEFINED || arrv.type == VAL_NULL) {
        arrv = value_array();
        env_set(env, varname, value_copy(arrv));
    }
    if (arrv.type != VAL_ARRAY) {
        *ok_flag = 0;
        value_free(arrv);
        free(indices);
        return NULL;
    }

    Array *current = arrv.a;
    for (int i = index_count - 1; i > 0; i--) {
        Value idx = eval_expr(indices[i], env, ok_flag);
        if (!*ok_flag) { value_free(arrv); free(indices); return NULL; }

        Value *slot = NULL;
        if (idx.type == VAL_STRING) {
            slot = array_get_ref(current, key_string(idx.s));
        } else {
            Value ii = value_to_int(idx);
            slot = array_get_ref(current, key_int(ii.i));
            value_free(ii);
        }
        value_free(idx);

        if (slot->type == VAL_UNDEFINED || slot->type == VAL_NULL) {
            Value nv = value_array();
            value_free(*slot);
            *slot = nv;
        }
        if (slot->type != VAL_ARRAY) {
            *ok_flag = 0;
            value_free(arrv);
            free(indices);
            return NULL;
        }
        current = slot->a;
    }

    Value last_idx = eval_expr(indices[0], env, ok_flag);
    if (!*ok_flag) { value_free(arrv); free(indices); return NULL; }

    Value *slot = NULL;
    if (last_idx.type == VAL_STRING) {
        slot = array_get_ref(current, key_string(last_idx.s));
    } else {
        Value ii = value_to_int(last_idx);
        slot = array_get_ref(current, key_int(ii.i));
        value_free(ii);
    }
    value_free(last_idx);

    *out_base = arrv;
    free(indices);
    return slot;
}

static Value string_index(Value s, int idx) {
    if (s.type != VAL_STRING || !s.s)
        return value_undefined();

    int len = (int)strlen(s.s);
    if (idx < 0 || idx >= len)
        return value_undefined();

    return value_string_n(&s.s[idx], 1);
}

static Value eval_index(Value target, Value index, Env *env, int *ok_flag) {
    (void)env;
    if (target.type == VAL_ARRAY) {
        if (index.type == VAL_STRING) {
            Value v = array_get(target.a, key_string(index.s));
            return v;
        } else {
            Value ii = value_to_int(index);
            Value v = array_get(target.a, key_int(ii.i));
            return v;
        }
    }
    if (target.type == VAL_STRING) {
        Value ii = value_to_int(index);
        Value v = string_index(target, ii.i);
        return v;
    }
    *ok_flag = 1;
    return value_undefined();
}

static Value eval_binary(AstNode *n, Operator op, AstNode *l, AstNode *r, Env *env, int *ok_flag) {
    /* short-circuit for && || handled here by evaluating left first */
    if (op == OP_AND) {
        Value lv = eval_expr(l, env, ok_flag);
        if (!*ok_flag) return value_null();
        if (!value_is_true(lv)) {
            value_free(lv);
            return value_bool(0);
        }
        value_free(lv);
        Value rv = eval_expr(r, env, ok_flag);
        if (!*ok_flag) return value_null();
        int b = value_is_true(rv);
        value_free(rv);
        return value_bool(b);
    }
    if (op == OP_OR) {
        Value lv = eval_expr(l, env, ok_flag);
        if (!*ok_flag) return value_null();
        if (value_is_true(lv)) {
            value_free(lv);
            return value_bool(1);
        }
        value_free(lv);
        Value rv = eval_expr(r, env, ok_flag);
        if (!*ok_flag) return value_null();
        int b = value_is_true(rv);
        value_free(rv);
        return value_bool(b);
    }

    Value a = eval_expr(l, env, ok_flag);
    if (!*ok_flag) return value_null();
    Value b = eval_expr(r, env, ok_flag);
    if (!*ok_flag) { value_free(a); return value_null(); }

    Value out = value_null();

    switch (op) {
        case OP_CONCAT:
            out = do_concat(a,b);
            break;

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD: {
            /* numeric promotion */
            int a_isf = (a.type==VAL_FLOAT), b_isf = (b.type==VAL_FLOAT);
            if (a.type==VAL_STRING) { Value t=value_to_float(a); value_free(a); a=t; a_isf=1; }
            if (b.type==VAL_STRING) { Value t=value_to_float(b); value_free(b); b=t; b_isf=1; }
            if (a_isf || b_isf) {
                Value af = value_to_float(a);
                Value bf = value_to_float(b);
                double res = 0.0;
                if (op==OP_ADD) res = af.f + bf.f;
                if (op==OP_SUB) res = af.f - bf.f;
                if (op==OP_MUL) res = af.f * bf.f;
                if (op==OP_DIV) {
                    if (bf.f == 0.0) {
                        runtime_error(n, LX_ERR_DIV_ZERO, "division by zero");
                        *ok_flag = 0;
                        value_free(a);
                        value_free(b);
                        value_free(af);
                        value_free(bf);
                        return value_null();
                    }
                    res = af.f / bf.f;
                }
                if (op==OP_MOD) {
                    if (bf.f == 0.0) {
                        runtime_error(n, LX_ERR_MOD_ZERO, "modulo by zero");
                        *ok_flag = 0;
                        value_free(a);
                        value_free(b);
                        value_free(af);
                        value_free(bf);
                        return value_null();
                    }
                    res = fmod(af.f, bf.f);
                }
                out = value_float(res);
            } else {
                Value ai = value_to_int(a);
                Value bi = value_to_int(b);
                int res = 0;
                if (op==OP_ADD) res = ai.i + bi.i;
                if (op==OP_SUB) res = ai.i - bi.i;
                if (op==OP_MUL) res = ai.i * bi.i;
                if (op==OP_DIV) {
                    if (bi.i == 0) {
                        runtime_error(n, LX_ERR_DIV_ZERO, "division by zero");
                        *ok_flag = 0;
                        value_free(a);
                        value_free(b);
                        value_free(ai);
                        value_free(bi);
                        return value_null();
                    }
                    res = ai.i / bi.i;
                }
                if (op==OP_MOD) {
                    if (bi.i == 0) {
                        runtime_error(n, LX_ERR_MOD_ZERO, "modulo by zero");
                        *ok_flag = 0;
                        value_free(a);
                        value_free(b);
                        value_free(ai);
                        value_free(bi);
                        return value_null();
                    }
                    res = ai.i % bi.i;
                }
                out = value_int(res);
            }
            break;
        }

        case OP_POW: {
            Value af = value_to_float(a);
            Value bf = value_to_float(b);
            out = value_float(pow(af.f, bf.f));
            break;
        }

        case OP_EQ:   out = value_bool(weak_equal(a,b)); break;
        case OP_NEQ:  out = value_bool(!weak_equal(a,b)); break;
        case OP_SEQ:  out = value_bool(strict_equal(a,b)); break;
        case OP_SNEQ: out = value_bool(!strict_equal(a,b)); break;

        case OP_LT:
        case OP_LTE:
        case OP_GT:
        case OP_GTE: {
            /* numeric if possible, else string compare */
            if ((a.type==VAL_INT||a.type==VAL_FLOAT||a.type==VAL_BOOL) &&
                (b.type==VAL_INT||b.type==VAL_FLOAT||b.type==VAL_BOOL)) {
                Value af = value_to_float(a);
                Value bf = value_to_float(b);
                int res = 0;
                if (op==OP_LT)  res = af.f <  bf.f;
                if (op==OP_LTE) res = af.f <= bf.f;
                if (op==OP_GT)  res = af.f >  bf.f;
                if (op==OP_GTE) res = af.f >= bf.f;
                out = value_bool(res);
            } else {
                Value sa = value_to_string(a);
                Value sb = value_to_string(b);
                int cmp = strcmp(sa.s, sb.s);
                int res = 0;
                if (op==OP_LT)  res = (cmp < 0);
                if (op==OP_LTE) res = (cmp <= 0);
                if (op==OP_GT)  res = (cmp > 0);
                if (op==OP_GTE) res = (cmp >= 0);
                out = value_bool(res);
                value_free(sa); value_free(sb);
            }
            break;
        }

        case OP_BIT_AND:
        case OP_BIT_OR:
        case OP_BIT_XOR:
        case OP_SHL:
        case OP_SHR: {
            Value ai = value_to_int(a);
            Value bi = value_to_int(b);
            int res = 0;
            if (op==OP_BIT_AND) res = ai.i & bi.i;
            if (op==OP_BIT_OR)  res = ai.i | bi.i;
            if (op==OP_BIT_XOR) res = ai.i ^ bi.i;
            if (op==OP_SHL)     res = ai.i << bi.i;
            if (op==OP_SHR)     res = ai.i >> bi.i;
            out = value_int(res);
            break;
        }

        default:
            *ok_flag = 0;
            out = value_null();
            break;
    }

    value_free(a);
    value_free(b);
    return out;
}

static Value eval_unary(Operator op, AstNode *e, Env *env, int *ok_flag) {
    Value v = eval_expr(e, env, ok_flag);
    if (!*ok_flag) return value_null();
    Value out = value_null();
    switch (op) {
        case OP_NOT:
            out = value_bool(!value_is_true(v));
            break;
        case OP_SUB: { /* unary minus reusing OP_SUB in some parsers; if you have OP_NEG, use that */
            if (v.type == VAL_INT) {
                out = value_int(-v.i);
                break;
            }
            if (v.type == VAL_BOOL) {
                out = value_int(-(v.b ? 1 : 0));
                break;
            }
            if (v.type == VAL_FLOAT) {
                out = value_float(-v.f);
                break;
            }
            Value f = value_to_float(v);
            out = value_float(-f.f);
            break;
        }
        case OP_BIT_NOT: {
            Value i = value_to_int(v);
            out = value_int(~i.i);
            break;
        }
        default:
            *ok_flag = 0;
            out = value_null();
            break;
    }
    value_free(v);
    return out;
}

static Value eval_call(AstNode *n, Env *env, int *ok_flag) {
    /* evaluate args */
    int argc = n->call.argc;
    Value *argv = NULL;
    if (argc > 0) {
        argv = (Value*)calloc((size_t)argc, sizeof(Value));
        if (!argv) { *ok_flag=0; return value_null(); }
    }
    for (int i=0;i<argc;i++) {
        argv[i] = eval_expr(n->call.args[i], env, ok_flag);
        if (!*ok_flag) {
            for (int j=0;j<=i;j++) value_free(argv[j]);
            free(argv);
            return value_null();
        }
    }

    /* native first */
    NativeFn nf = find_function(n->call.name);
    if (nf) {
        Value r = nf(env, argc, argv);
        for (int i=0;i<argc;i++) value_free(argv[i]);
        free(argv);
        return r;
    }

    /* user function */
    FunctionDef *uf = find_user_fn(n->call.name);
    if (!uf) {
        for (int i=0;i<argc;i++) value_free(argv[i]);
        free(argv);
        runtime_error(n, LX_ERR_UNDEFINED_FUNCTION, "undefined function '%s'", n->call.name);
        *ok_flag = 0;
        return value_null();
    }

    Env *local = env_new(env); /* lexical chain: local -> caller */
    for (int i=0;i<uf->param_count;i++) {
        Value v = (i < argc) ? value_copy(argv[i]) : value_null();
        env_set(local, uf->params[i], v);
    }

    for (int i=0;i<argc;i++) value_free(argv[i]);
    free(argv);

    EvalResult rr = eval_node(uf->body, local);
    env_free(local);

    if (rr.flow == FLOW_RETURN) return rr.value;
    if (rr.flow == FLOW_BREAK || rr.flow == FLOW_CONTINUE) {
        runtime_error(n, LX_ERR_BREAK_CONTINUE, "break/continue outside loop");
        *ok_flag = 0;
        value_free(rr.value);
        return value_null();
    }
    value_free(rr.value);
    return value_void();
}

static Value eval_expr(AstNode *n, Env *env, int *ok_flag) {
    if (lx_has_error()) {
        *ok_flag = 0;
        return value_null();
    }
    switch (n->type) {
        case AST_LITERAL:
            return literal_to_value(n->literal.token);

        case AST_ARRAY_LITERAL: {
            Value arrv = value_array();
            int next_index = 0;
            for (int i = 0; i < n->array.count; i++) {
                Value val = eval_expr(n->array.values[i], env, ok_flag);
                if (!*ok_flag) { value_free(arrv); return value_null(); }

                if (n->array.keys[i]) {
                    Value keyv = eval_expr(n->array.keys[i], env, ok_flag);
                    if (!*ok_flag) { value_free(val); value_free(arrv); return value_null(); }
                    if (keyv.type == VAL_STRING) {
                        array_set(arrv.a, key_string(keyv.s), val);
                    } else {
                        Value ki = value_to_int(keyv);
                        array_set(arrv.a, key_int(ki.i), val);
                        if (ki.i >= next_index) next_index = ki.i + 1;
                        value_free(ki);
                    }
                    value_free(keyv);
                } else {
                    array_set(arrv.a, key_int(next_index++), val);
                }
            }
            return arrv;
        }

        case AST_VAR: {
            Value v = env_get(env, n->var.name);
            return v; /* may be VAL_UNDEFINED */
        }

        case AST_ASSIGN: {
            Value rhs = eval_expr(n->assign.value, env, ok_flag);
            if (!*ok_flag) return value_null();
            if (n->assign.is_compound) {
                Value lhs = env_get(env, n->assign.name);
                if (lhs.type == VAL_UNDEFINED || lhs.type == VAL_NULL) {
                    if (n->assign.op == OP_CONCAT) {
                        lhs = value_string("");
                    } else {
                        lhs = value_int(0);
                    }
                }
                Value out = apply_assign_op(n, n->assign.op, lhs, value_copy(rhs));
                env_set(env, n->assign.name, value_copy(out));
                value_free(rhs);
                return out;
            }
            /* auto-create array not here; assignment just sets */
            env_set(env, n->assign.name, value_copy(rhs));
            return rhs; /* return assigned value */
        }

        case AST_UNARY:
            return eval_unary(n->unary.op, n->unary.expr, env, ok_flag);

        case AST_BINARY:
            return eval_binary(n, n->binary.op, n->binary.left, n->binary.right, env, ok_flag);

        case AST_CALL:
            return eval_call(n, env, ok_flag);

        case AST_INDEX: {
            Value tgt = eval_expr(n->index.target, env, ok_flag);
            if (!*ok_flag) return value_null();
            Value idx = eval_expr(n->index.index, env, ok_flag);
            if (!*ok_flag) { value_free(tgt); return value_null(); }
            Value out = eval_index(tgt, idx, env, ok_flag);
            value_free(tgt);
            value_free(idx);
            return out;
        }

        case AST_PRE_INC:
        case AST_PRE_DEC:
        case AST_POST_INC:
        case AST_POST_DEC: {
            int ok2 = 1;
            Value base = value_null();
            Value *slot = get_lvalue_ref(n->incdec.target, env, &ok2, &base);
            if (!ok2 || !slot) {
                if (ok_flag) *ok_flag = 0;
                value_free(base);
                return value_null();
            }

            Value oldv = value_copy(*slot);
            int delta = (n->type == AST_PRE_DEC || n->type == AST_POST_DEC) ? -1 : 1;
            Value newv = incdec_value(value_copy(*slot), delta);
            value_free(*slot);
            *slot = newv;

            if (n->type == AST_PRE_INC || n->type == AST_PRE_DEC) {
                value_free(oldv);
                value_free(base);
                return value_copy(newv);
            }
            value_free(newv);
            value_free(base);
            return oldv;
        }

        case AST_TERNARY: {
            Value c = eval_expr(n->ternary.cond, env, ok_flag);
            if (!*ok_flag) return value_null();
            int t = value_is_true(c);
            value_free(c);
            if (t) {
                return eval_expr(n->ternary.then_expr, env, ok_flag);
            }
            return eval_expr(n->ternary.else_expr, env, ok_flag);
        }

        default:
            *ok_flag = 0;
            return value_null();
    }
}

static EvalResult eval_block_like(AstNode *n, Env *env) {
    for (int i=0;i<n->block.count;i++) {
        EvalResult r = eval_node(n->block.items[i], env);
        if (lx_has_error()) {
            value_free(r.value);
            return ok(value_null());
        }
        if (r.flow != FLOW_NORMAL) return r;
        value_free(r.value);
        gc_maybe_collect(env);
    }
    return ok(value_null());
}

EvalResult eval_node(AstNode *n, Env *env) {
    if (lx_has_error()) return ok(value_null());
    int ok_flag = 1;

    switch (n->type) {
        case AST_PROGRAM:
        case AST_BLOCK:
            return eval_block_like(n, env);

        case AST_EXPR_STMT: {
            Value v = eval_expr(n->expr_stmt.expr, env, &ok_flag);
            if (!ok_flag) {
                value_free(v);
                return ok(value_null());
            }
            return ok(v);
        }

        case AST_INDEX_ASSIGN: {
            int ok2 = 1;

            AstNode *ix = n->index_assign.target; /* AST_INDEX */
            AstNode *cur = ix;
            AstNode **indices = NULL;
            int index_count = 0;

            while (cur && cur->type == AST_INDEX) {
                indices = realloc(indices, sizeof(AstNode *) * (index_count + 1));
                indices[index_count++] = cur->index.index;
                cur = cur->index.target;
            }

            if (!cur || cur->type != AST_VAR) {
                free(indices);
                runtime_error(n, LX_ERR_INDEX_ASSIGN, "index assignment only supports $var[index]");
                return ok(value_null());
            }
            const char *varname = cur->var.name;

            Value arrv = env_get(env, varname);
            if (arrv.type == VAL_UNDEFINED || arrv.type == VAL_NULL) {
                arrv = value_array();
                env_set(env, varname, value_copy(arrv));
            }

            if (arrv.type != VAL_ARRAY) {
                value_free(arrv);
                free(indices);
                runtime_error(n, LX_ERR_INDEX_ASSIGN, "index assignment on non-array");
                return ok(value_null());
            }

            Array *current = arrv.a;

            for (int i = index_count - 1; i > 0; i--) {
                Value idx = eval_expr(indices[i], env, &ok2);
                if (!ok2) { value_free(arrv); free(indices); return ok(value_null()); }

                Value *slot = NULL;
                if (idx.type == VAL_STRING) {
                    slot = array_get_ref(current, key_string(idx.s));
                } else {
                    Value ii = value_to_int(idx);
                    slot = array_get_ref(current, key_int(ii.i));
                    value_free(ii);
                }
                value_free(idx);

                if (slot->type == VAL_UNDEFINED || slot->type == VAL_NULL) {
                    Value nv = value_array();
                    value_free(*slot);
                    *slot = nv;
                }

                if (slot->type != VAL_ARRAY) {
                    value_free(arrv);
                    free(indices);
                    runtime_error(n, LX_ERR_INDEX_ASSIGN, "index assignment on non-array");
                    return ok(value_null());
                }

                current = slot->a;
            }

            Value last_idx = eval_expr(indices[0], env, &ok2);
            if (!ok2) { value_free(arrv); free(indices); return ok(value_null()); }

            Value val = eval_expr(n->index_assign.value, env, &ok2);
            if (!ok2) { value_free(arrv); value_free(last_idx); free(indices); return ok(value_null()); }

            if (val.type == VAL_ARRAY && val.a) {
                if (array_contains(val.a, current)) {
                    value_free(val);
                    value_free(last_idx);
                    value_free(arrv);
                    free(indices);
                    runtime_error(n, LX_ERR_CYCLE, "cyclic array reference");
                    return ok(value_null());
                }
            }

            Value *slot = NULL;
            if (last_idx.type == VAL_STRING) {
                slot = array_get_ref(current, key_string(last_idx.s));
            } else {
                Value ii = value_to_int(last_idx);
                slot = array_get_ref(current, key_int(ii.i));
                value_free(ii);
            }

            if (n->index_assign.is_compound) {
                Value lhs = value_copy(*slot);
                if (lhs.type == VAL_UNDEFINED || lhs.type == VAL_NULL) {
                    if (n->index_assign.op == OP_CONCAT) {
                        lhs = value_string("");
                    } else {
                        lhs = value_int(0);
                    }
                }
                Value out = apply_assign_op(n, n->index_assign.op, lhs, value_copy(val));
                value_free(*slot);
                *slot = out;
            } else {
                value_free(*slot);
                *slot = value_copy(val);
            }

            value_free(val);
            value_free(last_idx);
            value_free(arrv);
            free(indices);
            return ok(value_null());
        }

        case AST_BREAK:
            return brk();

        case AST_CONTINUE:
            return cont();

        case AST_IF: {
            Value c = eval_expr(n->if_stmt.cond, env, &ok_flag);
            if (!ok_flag) { value_free(c); return ok(value_null()); }
            int t = value_is_true(c);
            value_free(c);
            if (t) return eval_node(n->if_stmt.then_branch, env);
            if (n->if_stmt.else_branch) return eval_node(n->if_stmt.else_branch, env);
            return ok(value_null());
        }

        case AST_WHILE: {
            for (;;) {
                Value c = eval_expr(n->while_stmt.cond, env, &ok_flag);
                if (!ok_flag) { value_free(c); return ok(value_null()); }
                int t = value_is_true(c);
                value_free(c);
                if (!t) break;
                EvalResult r = eval_node(n->while_stmt.body, env);
                if (r.flow == FLOW_RETURN) return r;
                if (r.flow == FLOW_BREAK) { value_free(r.value); break; }
                if (r.flow == FLOW_CONTINUE) { value_free(r.value); continue; }
                value_free(r.value);
            }
            return ok(value_null());
        }

        case AST_DO_WHILE: {
            for (;;) {
                EvalResult r = eval_node(n->do_while_stmt.body, env);
                if (r.flow == FLOW_RETURN) return r;
                if (r.flow == FLOW_BREAK) { value_free(r.value); break; }
                if (r.flow == FLOW_CONTINUE) { value_free(r.value); }
                else { value_free(r.value); }

                Value c = eval_expr(n->do_while_stmt.cond, env, &ok_flag);
                if (!ok_flag) { value_free(c); return ok(value_null()); }
                int t = value_is_true(c);
                value_free(c);
                if (!t) break;
            }
            return ok(value_null());
        }

        case AST_FOR: {
            if (n->for_stmt.init) {
                EvalResult r0 = eval_node(n->for_stmt.init, env);
                if (r0.flow == FLOW_RETURN) return r0;
                if (r0.flow == FLOW_BREAK || r0.flow == FLOW_CONTINUE) {
                    value_free(r0.value);
                    return ok(value_null());
                }
                value_free(r0.value);
            }
            for (;;) {
                if (n->for_stmt.cond) {
                    Value c = eval_expr(n->for_stmt.cond, env, &ok_flag);
                    if (!ok_flag) { value_free(c); return ok(value_null()); }
                    int t = value_is_true(c);
                    value_free(c);
                    if (!t) break;
                }
                EvalResult rb = eval_node(n->for_stmt.body, env);
                if (rb.flow == FLOW_RETURN) return rb;
                if (rb.flow == FLOW_BREAK) { value_free(rb.value); break; }
                if (rb.flow == FLOW_CONTINUE) {
                    value_free(rb.value);
                    if (n->for_stmt.step) {
                        EvalResult rs = eval_node(n->for_stmt.step, env);
                        if (rs.flow == FLOW_RETURN) return rs;
                        if (rs.flow == FLOW_BREAK || rs.flow == FLOW_CONTINUE) {
                            value_free(rs.value);
                            return ok(value_null());
                        }
                        value_free(rs.value);
                    }
                    continue;
                }
                value_free(rb.value);

                if (n->for_stmt.step) {
                    EvalResult rs = eval_node(n->for_stmt.step, env);
                    if (rs.flow == FLOW_RETURN) return rs;
                    if (rs.flow == FLOW_BREAK || rs.flow == FLOW_CONTINUE) {
                        value_free(rs.value);
                        return ok(value_null());
                    }
                    value_free(rs.value);
                }
            }
            return ok(value_null());
        }

        case AST_FOREACH: {
            Value it = eval_expr(n->foreach_stmt.iterable, env, &ok_flag);
            if (!ok_flag) { value_free(it); return ok(value_null()); }

            if (it.type == VAL_ARRAY && it.a) {
                for (int i = 0; i < it.a->size; i++) {
                    ArrayEntry *e = &it.a->entries[i];
                    if (n->foreach_stmt.key_name) {
                        Value kv = (e->key.type == KEY_STRING)
                            ? value_string(e->key.s)
                            : value_int(e->key.i);
                        env_set(env, n->foreach_stmt.key_name, kv);
                    }
                    Value vv = value_copy(e->value);
                    env_set(env, n->foreach_stmt.value_name, vv);

                    EvalResult r = eval_node(n->foreach_stmt.body, env);
                    if (r.flow == FLOW_RETURN) { value_free(it); return r; }
                    if (r.flow == FLOW_BREAK) { value_free(r.value); break; }
                    if (r.flow == FLOW_CONTINUE) { value_free(r.value); continue; }
                    value_free(r.value);
                }
            } else if (it.type == VAL_STRING && it.s) {
                int len = (int)strlen(it.s);
                for (int i = 0; i < len; i++) {
                    if (n->foreach_stmt.key_name) {
                        env_set(env, n->foreach_stmt.key_name, value_int(i));
                    }
                    Value vv = value_string_n(&it.s[i], 1);
                    env_set(env, n->foreach_stmt.value_name, vv);

                    EvalResult r = eval_node(n->foreach_stmt.body, env);
                    if (r.flow == FLOW_RETURN) { value_free(it); return r; }
                    if (r.flow == FLOW_BREAK) { value_free(r.value); break; }
                    if (r.flow == FLOW_CONTINUE) { value_free(r.value); continue; }
                    value_free(r.value);
                }
            }

            value_free(it);
            return ok(value_null());
        }

        case AST_SWITCH: {
            Value sv = eval_expr(n->switch_stmt.expr, env, &ok_flag);
            if (!ok_flag) { value_free(sv); return ok(value_null()); }

            int start = -1;
            int default_idx = -1;
            for (int i = 0; i < n->switch_stmt.case_count; i++) {
                AstNode *ce = n->switch_stmt.case_exprs[i];
                if (!ce) { default_idx = i; continue; }
                Value cv = eval_expr(ce, env, &ok_flag);
                if (!ok_flag) { value_free(sv); value_free(cv); return ok(value_null()); }
                int eq = weak_equal(sv, cv);
                value_free(cv);
                if (eq) { start = i; break; }
            }

            if (start < 0) start = default_idx;
            if (start < 0) { value_free(sv); return ok(value_null()); }

            for (int i = start; i < n->switch_stmt.case_count; i++) {
                AstNode *body = n->switch_stmt.case_bodies[i];
                EvalResult r = eval_node(body, env);
                if (r.flow == FLOW_RETURN) { value_free(sv); return r; }
                if (r.flow == FLOW_BREAK) { value_free(r.value); break; }
                if (r.flow == FLOW_CONTINUE) { value_free(sv); return r; }
                value_free(r.value);
            }

            value_free(sv);
            return ok(value_null());
        }

        case AST_FUNCTION:
            register_user_fn(n);
            return ok(value_null());

        case AST_RETURN: {
            if (!n->ret.value) {
                return ret(value_void());
            }
            Value v = eval_expr(n->ret.value, env, &ok_flag);
            if (!ok_flag) { value_free(v); return ret(value_null()); }
            return ret(v);
        }

        case AST_UNSET: {
            int ok_flag = 1;

            AstNode *t = n->unset.target;

            /* Unset a variable binding. */
            if (t->type == AST_VAR) {
                env_unset(env, t->var.name);
                return ok(value_null());
            }

            /* Unset an array index. */
            if (t->type == AST_INDEX) {
                AstNode *base = t->index.target;

                if (base->type != AST_VAR) {
                    runtime_error(n, LX_ERR_UNSET_TARGET, "unset(index) only supports unset($var[index])");
                    return ok(value_null());
                }
                const char *varname = base->var.name;

                Value arrv = env_get(env, varname);
                if (arrv.type != VAL_ARRAY) {
                    /* Undefined/null: no-op, PHP-style. */
                    value_free(arrv);
                    return ok(value_null());
                }

                Value idx = eval_expr(t->index.index, env, &ok_flag);
                if (!ok_flag) { value_free(arrv); return ok(value_null()); }

                if (idx.type == VAL_STRING) {
                    array_unset(arrv.a, key_string(idx.s));
                } else {
                    Value ii = value_to_int(idx);
                    array_unset(arrv.a, key_int(ii.i));
                    value_free(ii);
                }

                /* Write back to the environment. */
                env_set(env, varname, value_copy(arrv));

                value_free(arrv);
                value_free(idx);
                return ok(value_null());
            }

            return ok(value_null());
        }

        default: {
            /* treat as expression */
            Value v = eval_expr(n, env, &ok_flag);
            if (!ok_flag) { value_free(v); return ok(value_null()); }
            return ok(v);
        }
    }
}

EvalResult eval_program(AstNode *program, Env *env) {
    return eval_node(program, env);
}
