#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

#include "on_error.h"
#include "nextinst.h"

#define ERROR_RESUME_NEXT (void*)-1
#define ERROR_SET_CONTEXT (void*)-2

void* volatile error_label = ERROR_RESUME_NEXT;
volatile sig_atomic_t on_error_signal = 0;
static ucontext_t on_error_context;

#define debug_enabled 0
#define debug(...) do { if (debug_enabled) printf(__VA_ARGS__); } while(0)

static void segv_action(int sig, siginfo_t* info, void* contextp) {
    // Save error code for error handler when using error goto
    on_error_signal = sig;

    debug("SIGSEGV received: fault addr %p, sig=%d errno=%d code=%d\n",
            info->si_addr, info->si_signo, info->si_errno, info->si_code);
    ucontext_t* context = contextp;
    void** prip = (void**)&context->uc_mcontext.gregs[REG_RIP];
    if (error_label == ERROR_RESUME_NEXT) {
        *prip = next_instruction(*prip);
        debug("RESUME_NEXT rip=%p\n", *prip);
    } else if (error_label == ERROR_SET_CONTEXT) {
        debug("SET_CONTEXT rip=%p\n", (void*)on_error_context.uc_mcontext.gregs[REG_RIP]);
        setcontext(&on_error_context);
        perror("setcontext");
    } else {
        debug("GOTO LABEL %p\n", error_label);
        // Yes, this just plops a new RIP in. Seems more funny than actually
        // doing a longjmp kind of thing that has code in the target function.
        // Probably a lot less safe though :)
        *prip = error_label;
    }
    // When we return, the signal handler return trampoline takes care of
    // restoring the context (which we've modified to redirect it to the
    // error handler).
}

int on_error_setsig(void) {
    debug("Setting signal handler...\n");
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

void on_error_setlabel(void* label) {
    debug("ON ERROR GOTO %p: was [%p, %d]\n", label, error_label, on_error_signal);
    error_label = label;
    on_error_signal = 0;
    debug("ON ERROR GOTO: now [%p, %d]\n", error_label, on_error_signal);
}

int on_error_resume_next(void) {
    debug("ON ERROR RESUME NEXT\n");
    error_label = ERROR_RESUME_NEXT;
    on_error_signal = 0;
    return on_error_setsig();
}

ucontext_t* on_error_set_context(void) {
    debug("ON ERROR SET CONTEXT\n");
    error_label = ERROR_SET_CONTEXT;
    on_error_signal = 0;
    return on_error_setsig() ? 0 : &on_error_context;
}
