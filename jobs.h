#ifndef JOBS_H
#define JOBS_H

#include <unistd.h>
#include "defines.h"

job * find_job (pid_t pgid);
int job_is_stopped (job *j);
int job_is_completed (job *j);

#endif /* JOBS_H */