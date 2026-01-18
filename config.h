/* Build-time extension switches. Set to 1 to enable, 0 to disable. */
#if defined(LX_TARGET_LXSH) && LX_TARGET_LXSH
#define LX_ENABLE_FS 0
#define LX_ENABLE_JSON 1
#define LX_ENABLE_SERIALIZER 1
#define LX_ENABLE_HEX 1
#define LX_ENABLE_BLAKE2B 0
#define LX_ENABLE_TIME 1
#define LX_ENABLE_ENV 0
#define LX_ENABLE_UTF8 1
#define LX_ENABLE_SQLITE 0
#define LX_ENABLE_AEAD 0
#define LX_ENABLE_ED25519 0
#define LX_ENABLE_EXEC 0
#define LX_ENABLE_CLI 0
#else
#define LX_ENABLE_FS 1
#define LX_ENABLE_JSON 1
#define LX_ENABLE_SERIALIZER 1
#define LX_ENABLE_HEX 1
#define LX_ENABLE_BLAKE2B 1
#define LX_ENABLE_TIME 1
#define LX_ENABLE_ENV 1
#define LX_ENABLE_UTF8 1
#define LX_ENABLE_SQLITE 1
#define LX_ENABLE_AEAD 1
#define LX_ENABLE_ED25519 1
#define LX_ENABLE_EXEC 1
#define LX_ENABLE_CLI 1
#endif

/* CGI upload settings (lx_cgi only). */
#define FILE_UPLOADS 1
#define UPLOAD_TMP_DIR "/tmp"
#define MAX_FILE_UPLOADS 20
#define UPLOAD_MAX_FILESIZE (2 * 1024 * 1024)
#define POST_MAX_SIZE (8 * 1024 * 1024)

/* CGI error display (lx_cgi only). */
#define LX_CGI_DISPLAY_ERRORS 1

/* CGI session settings (lx_cgi only). */
#define SESSION_NAME "LXSESSID"
#define SESSION_FILE_PATH "/tmp"
#define SESSION_FILE_PERMISSIONS 0600
#define SESSION_TTL 3600
#define SESSION_GC_PROB 1
#define SESSION_GC_DIV 100

/* Timezone default (ext_time). Empty string keeps the system default. */
#define LX_DEFAULT_TIMEZONE ""

/* 32 = int (assuming 32-bit int), 64 = long long */
#define LX_INT_BITS 64

#if defined(LX_TARGET_LXSH) && LX_TARGET_LXSH
#if defined(LXSH_ENABLE_INCLUDE) && LXSH_ENABLE_INCLUDE
#define LX_ENABLE_INCLUDE 1
#else
#define LX_ENABLE_INCLUDE 0
#endif
#else
#define LX_ENABLE_INCLUDE 1
#endif

#ifndef LX_ARENA_SIZE
#define LX_ARENA_SIZE (64 * 1024)
#endif
