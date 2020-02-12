#include <stdlib.h>
#include <string.h>

#include "cli_cmd.h"
#include "utils/macros.h"

int main(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);
  char *line = NULL;

  cli_command_init();

  // Enable multiline mode
  linenoiseSetMultiLine(1);

  /* Set the completion callback. This will be called every time the
   * user uses the <tab> key. */
  linenoiseSetCompletionCallback(completion_callback);
  linenoiseSetHintsCallback(hints_callback);

  /* Now this is the main loop of the typical linenoise-based application.
   * The call to linenoise() will block as long as the user types something
   * and presses enter.
   *
   * The typed string is returned as a malloc() allocated string by
   * linenoise, so the user needs to free() it. */
  while ((line = linenoise("IOTA> ")) != NULL) {
    /* Do something with the string. */
    if (line[0] != '\0' && line[0] != '/') {
      // printf("echo: '%s'\n", line);
      if (!strncmp(line, "exit", 4)) {
        break;
      }
      // parsing commands
      cli_err_t ret = 0;
      cli_command_run(line, &ret);
      linenoiseHistoryAdd(line); /* Add to the history. */
    }
    linenoiseFree(line);
  }

  linenoiseFree(line);
  cli_command_end();
  return 0;
}
