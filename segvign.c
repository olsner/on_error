#define _GNU_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "nextinst.h"
#include "on_error.h"

static void segfault(volatile int* flag) {
    *flag = 1234;
    *(int*)0xdeadbeef = 0xcafebabe;
    *flag = 1;
}

#define SEGFAULT(pflag) do { \
    *(pflag) = 1234; \
    *(int*)0xdeadbeef = 0xcafebabe; \
    *(pflag) = 1; \
} while (0)

static bool test_segfault(void) {
    printf("Segfaulting...\n");
    volatile int flag = 0;
    segfault(&flag);
    if (flag == 1) {
        printf("Resumed after segfault (flag=1)\n");
        return true;
    }
    else {
        printf("Not resumed after segfault (flag=%d)\n", flag);
        return false;
    }
}

void test_resume_next(void) {
    printf("Testing error_resume_next: should resume\n");
    on_error_resume_next();
    bool resumed = test_segfault();
    assert(resumed);
}

void test_error_goto(void) {
    printf("Testing error_goto: should not resume\n");
    on_error_goto(error_handler);
    test_segfault();
    assert(false); // should not get here - should go to the error handler instead
    return;

error_handler:
    printf("error_goto: At error handler as expected.\n");
}

void test_error_goto_unsafe(void) {
    // Note: only safe for errors in the same stack frame as the function,
    // otherwise it's as if the called function jumped to the label, then
    // returns to its caller.
    printf("Testing error_goto_unsafe: should not resume\n");
    on_error_goto_unsafe(error_handler);
    volatile int flag = 0;
    SEGFAULT(&flag);
    assert(false); // should not get here - should go to the error handler instead
    return;

error_handler:
    printf("error_goto_unsafe: At error handler as expected.\n");
    printf("flag = %d (expect 1234)\n", flag);
}

int main() {
    test_resume_next();
    test_error_goto();
    test_error_goto_unsafe();
    printf("\nTests completed successfully.\n");
}
