#pragma once

#include "plat_crash.h" // IWYU pragma: keep
#include "plat_defs.h"

#ifndef NDEBUG
#define M_ASSERT(expr)                         \
    do {                                       \
        if(!(expr)) {                          \
            plat_crash("M_ASSERT(%s)", #expr); \
        }                                      \
    } while(0)
#endif

// IWYU pragma: begin_exports
#include <mlib/m-array.h>
#include <mlib/m-bstring.h>
#include <mlib/m-buffer.h>
#include <mlib/m-list.h>
#include <mlib/m-string.h>
#include <mlib/m-tuple.h>
// IWYU pragma: end_exports

// String list
M_LIST_DUALF_DEF(m_string_list, m_string_t)
#define M_OPL_m_string_list_t() M_LIST_OPLIST(m_string_list, M_OPL_m_string_t())

// Byte string polyfills
typedef struct m_bstring_s* m_bstring_ptr;
#define M_BSTRING_FAILURE M_STRING_FAILURE
static inline const void* m_bstring_get_cstr(const m_bstring_t v) {
    const void* str = m_bstring_view(v, 0, m_bstring_size(v));
    if(!str) str = "";
    return str;
}
static inline void m_bstring_left(m_bstring_t v, size_t index) {
    m_bstring_resize(v, index);
}
static inline void m_bstring_right(m_bstring_t v, size_t index) {
    uint8_t buffer[128];
    while(index) {
        size_t pop = MIN(index, sizeof(buffer));
        m_bstring_pop_front_bytes(pop, buffer, v);
        index -= pop;
    }
}
static inline size_t m_bstring_search_cstr(
    const m_bstring_t v,
    const void* buf,
    size_t len,
    size_t start) {
    size_t size = m_bstring_size(v);
    if(len > size) return M_BSTRING_FAILURE;
    if(start + len > size) return M_BSTRING_FAILURE;
    const uint8_t* p = (const uint8_t*)m_bstring_get_cstr(v);
    for(size_t pos = start; pos <= size - len; pos++) {
        if(memcmp(p + pos, buf, len) == 0) return pos;
    }
    return M_BSTRING_FAILURE;
}
static inline size_t m_bstring_search(const m_bstring_t v1, const m_bstring_t v2, size_t start) {
    return m_bstring_search_cstr(v1, m_bstring_get_cstr(v2), m_bstring_size(v2), start);
}

// Based on M_EACH() but container and container_t are swapped to resemble variable declaration.
#define each(item_t, item, container_t, container) \
    eachi_oplist(item_t, item, container, M_GLOBAL_OPLIST(container_t))

// clang-format off
/* Internal for each */
#define eachi_oplist(item_t, item, container, oplist)             \
  M_IF_METHOD(IT_REF, oplist)(eachi, eachi_const)                 \
  (item_t, item, container, oplist, M_C(item, _it),               \
   M_C(local_cont_, __LINE__))

/* Internal for each with M_GET_IT_REF operator */
#define eachi(item_t,item,container,oplist, iterator, cont)       \
  (bool cont = true; cont; cont = false)                          \
  for(item_t item; cont ; cont = false)                           \
    for(M_GET_IT_TYPE oplist iterator; cont ; cont = false)       \
      for(M_GET_IT_FIRST oplist (iterator, container) ;           \
          !M_GET_IT_END_P oplist (iterator)                       \
            && (item = M_GET_IT_REF oplist (iterator), true) ;    \
          !M_GET_IT_END_P oplist (iterator)                       \
            && (M_GET_IT_NEXT oplist (iterator), true))

/* Internal for each with M_GET_IT_CREF operator */
#define eachi_const(item_t,item,container,oplist, iterator, cont) \
  (bool cont = true; cont; cont = false)                          \
  for(item_t item; cont ; cont = false)                           \
    for(M_GET_IT_TYPE oplist iterator; cont ; cont = false)       \
      for(M_GET_IT_FIRST oplist (iterator, container) ;           \
          !M_GET_IT_END_P oplist (iterator)                       \
            && (item = M_GET_IT_CREF oplist (iterator), true) ;   \
          !M_GET_IT_END_P oplist (iterator)                       \
            && (M_GET_IT_NEXT oplist (iterator), true))
// clang-format on

/*
 * Based on: https://github.com/P-p-H-d/mlib/blob/5c57ad1f5185446842210934d022266f926a8aeb/example/ex-dict05.c
 * Provide OOR methods for OA Hashmap.
 * See documentation for definition of OOR
 * OOR values are represented as:
 * empty (0) is INT_MIN
 * deleted (1) is INT_MIN + 1
 * So valid range for integers are [INT_MIN+2, INT_MAX]
 */
#define M_int8_t_OOR_EQUAL(n, oor)   (n == INT8_MIN + oor)
#define M_int8_t_OOR_SET(oor)        (INT8_MIN + oor)
#define M_int16_t_OOR_EQUAL(n, oor)  (n == INT16_MIN + oor)
#define M_int16_t_OOR_SET(oor)       (INT16_MIN + oor)
#define M_int32_t_OOR_EQUAL(n, oor)  (n == INT32_MIN + oor)
#define M_int32_t_OOR_SET(oor)       (INT32_MIN + oor)
#define M_int64_t_OOR_EQUAL(n, oor)  (n == INT64_MIN + oor)
#define M_int64_t_OOR_SET(oor)       (INT64_MIN + oor)
#define M_uint8_t_OOR_EQUAL(n, oor)  (n == UINT8_MAX - oor)
#define M_uint8_t_OOR_SET(oor)       (UINT8_MAX - oor)
#define M_uint16_t_OOR_EQUAL(n, oor) (n == UINT16_MAX - oor)
#define M_uint16_t_OOR_SET(oor)      (UINT16_MAX - oor)
#define M_uint32_t_OOR_EQUAL(n, oor) (n == UINT32_MAX - oor)
#define M_uint32_t_OOR_SET(oor)      (UINT32_MAX - oor)
#define M_uint64_t_OOR_EQUAL(n, oor) (n == UINT64_MAX - oor)
#define M_uint64_t_OOR_SET(oor)      (UINT64_MAX - oor)
#define M_INT_OOR_EQUAL(int_t)       M_##int_t##_OOR_EQUAL
#define M_INT_OOR_SET(int_t)         M_##int_t##_OOR_SET
#define M_INT_HASH(n)                ((size_t)n) // Identity hash!
#define M_INT_OPLIST(int_t)                       \
    M_OPEXTEND(                                   \
        M_BASIC_OPLIST,                           \
        OOR_EQUAL(API_0(M_INT_OOR_EQUAL(int_t))), \
        OOR_SET(API_4(M_INT_OOR_SET(int_t))),     \
        HASH(M_INT_HASH))

#define M_OPL_int8_t()   M_INT_OPLIST(int8_t)
#define M_OPL_int16_t()  M_INT_OPLIST(int16_t)
#define M_OPL_int32_t()  M_INT_OPLIST(int32_t)
#define M_OPL_int64_t()  M_INT_OPLIST(int64_t)
#define M_OPL_uint8_t()  M_INT_OPLIST(uint8_t)
#define M_OPL_uint16_t() M_INT_OPLIST(uint16_t)
#define M_OPL_uint32_t() M_INT_OPLIST(uint32_t)
#define M_OPL_uint64_t() M_INT_OPLIST(uint64_t)
#define M_OPL_f32()      M_BASIC_OPLIST
#define M_OPL_f64()      M_BASIC_OPLIST
#define M_OPL_f128()     M_BASIC_OPLIST
