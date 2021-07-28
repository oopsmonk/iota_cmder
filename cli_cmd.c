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

static int update_node_config(iota_wallet_t *w, char const host[], uint32_t port, bool tls) {
  // set connected node
  if (wallet_set_endpoint(w, host, port, tls) != 0) {
    printf("set endpoint failed\n");
    return -1;
  }

  // get node info and update HRP prefix
  res_node_info_t *info = res_node_info_new();
  if (info == NULL) {
    return -2;
  }

  cli_err_t ret = get_node_info(&w->endpoint, info);
  if (ret != 0) {
    printf("get_node_info API failed: %s:%d, TSL: %s\n", w->endpoint.host, w->endpoint.port,
           w->endpoint.use_tls ? "true" : "false");
  } else {
    printf("Connected to %s:%d, TSL: %s\n", w->endpoint.host, w->endpoint.port, w->endpoint.use_tls ? "true" : "false");
    if (info->is_error) {
      printf("Node response: \n%s\n", info->u.error->msg);
      ret = -3;
    } else {
      printf("\tName: %s\n", info->u.output_node_info->name);
      printf("\tVersion: %s\n", info->u.output_node_info->version);
      printf("\tisHealthy: %s\n", info->u.output_node_info->is_healthy ? "true" : "false");
      printf("\tNetwork ID: %s\n", info->u.output_node_info->network_id);
      printf("\tbech32HRP: %s\n", info->u.output_node_info->bech32hrp);
      strncpy(w->bech32HRP, info->u.output_node_info->bech32hrp, sizeof(w->bech32HRP));
    }
  }

  res_node_info_free(info);
  return ret;
}

static cli_err_t cli_wallet_init() {
  byte_t seed[IOTA_SEED_BYTES] = {};
  printf("Init client application...\n");

  if (strncmp(WALLET_CONFIG_SEED, "RANDOM", strlen("RANDOM")) == 0) {
    printf("Use random a SEED\n");
    random_seed(seed);
  } else {
    size_t str_seed_len = strlen(WALLET_CONFIG_SEED);
    // TODO get seed from terminal if the seed is invalid.
    if (str_seed_len != 64) {
      printf("invalid seed length, it should be a 64-character-string\n");
      return CLI_ERR_FAILED;
    }

    if (hex_2_bin(WALLET_CONFIG_SEED, str_seed_len, seed, sizeof(seed)) != 0) {
      printf("invalid seed\n");
      return CLI_ERR_FAILED;
    }
  }

  if ((cli_ctx.wallet = wallet_create(seed, WALLET_CONFIG_PATH)) == NULL) {
    printf("create wallet instance failed\n");
    return CLI_ERR_FAILED;
  }

  if (update_node_config(cli_ctx.wallet, CLIENT_CONFIG_NODE, CLIENT_CONFIG_PORT, NODE_USE_TLS) != 0) {
    printf("connect to node failed\n");
    wallet_destroy(cli_ctx.wallet);
    cli_ctx.wallet = NULL;
    return CLI_ERR_FAILED;
  }
  printf("Init client application...done\n");
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
    printf("get_node_info failed\n");
  } else {
    if (info->is_error) {
      printf("Node response: \n%s\n", info->u.error->msg);
    } else {
      printf("Name: %s\n", info->u.output_node_info->name);
      printf("Version: %s\n", info->u.output_node_info->version);
      printf("isHealthy: %s\n", info->u.output_node_info->is_healthy ? "true" : "false");
      printf("Network ID: %s\n", info->u.output_node_info->network_id);
      printf("bech32HRP: %s\n", info->u.output_node_info->bech32hrp);
      printf("minPoWScore: %" PRIu64 "\n", info->u.output_node_info->min_pow_score);
      printf("Latest Milestone Index: %" PRIu64 "\n", info->u.output_node_info->latest_milestone_index);
      printf("Latest Milestone Timestamp: %" PRIu64 "\n", info->u.output_node_info->latest_milestone_timestamp);
      printf("Confirmed Milestone Index: %" PRIu64 "\n", info->u.output_node_info->confirmed_milestone_index);
      printf("Pruning Index: %" PRIu64 "\n", info->u.output_node_info->pruning_milestone_index);
      printf("MSP: %0.2f\n", info->u.output_node_info->msg_pre_sec);
      printf("Referenced MPS: %0.2f\n", info->u.output_node_info->referenced_msg_pre_sec);
      printf("Reference Rate: %0.2f%%\n", info->u.output_node_info->referenced_rate);
    }
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
  struct arg_str *host;
  struct arg_int *port;
  struct arg_int *is_https;
  struct arg_end *end;
} node_set_args;

static cli_err_t fn_node_set(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&node_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, node_set_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  // create a new wallet config
  iota_wallet_t w = {};
  memcpy(&w, cli_ctx.wallet, sizeof(iota_wallet_t));

  if (update_node_config(&w, node_set_args.host->sval[0], node_set_args.port->ival[0],
                         node_set_args.is_https->ival[0]) == 0) {
    // update wallet config
    memcpy(cli_ctx.wallet, &w, sizeof(iota_wallet_t));
  } else {
    printf("Node config is not updated.\n");
  }
  return CLI_OK;
}

static void cmd_register_node_set() {
  node_set_args.host = arg_str1(NULL, NULL, "<host>", "hostname");
  node_set_args.port = arg_int1(NULL, NULL, "<port>", "port number");
  node_set_args.is_https = arg_int1(NULL, NULL, "<is_https>", "0 or 1");
  node_set_args.end = arg_end(5);

  cli_cmd_t cmd = {
      .command = "node_set",
      .help = "Set connected node",
      .hint = " <host> <port> <is_https (0|1)> ",
      .func = &fn_node_set,
      .argtable = &node_set_args,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========CLIENT_CONF==========
static cli_err_t fn_node_conf(int argc, char **argv) {
  printf("%s:%d, TLS: %s\n", cli_ctx.wallet->endpoint.host, cli_ctx.wallet->endpoint.port,
         cli_ctx.wallet->endpoint.use_tls ? "true" : "false");
  return CLI_OK;
}

static void cmd_register_node_conf() {
  cli_cmd_t cmd = {
      .command = "node_conf",
      .help = "Show connected node configuration",
      .hint = NULL,
      .func = &fn_node_conf,
      .argtable = NULL,
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
  seed_set_args.seed = arg_str1(NULL, NULL, "<seed>", "A 64-character-string");
  seed_set_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "seed_set",
      .help = "Set the SEED of this wallet",
      .hint = " <seed>",
      .func = &fn_seed_set,
      .argtable = &seed_set_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

//==========GET_ADDRESSES==========
static struct {
  struct arg_str *start_idx;
  struct arg_str *count;
  struct arg_end *end;
} get_addresses_args;

static cli_err_t fn_get_addresses(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void cmd_register_get_addresses() {
  get_addresses_args.start_idx = arg_str1(NULL, NULL, "<start>", "start index");
  get_addresses_args.count = arg_str1(NULL, NULL, "<count>", "number of addresses");
  get_addresses_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "address",
      .help = "Gets address by index",
      .hint = " <start> <count>",
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

  // configuration
  cmd_register_node_set();
  cmd_register_node_conf();

  // client APIs
  cmd_register_node_info();

  // wallet APIs
  cmd_register_seed();
  cmd_register_seed_set();
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
