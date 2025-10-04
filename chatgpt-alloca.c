/*
 * example_alloca_segv.c
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -o example_alloca_segv example_alloca_segv.c
 *
 * Run (example large size to provoke stack growth; adjust as needed):
 *   ./example_alloca_segv 1000000000
 *
 * NOTE: provoking a real stack overflow is platform-dependent; this program
 * demonstrates the technique but cannot make stack overflows safe.
 */

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <alloca.h>   /* If not available on your platform, use __builtin_alloca */

static void segv_handler(int sig, siginfo_t *si, void *ucontext)
{
    const char msg[] = "SIGSEGV handler: caught SIGSEGV\n";
    /* Use async-signal-safe write() instead of printf */
    (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
    /* It's generally unsafe to attempt complex cleanup here */
    _exit(128 + sig);
}

int main(int argc, char **argv)
{
    size_t size = 1UL << 30; /* default ~1 GiB (likely to trigger overflow on many systems) */
    if (argc > 1) {
        char *end;
        unsigned long long v = strtoull(argv[1], &end, 10);
        if (end != argv[1] && v > 0) size = (size_t)v;
    }

    /* --- Prepare an alternate signal stack --- */
    stack_t ss, old_ss;
    ss.ss_size = SIGSTKSZ;
    ss.ss_sp = malloc(ss.ss_size);
    if (!ss.ss_sp) {
        perror("malloc for sigaltstack");
        return 1;
    }
    ss.ss_flags = 0;
    if (sigaltstack(&ss, &old_ss) == -1) {
        perror("sigaltstack");
        free(ss.ss_sp);
        return 1;
    }

    /* --- Install SIGSEGV handler that runs on the alternate stack --- */
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGSEGV, &sa, &old_sa) == -1) {
        perror("sigaction");
        /* restore altstack */
        sigaltstack(&old_ss, NULL);
        free(ss.ss_sp);
        return 1;
    }

    /* --- Use alloca and touch memory to force real stack growth --- */
    printf("Attempting alloca(%zu) and touching pages...\n", size);

    /* alloca() never returns NULL; it adjusts the stack pointer */
    char *p = alloca(size);

    /* Touch pages to force the OS to allocate stack pages; this is what usually
       triggers a SIGSEGV on overflow.  Use a volatile pointer to avoid optimization. */
    volatile char *q = p;
    size_t i;
    for (i = 0; i < size; i += 4096) { /* step by page-size to minimize time */
        q[i] = 0;
    }

    /* If we reach here, allocation + touching succeeded */
    printf("Successfully allocated and touched %zu bytes on the stack (no SIGSEGV).\n", size);

    /* --- Clean up: restore original SIGSEGV handler and altstack --- */
    if (sigaction(SIGSEGV, &old_sa, NULL) == -1) {
        perror("restoring sigaction");
        /* fall through to restore altstack anyway */
    }

    /* restore previous alternate stack (if any) */
    if (sigaltstack(&old_ss, NULL) == -1) {
        perror("restoring sigaltstack");
        /* continue */
    }

    /* free our altstack memory only if we allocated one earlier and there was no previous stack
       Note: if old_ss.ss_sp was non-NULL, it means there was a previous altstack; we replaced it,
       but it's up to the original owner to free it. Here we only free the one we malloc'd. */
    free(ss.ss_sp);

    return 0;
}
