#include "plat_assert.h"

PLAT_NO_RETURN void __assert_func(const char* file, int line, const char* func, const char* expr) {
    plat_crash_detail(file, line, func, "assert(%s)", expr);
}
