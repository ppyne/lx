/**
 * @file ext_time.c
 * @brief Time extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include "config.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void buf_append(char **buf, size_t *cap, size_t *len, const char *s, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 64 : *cap;
        while (ncap < *len + n + 1) ncap *= 2;
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return;
        *buf = nb;
        *cap = ncap;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
}

static void buf_append_int(char **buf, size_t *cap, size_t *len, int value, int width) {
    char tmp[32];
    if (width > 0) snprintf(tmp, sizeof(tmp), "%0*d", width, value);
    else snprintf(tmp, sizeof(tmp), "%d", value);
    buf_append(buf, cap, len, tmp, strlen(tmp));
}

static int is_leap_year(int year) {
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    return (year % 4 == 0);
}

static int days_in_month(int year, int month) {
    static const int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) return days[1] + (is_leap_year(year) ? 1 : 0);
    return days[month - 1];
}

static int iso_weeks_in_year(int year, int jan1_wday) {
    int jan1_iso = (jan1_wday == 0) ? 7 : jan1_wday;
    if (jan1_iso == 4 || (jan1_iso == 3 && is_leap_year(year))) return 53;
    return 52;
}

static int iso_week_number(const struct tm *tmv) {
    int year = tmv->tm_year + 1900;
    int doy = tmv->tm_yday + 1;
    int wday = (tmv->tm_wday == 0) ? 7 : tmv->tm_wday;
    int jan1_wday = (tmv->tm_wday - (tmv->tm_yday % 7) + 7) % 7;
    int week = (doy + 10 - wday) / 7;
    if (week < 1) {
        int prev_jan1 = (jan1_wday - (is_leap_year(year - 1) ? 2 : 1) + 7) % 7;
        return iso_weeks_in_year(year - 1, prev_jan1);
    }
    if (week > iso_weeks_in_year(year, jan1_wday)) return 1;
    return week;
}

static const char *ordinal_suffix(int day) {
    int mod100 = day % 100;
    if (mod100 >= 11 && mod100 <= 13) return "th";
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

static int tz_offset_seconds(time_t ts) {
    struct tm gm;
    struct tm *gm_ptr = gmtime_r(&ts, &gm);
    if (!gm_ptr) return 0;
    time_t gm_as_local = mktime(&gm);
    return (int)difftime(ts, gm_as_local);
}

static void format_tz_offset(char *buf, size_t size, int offset_sec, int with_colon) {
    int sign = (offset_sec >= 0) ? 1 : -1;
    int abs_sec = offset_sec * sign;
    int hours = abs_sec / 3600;
    int mins = (abs_sec % 3600) / 60;
    if (with_colon) {
        snprintf(buf, size, "%c%02d:%02d", (sign >= 0) ? '+' : '-', hours, mins);
    } else {
        snprintf(buf, size, "%c%02d%02d", (sign >= 0) ? '+' : '-', hours, mins);
    }
}

static Value format_date(const char *fmt, const struct tm *tmv, time_t ts, int utc) {
    static const char *months[] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };
    static const char *months_short[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    static const char *days[] = {
        "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
    };
    static const char *days_short[] = {
        "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
    };

    char *out = NULL;
    size_t cap = 0;
    size_t len = 0;
    for (size_t i = 0; fmt[i]; i++) {
        char c = fmt[i];
        if (c == '\\' && fmt[i + 1]) {
            buf_append(&out, &cap, &len, &fmt[i + 1], 1);
            i++;
            continue;
        }
        switch (c) {
            case 'Y': buf_append_int(&out, &cap, &len, tmv->tm_year + 1900, 4); break;
            case 'y': buf_append_int(&out, &cap, &len, (tmv->tm_year + 1900) % 100, 2); break;
            case 'm': buf_append_int(&out, &cap, &len, tmv->tm_mon + 1, 2); break;
            case 'n': buf_append_int(&out, &cap, &len, tmv->tm_mon + 1, 0); break;
            case 'd': buf_append_int(&out, &cap, &len, tmv->tm_mday, 2); break;
            case 'j': buf_append_int(&out, &cap, &len, tmv->tm_mday, 0); break;
            case 'S':
                buf_append(&out, &cap, &len, ordinal_suffix(tmv->tm_mday), 2);
                break;
            case 'H': buf_append_int(&out, &cap, &len, tmv->tm_hour, 2); break;
            case 'h': {
                int hour = tmv->tm_hour % 12;
                if (hour == 0) hour = 12;
                buf_append_int(&out, &cap, &len, hour, 2);
                break;
            }
            case 'G': buf_append_int(&out, &cap, &len, tmv->tm_hour, 0); break;
            case 'i': buf_append_int(&out, &cap, &len, tmv->tm_min, 2); break;
            case 's': buf_append_int(&out, &cap, &len, tmv->tm_sec, 2); break;
            case 'a':
                buf_append(&out, &cap, &len, (tmv->tm_hour < 12) ? "am" : "pm", 2);
                break;
            case 'A':
                buf_append(&out, &cap, &len, (tmv->tm_hour < 12) ? "AM" : "PM", 2);
                break;
            case 'M':
                buf_append(&out, &cap, &len, months_short[tmv->tm_mon], strlen(months_short[tmv->tm_mon]));
                break;
            case 'F':
                buf_append(&out, &cap, &len, months[tmv->tm_mon], strlen(months[tmv->tm_mon]));
                break;
            case 'D':
                buf_append(&out, &cap, &len, days_short[tmv->tm_wday], strlen(days_short[tmv->tm_wday]));
                break;
            case 'l':
                buf_append(&out, &cap, &len, days[tmv->tm_wday], strlen(days[tmv->tm_wday]));
                break;
            case 'w':
                buf_append_int(&out, &cap, &len, tmv->tm_wday, 0);
                break;
            case 'z':
                buf_append_int(&out, &cap, &len, tmv->tm_yday, 0);
                break;
            case 'W': {
                int week = iso_week_number(tmv);
                buf_append_int(&out, &cap, &len, week, 2);
                break;
            }
            case 'L':
                buf_append_int(&out, &cap, &len, is_leap_year(tmv->tm_year + 1900), 0);
                break;
            case 'c': {
                char tz[8];
                int offset = utc ? 0 : tz_offset_seconds(ts);
                format_tz_offset(tz, sizeof(tz), offset, 1);
                char tmp[32];
                snprintf(
                    tmp,
                    sizeof(tmp),
                    "%04d-%02d-%02dT%02d:%02d:%02d",
                    tmv->tm_year + 1900,
                    tmv->tm_mon + 1,
                    tmv->tm_mday,
                    tmv->tm_hour,
                    tmv->tm_min,
                    tmv->tm_sec
                );
                buf_append(&out, &cap, &len, tmp, strlen(tmp));
                buf_append(&out, &cap, &len, tz, strlen(tz));
                break;
            }
            case 'r': {
                char tz[8];
                int offset = utc ? 0 : tz_offset_seconds(ts);
                format_tz_offset(tz, sizeof(tz), offset, 0);
                char tmp[64];
                snprintf(
                    tmp,
                    sizeof(tmp),
                    "%s, %02d %s %04d %02d:%02d:%02d",
                    days_short[tmv->tm_wday],
                    tmv->tm_mday,
                    months_short[tmv->tm_mon],
                    tmv->tm_year + 1900,
                    tmv->tm_hour,
                    tmv->tm_min,
                    tmv->tm_sec
                );
                buf_append(&out, &cap, &len, tmp, strlen(tmp));
                buf_append(&out, &cap, &len, " ", 1);
                buf_append(&out, &cap, &len, tz, strlen(tz));
                break;
            }
            case 'e': {
                const char *tz = utc ? "UTC" : getenv("TZ");
                if (!tz || !*tz) {
                    tz = tzname[tmv->tm_isdst > 0];
                }
                buf_append(&out, &cap, &len, tz ? tz : "", tz ? strlen(tz) : 0);
                break;
            }
            case 'T': {
                const char *abbr = utc ? "UTC" : tzname[tmv->tm_isdst > 0];
                buf_append(&out, &cap, &len, abbr ? abbr : "", abbr ? strlen(abbr) : 0);
                break;
            }
            case 'I':
                buf_append_int(&out, &cap, &len, tmv->tm_isdst > 0, 0);
                break;
            case 'Z': {
                int offset = utc ? 0 : tz_offset_seconds(ts);
                buf_append_int(&out, &cap, &len, offset, 0);
                break;
            }
            case 'O': {
                char tz[8];
                int offset = utc ? 0 : tz_offset_seconds(ts);
                format_tz_offset(tz, sizeof(tz), offset, 0);
                buf_append(&out, &cap, &len, tz, strlen(tz));
                break;
            }
            case 'P': {
                char tz[8];
                int offset = utc ? 0 : tz_offset_seconds(ts);
                format_tz_offset(tz, sizeof(tz), offset, 1);
                buf_append(&out, &cap, &len, tz, strlen(tz));
                break;
            }
            case 'p': {
                char tz[8];
                int offset = utc ? 0 : tz_offset_seconds(ts);
                if (offset == 0) {
                    buf_append(&out, &cap, &len, "Z", 1);
                } else {
                    format_tz_offset(tz, sizeof(tz), offset, 1);
                    buf_append(&out, &cap, &len, tz, strlen(tz));
                }
                break;
            }
            case 't': {
                int year = tmv->tm_year + 1900;
                int month = tmv->tm_mon + 1;
                buf_append_int(&out, &cap, &len, days_in_month(year, month), 0);
                break;
            }
            case 'U': {
                char tmp[32];
                snprintf(tmp, sizeof(tmp), "%ld", (long)ts);
                buf_append(&out, &cap, &len, tmp, strlen(tmp));
                break;
            }
            default:
                buf_append(&out, &cap, &len, &c, 1);
                break;
        }
    }
    Value v = value_string(out ? out : "");
    free(out);
    return v;
}

static Value n_time(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_int(0);
    return value_int((int)time(NULL));
}

static Value n_date_like(int argc, Value *argv, int utc){
    if (argc < 1 || argv[0].type != VAL_STRING) return value_string("");
    time_t ts = (argc >= 2) ? (time_t)value_to_int(argv[1]).i : time(NULL);
    struct tm tmbuf;
    struct tm *tmv = utc ? gmtime_r(&ts, &tmbuf) : localtime_r(&ts, &tmbuf);
    if (!tmv) return value_string("");
    return format_date(argv[0].s ? argv[0].s : "", tmv, ts, utc);
}

static Value n_date(Env *env, int argc, Value *argv){
    (void)env;
    return n_date_like(argc, argv, 0);
}

static Value n_gmdate(Env *env, int argc, Value *argv){
    (void)env;
    return n_date_like(argc, argv, 1);
}

static Value n_tz_set(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_bool(0);
    Value zv = value_to_string(argv[0]);
    const char *tz = zv.s ? zv.s : "";
    if (*tz) {
        setenv("TZ", tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    value_free(zv);
    return value_bool(1);
}

static Value n_tz_get(Env *env, int argc, Value *argv){
    (void)env;
    (void)argc;
    (void)argv;
    const char *tz = getenv("TZ");
    return value_string(tz ? tz : "");
}

static Value n_date_tz(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 2) return value_string("");
    if (argv[0].type != VAL_STRING) return value_string("");
    time_t ts = time(NULL);
    const char *tz = NULL;
    if (argc == 2) {
        if (argv[1].type != VAL_STRING) return value_string("");
        tz = argv[1].s ? argv[1].s : "";
    } else {
        ts = (time_t)value_to_int(argv[1]).i;
        if (argv[2].type != VAL_STRING) return value_string("");
        tz = argv[2].s ? argv[2].s : "";
    }
    char *old = NULL;
    const char *cur = getenv("TZ");
    if (cur) old = strdup(cur);
    if (tz && *tz) setenv("TZ", tz, 1);
    else unsetenv("TZ");
    tzset();

    struct tm tmbuf;
    struct tm *tmv = localtime_r(&ts, &tmbuf);
    Value out = tmv ? format_date(argv[0].s ? argv[0].s : "", tmv, ts, 0)
                    : value_string("");

    if (old) {
        setenv("TZ", old, 1);
        free(old);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return out;
}

static Value n_tz_list(Env *env, int argc, Value *argv){
    (void)env;
    (void)argc;
    (void)argv;
    Value out = value_array();
    const char *paths[] = {
        "/usr/share/zoneinfo/zone.tab",
        "/usr/share/zoneinfo/zone1970.tab"
    };
    FILE *f = NULL;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        f = fopen(paths[i], "r");
        if (f) break;
    }
    if (!f) return out;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') continue;
        char *p = line;
        char *cols[3] = {0};
        int c = 0;
        while (*p && c < 3) {
            char *tab = strchr(p, '\t');
            if (!tab) tab = strchr(p, ' ');
            if (tab) {
                *tab = '\0';
                cols[c++] = p;
                p = tab + 1;
                while (*p == '\t' || *p == ' ') p++;
            } else {
                cols[c++] = p;
                break;
            }
        }
        if (c >= 3 && cols[2]) {
            char *name = cols[2];
            char *nl = strchr(name, '\n');
            if (nl) *nl = '\0';
            if (*name) {
                lx_int_t idx = array_next_index(out.a);
                array_set(out.a, key_int(idx), value_string(name));
            }
        }
    }
    fclose(f);
    return out;
}

static Value n_mktime(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 6) return value_int(0);
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_hour = (int)value_to_int(argv[0]).i;
    t.tm_min = (int)value_to_int(argv[1]).i;
    t.tm_sec = (int)value_to_int(argv[2]).i;
    t.tm_mon = (int)value_to_int(argv[3]).i - 1;
    t.tm_mday = (int)value_to_int(argv[4]).i;
    t.tm_year = (int)value_to_int(argv[5]).i - 1900;
    time_t ts = mktime(&t);
    return value_int((int)ts);
}

static Value n_sleep(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    int sec = (int)value_to_int(argv[0]).i;
    if (sec < 0) sec = 0;
    unsigned int rem = sleep((unsigned int)sec);
    return value_int((int)rem);
}

static Value n_usleep(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_void();
    int usec = (int)value_to_int(argv[0]).i;
    if (usec < 0) usec = 0;
    usleep((useconds_t)usec);
    return value_void();
}

static void time_module_init(Env *global){
    if (LX_DEFAULT_TIMEZONE[0] != '\0' && (!getenv("TZ") || !*getenv("TZ"))) {
        setenv("TZ", LX_DEFAULT_TIMEZONE, 1);
        tzset();
    }
    lx_register_function("time", n_time);
    lx_register_function("date", n_date);
    lx_register_function("gmdate", n_gmdate);
    lx_register_function("date_tz", n_date_tz);
    lx_register_function("tz_set", n_tz_set);
    lx_register_function("tz_get", n_tz_get);
    lx_register_function("tz_list", n_tz_list);
    lx_register_function("mktime", n_mktime);
    lx_register_function("sleep", n_sleep);
    lx_register_function("usleep", n_usleep);
    (void)global;
}

void register_time_module(void) {
    lx_register_extension("time");
    lx_register_module(time_module_init);
}
