#ifndef __CLI_CMD_H__
#define __CLI_CMD_H__

#include "cli_config.h"
#include "linenoise.h"

typedef int32_t cli_err_t;

// command callback
typedef cli_err_t (*cli_cmd_cb_t)(int argc, char **argv);

typedef struct {
  /**
   * Command name. Must not be NULL, must not contain spaces.
   * The pointer must be valid until the call to esp_console_deinit.
   */
  char *command;  //!< command name
  /**
   * Help text for the command, shown by help command.
   * If set, the pointer must be valid until the call to esp_console_deinit.
   * If not set, the command will not be listed in 'help' output.
   */
  char *help;
  /**
   * Hint text, usually lists possible arguments.
   * If set to NULL, and 'argtable' field is non-NULL, hint will be generated
   * automatically
   */
  char *hint;
  /**
   * Pointer to a function which implements the command.
   */
  cli_cmd_cb_t func;
  /**
   * Array or structure of pointers to arg_xxx structures, may be NULL.
   * Used to generate hint text if 'hint' is set to NULL.
   * Array/structure which this field points to must end with an arg_end.
   * Only used for the duration of esp_console_cmd_register call.
   */
  void *argtable;
} cli_cmd_t;

#define HINT_COLOR_RED 31
#define HINT_COLOR_GREEN 32
#define HINT_COLOR_YELLOW 33
#define HINT_COLOR_BLUE 34
#define HINT_COLOR_MAGENTA 35
#define HINT_COLOR_CYAN 36
#define HINT_COLOR_WHITE 37

#define CLI_OK 0
#define CLI_ERR_FAILED 0xFFFF
#define CLI_ERR_OOM 0x0101
#define CLI_ERR_NULL_POINTER 0x0102

#define CLI_ERR_INVALID_CMD 0x0201
#define CLI_ERR_CMD_NOT_FOUND 0x0202
#define CLI_ERR_CMD_PARSING 0x0203
#define CLI_ERR_INVALID_ARG 0x0204

#define CLI_NODE_INFO_FAILED 0x0301

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Split command line into arguments in place
 *
 * - This function finds whitespace-separated arguments in the given input line.
 *
 *     'abc def 1 20 .3' -> [ 'abc', 'def', '1', '20', '.3' ]
 *
 * - Argument which include spaces may be surrounded with quotes. In this case
 *   spaces are preserved and quotes are stripped.
 *
 *     'abc "123 456" def' -> [ 'abc', '123 456', 'def' ]
 *
 * - Escape sequences may be used to produce backslash, double quote, and space:
 *
 *     'a\ b\\c\"' -> [ 'a b\c"' ]
 *
 * Pointers to at most argv_size - 1 arguments are returned in argv array.
 * The pointer after the last one (i.e. argv[argc]) is set to NULL.
 *
 * @param line pointer to buffer to parse; it is modified in place
 * @param argv array where the pointers to arguments are written
 * @param argv_size number of elements in argv_array (max. number of arguments)
 * @return number of arguments found (argc)
 */
size_t esp_console_split_argv(char *line, char **argv, size_t argv_size);

void completion_callback(char const *buf, linenoiseCompletions *lc);
char *hints_callback(char const *buf, int *color, int *bold);

cli_err_t cli_command_init();
cli_err_t cli_command_end();
cli_err_t cli_command_run(char const *const cmdline, cli_err_t *cmd_ret);

#ifdef __cplusplus
}
#endif

#endif  // __CLI_CMD_H__
