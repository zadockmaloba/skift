#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <libsystem/Macros.h>

typedef unsigned int uint;

typedef char s8;
typedef short s16;
typedef long s32;
typedef long long s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct
{
    const char *file;
    const char *function;
    int line;
} __SOURCE_LOCATION__;

#define SOURCE_LOCATION \
    ((__SOURCE_LOCATION__){__FILE__, __FUNCTION__, __LINE__})

/* --- Raw memory allocation ------------------------------------------------ */

__attribute__((malloc)) __attribute__((alloc_size(1))) void *malloc(size_t size);
__attribute__((malloc)) __attribute__((alloc_size(1, 2))) void *calloc(size_t, size_t);

void *realloc(void *p, size_t size);

void free(void *);

void malloc_cleanup(void *buffer);

/* --- Loops ---------------------------------------------------------------- */

typedef enum
{
    ITERATION_CONTINUE,
    ITERATION_STOP,
} IterationDecision;
