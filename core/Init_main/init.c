#include "init.h"

void handle_sigint(int signal) {
  int xReturn;

  xReturn = chdir(BUILD); /* changing dir to place gmon.out inside build */

  if (xReturn == -1) {
    printf("chdir into %s error is %d\n", BUILD, errno);
  }

  exit(2);
}

void init_main(void) {
  /* SIGINT is not blocked by the posix port */
  signal(SIGINT, handle_sigint);

  /* Initialise the trace recorder.  Use of the trace recorder is optional.
   * See http://www.FreeRTOS.org/trace for more information. */
  vTraceEnable(TRC_START);

  console_init();
}