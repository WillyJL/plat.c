#include "plat_random.h"

#include "plat_assert.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PFX "Random"

static int plat_random_fd = -1;

void plat_random_init() {
    plat_random_fd = open("/dev/urandom", O_RDONLY);
    plat_assert(plat_random_fd >= 0);
}

uint64_t plat_random_make() {
    uint64_t random;
    plat_assert(read(plat_random_fd, &random, sizeof(random)) == sizeof(random));
    return random;
}

void plat_random_fill(void* data, size_t size) {
    ssize_t res = read(plat_random_fd, data, size);
    plat_assert(res >= 0 && (size_t)res == size);
}
