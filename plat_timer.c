#include "plat_timer.h"

#include "plat_assert.h"
#include "plat_mlib.h"
#include "plat_mutex.h"
#include "plat_os.h"
#include "plat_thread.h"

#include <errno.h> // IWYU pragma: keep

struct plat_timer_t {
#if PLAT_OS == PLAT_OS_LINUX
    int fd;
#elif PLAT_OS == PLAT_OS_WINDOWS || PLAT_OS == PLAT_OS_MACOS
    plat_time_ms_t next; // Monotonic time of next expiration
    plat_time_ms_t interval; // 0 for oneshot timers
#endif
    plat_timer_handler_t handler;
    void* ctx;
};
#define M_OPL_plat_timer_t() M_POD_OPLIST

M_LIST_DEF(plat_timer_list, plat_timer_t, M_OPL_plat_timer_t());
#define M_OPL_plat_timer_list_t() M_LIST_OPLIST(plat_timer_list, M_OPL_plat_timer_t())

static plat_timer_list_t plat_timer_list;
static plat_mutex_t* plat_timer_mutex = NULL;

#if PLAT_OS == PLAT_OS_LINUX

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

static int plat_timer_eventfd;

static plat_thread_result_t plat_timer_task(void* ctx) {
    UNUSED(ctx);
    while(true) {
        plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);
        struct pollfd pollfds[1 + plat_timer_list_size(plat_timer_list)];
        struct pollfd* pollfds_it = pollfds;
        *pollfds_it++ = (struct pollfd){
            .fd = plat_timer_eventfd,
            .events = POLLIN,
            .revents = 0,
        };
        for each(plat_timer_t*, plat_timer, plat_timer_list_t, plat_timer_list) {
            *pollfds_it++ = (struct pollfd){
                .fd = plat_timer->fd,
                .events = POLLIN,
                .revents = 0,
            };
        }
        plat_mutex_release(plat_timer_mutex);

        int rc = poll(pollfds, ARRAY_LEN(pollfds), -1);
        plat_assert(rc >= 0);
        if(rc == 0) continue;

        plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);
        if(pollfds[0].revents & POLLIN) {
            uint64_t val;
            plat_assert(read(plat_timer_eventfd, &val, sizeof(val)) == sizeof(val));
        }
        for(size_t pollfd_i = 1; pollfd_i < ARRAY_LEN(pollfds); pollfd_i++) {
            struct pollfd* pollfd = &pollfds[pollfd_i];
            if(!(pollfd->revents & POLLIN)) continue;
            uint64_t expired;
            int rc = read(pollfd->fd, &expired, sizeof(expired));
            if(rc == EAGAIN) continue;
            plat_assert(rc == sizeof(expired));
            plat_timer_t* plat_timer = NULL;
            for each(plat_timer_t*, timer, plat_timer_list_t, plat_timer_list) {
                if(timer->fd != pollfd->fd) continue;
                plat_timer = timer;
                break;
            }
            if(!plat_timer) continue;
            for(size_t expire_i = 0; expire_i < expired; expire_i++) {
                plat_timer->handler(plat_timer, plat_timer->ctx);
            }
            struct itimerspec finished = {};
            struct itimerspec delay;
            rc = timerfd_gettime(pollfd->fd, &delay);
            if(rc != 0) continue;
            if(memcmp(&delay, &finished, sizeof(delay)) == 0) {
                plat_timer_stop(plat_timer);
            }
        }
        plat_mutex_release(plat_timer_mutex);
    }
    return 0;
}

void plat_timer_init() {
    plat_timer_list_init(plat_timer_list);
    plat_timer_eventfd = eventfd(0, EFD_NONBLOCK);
    plat_assert(plat_timer_eventfd >= 0);
    plat_timer_mutex = plat_mutex_alloc(PLAT_MUTEX_TYPE_RECURSIVE);
    plat_thread_start("plat-timer", plat_timer_task, NULL, PLAT_THREAD_STACK_DEFAULT);
}

plat_timer_t* plat_timer_start(plat_timer_type_t type, plat_time_ms_t duration, plat_timer_handler_t handler,
                               void* ctx) {
    plat_assert(handler);

    plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);
    plat_timer_t* plat_timer = plat_timer_list_push_new(plat_timer_list);
    plat_timer->handler = handler;
    plat_timer->ctx = ctx;
    plat_timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    plat_assert(plat_timer->fd >= 0);

    plat_time_spec_t duration_ts = {};
    duration_ts = plat_time_spec_add_ms(duration_ts, duration);
    struct itimerspec delay = {};
    switch(type) {
    case PLAT_TIMER_TYPE_ONESHOT:
        delay.it_value = duration_ts;
        break;
    case PLAT_TIMER_TYPE_PERIODIC:
        delay.it_value = duration_ts;
        delay.it_interval = duration_ts;
        break;
    default:
        plat_crash("Wrong plat_timer_type_t");
    }
    plat_assert(timerfd_settime(plat_timer->fd, 0, &delay, NULL) == 0);

    // Wake up the thread to start polling the new timer
    uint64_t add = 1;
    plat_assert(write(plat_timer_eventfd, &add, sizeof(add)) == sizeof(add));

    plat_mutex_release(plat_timer_mutex);
    return plat_timer;
}

void plat_timer_stop(plat_timer_t* plat_timer) {
    plat_assert(plat_timer);

    plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);

    for each(plat_timer_t*, timer, plat_timer_list_t, plat_timer_list) {
        if(timer != plat_timer) continue;
        plat_assert(close(plat_timer->fd) == 0);
        plat_timer_list_remove(plat_timer_list, timer_it);
        plat_timer = NULL;
        break;
    }
    plat_assert(plat_timer == NULL);

    // Wake up the thread to stop polling the removed timer
    uint64_t add = 1;
    plat_assert(write(plat_timer_eventfd, &add, sizeof(add)) == sizeof(add));

    plat_mutex_release(plat_timer_mutex);
}

#elif PLAT_OS == PLAT_OS_WINDOWS || PLAT_OS == PLAT_OS_MACOS

// No timerfd equivalent, so the timer thread tracks deadlines itself and sleeps until the nearest one

#include "plat_eventbits.h"

typedef enum : uint8_t {
    PLAT_TIMER_EVENT_WAKE = (1 << 0),
} plat_timer_event_t;

static plat_eventbits_t* plat_timer_eventbits = NULL;

static bool plat_timer_exists(plat_timer_t* plat_timer) {
    for each(plat_timer_t*, timer, plat_timer_list_t, plat_timer_list) {
        if(timer == plat_timer) return true;
    }
    return false;
}

static plat_thread_result_t plat_timer_task(void* ctx) {
    UNUSED(ctx);
    while(true) {
        plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);
        plat_time_ms_t now = plat_time_get_mono_ms();
        plat_timer_t* plat_timer = NULL;
        plat_timeout_t wait = PLAT_TIMEOUT_FOREVER;
        for each(plat_timer_t*, timer, plat_timer_list_t, plat_timer_list) {
            if(timer->next <= now) {
                plat_timer = timer;
                break;
            }
            wait = MIN(wait, (plat_timeout_t)(timer->next - now));
        }

        if(!plat_timer) {
            plat_mutex_release(plat_timer_mutex);
            plat_eventbits_wait(plat_timer_eventbits, PLAT_TIMER_EVENT_WAKE, true, true, wait);
            continue;
        }

        uint64_t expired = 1;
        bool oneshot = plat_timer->interval == 0;
        if(!oneshot) {
            expired += (now - plat_timer->next) / plat_timer->interval;
            plat_timer->next += expired * plat_timer->interval;
        }
        for(uint64_t expire_i = 0; expire_i < expired; expire_i++) {
            plat_timer->handler(plat_timer, plat_timer->ctx);
            // Handler might have stopped the timer
            if(!plat_timer_exists(plat_timer)) break;
        }
        if(oneshot && plat_timer_exists(plat_timer)) {
            plat_timer_stop(plat_timer);
        }
        plat_mutex_release(plat_timer_mutex);
    }
    return 0;
}

void plat_timer_init() {
    plat_timer_list_init(plat_timer_list);
    plat_timer_eventbits = plat_eventbits_alloc();
    plat_timer_mutex = plat_mutex_alloc(PLAT_MUTEX_TYPE_RECURSIVE);
    plat_thread_start("plat-timer", plat_timer_task, NULL, PLAT_THREAD_STACK_DEFAULT);
}

plat_timer_t* plat_timer_start(plat_timer_type_t type, plat_time_ms_t duration, plat_timer_handler_t handler,
                               void* ctx) {
    plat_assert(handler);

    plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);
    plat_timer_t* plat_timer = plat_timer_list_push_new(plat_timer_list);
    plat_timer->handler = handler;
    plat_timer->ctx = ctx;
    plat_timer->next = plat_time_get_mono_ms() + duration;
    switch(type) {
    case PLAT_TIMER_TYPE_ONESHOT:
        plat_timer->interval = 0;
        break;
    case PLAT_TIMER_TYPE_PERIODIC:
        plat_assert(duration > 0);
        plat_timer->interval = duration;
        break;
    default:
        plat_crash("Wrong plat_timer_type_t");
    }

    // Wake up the thread to recalculate the next deadline
    plat_eventbits_set(plat_timer_eventbits, PLAT_TIMER_EVENT_WAKE);

    plat_mutex_release(plat_timer_mutex);
    return plat_timer;
}

void plat_timer_stop(plat_timer_t* plat_timer) {
    plat_assert(plat_timer);

    plat_mutex_acquire(plat_timer_mutex, PLAT_TIMEOUT_FOREVER);

    for each(plat_timer_t*, timer, plat_timer_list_t, plat_timer_list) {
        if(timer != plat_timer) continue;
        plat_timer_list_remove(plat_timer_list, timer_it);
        plat_timer = NULL;
        break;
    }
    plat_assert(plat_timer == NULL);

    // Wake up the thread to recalculate the next deadline
    plat_eventbits_set(plat_timer_eventbits, PLAT_TIMER_EVENT_WAKE);

    plat_mutex_release(plat_timer_mutex);
}

#endif
