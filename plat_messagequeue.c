#include "plat_messagequeue.h"

#include "plat_assert.h"
#include "plat_mlib.h"

#if __has_include("plat_conf.h")
#include "plat_conf.h"
#endif

#ifndef PLAT_MESSAGEQUEUE_FOR_EACH_SIZE
#define PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)                                                                             \
    _(1)                                                                                                               \
    _(2)                                                                                                               \
    _(4)                                                                                                               \
    _(8)                                                                                                               \
    _(16)                                                                                                              \
    _(32)                                                                                                              \
    _(64)                                                                                                              \
    _(128)
#endif

typedef enum : uint8_t {
#define _(size) M_MESSAGEQUEUE_SIZE_##size = size,
    PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
    _M_MESSAGEQUEUE_SIZE_MAX,
} m_messagequeue_size_t;

#define _(size)                                                                                                        \
    typedef struct {                                                                                                   \
        uint8_t data[M_MESSAGEQUEUE_SIZE_##size];                                                                      \
    } m_messagequeue_msg_##size##_t;                                                                                   \
    M_BUFFER_DEF(m_messagequeue_##size, m_messagequeue_msg_##size##_t, 0, M_BUFFER_QUEUE, M_POD_OPLIST)
PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _

struct plat_messagequeue_t {
    m_messagequeue_size_t m_messagequeue_size;
    union {
#define _(size) m_messagequeue_##size##_t m_messagequeue_##size;
        PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
    };
    size_t item_size;
};

plat_messagequeue_t* plat_messagequeue_alloc(size_t item_count, size_t item_size) {
    plat_assert(item_count);
    plat_assert(item_size);
    plat_assert(item_size < _M_MESSAGEQUEUE_SIZE_MAX);

    plat_messagequeue_t* plat_messagequeue = malloc(sizeof(plat_messagequeue_t));
    plat_messagequeue->item_size = item_size;

#define _(size)                                                                                                        \
    if(item_size <= size) {                                                                                            \
        plat_messagequeue->m_messagequeue_size = size;                                                                 \
    }
    PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _

    switch(plat_messagequeue->m_messagequeue_size) {
#define _(size)                                                                                                        \
    case M_MESSAGEQUEUE_SIZE_##size: {                                                                                 \
        m_messagequeue_##size##_init(plat_messagequeue->m_messagequeue_##size, item_count);                            \
        break;                                                                                                         \
    }
        PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
    default:
        plat_crash("Update this code, would you kindly?");
    }

    return plat_messagequeue;
}

void plat_messagequeue_free(plat_messagequeue_t* plat_messagequeue) {
    plat_assert(plat_messagequeue);

    switch(plat_messagequeue->m_messagequeue_size) {
#define _(size)                                                                                                        \
    case M_MESSAGEQUEUE_SIZE_##size: {                                                                                 \
        m_messagequeue_##size##_clear(plat_messagequeue->m_messagequeue_##size);                                       \
        break;                                                                                                         \
    }
        PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
    default:
        plat_crash("Update this code, would you kindly?");
    }

    free(plat_messagequeue);
}

bool plat_messagequeue_send(plat_messagequeue_t* plat_messagequeue, const void* item, plat_timeout_t timeout) {
    plat_assert(plat_messagequeue);
    plat_assert(item);

    if(timeout == PLAT_TIMEOUT_FOREVER || timeout == PLAT_TIMEOUT_NONE) {
        switch(plat_messagequeue->m_messagequeue_size) {
#define _(size)                                                                                                        \
    case M_MESSAGEQUEUE_SIZE_##size: {                                                                                 \
        m_messagequeue_msg_##size##_t msg = {};                                                                        \
        memcpy(&msg, item, plat_messagequeue->item_size);                                                              \
        bool send = m_messagequeue_##size##_push_blocking(                                                             \
            plat_messagequeue->m_messagequeue_##size, msg, timeout == PLAT_TIMEOUT_FOREVER);                           \
        if(timeout == PLAT_TIMEOUT_FOREVER) plat_assert(send);                                                         \
        return send;                                                                                                   \
    }
            PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
        default:
            plat_crash("Update this code, would you kindly?");
        }
    } else {
        plat_crash("%s(timeout=any) not implemented", __FUNC__);
    }
    plat_crash("huh");
}

bool plat_messagequeue_recv(plat_messagequeue_t* plat_messagequeue, void* item, plat_timeout_t timeout) {
    plat_assert(plat_messagequeue);
    plat_assert(item);

    if(timeout == PLAT_TIMEOUT_FOREVER || timeout == PLAT_TIMEOUT_NONE) {
        switch(plat_messagequeue->m_messagequeue_size) {
#define _(size)                                                                                                        \
    case M_MESSAGEQUEUE_SIZE_##size: {                                                                                 \
        m_messagequeue_msg_##size##_t msg = {};                                                                        \
        bool recv = m_messagequeue_##size##_pop_blocking(                                                              \
            &msg, plat_messagequeue->m_messagequeue_##size, timeout == PLAT_TIMEOUT_FOREVER);                          \
        if(timeout == PLAT_TIMEOUT_FOREVER) plat_assert(recv);                                                         \
        if(recv) memcpy(item, &msg, plat_messagequeue->item_size);                                                     \
        return recv;                                                                                                   \
    }
            PLAT_MESSAGEQUEUE_FOR_EACH_SIZE(_)
#undef _
        default:
            plat_crash("Update this code, would you kindly?");
        }
    } else {
        plat_crash("%s(timeout=any) not implemented", __FUNC__);
    }
    plat_crash("huh");
}
