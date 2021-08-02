#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "argtable3.h"
#include "cli_cmd.h"
#include "utarray.h"

#include "client/api/v1/find_message.h"
#include "client/api/v1/get_balance.h"
#include "client/api/v1/get_message.h"
#include "client/api/v1/get_message_children.h"
#include "client/api/v1/get_message_metadata.h"
#include "client/api/v1/get_node_info.h"
#include "client/api/v1/get_output.h"
#include "client/api/v1/get_outputs_from_address.h"
#include "client/api/v1/get_tips.h"
#include "client/api/v1/send_message.h"
#include "core/utils/byte_buffer.h"
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
    printf("using a random SEED\n");
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

/* 'help' command */
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

static void register_help() {
  cli_cmd_t cmd = {
      .command = "help",
      .help = "Show this help",
      .hint = NULL,
      .func = &fn_help,
      .argtable = NULL,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'version' command */
static cli_err_t fn_version(int argc, char **argv) {
  printf("TODO\n");
  return CLI_OK;
}

static void register_version() {
  cli_cmd_t cmd = {
      .command = "version",
      .help = "Show version info",
      .hint = NULL,
      .func = &fn_version,
      .argtable = NULL,
  };

  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'node_info' command */
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

static void register_node_info() {
  cli_cmd_t cmd = {
      .command = "node_info",
      .help = "Shows node info",
      .hint = NULL,
      .func = &fn_node_info,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'info_set' command */
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

static void register_node_set() {
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

/* 'node_conf' command */
static cli_err_t fn_node_conf(int argc, char **argv) {
  printf("Host: %s:%d, TLS: %s\n", cli_ctx.wallet->endpoint.host, cli_ctx.wallet->endpoint.port,
         cli_ctx.wallet->endpoint.use_tls ? "true" : "false");
  printf("HRP: %s\n", cli_ctx.wallet->bech32HRP);
  return CLI_OK;
}

static void register_node_conf() {
  cli_cmd_t cmd = {
      .command = "node_conf",
      .help = "Show connected node configuration",
      .hint = NULL,
      .func = &fn_node_conf,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'seed' command */
static cli_err_t fn_seed(int argc, char **argv) {
  dump_hex_str(cli_ctx.wallet->seed, IOTA_SEED_BYTES);
  return CLI_OK;
}

static void register_seed() {
  cli_cmd_t cmd = {
      .command = "seed",
      .help = "Show SEED",
      .hint = NULL,
      .func = &fn_seed,
      .argtable = NULL,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'seed_set' command */
static struct {
  struct arg_str *seed;
  struct arg_end *end;
} seed_set_args;

static cli_err_t fn_seed_set(int argc, char **argv) {
  byte_t new_seed[IOTA_SEED_BYTES] = {};

  int nerrors = arg_parse(argc, argv, (void **)&seed_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, seed_set_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  char const *const seed_in = seed_set_args.seed->sval[0];
  size_t len = strlen(seed_in);
  if (len != IOTA_SEED_HEX_BYTES) {
    printf("SEED is a %d-character string, the input length is %zu\n", IOTA_SEED_HEX_BYTES, len);
    return CLI_ERR_INVALID_ARG;
  }

  if (hex_2_bin(seed_in, len, new_seed, sizeof(new_seed)) == 0) {
    // update seed
    memcpy(cli_ctx.wallet->seed, new_seed, IOTA_SEED_BYTES);
  } else {
    printf("Convert hex string to binary failed\n");
    return -1;
  }

  return CLI_OK;
}

static void register_seed_set() {
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

/* 'address' command */
static struct {
  struct arg_dbl *start_idx;
  struct arg_dbl *count;
  struct arg_end *end;
} get_addresses_args;

static cli_err_t fn_get_addresses(int argc, char **argv) {
  byte_t addr_with_version[IOTA_ADDRESS_BYTES] = {};
  char tmp_bech32_addr[100] = {};
  int nerrors = arg_parse(argc, argv, (void **)&get_addresses_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, get_addresses_args.end, argv[0]);
    return -1;
  }
  uint32_t start = (uint32_t)get_addresses_args.start_idx->dval[0];
  uint32_t count = (uint32_t)get_addresses_args.count->dval[0];

  for (uint32_t i = start; i < start + count; i++) {
    addr_with_version[0] = ADDRESS_VER_ED25519;
    nerrors = wallet_address_by_index(cli_ctx.wallet, i, addr_with_version + 1);
    if (nerrors != 0) {
      printf("wallet_address_by_index error\n");
      break;
    } else {
      if (address_2_bech32(addr_with_version, cli_ctx.wallet->bech32HRP, tmp_bech32_addr) == 0) {
        printf("Addr[%" PRIu32 "]\n", i);
        // print ed25519 address without version.
        printf("\t");
        dump_hex_str(addr_with_version + 1, ED25519_ADDRESS_BYTES);
        printf("\t%s\n", tmp_bech32_addr);
      }
    }
  }
  return CLI_OK;
}

static void register_get_addresses() {
  get_addresses_args.start_idx = arg_dbl1(NULL, NULL, "<start>", "start index");
  get_addresses_args.count = arg_dbl1(NULL, NULL, "<count>", "number of addresses");
  get_addresses_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "address",
      .help = "Get addresses from an index",
      .hint = " <start> <count>",
      .func = &fn_get_addresses,
      .argtable = &get_addresses_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_msg_index' command */
static struct {
  struct arg_str *index;
  struct arg_end *end;
} api_find_msg_index_args;

static cli_err_t fn_api_find_msg_index(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_find_msg_index_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_find_msg_index_args.end, argv[0]);
    return -1;
  }

  res_find_msg_t *res = res_find_msg_new();
  if (!res) {
    printf("new res_find_msg_t failed\n");
    return -2;
  }

  int err = find_message_by_index(&cli_ctx.wallet->endpoint, api_find_msg_index_args.index->sval[0], res);
  if (err) {
    printf("find message API failed\n");
  } else {
    if (res->is_error) {
      printf("%s\n", res->u.error->msg);
    } else {
      size_t count = res_find_msg_get_id_len(res);
      for (size_t i = 0; i < count; i++) {
        printf("%s\n", res_find_msg_get_id(res, i));
      }
      printf("message ID count %zu\n", count);
    }
  }

  res_find_msg_free(res);
  return err;
}

static void register_api_find_msg_index() {
  api_find_msg_index_args.index = arg_str1(NULL, NULL, "<index>", "Index string");
  api_find_msg_index_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_msg_index",
      .help = "Find messages from a given index",
      .hint = " <index>",
      .func = &fn_api_find_msg_index,
      .argtable = &api_find_msg_index_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_get_balance' command */
static struct {
  struct arg_str *addr;
  struct arg_end *end;
} api_get_balance_args;

static int fn_api_get_balance(int argc, char **argv) {
  char hex_addr[IOTA_ADDRESS_HEX_BYTES + 1] = {};
  int nerrors = arg_parse(argc, argv, (void **)&api_get_balance_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_get_balance_args.end, argv[0]);
    return -1;
  }

  char const *const bech32_add_str = api_get_balance_args.addr->sval[0];
  if (strncmp(bech32_add_str, cli_ctx.wallet->bech32HRP, strlen(cli_ctx.wallet->bech32HRP)) != 0) {
    printf("Invalid address hash\n");
    return -2;
  } else {
    // bech32 address to hex string
    if (address_bech32_to_hex(cli_ctx.wallet->bech32HRP, bech32_add_str, hex_addr, sizeof(hex_addr)) != 0) {
      printf("Convert ed25519 address to hex string failed\n");
      return -3;
    }

    // get balance from connected node
    res_balance_t *res = res_balance_new();
    if (!res) {
      printf("Create res_balance_t object failed\n");
      return -4;
    } else {
      nerrors = get_balance(&cli_ctx.wallet->endpoint, hex_addr, res);
      if (nerrors != 0) {
        printf("get_balance API failed\n");
      } else {
        if (res->is_error) {
          printf("Err: %s\n", res->u.error->msg);
        } else {
          printf("balance: %" PRIu64 "\n", res->u.output_balance->balance);
        }
      }
      res_balance_free(res);
    }
  }
  return nerrors;
}

static void register_api_get_balance() {
  api_get_balance_args.addr = arg_str1(NULL, NULL, "<address>", "Address HASH");
  api_get_balance_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_get_balance",
      .help = "Get balance from a given address",
      .hint = " <address>",
      .func = &fn_api_get_balance,
      .argtable = &api_get_balance_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_msg_children' command */
static struct {
  struct arg_str *msg_id;
  struct arg_end *end;
} api_msg_children_args;

static int fn_api_msg_children(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_msg_children_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_msg_children_args.end, argv[0]);
    return -1;
  }

  // check message id length
  char const *const msg_id_str = api_msg_children_args.msg_id->sval[0];
  if (strlen(msg_id_str) != IOTA_MESSAGE_ID_HEX_BYTES) {
    printf("Invalid message ID length\n");
    return -2;
  }

  res_msg_children_t *res = res_msg_children_new();
  if (!res) {
    printf("Allocate response failed\n");
    return -3;
  } else {
    nerrors = get_message_children(&cli_ctx.wallet->endpoint, msg_id_str, res);
    if (nerrors) {
      printf("get_message_children error %d\n", nerrors);
    } else {
      if (res->is_error) {
        printf("Err: %s\n", res->u.error->msg);
      } else {
        size_t count = res_msg_children_len(res);
        if (count == 0) {
          printf("Message not found\n");
        } else {
          for (size_t i = 0; i < count; i++) {
            printf("%s\n", res_msg_children_get(res, i));
          }
        }
      }
    }
    res_msg_children_free(res);
  }

  return nerrors;
}

static void register_api_msg_children() {
  api_msg_children_args.msg_id = arg_str1(NULL, NULL, "<ID>", "Message ID");
  api_msg_children_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_msg_children",
      .help = "Get children from a given message ID",
      .hint = " <ID>",
      .func = &fn_api_msg_children,
      .argtable = &api_msg_children_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_msg_meta' command */
static struct {
  struct arg_str *msg_id;
  struct arg_end *end;
} api_msg_meta_args;

static int fn_api_msg_meta(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_msg_meta_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_msg_meta_args.end, argv[0]);
    return -1;
  }

  // check message id length
  char const *const msg_id_str = api_msg_meta_args.msg_id->sval[0];
  if (strlen(msg_id_str) != IOTA_MESSAGE_ID_HEX_BYTES) {
    printf("Invalid message ID length\n");
    return -2;
  }

  res_msg_meta_t *res = res_msg_meta_new();
  if (!res) {
    printf("Allocate response failed\n");
    return -3;
  } else {
    nerrors = get_message_metadata(&cli_ctx.wallet->endpoint, msg_id_str, res);
    if (nerrors) {
      printf("get_message_metadata error %d\n", nerrors);
    } else {
      if (res->is_error) {
        printf("%s\n", res->u.error->msg);
      } else {
        printf("Message ID: %s\nisSolid: %s\n", res->u.meta->msg_id, res->u.meta->is_solid ? "True" : "False");
        size_t parents = res_msg_meta_parents_len(res);
        printf("%zu parents:\n", parents);
        for (size_t i = 0; i < parents; i++) {
          printf("\t%s\n", res_msg_meta_parent_get(res, i));
        }
        printf("ledgerInclusionState: %s\n", res->u.meta->inclusion_state);

        // check milestone index
        if (res->u.meta->milestone_idx != 0) {
          printf("milestoneIndex: %" PRIu64 "\n", res->u.meta->milestone_idx);
        }

        // check referenced milestone index
        if (res->u.meta->referenced_milestone != 0) {
          printf("referencedByMilestoneIndex: %" PRIu64 "\n", res->u.meta->referenced_milestone);
        }

        // check should promote
        if (res->u.meta->should_promote >= 0) {
          printf("shouldPromote: %s\n", res->u.meta->should_promote ? "True" : "False");
        }
        // check should reattach
        if (res->u.meta->should_reattach >= 0) {
          printf("shouldReattach: %s\n", res->u.meta->should_reattach ? "True" : "False");
        }
      }
    }
    res_msg_meta_free(res);
  }
  return nerrors;
}

static void register_api_msg_meta() {
  api_msg_meta_args.msg_id = arg_str1(NULL, NULL, "<Message ID>", "Message ID");
  api_msg_meta_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_msg_meta",
      .help = "Get metadata from a given message ID",
      .hint = " <Message ID>",
      .func = &fn_api_msg_meta,
      .argtable = &api_msg_meta_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_address_outputs' command */
static struct {
  struct arg_str *addr;
  struct arg_end *end;
} api_address_outputs_args;

static int fn_api_address_outputs(int argc, char **argv) {
  char hex_addr[IOTA_ADDRESS_HEX_BYTES + 1] = {};
  int nerrors = arg_parse(argc, argv, (void **)&api_address_outputs_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_address_outputs_args.end, argv[0]);
    return -1;
  }

  // check address
  char const *const bech32_add_str = api_address_outputs_args.addr->sval[0];
  if (strncmp(bech32_add_str, cli_ctx.wallet->bech32HRP, strlen(cli_ctx.wallet->bech32HRP)) != 0) {
    printf("Invalid address hash\n");
    return -2;
  }

  // bech32 address to hex string
  if (address_bech32_to_hex(cli_ctx.wallet->bech32HRP, bech32_add_str, hex_addr, sizeof(hex_addr)) != 0) {
    printf("Convert address failed\n");
    return -3;
  }

  res_outputs_address_t *res = res_outputs_address_new();
  if (!res) {
    printf("Allocate res_outputs_address_t failed\n");
    return -4;
  } else {
    nerrors = get_outputs_from_address(&cli_ctx.wallet->endpoint, hex_addr, res);
    if (nerrors != 0) {
      printf("get_outputs_from_address error\n");
    } else {
      if (res->is_error) {
        printf("%s\n", res->u.error->msg);
      } else {
        printf("Output IDs:\n");
        for (uint32_t i = 0; i < res_outputs_address_output_id_count(res); i++) {
          printf("%s\n", res_outputs_address_output_id(res, i));
        }
      }
    }

    res_outputs_address_free(res);
  }

  return nerrors;
}

static void register_api_address_outputs() {
  api_address_outputs_args.addr = arg_str1(NULL, NULL, "<Address>", "Address hash");
  api_address_outputs_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_address_outputs",
      .help = "Get output ID list from a given address",
      .hint = " <Address>",
      .func = &fn_api_address_outputs,
      .argtable = &api_address_outputs_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_get_output' command */
static struct {
  struct arg_str *output_id;
  struct arg_end *end;
} api_get_output_args;

static int fn_api_get_output(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_get_output_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_get_output_args.end, argv[0]);
    return -1;
  }

  res_output_t res = {};
  nerrors = get_output(&cli_ctx.wallet->endpoint, api_get_output_args.output_id->sval[0], &res);
  if (nerrors != 0) {
    printf("get_output error\n");
    return -2;
  } else {
    if (res.is_error) {
      printf("%s\n", res.u.error->msg);
      res_err_free(res.u.error);
    } else {
      dump_output_response(&res);
    }
  }

  return nerrors;
}

static void register_api_get_output() {
  api_get_output_args.output_id = arg_str1(NULL, NULL, "<Output ID>", "An output ID");
  api_get_output_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_get_output",
      .help = "Get the output object from a given output ID",
      .hint = " <Output ID>",
      .func = &fn_api_get_output,
      .argtable = &api_get_output_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_tips' command */
static int fn_api_tips(int argc, char **argv) {
  int err = 0;
  res_tips_t *res = res_tips_new();
  if (!res) {
    printf("Allocate tips object failed\n");
    return -1;
  }

  err = get_tips(&cli_ctx.wallet->endpoint, res);
  if (err != 0) {
    printf("get_tips error\n");
  } else {
    if (res->is_error) {
      printf("%s\n", res->u.error->msg);
    } else {
      for (size_t i = 0; i < get_tips_id_count(res); i++) {
        printf("%s\n", get_tips_id(res, i));
      }
    }
  }

  res_tips_free(res);
  return err;
}

static void register_api_tips() {
  cli_cmd_t cmd = {
      .command = "api_tips",
      .help = "Get tips from connected node",
      .hint = NULL,
      .func = &fn_api_tips,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_get_output' command */

static void dump_index_payload(payload_index_t *idx) {
  // dump Indexaction message

  byte_buf_t *index_str = byte_buf_hex2str(idx->index);
  byte_buf_t *data_str = byte_buf_hex2str(idx->data);
  if (index_str != NULL && data_str != NULL) {
    printf("Index: %s\n\t%s\n", idx->index->data, index_str->data);
    printf("Data: %s\n\t%s\n", idx->data->data, data_str->data);
  } else {
    printf("buffer allocate failed\n");
  }
  byte_buf_free(index_str);
  byte_buf_free(data_str);
}

static void dump_tx_payload(payload_tx_t *tx) {
  char temp_addr[128] = {};
  byte_t addr[IOTA_ADDRESS_BYTES] = {};
  byte_t pub_key_bin[ED_PUBLIC_KEY_BYTES] = {};

  // inputs
  printf("Inputs:\n");
  for (size_t i = 0; i < payload_tx_inputs_count(tx); i++) {
    printf("\ttx ID[%zu]: %s\n\ttx output index[%zu]: %" PRIu32 "\n", i, payload_tx_inputs_tx_id(tx, i), i,
           payload_tx_inputs_tx_output_index(tx, i));

    // get input address from public key
    if (hex_2_bin(payload_tx_blocks_public_key(tx, payload_tx_inputs_tx_output_index(tx, i) - 1),
                  API_PUB_KEY_HEX_STR_LEN, pub_key_bin, ED_PUBLIC_KEY_BYTES) == 0) {
      if (address_from_ed25519_pub(pub_key_bin, addr + 1) == 0) {
        addr[0] = ADDRESS_VER_ED25519;
        // address bin to bech32 hex string
        if (address_2_bech32(addr, cli_ctx.wallet->bech32HRP, temp_addr) == 0) {
          printf("\taddress[%zu]: %s\n", i, temp_addr);
        } else {
          printf("convert address to bech32 error\n");
        }
      } else {
        printf("get address from public key error\n");
      }
    } else {
      printf("convert pub key to binary failed\n");
    }
  }

  // outputs
  printf("Outputs:\n");
  for (size_t i = 0; i < payload_tx_outputs_count(tx); i++) {
    addr[0] = ADDRESS_VER_ED25519;
    // address hex to bin
    if (hex_2_bin(payload_tx_outputs_address(tx, i), IOTA_ADDRESS_HEX_BYTES + 1, addr + 1, ED25519_ADDRESS_BYTES) ==
        0) {
      // address bin to bech32
      if (address_2_bech32(addr, cli_ctx.wallet->bech32HRP, temp_addr) == 0) {
        printf("\tAddress[%zu]: %s\n\tAmount[%zu]: %" PRIu64 "\n", i, temp_addr, i, payload_tx_outputs_amount(tx, i));
      } else {
        printf("[%s:%d] converting bech32 address failed\n", __FILE__, __LINE__);
      }
    } else {
      printf("[%s:%d] converting binary address failed\n", __FILE__, __LINE__);
    }
  }

  // unlock blocks
  printf("Unlock blocks:\n");
  for (size_t i = 0; i < payload_tx_blocks_count(tx); i++) {
    printf("\tPublic Key[%zu]: %s\n\tSignature[%zu]: %s\n", i, payload_tx_blocks_public_key(tx, i), i,
           payload_tx_blocks_signature(tx, i));
  }

  // payload?
  if (tx->payload != NULL && tx->type == MSG_PAYLOAD_INDEXATION) {
    dump_index_payload((payload_index_t *)tx->payload);
  }
}

static struct {
  struct arg_str *index;
  struct arg_str *data;
  struct arg_end *end;
} api_send_msg_args;

static int fn_api_send_msg(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_send_msg_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_send_msg_args.end, argv[0]);
    return -1;
  }
  // send indexaction payload
  res_send_message_t res = {};
  nerrors = send_indexation_msg(&cli_ctx.wallet->endpoint, api_send_msg_args.index->sval[0],
                                api_send_msg_args.data->sval[0], &res);
  if (nerrors != 0) {
    printf("send_indexation_msg error\n");
  } else {
    if (res.is_error) {
      printf("%s\n", res.u.error->msg);
      res_err_free(res.u.error);
    } else {
      printf("Message ID: %s\n", res.u.msg_id);
    }
  }
  return nerrors;
}

static void register_api_send_msg() {
  api_send_msg_args.index = arg_str1(NULL, NULL, "<Index>", "Message Index");
  api_send_msg_args.data = arg_str1(NULL, NULL, "<Data>", "Message data");
  api_send_msg_args.end = arg_end(3);
  cli_cmd_t cmd = {
      .command = "api_send_msg",
      .help = "Send out a data message to the Tangle",
      .hint = " <Index> <Data>",
      .func = &fn_api_send_msg,
      .argtable = &api_send_msg_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'api_get_msg' command */
static struct {
  struct arg_str *msg_id;
  struct arg_end *end;
} api_get_msg_args;

static int fn_api_get_msg(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&api_get_msg_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, api_get_msg_args.end, argv[0]);
    return -1;
  }

  res_message_t *res = res_message_new();
  if (res == NULL) {
    return CLI_ERR_OOM;
  }

  nerrors = get_message_by_id(&cli_ctx.wallet->endpoint, api_get_msg_args.msg_id->sval[0], res);
  if (nerrors == 0) {
    if (res->is_error) {
      printf("%s\n", res->u.error->msg);
    } else {
      message_t *msg = res->u.msg;
      printf("Network ID: %s\n", msg->net_id);
      printf("Parent Message ID:\n");
      for (size_t i = 0; i < api_message_parent_count(msg); i++) {
        printf("\t%s\n", api_message_parent_id(msg, i));
      }
      if (msg->type == MSG_PAYLOAD_INDEXATION) {
        dump_index_payload((payload_index_t *)msg->payload);
      } else if (msg->type == MSG_PAYLOAD_TRANSACTION) {
        dump_tx_payload((payload_tx_t *)msg->payload);
      } else {
        printf("TODO: payload type: %d\n", msg->type);
      }
    }
  } else {
    printf("get_message_by_id API error\n");
  }

  res_message_free(res);
  return nerrors;
}

static void register_api_get_msg() {
  api_get_msg_args.msg_id = arg_str1(NULL, NULL, "<Message ID>", "Message ID");
  api_get_msg_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "api_get_msg",
      .help = "Get a message from a given message ID",
      .hint = " <Message ID>",
      .func = &fn_api_get_msg,
      .argtable = &api_get_msg_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'balance' command */
static struct {
  struct arg_dbl *idx_start;
  struct arg_dbl *idx_count;
  struct arg_end *end;
} get_balance_args;

static int fn_get_balance(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&get_balance_args);
  uint64_t balance = 0;
  if (nerrors != 0) {
    arg_print_errors(stderr, get_balance_args.end, argv[0]);
    return -1;
  }

  uint32_t start = get_balance_args.idx_start->dval[0];
  uint32_t count = get_balance_args.idx_count->dval[0];

  for (uint32_t i = start; i < start + count; i++) {
    if (wallet_balance_by_index(cli_ctx.wallet, i, &balance) != 0) {
      printf("Err: get balance failed on index %u\n", i);
      return -2;
    }
    printf("balance on address [%" PRIu32 "]: %" PRIu64 "i\n", i, balance);
  }
  return 0;
}

static void register_get_balance() {
  get_balance_args.idx_start = arg_dbl1(NULL, NULL, "<start>", "start index");
  get_balance_args.idx_count = arg_dbl1(NULL, NULL, "<count>", "number of address");
  get_balance_args.end = arg_end(2);
  cli_cmd_t cmd = {
      .command = "balance",
      .help = "Get the balance from a range of address index",
      .hint = " <start> <count>",
      .func = &fn_get_balance,
      .argtable = &get_balance_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &cmd);
}

/* 'send' command */
static struct {
  struct arg_dbl *sender;
  struct arg_str *receiver;
  struct arg_dbl *balance;
  struct arg_end *end;
} send_msg_args;

static int fn_send_msg(int argc, char **argv) {
  char msg_id[IOTA_MESSAGE_ID_HEX_BYTES + 1] = {};
  char data[] = "sent from iota_comder";
  int nerrors = arg_parse(argc, argv, (void **)&send_msg_args);
  byte_t recv[IOTA_ADDRESS_BYTES] = {};
  if (nerrors != 0) {
    arg_print_errors(stderr, send_msg_args.end, argv[0]);
    return -1;
  }

  char const *const recv_addr = send_msg_args.receiver->sval[0];
  // validating receiver address
  if (strncmp(recv_addr, cli_ctx.wallet->bech32HRP, strlen(cli_ctx.wallet->bech32HRP)) == 0) {
    // convert bech32 address to binary
    if ((nerrors = address_from_bech32(cli_ctx.wallet->bech32HRP, recv_addr, recv))) {
      printf("invalid bech32 address\n");
      return -2;
    }
  } else if (strlen(recv_addr) == IOTA_ADDRESS_HEX_BYTES) {
    // convert ed25519 string to binary
    if (hex_2_bin(recv_addr, strlen(recv_addr), recv + 1, ED25519_ADDRESS_BYTES) != 0) {
      printf("invalid ed25519 address\n");
      return -3;
    }

  } else {
    printf("invalid receiver address\n");
    return -4;
  }

  // balance = number * Mi
  uint64_t balance = (uint64_t)send_msg_args.balance->dval[0] * 1000000;

  if (balance > 0) {
    printf("send %" PRIu64 "Mi to %s\n", (uint64_t)send_msg_args.balance->dval[0], recv_addr);
  } else {
    printf("send indexation payload to tangle\n");
  }

  nerrors = wallet_send(cli_ctx.wallet, (uint32_t)send_msg_args.sender->dval[0], recv + 1, balance, "iota_comder",
                        (byte_t *)data, sizeof(data), msg_id, sizeof(msg_id));
  if (nerrors) {
    printf("send message failed\n");
    return -5;
  }
  printf("Message Hash: %s\n", msg_id);
  return nerrors;
}

static void register_send_tokens() {
  send_msg_args.sender = arg_dbl1(NULL, NULL, "<index>", "Address index");
  send_msg_args.receiver = arg_str1(NULL, NULL, "<receiver>", "Receiver address");
  send_msg_args.balance = arg_dbl1(NULL, NULL, "<balance>", "balance");
  send_msg_args.end = arg_end(20);
  cli_cmd_t cmd = {
      .command = "send",
      .help = "send message to tangle",
      .hint = " <addr_index> <receiver> <balance>",
      .func = &fn_send_msg,
      .argtable = &send_msg_args,
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
  register_help();
  register_version();

  // configuration
  register_node_set();
  register_node_conf();

  // client APIs
  register_node_info();
  register_api_find_msg_index();
  register_api_get_balance();
  register_api_msg_children();
  register_api_msg_meta();
  register_api_address_outputs();
  register_api_get_output();
  register_api_tips();
  register_api_send_msg();
  register_api_get_msg();

  // wallet APIs
  register_seed();
  register_seed_set();
  register_get_addresses();
  register_get_balance();
  register_send_tokens();

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
