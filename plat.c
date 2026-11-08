#include "plat.h"
#include "_plat.h"

void plat_init() {
    plat_log_init();
    plat_random_init();
    plat_run_init();
    plat_timer_init();
}
