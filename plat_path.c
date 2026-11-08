#include "plat_path.h"

#include "_plat_os.h"
#include "plat_assert.h"
#include "plat_log.h"
#include "plat_os.h"

#include <cwalk.h>

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#if PLAT_OS == PLAT_OS_WINDOWS
#include <direct.h>
#include <wchar.h>
#endif

#define PFX "Path"

#define PLAT_PATH_LOG_ERRNO(plat_path)                                                                                 \
    do {                                                                                                               \
        char _err_buf[PLAT_OS_STRERROR_BUF_LEN];                                                                       \
        PLAT_LOG_E(PFX, "%s: %s", plat_path_cstr(plat_path), plat_os_strerror(errno, _err_buf, sizeof(_err_buf)));     \
    } while(0)

#if PLAT_OS == PLAT_OS_WINDOWS

// Plain stat() has a 32 bit st_size on Windows
typedef struct _stat64 plat_path_stat_t;

// UCRT does not provide S_ISDIR(), MinGW does
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif

// Paths are UTF-8, narrow CRT functions would interpret them in the ANSI codepage, so use the wide variants
static int plat_path_os_stat(const char* path, plat_path_stat_t* st) {
    wchar_t* wpath = plat_os_windows_utf8_to_wide(path);
    if(wpath == NULL) return -1;
    int res = _wstat64(wpath, st);
    int err = errno;
    free(wpath);
    errno = err;
    return res;
}
static int plat_path_os_mkdir(const char* path) {
    wchar_t* wpath = plat_os_windows_utf8_to_wide(path);
    if(wpath == NULL) return -1;
    int res = _wmkdir(wpath);
    int err = errno;
    free(wpath);
    errno = err;
    return res;
}
static FILE* plat_path_os_fopen(const char* path, const char* mode) {
    wchar_t* wpath = plat_os_windows_utf8_to_wide(path);
    if(wpath == NULL) return NULL;
    wchar_t* wmode = plat_os_windows_utf8_to_wide(mode);
    if(wmode == NULL) {
        free(wpath);
        return NULL;
    }
    FILE* file = _wfopen(wpath, wmode);
    int err = errno;
    free(wpath);
    free(wmode);
    errno = err;
    return file;
}

#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS

typedef struct stat plat_path_stat_t;

static int plat_path_os_stat(const char* path, plat_path_stat_t* st) {
    return stat(path, st);
}
static int plat_path_os_mkdir(const char* path) {
    return mkdir(path, 0755);
}
static FILE* plat_path_os_fopen(const char* path, const char* mode) {
    return fopen(path, mode);
}

#endif

#define PLAT_PATH_BUF_LEN (4096)

struct plat_path_t {
    m_string_t str;
};

plat_path_t* plat_path_init(const char* init) {
    plat_path_t* plat_path = malloc(sizeof(plat_path_t));
    m_string_init_set(plat_path->str, init);
    plat_path_normalize(plat_path);
    return plat_path;
}

plat_path_t* plat_path_dup(plat_path_t* plat_path) {
    return plat_path_init(plat_path_cstr(plat_path));
}

const char* plat_path_cstr(plat_path_t* plat_path) {
    return m_string_get_cstr(plat_path->str);
}

const char* plat_path_name(plat_path_t* plat_path) {
    const char* basename = NULL;
    size_t basename_len = 0;
    cwk_path_get_basename(plat_path_cstr(plat_path), &basename, &basename_len);
    if(basename == NULL) {
        return "";
    }
    plat_assert(strlen(basename) == basename_len);
    return basename;
}

const char* plat_path_extension(plat_path_t* plat_path) {
    const char* extension = NULL;
    size_t extension_len = 0;
    cwk_path_get_extension(plat_path_cstr(plat_path), &extension, &extension_len);
    if(extension == NULL) {
        return "";
    }
    plat_assert(strlen(extension) == extension_len);
    return extension;
}

void plat_path_set(plat_path_t* plat_path, const char* set) {
    m_string_set(plat_path->str, set);
    plat_path_normalize(plat_path);
}

void plat_path_set_name(plat_path_t* plat_path, const char* name) {
    char path_buf[PLAT_PATH_BUF_LEN];
    plat_assert(cwk_path_change_basename(plat_path_cstr(plat_path), name, path_buf, sizeof(path_buf)) <
                sizeof(path_buf));
    m_string_set(plat_path->str, path_buf);
}

void plat_path_set_extension(plat_path_t* plat_path, const char* extension) {
    char path_buf[PLAT_PATH_BUF_LEN];
    plat_assert(cwk_path_change_extension(plat_path_cstr(plat_path), extension, path_buf, sizeof(path_buf)) <
                sizeof(path_buf));
    m_string_set(plat_path->str, path_buf);
}

void plat_path_join(plat_path_t* plat_path, const char* join) {
    char path_buf[PLAT_PATH_BUF_LEN];
    plat_assert(cwk_path_join(plat_path_cstr(plat_path), join, path_buf, sizeof(path_buf)) < sizeof(path_buf));
    m_string_set(plat_path->str, path_buf);
}

void plat_path_normalize(plat_path_t* plat_path) {
    char path_buf[PLAT_PATH_BUF_LEN];
    plat_assert(cwk_path_normalize(plat_path_cstr(plat_path), path_buf, sizeof(path_buf)) < sizeof(path_buf));
    m_string_set(plat_path->str, path_buf);
}

void plat_path_parent(plat_path_t* plat_path) {
    plat_path_join(plat_path, "..");
}

static bool plat_path_stat(plat_path_t* plat_path, plat_path_stat_t* st, bool log_enoent) {
    int res = plat_path_os_stat(plat_path_cstr(plat_path), st);
    if(res != 0) {
        plat_assert(res == -1);
        if(log_enoent || errno != ENOENT) {
            PLAT_PATH_LOG_ERRNO(plat_path);
        }
        return false;
    }
    return true;
}

bool plat_path_is_dir(plat_path_t* plat_path) {
    plat_path_stat_t st;
    if(!plat_path_stat(plat_path, &st, false)) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool plat_path_is_empty(plat_path_t* plat_path) {
    return m_string_empty_p(plat_path->str);
}

bool plat_path_is_file(plat_path_t* plat_path) {
    plat_path_stat_t st;
    if(!plat_path_stat(plat_path, &st, false)) {
        return false;
    }
    return !S_ISDIR(st.st_mode);
}

bool plat_path_mkdir(plat_path_t* plat_path, bool recursive) {
    if(plat_path_is_dir(plat_path)) {
        return true;
    }
    bool ret = true;

    plat_path_t* parent = plat_path_dup(plat_path);
    plat_path_parent(parent);
    if(!plat_path_is_dir(parent)) {
        if(recursive) {
            ret = plat_path_mkdir(parent, recursive);
        } else {
            ret = false;
        }
    }
    plat_path_free(parent);

    if(ret) {
        int res = plat_path_os_mkdir(plat_path_cstr(plat_path));
        if(res != 0) {
            plat_assert(res == -1);
            PLAT_PATH_LOG_ERRNO(plat_path);
            ret = false;
        }
    }

    return ret;
}

bool plat_path_read_bytes(plat_path_t* plat_path, m_bstring_ptr bytes) {
    plat_path_stat_t st;
    if(!plat_path_stat(plat_path, &st, true)) {
        return false;
    }
    if(S_ISDIR(st.st_mode)) {
        return false;
    }

    FILE* file = plat_path_os_fopen(plat_path_cstr(plat_path), "rb");
    if(file == NULL) {
        PLAT_PATH_LOG_ERRNO(plat_path);
        return false;
    }

    bool read = m_bstring_fread(bytes, file, (size_t)st.st_size);
    fclose(file);

    return read;
}

bool plat_path_read_lines(plat_path_t* plat_path, m_string_list_ptr lines) {
    FILE* file = plat_path_os_fopen(plat_path_cstr(plat_path), "rb");
    if(file == NULL) {
        PLAT_PATH_LOG_ERRNO(plat_path);
        return false;
    }

    m_string_t line;
    m_string_init(line);
    while(m_string_fgets(line, file, M_STRING_READ_PURE_LINE)) {
        m_string_list_push_back(lines, line);
    }
    m_string_clear(line);

    fclose(file);

    return true;
}

bool plat_path_read_text(plat_path_t* plat_path, m_string_ptr text) {
    FILE* file = plat_path_os_fopen(plat_path_cstr(plat_path), "rb");
    if(file == NULL) {
        PLAT_PATH_LOG_ERRNO(plat_path);
        return false;
    }

    bool read = m_string_fgets(text, file, M_STRING_READ_FILE);
    fclose(file);

    return read;
}

bool plat_path_write_bytes(plat_path_t* plat_path, m_bstring_ptr bytes) {
    return plat_path_write_raw(plat_path, m_bstring_view(bytes, 0, m_bstring_size(bytes)), m_bstring_size(bytes));
}

bool plat_path_write_raw(plat_path_t* plat_path, const void* data, size_t size) {
    FILE* file = plat_path_os_fopen(plat_path_cstr(plat_path), "wb");
    if(file == NULL) {
        PLAT_PATH_LOG_ERRNO(plat_path);
        return false;
    }

    size_t wrote = fwrite(data, 1, size, file);
    fclose(file);

    return wrote == size;
}

bool plat_path_write_text(plat_path_t* plat_path, m_string_ptr text) {
    FILE* file = plat_path_os_fopen(plat_path_cstr(plat_path), "wb");
    if(file == NULL) {
        PLAT_PATH_LOG_ERRNO(plat_path);
        return false;
    }

    bool wrote = m_string_fputs(file, text);
    fclose(file);

    return wrote;
}

void plat_path_free(plat_path_t* plat_path) {
    m_string_clear(plat_path->str);
    free(plat_path);
}
