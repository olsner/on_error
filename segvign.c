#define _GNU_SOURCE

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#define REGISTERS_H // conflicts with ucontext
#include <bddisasm.h>

#define print_instruction 0
typedef uint64_t u64;

void segv_action(int sig, siginfo_t* info, void* contextp) {
    printf("SIGSEGV received (w/ info and context)\n");
    ucontext_t* context = contextp;
    void** prip = (void**)&context->uc_mcontext.gregs[REG_RIP];
    INSTRUX ix;
    NDSTATUS status = NdDecodeEx(&ix, *prip, 15, ND_CODE_64, ND_DATA_64);
    if (!ND_SUCCESS(status)) {
        printf("ND error %#x\n", status);
        exit(1);
    }
#if print_instruction
    char buf[ND_MIN_BUF_SIZE];
    NdToText(&ix, (u64)*prip, sizeof(buf), buf);
    printf("Fault instruction: %s\n", buf);
#endif
    *prip += ix.Length;
    setcontext(context);
    perror("setcontext");
}

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

int on_error_resume_next(void) {
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

void segfault(int* flag) {
    *flag = 1234;
    *(int*)0xdeadbeef = 0xcafebabe;
    *flag = 1;
}

int main() {
    printf("Setting signal handler...\n");
    on_error_resume_next();
    printf("Segfaulting...\n");
    int flag = 0;
    segfault(&flag);
    if (flag == 1) {
        printf("Resumed successfully, exiting.\n");
        return 0;
    }
    else {
        printf("Didn't resume correctly? flag is %d but should be 1\n", flag);
        return 1;
    }
}
