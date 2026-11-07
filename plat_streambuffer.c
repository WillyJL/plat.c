#include "plat_streambuffer.h"

#include "plat_assert.h"
#include "plat_eventbits.h"
#include "plat_time.h"

#include <mlib/m-buffer.h>

M_QUEUE_SPSC_DEF(m_streambuffer, uint8_t, M_BUFFER_QUEUE)

typedef enum : uint8_t {
    PLAT_STREAMBUFFER_EVENT_SEND = (1 << 0),
    PLAT_STREAMBUFFER_EVENT_RECV = (1 << 1),
} plat_streambuffer_event_t;

struct plat_streambuffer_t {
    m_streambuffer_t m_streambuffer;
    plat_eventbits_t* eventbits;
};

plat_streambuffer_t* plat_streambuffer_alloc(size_t size) {
    plat_assert(size);
    plat_assert(POWER_OF_2(size));

    plat_streambuffer_t* plat_streambuffer = malloc(sizeof(plat_streambuffer_t));
    m_streambuffer_init(plat_streambuffer->m_streambuffer, size);
    plat_streambuffer->eventbits = plat_eventbits_alloc();
    return plat_streambuffer;
}

void plat_streambuffer_free(plat_streambuffer_t* plat_streambuffer) {
    plat_assert(plat_streambuffer);

    m_streambuffer_clear(plat_streambuffer->m_streambuffer);
    plat_eventbits_free(plat_streambuffer->eventbits);
    free(plat_streambuffer);
}

size_t plat_streambuffer_send(plat_streambuffer_t* plat_streambuffer, const void* data, size_t size,
                              plat_timeout_t timeout) {
    plat_assert(plat_streambuffer);
    plat_assert(data);
    plat_assert(size);

    if(timeout == PLAT_TIMEOUT_FOREVER) {
        size_t send = 0;
        while(send < size) {
            if(m_streambuffer_full_p(plat_streambuffer->m_streambuffer)) {
                plat_eventbits_wait(
                    plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_RECV, true, true, PLAT_TIMEOUT_FOREVER);
            }
            send += m_streambuffer_push_bulk(plat_streambuffer->m_streambuffer, size - send, data + send);
            if(send) {
                plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_SEND);
            }
        }
        plat_assert(send == size);
        return send;
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        size_t send = m_streambuffer_push_bulk(plat_streambuffer->m_streambuffer, size, data);
        if(send) {
            plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_SEND);
        }
        return send;
    } else {
        plat_time_ms_t now = plat_time_get_mono_ms();
        plat_time_ms_t exp = now + timeout;
        size_t send = 0;
        while(now < exp && send < size) {
            if(m_streambuffer_full_p(plat_streambuffer->m_streambuffer)) {
                plat_eventbits_wait(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_RECV, true, true, exp - now);
            }
            send += m_streambuffer_push_bulk(plat_streambuffer->m_streambuffer, size - send, data + send);
            if(send) {
                plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_SEND);
            }
            now = plat_time_get_mono_ms();
        }
        return send;
    }
}

size_t plat_streambuffer_recv(plat_streambuffer_t* plat_streambuffer, void* data, size_t size, size_t want,
                              plat_timeout_t timeout) {
    plat_assert(plat_streambuffer);
    plat_assert(data);
    plat_assert(size);
    plat_assert(want <= size);
    size_t capacity = m_streambuffer_capacity(plat_streambuffer->m_streambuffer);

    if(timeout == PLAT_TIMEOUT_FOREVER) {
        size_t recv = 0;
        while(true) {
            if(m_streambuffer_empty_p(plat_streambuffer->m_streambuffer)) {
                plat_eventbits_wait(
                    plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_SEND, true, true, PLAT_TIMEOUT_FOREVER);
            }
            recv += m_streambuffer_pop_bulk(MIN(size - recv, capacity), data + recv, plat_streambuffer->m_streambuffer);
            if(recv) {
                plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_RECV);
            }
            if(recv >= want) {
                return recv;
            }
        }
    } else if(timeout == PLAT_TIMEOUT_NONE) {
        size_t recv = m_streambuffer_pop_bulk(MIN(size, capacity), data, plat_streambuffer->m_streambuffer);
        if(recv) {
            plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_RECV);
        }
        return recv;
    } else {
        plat_time_ms_t now = plat_time_get_mono_ms();
        plat_time_ms_t exp = now + timeout;
        size_t recv = 0;
        while(recv < want && now < exp) {
            if(m_streambuffer_empty_p(plat_streambuffer->m_streambuffer)) {
                plat_eventbits_wait(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_SEND, true, true, exp - now);
            }
            recv += m_streambuffer_pop_bulk(MIN(size - recv, capacity), data + recv, plat_streambuffer->m_streambuffer);
            if(recv) {
                plat_eventbits_set(plat_streambuffer->eventbits, PLAT_STREAMBUFFER_EVENT_RECV);
            }
            now = plat_time_get_mono_ms();
        }
        return recv;
    }
}

size_t plat_streambuffer_size(plat_streambuffer_t* plat_streambuffer) {
    plat_assert(plat_streambuffer);

    return m_streambuffer_capacity(plat_streambuffer->m_streambuffer);
}

size_t plat_streambuffer_avail_send(plat_streambuffer_t* plat_streambuffer) {
    plat_assert(plat_streambuffer);

    return (m_streambuffer_capacity(plat_streambuffer->m_streambuffer) -
            m_streambuffer_size(plat_streambuffer->m_streambuffer));
}

size_t plat_streambuffer_avail_recv(plat_streambuffer_t* plat_streambuffer) {
    plat_assert(plat_streambuffer);

    return m_streambuffer_size(plat_streambuffer->m_streambuffer);
}

void plat_streambuffer_drain(plat_streambuffer_t* plat_streambuffer) {
    plat_assert(plat_streambuffer);

    while(plat_streambuffer_avail_recv(plat_streambuffer)) {
        uint8_t drain[128];
        plat_streambuffer_recv(plat_streambuffer, drain, sizeof(drain), sizeof(drain), PLAT_TIMEOUT_NONE);
    }
}
