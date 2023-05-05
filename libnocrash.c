#include "on_error.h"

__attribute__((constructor)) void set_resume_next() {
    on_error_resume_next();
}
