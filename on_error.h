#pragma once

#include <signal.h>
#include <ucontext.h>

/**{{*/
/** Internals used by macros. Don't use directly. */

#ifndef ON_ERROR_API
#define ON_ERROR_API
#endif

ON_ERROR_API extern volatile sig_atomic_t on_error_signal;

ON_ERROR_API extern void on_error_setlabel(void* label);
ON_ERROR_API extern int on_error_setsig(void);
ON_ERROR_API extern ucontext_t* on_error_set_context(void);
/**}}*/

/**
 * When a segmentation fault happens, resume at the next instruction.
 */
ON_ERROR_API extern int on_error_resume_next(void);

/**
 * On error, goto the given label in the current function.
 * Don't cause any errors after returning, or the program will likely crash.
 */
#define on_error_goto(label) do { \
    getcontext(on_error_set_context()); \
    if (on_error_signal) { \
        goto label; \
    } \
} while (0)

#ifdef __GNUC__
/**
 * More unsafe version of on_error_goto.
 *
 * Only works for errors within the calling function, otherwise weird control
 * flow will happen. For example, if foo calls bar and bar has an error, bar
 * will run foo's error handling inline and when foo's return statement runs it
 * will return from bar instead.
 */
#define on_error_goto_unsafe(label) do { \
    on_error_setlabel(&&label); \
    /* Trick the compiler into keeping label reachable. */ \
    if (on_error_signal) goto label; \
    on_error_setsig(); \
} while (0)
#endif
