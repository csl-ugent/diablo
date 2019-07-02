#include "diablosupport.hpp"

#include <sys/wait.h>

#define FORK_VERBOSE 1
#define FORK_PREFIX "[fork] "

void ForkAndWait(F_ForkFunction child) {
  VERBOSE(0, ("fork-and-wait from process %d", getpid()));

  /* flush all open file buffers */
  for (auto f : GetOpenedLogFiles()) {
    int fno = fileno(f);
    VERBOSE(FORK_VERBOSE, (FORK_PREFIX "flushing file /proc/self/fd/%d", fno));
    fflush(f);
  }

  /* flush stdout and stderr */
  fflush(stdout);
  fflush(stderr);

  /* fork and return child pid */
  pid_t child_pid = fork();

  if (child_pid == -1) {
    /* ERROR */
    FATAL((FORK_PREFIX "fork failed"));
  }
  else if (child_pid == 0) {
    VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Child: forked to child %d", getpid()));
    child();

    exit(0);
  }
  else {
    VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Parent: waiting for child %d", child_pid));

    int wstatus;
    do {
      pid_t w = waitpid(child_pid, &wstatus, WUNTRACED | WCONTINUED);

      if (w == -1) {
        FATAL((FORK_PREFIX "Parent: waitpid failed"));
      }
      else {
        if (WIFEXITED(wstatus)) {
          VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Parent: child exited, exit code %d", WEXITSTATUS(wstatus)));
        }
        else if (WIFSIGNALED(wstatus)) {
          VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Parent: child killed by signal %d", WTERMSIG(wstatus)));
        }
        else if (WIFSTOPPED(wstatus)) {
          VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Parent: child stopped by signal %d", WSTOPSIG(wstatus)));
        }
        else if (WIFCONTINUED(wstatus)) {
          VERBOSE(FORK_VERBOSE, (FORK_PREFIX "Parent: child continued"));
        }
      }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
  }

}
