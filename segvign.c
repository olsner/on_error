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

#define ERROR_RESUME_NEXT (void*)-1
#define ERROR_SET_CONTEXT (void*)-2

void* volatile error_label = ERROR_RESUME_NEXT;
volatile sig_atomic_t error_signal = 0;
ucontext_t error_context;

void segv_action(int sig, siginfo_t* info, void* contextp) {
    // Save error code for error handler when using error goto
    error_signal = sig;

    printf("SIGSEGV received (w/ info and context)\n");
    ucontext_t* context = contextp;
    void** prip = (void**)&context->uc_mcontext.gregs[REG_RIP];
    // TODO Save state for a future Resume Next function called from the error handler.
    if (error_label == ERROR_RESUME_NEXT) {
        *prip = next_instruction(*prip);
        printf("RESUME_NEXT rip=%p\n", *prip);
    } else if (error_label == ERROR_SET_CONTEXT) {
        printf("SET_CONTEXT rip=%p\n", (void*)error_context.uc_mcontext.gregs[REG_RIP]);
        setcontext(&error_context);
    } else {
        printf("GOTO LABEL %p\n", error_label);
        // Yes, this just plops a new RIP in. Seems more funny than actually doing a longjmp kind of thing
        // that has code in the target function. Probably a lot less safe though :)
        *prip = error_label;
    }
    setcontext(context);
    perror("setcontext");
}

int set_signal_handler(void) {
    printf("Setting signal handler...\n");
    struct sigaction sa;
    if (sigaction(SIGSEGV, NULL, &sa)) {
        perror("sigaction");
        return -1;
    }
    sa.sa_sigaction = segv_action;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL)) {
        perror("sigaction");
        return -1;
    }
    return 0;
}

int on_error_resume_next(void) {
    error_label = ERROR_RESUME_NEXT;
    return set_signal_handler();
}

#define on_error_goto(label) do { \
    error_label = ERROR_SET_CONTEXT; \
    error_signal = 0; \
    set_signal_handler(); \
    getcontext(&error_context); \
    printf("on_error_getcontext returned with error_signal=%d\n", error_signal); \
    if (error_signal) { \
        printf("going to error label " #label "\n"); \
        goto label; \
    } \
} while (0)

#define on_error_goto_unsafe(label) do { \
    error_signal = 0; error_label = &&label; \
    /* Trick the compiler into keeping label reachable. */ \
    if (error_signal) goto label; \
    set_signal_handler(); \
} while (0)

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
