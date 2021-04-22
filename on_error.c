#define _GNU_SOURCE

#include <ucontext.h>

#include "on_error.h"
#include "nextinst.h"

#define ERROR_RESUME_NEXT (void*)-1
#define ERROR_SET_CONTEXT (void*)-2

void* volatile error_label = ERROR_RESUME_NEXT;
volatile sig_atomic_t on_error_signal = 0;
ucontext_t on_error_context;

static void segv_action(int sig, siginfo_t* info, void* contextp) {
    // Save error code for error handler when using error goto
    on_error_signal = sig;

    printf("SIGSEGV received (w/ info and context)\n");
    ucontext_t* context = contextp;
    void** prip = (void**)&context->uc_mcontext.gregs[REG_RIP];
    // TODO Save state for a future Resume Next function called from the error handler.
    if (error_label == ERROR_RESUME_NEXT) {
        *prip = next_instruction(*prip);
        printf("RESUME_NEXT rip=%p\n", *prip);
    } else if (error_label == ERROR_SET_CONTEXT) {
        printf("SET_CONTEXT rip=%p\n", (void*)on_error_context.uc_mcontext.gregs[REG_RIP]);
        setcontext(&on_error_context);
    } else {
        printf("GOTO LABEL %p\n", error_label);
        // Yes, this just plops a new RIP in. Seems more funny than actually doing a longjmp kind of thing
        // that has code in the target function. Probably a lot less safe though :)
        *prip = error_label;
    }
    setcontext(context);
    perror("setcontext");
}

int on_error_setsig(void) {
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

void on_error_setlabel(void* label) {
    error_label = label;
    on_error_signal = 0;
}

int on_error_resume_next(void) {
    printf("ON ERROR RESUME NEXT\n");
    on_error_setlabel(ERROR_RESUME_NEXT);
    return on_error_setsig();
}

int on_error_set_context(void) {
    printf("ON ERROR SET CONTEXT\n");
    on_error_setlabel(ERROR_SET_CONTEXT);
    return on_error_setsig();
}
