#pragma once

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

// TODO Re

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

