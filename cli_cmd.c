#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "argtable3.h"
#include "cli_cmd.h"
#include "client/api/v1/get_node_info.h"
#include "core/utils/byte_buffer.h"
#include "utarray.h"
#include "wallet/wallet.h"

#define CMDER_VERSION_MAJOR 0
#define CMDER_VERSION_MINOR 0
#define CMDER_VERSION_MICRO 1

typedef struct {
  iota_wallet_t *wallet;
  char *parsing_buf;   /*!< buffer for command line parsing */
  UT_array *cmd_array; /*!< an array of registed commands */
} cli_ctx_t;

static cli_ctx_t cli_ctx;

static char const *amazon_ca1_pem =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"
    "-----END CERTIFICATE-----\r\n";

static cli_err_t cli_wallet_init() {
  byte_t seed[IOTA_SEED_BYTES] = {};
  size_t str_seed_len = strlen(WALLET_CONFIG_SEED);

  // TODO get seed from terminal if the seed is invalid.
  if (str_seed_len != 64) {
    printf("invalid seed length, it should be a 64-character-string\n");
    return CLI_ERR_FAILED;
  }

  if (hex2bin(WALLET_CONFIG_SEED, str_seed_len, seed, sizeof(seed)) != 0) {
    printf("invalid seed\n");
    return CLI_ERR_FAILED;
  }
  if ((cli_ctx.wallet = wallet_create(seed, WALLET_CONFIG_PATH)) == NULL) {
    printf("create wallet instance failed\n");
    return CLI_ERR_FAILED;
  }
  if (wallet_set_endpoint(cli_ctx.wallet, CLIENT_CONFIG_NODE, CLIENT_CONFIG_PORT) != 0) {
    printf("set endpoint failed\n");
    wallet_destroy(cli_ctx.wallet);
    cli_ctx.wallet = NULL;
    return CLI_ERR_FAILED;
  }
  return CLI_OK;
}

static void cmd_icd_copy(void *_dst, const void *_src) {
  cli_cmd_t *dst = (cli_cmd_t *)_dst, *src = (cli_cmd_t *)_src;
  dst->command = src->command ? strdup(src->command) : NULL;
  dst->help = src->help ? strdup(src->help) : NULL;
  dst->hint = src->hint ? strdup(src->hint) : NULL;
  dst->func = src->func;
  dst->argtable = src->argtable;
}

static void cmd_icd_dtor(void *_elt) {
  cli_cmd_t *elt = (cli_cmd_t *)_elt;
  if (elt->command) {
    free(elt->command);
  }
  if (elt->help) {
    free(elt->help);
  }
  if (elt->hint) {
    free(elt->hint);
  }
}

UT_icd cli_cmd_icd = {sizeof(cli_cmd_t), NULL, cmd_icd_copy, cmd_icd_dtor};

void completion_callback(char const *buf, linenoiseCompletions *lc) {
  size_t len = strlen(buf);
  cli_cmd_t *cmd_p = NULL;
  if (len == 0) {
    return;
  }

  while ((cmd_p = (cli_cmd_t *)utarray_next(cli_ctx.cmd_array, cmd_p))) {
    if (strncmp(buf, cmd_p->command, len) == 0) {
      linenoiseAddCompletion(lc, cmd_p->command);
    }
  }
}

char *hints_callback(char const *buf, int *color, int *bold) {
  size_t len = strlen(buf);
  cli_cmd_t *cmd_p = NULL;
  if (len == 0) {
    return NULL;
  }

  while ((cmd_p = (cli_cmd_t *)utarray_next(cli_ctx.cmd_array, cmd_p))) {
    if (strlen(cmd_p->command) == len && strncmp(buf, cmd_p->command, len) == 0) {
      *color = HINT_COLOR_GREEN;
      *bold = 0;
      return cmd_p->hint;
    }
  }
  return NULL;
}

void to_uppercase(char *sPtr, int nchar) {
  for (int i = 0; i < nchar; i++) {
    char *element = sPtr + i;
    *element = toupper((unsigned char)*element);
  }
}

//==========COMMANDS==========

static cli_err_t fn_help(int argc, char **argv) {
  cli_cmd_t *cmd_p = NULL;

  while ((cmd_p = (cli_cmd_t *)utarray_next(cli_ctx.cmd_array, cmd_p))) {
    char const *help = (cmd_p->help) ? cmd_p->help : "";
    if (cmd_p->hint != NULL) {
      printf("- %s %s\n", cmd_p->command, cmd_p->hint);
      printf("    %s\n", cmd_p->help);
    } else {
      printf("- %s %s\n", cmd_p->command, help);
    }
    if (cmd_p->argtable) {
      arg_print_glossary(stdout, (void **)cmd_p->argtable, "  %12s  %s\n");
    }
    printf("\n");
  }
  return CLI_OK;
}

static void cmd_register_help() {
  cli_cmd_t cmd = {
      .command = "help",
      .help = "Show this help",
      .hint = NULL,
      .func = &fn_help,
      .argtable = NULL,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========NODE_INFO==========
static cli_err_t fn_version(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_version() {
  cli_cmd_t cmd = {
      .command = "version",
      .help = "Show version info",
      .hint = NULL,
      .func = &fn_version,
      .argtable = NULL,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========NODE_INFO==========
static cli_err_t fn_node_info(int argc, char **argv) {
  res_node_info_t *info = res_node_info_new();
  if (!info) {
    return CLI_ERR_OOM;
  }
  cli_err_t ret = get_node_info(&cli_ctx.wallet->endpoint, info);
  if (ret != 0) {
    if (info->is_error) {
      printf("node_info error message: %s\n", info->u.error->msg);
    }
  } else {
    printf("name: %s\n", info->u.output_node_info->name);
    printf("version: %s\n", info->u.output_node_info->version);
    printf("isHealthy: %s\n", info->u.output_node_info->is_healthy ? "true" : "false");
    printf("networkId: %s\n", info->u.output_node_info->network_id);
    printf("bech32HRP: %s\n", info->u.output_node_info->bech32hrp);
    printf("minPoWScore: %" PRIu64 "\n", info->u.output_node_info->min_pow_score);
    printf("latestMilestoneIndex: %" PRIu64 "\n", info->u.output_node_info->latest_milestone_index);
    printf("confirmedMilestoneIndex: %" PRIu64 "\n", info->u.output_node_info->confirmed_milestone_index);
    printf("pruningIndex: %" PRIu64 "\n", info->u.output_node_info->pruning_milestone_index);
  }

  res_node_info_free(info);
  return ret;
}

static void cmd_register_node_info() {
  cli_cmd_t cmd = {
      .command = "node_info",
      .help = "Shows node info",
      .hint = NULL,
      .func = &fn_node_info,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========NODE_INFO_SET==========
static struct {
  struct arg_str *url;
  struct arg_int *port;
  struct arg_int *is_https;
  struct arg_end *end;
} node_info_set_args;

static cli_err_t fn_node_info_set(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_node_info_set() {
  node_info_set_args.url = arg_str1(NULL, NULL, "<url>", "IP or URI");
  node_info_set_args.port = arg_int1(NULL, NULL, "<port>", "port number");
  node_info_set_args.is_https = arg_int1(NULL, NULL, "<is_https>", "0 or 1");
  node_info_set_args.end = arg_end(5);
  // node_info_set_args.url->hdr.resetfn = (arg_resetfn*)arg_str_reset;

  cli_cmd_t cmd = {
      .command = "node_info_set",
      .help = "Sets node info",
      .hint = " <url> <port> <is_https (0|1)> ",
      .func = &fn_node_info_set,
      .argtable = &node_info_set_args,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========CLIENT_CONF==========
static cli_err_t fn_client_conf(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_client_conf() {
  cli_cmd_t cmd = {
      .command = "client_conf",
      .help = "Show client configurations",
      .hint = NULL,
      .func = &fn_client_conf,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========CLIENT_CONF_SET==========
static struct {
  struct arg_int *mwm;
  struct arg_int *depth;
  struct arg_int *security;
  struct arg_end *end;
} client_conf_set_args;

static cli_err_t fn_client_conf_set(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&client_conf_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, client_conf_set_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_client_conf_set() {
  client_conf_set_args.mwm = arg_int1(NULL, NULL, "<mwm>", "9 for testnet, 14 for mainnet");
  client_conf_set_args.depth =
      arg_int1(NULL, NULL, "<depth>", "The depth as which Random Walk starts, 3 is a typicall value used by wallets.");
  client_conf_set_args.security =
      arg_int1(NULL, NULL, "<security>", "The security level of addresses, can be 1, 2 or 3");
  client_conf_set_args.end = arg_end(4);
  cli_cmd_t cmd = {
      .command = "client_conf_set",
      .help = "Set client configurations",
      .hint = " <mwm> <depth> <security>",
      .func = &fn_client_conf_set,
      .argtable = &client_conf_set_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========SEED==========
static cli_err_t fn_seed(int argc, char **argv) {
  dump_hex_str(cli_ctx.wallet->seed, IOTA_SEED_BYTES);
  return CLI_OK;
}

static void cmd_register_seed() {
  cli_cmd_t cmd = {
      .command = "seed",
      .help = "Show SEED",
      .hint = NULL,
      .func = &fn_seed,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========SEED_SET==========
static struct {
  struct arg_str *seed;
  struct arg_end *end;
} seed_set_args;

static cli_err_t fn_seed_set(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_seed_set() {
  seed_set_args.seed = arg_str1(NULL, NULL, "<seed>", "Hash with 81 trytes");
  seed_set_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "seed_set",
      .help = "Set the SEED hash of this wallet",
      .hint = " <seed>",
      .func = &fn_seed_set,
      .argtable = &seed_set_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========ACCOUNT==========
static cli_err_t fn_account_data(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_account_data() {
  cli_cmd_t cmd = {
      .command = "account",
      .help = "Get account data",
      .hint = NULL,
      .func = &fn_account_data,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========BALANCE==========
static struct {
  struct arg_str *address;
  struct arg_end *end;
} get_balance_args;

static int fn_get_balance(int argc, char **argv) {
  int ret_code = 0;
  printf("TODO\n");
  return ret_code;
}

static void cmd_register_get_balance() {
  get_balance_args.address = arg_strn(NULL, NULL, "<address>", 1, 10, "Address hashes");
  get_balance_args.end = arg_end(20);
  cli_cmd_t get_balance_cmd = {
      .command = "balance",
      .help = "Get the balance from addresses",
      .hint = " <addresses...>",
      .func = &fn_get_balance,
      .argtable = &get_balance_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &get_balance_cmd);
}

//==========SEND==========
static struct {
  struct arg_str *receiver;
  struct arg_str *value;
  struct arg_str *tag;
  struct arg_str *remainder;
  struct arg_str *message;
  struct arg_end *end;
} send_args;

static int fn_send(int argc, char **argv) {
  int ret_code = 0;
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_send() {
  send_args.receiver = arg_str1(NULL, NULL, "<RECEIVER>", "A receiver address");
  send_args.value = arg_str0("v", "value", "<VALUE>", "A token value");
  send_args.remainder = arg_str0("r", "remainder", "<REMAINDER>", "A remainder address");
  send_args.message = arg_str0("m", "message", "<MESSAGE>", "a message for this transaction");
  send_args.tag = arg_str0("t", "tag", "<TAG>", "A tag for this transaction");
  send_args.end = arg_end(20);

  cli_cmd_t send_cmd = {
      .command = "send",
      .help = "send value or data to the Tangle.\n\tex: send [RECEIVER_ADDRESS] -v=100",
      .hint = " <receiver> -v <value> -m <message> -t <tag>",
      .func = &fn_send,
      .argtable = &send_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &send_cmd);
}

//==========GEN_HASH==========
uint8_t const tryte_chars[27] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                                 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '9'};

static struct {
  struct arg_int *len;
  struct arg_end *end;
} gen_hash_args;

static cli_err_t fn_gen_hash(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&gen_hash_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, gen_hash_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  int len = gen_hash_args.len->ival[0];
  char *hash = (char *)malloc(sizeof(char) * (len + 1));
  if (hash == NULL) {
    printf("Out of Memory\n");
    return CLI_ERR_OOM;
  }

  srand(time(0));
  for (int i = 0; i < len; i++) {
    memset(hash + i, tryte_chars[rand() % 27], 1);
  }
  hash[len] = '\0';

  printf("Hash: %s\n", hash);
  free(hash);
  return CLI_OK;
}

static void cmd_register_gen_hash() {
  gen_hash_args.len = arg_int1(NULL, NULL, "<lenght>", "a length for the hash");
  gen_hash_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "gen_hash",
      .help = "Genrating a hash with the given length.\n  `gen_hash 81` for a random SEED",
      .hint = " <length>",
      .func = &fn_gen_hash,
      .argtable = &gen_hash_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========GET_ADDRESSES==========
static struct {
  struct arg_str *start_idx;
  struct arg_str *end_idx;
  struct arg_end *end;
} get_addresses_args;

static cli_err_t fn_get_addresses(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_get_addresses() {
  get_addresses_args.start_idx = arg_str1(NULL, NULL, "<start>", "start index");
  get_addresses_args.end_idx = arg_str1(NULL, NULL, "<end>", "end index");
  get_addresses_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "get_addresses",
      .help = "Gets addresses by index",
      .hint = " <start> <end>",
      .func = &fn_get_addresses,
      .argtable = &get_addresses_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========END OF COMMANDS==========

cli_err_t cli_command_init() {
  // init command line paring buffer
  cli_ctx.parsing_buf = calloc(CLI_LINE_BUFFER, 1);
  if (cli_ctx.parsing_buf == NULL) {
    return CLI_ERR_OOM;
  }

  // create cmd list
  utarray_new(cli_ctx.cmd_array, &cli_cmd_icd);
  if (cli_ctx.cmd_array == NULL) {
    return CLI_ERR_OOM;
  }

  // registing commands
  cmd_register_help();
  cmd_register_version();
  cmd_register_node_info();
  cmd_register_node_info_set();
  cmd_register_client_conf();
  cmd_register_client_conf_set();
  cmd_register_seed();
  cmd_register_seed_set();
  cmd_register_account_data();
  cmd_register_get_balance();
  cmd_register_send();
  cmd_register_gen_hash();
  cmd_register_get_addresses();

  return cli_wallet_init();
}

cli_err_t cli_command_end() {
  wallet_destroy(cli_ctx.wallet);
  free(cli_ctx.parsing_buf);
  utarray_free(cli_ctx.cmd_array);
  return CLI_OK;
}

cli_err_t cli_command_run(char const *const cmdline, cli_err_t *cmd_ret) {
  if (cli_ctx.parsing_buf == NULL) {
    return CLI_ERR_NULL_POINTER;
  }

  char **argv = (char **)calloc(CLI_MAX_ARGC, sizeof(char *));
  if (argv == NULL) {
    return CLI_ERR_OOM;
  }

  strncpy(cli_ctx.parsing_buf, cmdline, CLI_LINE_BUFFER);

  // split command line
  size_t argc = esp_console_split_argv(cli_ctx.parsing_buf, argv, CLI_MAX_ARGC);
  if (argc == 0) {
    free(argv);
    return CLI_ERR_INVALID_ARG;
  }

  // run command
  cli_cmd_t *cmd_p = NULL;
  while ((cmd_p = (cli_cmd_t *)utarray_next(cli_ctx.cmd_array, cmd_p))) {
    if (!strcmp(cmd_p->command, argv[0])) {
      *cmd_ret = (*cmd_p->func)(argc, argv);
    }
  }
  free(argv);
  return CLI_OK;
}
