/* Build-time extension switches. Set to 1 to enable, 0 to disable. */
#define LX_ENABLE_FS 1
#define LX_ENABLE_JSON 1
#define LX_ENABLE_SERIALIZER 1
#define LX_ENABLE_HEX 1
#define LX_ENABLE_BLAKE2B 1
#define LX_ENABLE_TIME 1
#define LX_ENABLE_ENV 1
#define LX_ENABLE_UTF8 1
#define LX_ENABLE_SQLITE 1
#define LX_ENABLE_INCLUDE 1

/* CGI upload settings (lx_cgi only). */
#define FILE_UPLOADS 1
#define UPLOAD_TMP_DIR "/tmp"
#define MAX_FILE_UPLOADS 20
#define UPLOAD_MAX_FILESIZE (2 * 1024 * 1024)
#define POST_MAX_SIZE (8 * 1024 * 1024)

/* 32 = int (assuming 32-bit int), 64 = long long */
#define LX_INT_BITS 64
