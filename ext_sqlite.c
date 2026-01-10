/**
 * @file ext_sqlite.c
 * @brief SQLite (PDO-like) helpers.
 */
#include "lx_ext.h"
#include "array.h"
#include "value.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    sqlite3 *db;
    int in_use;
} DbHandle;

typedef struct {
    sqlite3_stmt *stmt;
    int db_id;
    int in_use;
    int done;
    int has_row;
    Value row_cache;
} StmtHandle;

static DbHandle *g_dbs = NULL;
static int g_db_count = 0;
static int g_db_cap = 0;

static StmtHandle *g_stmts = NULL;
static int g_stmt_count = 0;
static int g_stmt_cap = 0;

static int db_handle_add(sqlite3 *db) {
    for (int i = 0; i < g_db_count; i++) {
        if (!g_dbs[i].in_use) {
            g_dbs[i].db = db;
            g_dbs[i].in_use = 1;
            return i + 1;
        }
    }
    if (g_db_count >= g_db_cap) {
        int cap = g_db_cap ? g_db_cap * 2 : 8;
        DbHandle *nb = (DbHandle *)realloc(g_dbs, (size_t)cap * sizeof(DbHandle));
        if (!nb) return 0;
        g_dbs = nb;
        g_db_cap = cap;
    }
    g_dbs[g_db_count].db = db;
    g_dbs[g_db_count].in_use = 1;
    g_db_count++;
    return g_db_count;
}

static void db_handle_close(int id) {
    if (id <= 0 || id > g_db_count) return;
    DbHandle *h = &g_dbs[id - 1];
    if (!h->in_use) return;
    sqlite3_close(h->db);
    h->db = NULL;
    h->in_use = 0;
}

static sqlite3 *db_handle_get(int id) {
    if (id <= 0 || id > g_db_count) return NULL;
    if (!g_dbs[id - 1].in_use) return NULL;
    return g_dbs[id - 1].db;
}

static void stmt_clear_cache(StmtHandle *h) {
    if (h->has_row) {
        value_free(h->row_cache);
        h->has_row = 0;
        h->row_cache = value_null();
    }
}

static int stmt_handle_add(sqlite3_stmt *stmt, int db_id) {
    for (int i = 0; i < g_stmt_count; i++) {
        if (!g_stmts[i].in_use) {
            g_stmts[i].stmt = stmt;
            g_stmts[i].db_id = db_id;
            g_stmts[i].in_use = 1;
            g_stmts[i].done = 0;
            g_stmts[i].has_row = 0;
            g_stmts[i].row_cache = value_null();
            return i + 1;
        }
    }
    if (g_stmt_count >= g_stmt_cap) {
        int cap = g_stmt_cap ? g_stmt_cap * 2 : 8;
        StmtHandle *ns = (StmtHandle *)realloc(g_stmts, (size_t)cap * sizeof(StmtHandle));
        if (!ns) return 0;
        g_stmts = ns;
        g_stmt_cap = cap;
    }
    g_stmts[g_stmt_count].stmt = stmt;
    g_stmts[g_stmt_count].db_id = db_id;
    g_stmts[g_stmt_count].in_use = 1;
    g_stmts[g_stmt_count].done = 0;
    g_stmts[g_stmt_count].has_row = 0;
    g_stmts[g_stmt_count].row_cache = value_null();
    g_stmt_count++;
    return g_stmt_count;
}

static StmtHandle *stmt_handle_get(int id) {
    if (id <= 0 || id > g_stmt_count) return NULL;
    if (!g_stmts[id - 1].in_use) return NULL;
    return &g_stmts[id - 1];
}

static void stmt_handle_close(int id) {
    if (id <= 0 || id > g_stmt_count) return;
    StmtHandle *h = &g_stmts[id - 1];
    if (!h->in_use) return;
    stmt_clear_cache(h);
    sqlite3_finalize(h->stmt);
    h->stmt = NULL;
    h->in_use = 0;
    h->done = 1;
}

static Value row_from_stmt(sqlite3_stmt *stmt) {
    int cols = sqlite3_column_count(stmt);
    Value row = value_array();
    for (int i = 0; i < cols; i++) {
        const char *name = sqlite3_column_name(stmt, i);
        Key key = key_string(name ? name : "");
        int t = sqlite3_column_type(stmt, i);
        Value v = value_undefined();
        switch (t) {
            case SQLITE_INTEGER:
                v = value_int((int)sqlite3_column_int64(stmt, i));
                break;
            case SQLITE_FLOAT:
                v = value_float(sqlite3_column_double(stmt, i));
                break;
            case SQLITE_TEXT: {
                const unsigned char *txt = sqlite3_column_text(stmt, i);
                int n = sqlite3_column_bytes(stmt, i);
                v = value_string_n((const char *)txt, (size_t)n);
                break;
            }
            case SQLITE_NULL:
                v = value_null();
                break;
            case SQLITE_BLOB:
            default:
                v = value_undefined();
                break;
        }
        array_set(row.a, key, v);
    }
    return row;
}

static int bind_value(sqlite3_stmt *stmt, int idx, Value v) {
    switch (v.type) {
        case VAL_INT:
            return sqlite3_bind_int64(stmt, idx, (sqlite3_int64)v.i) == SQLITE_OK;
        case VAL_FLOAT:
            return sqlite3_bind_double(stmt, idx, v.f) == SQLITE_OK;
        case VAL_BOOL:
            return sqlite3_bind_int(stmt, idx, v.b ? 1 : 0) == SQLITE_OK;
        case VAL_STRING:
            return sqlite3_bind_text(stmt, idx, v.s ? v.s : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
        case VAL_NULL:
        case VAL_UNDEFINED:
        case VAL_VOID:
            return sqlite3_bind_null(stmt, idx) == SQLITE_OK;
        default:
            return 0;
    }
}

static int bind_params(sqlite3_stmt *stmt, Value params) {
    if (params.type != VAL_ARRAY || !params.a) return 0;
    Array *a = params.a;
    for (int i = 0; i < a->size; i++) {
        Key key = a->entries[i].key;
        Value v = a->entries[i].value;
        int idx = 0;
        if (key.type == KEY_STRING) {
            const char *name = key.s ? key.s : "";
            idx = sqlite3_bind_parameter_index(stmt, name);
            if (idx == 0 && name[0] != ':' && *name) {
                size_t len = strlen(name);
                char *tmp = (char *)malloc(len + 2);
                if (!tmp) return 0;
                tmp[0] = ':';
                memcpy(tmp + 1, name, len + 1);
                idx = sqlite3_bind_parameter_index(stmt, tmp);
                free(tmp);
            }
        } else {
            idx = key.i + 1;
        }
        if (idx <= 0) return 0;
        if (!bind_value(stmt, idx, v)) return 0;
    }
    return 1;
}

static Value n_pdo_sqlite_open(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const char *path = argv[0].s ? argv[0].s : "";
    sqlite3 *db = NULL;
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return value_undefined();
    }
    int id = db_handle_add(db);
    if (id == 0) {
        sqlite3_close(db);
        return value_undefined();
    }
    return value_int(id);
}

static Value n_pdo_query(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_INT || argv[1].type != VAL_STRING) return value_undefined();
    sqlite3 *db = db_handle_get(argv[0].i);
    if (!db) return value_undefined();
    const char *sql = argv[1].s ? argv[1].s : "";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return value_undefined();

    Value out = value_array();
    int row_idx = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            Value row = row_from_stmt(stmt);
            array_set(out.a, key_int(row_idx++), row);
            continue;
        }
        if (rc == SQLITE_DONE) break;
        value_free(out);
        sqlite3_finalize(stmt);
        return value_undefined();
    }
    sqlite3_finalize(stmt);
    return out;
}

static Value n_pdo_prepare(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 2 || argc > 3 || argv[0].type != VAL_INT || argv[1].type != VAL_STRING) {
        return value_undefined();
    }
    sqlite3 *db = db_handle_get(argv[0].i);
    if (!db) return value_undefined();
    const char *sql = argv[1].s ? argv[1].s : "";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return value_undefined();
    int id = stmt_handle_add(stmt, argv[0].i);
    if (id == 0) {
        sqlite3_finalize(stmt);
        return value_undefined();
    }
    return value_int(id);
}

static Value n_pdo_execute(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1 || argc > 2 || argv[0].type != VAL_INT) return value_bool(0);
    StmtHandle *h = stmt_handle_get(argv[0].i);
    if (!h || !h->stmt) return value_bool(0);
    sqlite3_stmt *stmt = h->stmt;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    h->done = 0;
    stmt_clear_cache(h);

    if (argc == 2) {
        if (!bind_params(stmt, argv[1])) return value_bool(0);
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        h->row_cache = row_from_stmt(stmt);
        h->has_row = 1;
        return value_bool(1);
    }
    if (rc == SQLITE_DONE) {
        h->done = 1;
        return value_bool(1);
    }
    return value_bool(0);
}

static Value n_pdo_fetch(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_INT) return value_undefined();
    StmtHandle *h = stmt_handle_get(argv[0].i);
    if (!h || !h->stmt) return value_undefined();
    if (h->has_row) {
        Value row = value_copy(h->row_cache);
        stmt_clear_cache(h);
        return row;
    }
    if (h->done) return value_undefined();
    int rc = sqlite3_step(h->stmt);
    if (rc == SQLITE_ROW) {
        return row_from_stmt(h->stmt);
    }
    if (rc == SQLITE_DONE) {
        h->done = 1;
        return value_undefined();
    }
    return value_undefined();
}

static Value n_pdo_fetch_all(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_INT) return value_array();
    StmtHandle *h = stmt_handle_get(argv[0].i);
    if (!h || !h->stmt) return value_array();
    Value out = value_array();
    int row_idx = 0;
    if (h->has_row) {
        array_set(out.a, key_int(row_idx++), value_copy(h->row_cache));
        stmt_clear_cache(h);
    }
    while (!h->done) {
        int rc = sqlite3_step(h->stmt);
        if (rc == SQLITE_ROW) {
            Value row = row_from_stmt(h->stmt);
            array_set(out.a, key_int(row_idx++), row);
            continue;
        }
        if (rc == SQLITE_DONE) {
            h->done = 1;
            break;
        }
        break;
    }
    return out;
}

static Value n_pdo_last_insert_id(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_INT) return value_int(0);
    sqlite3 *db = db_handle_get(argv[0].i);
    if (!db) return value_int(0);
    return value_int((int)sqlite3_last_insert_rowid(db));
}

static Value n_pdo_close(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_INT) return value_bool(0);
    int id = argv[0].i;
    if (!db_handle_get(id)) return value_bool(0);
    for (int i = 0; i < g_stmt_count; i++) {
        if (g_stmts[i].in_use && g_stmts[i].db_id == id) {
            stmt_handle_close(i + 1);
        }
    }
    db_handle_close(id);
    return value_bool(1);
}

static void sqlite_module_init(Env *global){
    lx_register_function("pdo_sqlite_open", n_pdo_sqlite_open);
    lx_register_function("pdo_query", n_pdo_query);
    lx_register_function("pdo_prepare", n_pdo_prepare);
    lx_register_function("pdo_execute", n_pdo_execute);
    lx_register_function("pdo_fetch", n_pdo_fetch);
    lx_register_function("pdo_fetch_all", n_pdo_fetch_all);
    lx_register_function("pdo_last_insert_id", n_pdo_last_insert_id);
    lx_register_function("pdo_close", n_pdo_close);
    (void)global;
}

void register_sqlite_module(void) {
    lx_register_extension("sqlite");
    lx_register_module(sqlite_module_init);
}
