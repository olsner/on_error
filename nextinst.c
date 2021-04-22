#define _GNU_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bddisasm.h>

#define print_instruction 0

void* next_instruction(void* rip) {
    INSTRUX ix;
    NDSTATUS status = NdDecodeEx(&ix, rip, 15, ND_CODE_64, ND_DATA_64);
    if (!ND_SUCCESS(status)) {
        printf("ND error %#x\n", status);
        exit(1);
    }
#if print_instruction
    char buf[ND_MIN_BUF_SIZE];
    NdToText(&ix, (uint64_t)rip, sizeof(buf), buf);
    printf("Fault instruction: [%p] %s\n", *prip, buf);
#endif
    return rip + ix.Length;
}

// Porting interfaces for bddisasm
int nd_vsnprintf_s(
    char *buffer,
    size_t sizeOfBuffer,
    size_t count,
    const char *format,
    va_list argptr
    )
{
    return vsnprintf(buffer, sizeOfBuffer, format, argptr);
}

void* nd_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

