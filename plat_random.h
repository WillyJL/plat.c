#pragma once

#include <stddef.h>
#include <stdint.h>

void plat_random_init();

uint64_t plat_random_make();
void plat_random_fill(void* data, size_t size);
