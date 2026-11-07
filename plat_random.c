#include "plat_random.h"

#include "plat_assert.h"
#include "plat_os.h"

#include <stdlib.h>
#include <string.h>

#define PFX "Random"

#if PLAT_OS == PLAT_OS_WINDOWS

#include <limits.h>
// clang-format off
#include <windows.h>
#include <bcrypt.h>
// clang-format on

void plat_random_init() {
}

uint64_t plat_random_make() {
    uint64_t random;
    plat_random_fill(&random, sizeof(random));
    return random;
}

void plat_random_fill(void* data, size_t size) {
    uint8_t* ptr = data;
    while(size) {
        ULONG chunk = (ULONG)MIN(size, (size_t)ULONG_MAX);
        NTSTATUS status = BCryptGenRandom(NULL, ptr, chunk, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        plat_assert(BCRYPT_SUCCESS(status));
        ptr += chunk;
        size -= chunk;
    }
}

#elif PLAT_OS == PLAT_OS_LINUX || PLAT_OS == PLAT_OS_MACOS

#include <fcntl.h>
#include <unistd.h>

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

#endif
