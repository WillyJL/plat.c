#pragma once

#include "plat_mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plat_path_t plat_path_t;

plat_path_t* plat_path_init(const char* init);

plat_path_t* plat_path_dup(plat_path_t* path);

const char* plat_path_cstr(plat_path_t* path);
const char* plat_path_name(plat_path_t* path);
const char* plat_path_extension(plat_path_t* path);

void plat_path_set(plat_path_t* path, const char* set);
void plat_path_set_name(plat_path_t* path, const char* name);
void plat_path_set_extension(plat_path_t* path, const char* extension);

void plat_path_join(plat_path_t* path, const char* join);
void plat_path_normalize(plat_path_t* path);
void plat_path_parent(plat_path_t* path);

bool plat_path_is_dir(plat_path_t* path);
bool plat_path_is_empty(plat_path_t* path);
bool plat_path_is_file(plat_path_t* path);

bool plat_path_mkdir(plat_path_t* path, bool recursive);
bool plat_path_read_bytes(plat_path_t* path, m_bstring_ptr bytes);
bool plat_path_read_lines(plat_path_t* path, m_string_list_ptr lines);
bool plat_path_read_text(plat_path_t* path, m_string_ptr text);
bool plat_path_write_bytes(plat_path_t* path, m_bstring_ptr bytes);
bool plat_path_write_raw(plat_path_t* path, const void* data, size_t size);
bool plat_path_write_text(plat_path_t* path, m_string_ptr text);

void plat_path_free(plat_path_t* path);

#ifdef __cplusplus
}
#endif
