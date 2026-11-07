#pragma once

// IWYU pragma: begin_exports
#include "plat_assert.h"
#include "plat_crash.h"
#include "plat_defs.h"
#include "plat_eventbits.h"
#include "plat_log.h"
#include "plat_marker.h"
#include "plat_messagequeue.h"
#include "plat_mutex.h"
#include "plat_os.h"
#include "plat_random.h"
#include "plat_run.h"
#include "plat_streambuffer.h"
#include "plat_thread.h"
#include "plat_time.h"
#include "plat_timer.h"
#include "plat_types.h"
// IWYU pragma: end_exports

// IWYU pragma: begin_exports
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
// IWYU pragma: end_exports

// IWYU pragma: begin_exports
#include "plat_mlib.h"
// IWYU pragma: end_exports

void plat_init();
