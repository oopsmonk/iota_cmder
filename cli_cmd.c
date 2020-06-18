#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include "argtable3.h"
#include "cli_cmd.h"

#include "cclient/api/core/core_api.h"
#include "cclient/api/extended/extended_api.h"
#include "common/helpers/sign.h"
#include "utils/input_validators.h"
#include "utils/macros.h"

#define CMDER_VERSION_MAJOR 0
#define CMDER_VERSION_MINOR 0
#define CMDER_VERSION_MICRO 1

#define CMDER_VERSION          \
  VER_STR(CMDER_VERSION_MAJOR) \
  "." VER_STR(CMDER_VERSION_MINOR) "." VER_STR(CMDER_VERSION_MICRO)

typedef struct {
  uint32_t depth;                     /*!< number of bundles to go back to determine the transactions for approval. */
  uint8_t mwm;                        /*!< Minimum Weight Magnitude for doing Proof-of-Work */
  uint8_t security;                   /*!< security level of addresses, value could be 1,2,3. */
  iota_client_service_t *iota_client; /*!< iota client service */
  char *parsing_buf;                  /*!< buffer for command line parsing */
  char *seed;                         /*!< seed */
  UT_array *cmd_array;                /*!< an array of registed commands */
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

static cli_err_t iota_client_init() {
#ifdef CLIENT_CONFIG_HTTPS
  cli_ctx.iota_client = iota_client_core_init(CLIENT_CONFIG_NODE, CLIENT_CONFIG_PORT, amazon_ca1_pem);
#else
  cli_ctx.iota_client = iota_client_core_init(CLIENT_CONFIG_NODE, CLIENT_CONFIG_PORT, NULL);
#endif

  if (cli_ctx.iota_client == NULL) {
    return CLI_ERR_OOM;
  }

#ifdef IOTA_CLIENT_DEBUG
  // init logger
  logger_helper_init(LOGGER_DEBUG);
  logger_init_client_core(LOGGER_DEBUG);
  logger_init_client_extended(LOGGER_WARNING);
  logger_init_json_serializer(LOGGER_WARNING);
#endif
  printf("init endpoint: %s:%u %s\n", cli_ctx.iota_client->http.host, cli_ctx.iota_client->http.port,
         cli_ctx.iota_client->http.ca_pem ? "with TLS" : "");
  return CLI_OK;
}

static void iota_client_destroy() {
  iota_client_core_destroy(&cli_ctx.iota_client);
#ifdef IOTA_CLIENT_DEBUG
  // cleanup logger
  logger_destroy_client_core();
  logger_destroy_client_extended();
  logger_destroy_json_serializer();
  logger_helper_destroy();
#endif
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
  UNUSED(argc);
  UNUSED(argv);
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
  UNUSED(argc);
  UNUSED(argv);
  cli_cmd_t *cmd_p = NULL;

  printf("IOTA_COMMON_VERSION: %s, IOTA_CLIENT_VERSION: %s\n", IOTA_COMMON_VERSION, CCLIENT_VERSION);
  printf("APP_VERSION: %s\n", CMDER_VERSION);
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
  UNUSED(argc);
  UNUSED(argv);

  retcode_t ret = RC_ERROR;
  get_node_info_res_t *node_res = get_node_info_res_new();
  if (node_res == NULL) {
    printf("Error: OOM\n");
    return CLI_ERR_OOM;
  }

  if ((ret = iota_client_get_node_info(cli_ctx.iota_client, node_res)) == RC_OK) {
    printf("appName: %s \n", get_node_info_res_app_name(node_res));
    printf("appVersion: %s \n", get_node_info_res_app_version(node_res));

    printf("latestMilestone: ");
    flex_trit_print(node_res->latest_milestone, NUM_TRITS_HASH);
    printf("\n");

    printf("latestMilestoneIndex: %u \n", node_res->latest_milestone_index);

    printf("latestSolidSubtangleMilestone: ");
    flex_trit_print(node_res->latest_solid_subtangle_milestone, NUM_TRITS_HASH);
    printf("\n");

    printf("latestSolidSubtangleMilestoneIndex: %u \n", node_res->latest_solid_subtangle_milestone_index);
    printf("neighbors: %d \n", node_res->neighbors);
    printf("packetsQueueSize: %d \n", node_res->packets_queue_size);
    printf("time: %" PRIu64 " \n", node_res->time);
    printf("tips: %d \n", node_res->tips);
    printf("transactionsToRequest: %d \n", node_res->transactions_to_request);
    printf("isSynced: %s \n", node_res->is_synced ? "true" : "false");

    // print out features
    printf("features: ");
    size_t num_features = get_node_info_req_features_num(node_res);
    for (; num_features > 0; num_features--) {
      printf("%s, ", get_node_info_res_features_at(node_res, num_features - 1));
    }
    printf("\n");

    // print out the coordinator address
    printf("coordinatorAddress: ");
    flex_trit_print(node_res->coordinator_address, NUM_TRITS_ADDRESS);
    printf("\n");

    printf("Client Configuration: MWM %d, Depth %d, Security %d\n", cli_ctx.mwm, cli_ctx.depth, cli_ctx.security);

  } else {
    printf("Error: %s", error_2_string(ret));
  }

  get_node_info_res_free(&node_res);
  return CLI_OK;
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
  int nerrors = arg_parse(argc, argv, (void **)&node_info_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, node_info_set_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }
  char const *url = node_info_set_args.url->sval[0];
  int port = node_info_set_args.port->ival[0];
  int is_https = node_info_set_args.is_https->ival[0];

  printf("%s, port %d, https %d\n", url, port, is_https);
  iota_client_service_t *service = iota_client_core_init(url, port, is_https ? amazon_ca1_pem : NULL);
  if (service == NULL) {
    return CLI_ERR_OOM;
  }

  get_node_info_res_t *node_res = get_node_info_res_new();
  if (node_res == NULL) {
    printf("Error: OOM\n");
    return CLI_ERR_OOM;
  }

  if (iota_client_get_node_info(service, node_res) == RC_OK) {
    iota_client_core_destroy(&cli_ctx.iota_client);
    cli_ctx.iota_client = service;
  } else {
    iota_client_core_destroy(&service);
  }

  get_node_info_res_free(&node_res);
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
  UNUSED(argc);
  UNUSED(argv);
  printf("MWM %d, Depth %d, Security %d\n", cli_ctx.mwm, cli_ctx.depth, cli_ctx.security);
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

  int security = client_conf_set_args.security->ival[0];
  if (!is_security_level(security)) {
    printf("Invalid Security level.\n");
    return CLI_ERR_INVALID_ARG;
  }

  cli_ctx.security = security;
  cli_ctx.mwm = client_conf_set_args.mwm->ival[0];
  cli_ctx.depth = client_conf_set_args.depth->ival[0];
  printf("Set: MWM %d, Depth %d, Security %d\n", cli_ctx.mwm, cli_ctx.depth, cli_ctx.security);

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
  UNUSED(argc);
  UNUSED(argv);
  printf("%s\n", cli_ctx.seed);
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
  int nerrors = arg_parse(argc, argv, (void **)&seed_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, seed_set_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  char *seed = (char *)seed_set_args.seed->sval[0];
  to_uppercase(seed, strlen(seed));
  if (!is_seed((tryte_t *)seed)) {
    printf("Invalid SEED hash(%ld), expect %d\n", strlen(seed), NUM_TRYTES_HASH);
    return CLI_ERR_INVALID_ARG;
  }

  strncpy(cli_ctx.seed, seed, NUM_TRYTES_HASH);

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
  UNUSED(argc);
  UNUSED(argv);
  retcode_t ret = RC_ERROR;
  flex_trit_t flex_seed[FLEX_TRIT_SIZE_243];

  if (flex_trits_from_trytes(flex_seed, NUM_TRITS_ADDRESS, (tryte_t *)cli_ctx.seed, NUM_TRYTES_ADDRESS,
                             NUM_TRYTES_ADDRESS) == 0) {
    printf("Err: converting seed to flex_trit failed");
    return CLI_ERR_FAILED;
  }

  // init account data
  account_data_t account = {};
  account_data_init(&account);

  if ((ret = iota_client_get_account_data(cli_ctx.iota_client, flex_seed, cli_ctx.security, &account)) == RC_OK) {
#if 0  // dump transaction hashes
    size_t tx_count = hash243_queue_count(account.transactions);
    for (size_t i = 0; i < tx_count; i++) {
      printf("[%zu]: ", i);
      flex_trit_print(hash243_queue_at(account.transactions, i), NUM_TRITS_ADDRESS);
      printf("\n");
    }
    printf("transaction count %zu\n", tx_count);
#endif

    // dump balance
    printf("total balance: %" PRIu64 "\n", account.balance);

    // dump unused address
    printf("unused address: ");
    flex_trit_print(account.latest_address, NUM_TRITS_ADDRESS);
    printf("\n");

    // dump addresses
    size_t addr_count = hash243_queue_count(account.addresses);
    printf("address count %zu\n", addr_count);
    for (size_t i = 0; i < addr_count; i++) {
      printf("[%zu] ", i);
      flex_trit_print(hash243_queue_at(account.addresses, i), NUM_TRITS_ADDRESS);
      printf(" : %" PRIu64 "\n", account_data_get_balance(&account, i));
    }

    account_data_clear(&account);
  } else {
    printf("Error: %s\n", error_2_string(ret));
    return CLI_ERR_FAILED;
  }
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

//==========TRANSACTIONS==========
static struct {
  struct arg_str *address;
  struct arg_str *tag;
  struct arg_str *bundle;
  struct arg_end *end;
} get_transactions_args;

static int fn_get_transactions(int argc, char **argv) {
  retcode_t ret_code = RC_OK;
  flex_trit_t tmp_hash[FLEX_TRIT_SIZE_243];
  flex_trit_t tmp_tag[FLEX_TRIT_SIZE_81];
  find_transactions_req_t *transactions_req = find_transactions_req_new();
  find_transactions_res_t *transactions_res = find_transactions_res_new();

  if (!transactions_req || !transactions_res) {
    printf("Err: Out of Memory\n");
    goto done;
  }

  int nerrors = arg_parse(argc, argv, (void **)&get_transactions_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, get_transactions_args.end, argv[0]);
    return CLI_ERR_CMD_PARSING;
  }

  int addr_count = get_transactions_args.address->count;
  int bundle_count = get_transactions_args.bundle->count;
  int tag_count = get_transactions_args.tag->count;
  if ((addr_count + bundle_count + tag_count) == 0) {
    printf("No arguments\n");
    return CLI_ERR_INVALID_ARG;
  }

  // adding addresses
  if (addr_count > 0) {
    for (int i = 0; i < addr_count; i++) {
      tryte_t const *address_ptr = (tryte_t *)get_transactions_args.address->sval[i];
      if (!is_address(address_ptr)) {
        printf("Invalid address\n");
        goto done;
      }
      if (flex_trits_from_trytes(tmp_hash, NUM_TRITS_HASH, address_ptr, NUM_TRYTES_HASH, NUM_TRYTES_HASH) == 0) {
        printf("Err: converting flex_trit failed\n");
        goto done;
      }

      if (find_transactions_req_address_add(transactions_req, tmp_hash) != RC_OK) {
        printf("Err: adding the hash to queue failed.\n");
        goto done;
      }
    }
  }

  // adding bundles
  if (bundle_count > 0) {
    for (int i = 0; i < bundle_count; i++) {
      tryte_t const *bundle_ptr = (tryte_t *)get_transactions_args.bundle->sval[i];
      if (!is_trytes(bundle_ptr, NUM_TRYTES_BUNDLE)) {
        printf("Invalid bundle hash\n");
        goto done;
      }
      if (flex_trits_from_trytes(tmp_hash, NUM_TRITS_HASH, bundle_ptr, NUM_TRYTES_HASH, NUM_TRYTES_HASH) == 0) {
        printf("Err: converting flex_trit failed\n");
        goto done;
      }

      if (find_transactions_req_bundle_add(transactions_req, tmp_hash) != RC_OK) {
        printf("Err: adding the hash to queue failed.\n");
        goto done;
      }
    }
  }

  // adding tags
  if (tag_count > 0) {
    for (int i = 0; i < tag_count; i++) {
      tryte_t const *tag_ptr = (tryte_t *)get_transactions_args.tag->sval[i];
      if (!is_tag(tag_ptr)) {
        printf("Invalid tag hash\n");
        goto done;
      }
      if (flex_trits_from_trytes(tmp_tag, NUM_TRITS_TAG, tag_ptr, NUM_TRYTES_TAG, NUM_TRYTES_TAG) == 0) {
        printf("Err: converting flex_trit failed\n");
        goto done;
      }

      if (find_transactions_req_tag_add(transactions_req, tmp_tag) != RC_OK) {
        printf("Err: adding the hash to queue failed.\n");
        goto done;
      }
    }
  }

  if (iota_client_find_transactions(cli_ctx.iota_client, transactions_req, transactions_res) == RC_OK) {
    size_t count = hash243_queue_count(transactions_res->hashes);
    hash243_queue_t curr = transactions_res->hashes;
    for (size_t i = 0; i < count; i++) {
      printf("[%ld] ", (long int)i);
      flex_trit_print(curr->hash, NUM_TRITS_HASH);
      printf("\n");
      curr = curr->next;
    }
    printf("tx count = %ld\n", (long int)count);
  }

done:
  find_transactions_req_free(&transactions_req);
  find_transactions_res_free(&transactions_res);
  return ret_code;
}

static void cmd_register_get_transactions() {
  get_transactions_args.address = arg_strn("a", NULL, "<address>", 0, 5, "Address hashes");
  get_transactions_args.tag = arg_strn("t", NULL, "<tag>", 0, 5, "Tag hashes");
  get_transactions_args.bundle = arg_strn("b", NULL, "<bundle>", 0, 5, "Bundle hashes");
  get_transactions_args.end = arg_end(20);
  cli_cmd_t get_transactions_cmd = {
      .command = "transactions",
      .help = "Get transactions associate to an address",
      .hint = " [-a] [-t] [-b]",
      .func = &fn_get_transactions,
      .argtable = &get_transactions_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &get_transactions_cmd);
}

//==========BALANCE==========
static struct {
  struct arg_str *address;
  struct arg_end *end;
} get_balance_args;

static int fn_get_balance(int argc, char **argv) {
  retcode_t ret_code = RC_OK;
  flex_trit_t tmp_address[FLEX_TRIT_SIZE_243];

  int nerrors = arg_parse(argc, argv, (void **)&get_balance_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, get_balance_args.end, argv[0]);
    return CLI_ERR_CMD_PARSING;
  }

  get_balances_req_t *balance_req = get_balances_req_new();
  get_balances_res_t *balance_res = get_balances_res_new();

  if (!balance_req || !balance_res) {
    printf("Err: Out of Memory");
    return CLI_ERR_OOM;
  }

  int addr_count = get_balance_args.address->count;
  for (int i = 0; i < addr_count; i++) {
    tryte_t const *address_ptr = (tryte_t *)get_balance_args.address->sval[i];
    if (!is_address(address_ptr)) {
      printf("Invalid address\n");
      goto done;
    }
    if (flex_trits_from_trytes(tmp_address, NUM_TRITS_HASH, address_ptr, NUM_TRYTES_HASH, NUM_TRYTES_HASH) == 0) {
      printf("Err: converting flex_trit failed\n");
      goto done;
    }

    if (get_balances_req_address_add(balance_req, tmp_address) != RC_OK) {
      printf("Err: adding the hash to queue failed.\n");
      goto done;
    }
  }

  balance_req->threshold = 100;

  if ((ret_code = iota_client_get_balances(cli_ctx.iota_client, balance_req, balance_res)) == RC_OK) {
    hash243_queue_entry_t *q_iter = NULL;
    size_t balance_cnt = get_balances_res_balances_num(balance_res);
    printf("balances: \n");
    for (size_t i = 0; i < balance_cnt; i++) {
      printf("[%" PRIu64 " ", get_balances_res_balances_at(balance_res, i));
      printf("] ");
      flex_trit_print(get_balances_req_address_get(balance_req, i), NUM_TRITS_HASH);
      printf("\n");
    }

    CDL_FOREACH(balance_res->references, q_iter) {
      printf("reference: ");
      flex_trit_print(q_iter->hash, NUM_TRITS_HASH);
      printf("\n");
    }
  }

done:
  get_balances_req_free(&balance_req);
  get_balances_res_free(&balance_res);
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
  retcode_t ret_code = RC_OK;
  bool has_remainder = false;
  flex_trit_t seed[NUM_FLEX_TRITS_ADDRESS];

  int nerrors = arg_parse(argc, argv, (void **)&send_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, send_args.end, argv[0]);
    return CLI_ERR_CMD_PARSING;
  }

  // check parameters
  char const *receiver = send_args.receiver->sval[0];
  if (!is_address((tryte_t const *const)receiver)) {
    printf("Invalid receiver address!\n");
    return CLI_ERR_INVALID_ARG;
  }
  char const *remainder = send_args.remainder->sval[0];
  if (strlen(remainder)) {
    if (!is_address((tryte_t const *const)remainder)) {
      printf("Invalid remainder address!\n");
      return CLI_ERR_INVALID_ARG;
    }
    has_remainder = true;
  }

  char const *msg = send_args.message->sval[0];
  char *endptr = NULL;
  int64_t value = strtoll(send_args.value->sval[0], &endptr, 10);
  if (endptr == send_args.value->sval[0]) {
    printf("No digits were found in value\n");
    return CLI_ERR_INVALID_ARG;
  }

  char padded_tag[NUM_TRYTES_TAG + 1];
  char *tag = (char *)send_args.tag->sval[0];
  size_t tag_size = strlen(tag);
  to_uppercase(tag, tag_size);
  if (tag_size < NUM_TRYTES_TAG) {
    for (size_t i = 0; i < NUM_TRYTES_TAG; i++) {
      if (i < tag_size)
        padded_tag[i] = tag[i];
      else
        padded_tag[i] = '9';
    }
  }
  padded_tag[NUM_TRYTES_TAG] = '\0';

  if (!is_tag((tryte_t const *const)padded_tag)) {
    printf("Invalid tag!\n");
    return CLI_ERR_INVALID_ARG;
  }

  printf("sending %" PRId64 " to %s\n", value, receiver);
  printf("security %d, depth %d, MWM %d, tag [%s]\n", cli_ctx.security, cli_ctx.depth, cli_ctx.mwm,
         strlen(tag) ? padded_tag : "empty");
  printf("remainder [%s]\n", strlen(remainder) ? remainder : "empty");
  printf("message [%s]\n", strlen(msg) ? msg : "empty");

  bundle_transactions_t *bundle = NULL;
  bundle_transactions_new(&bundle);
  transfer_array_t *transfers = transfer_array_new();

  /* transfer setup */
  transfer_t tf = {};
  // seed
  if (has_remainder) {
    if (flex_trits_from_trytes(seed, NUM_TRITS_ADDRESS, (tryte_t const *)CLIENT_CONFIG_SEED, NUM_TRYTES_ADDRESS,
                               NUM_TRYTES_ADDRESS) == 0) {
      printf("seed flex_trits convertion failed\n");
      goto done;
    }
  }

  // receiver
  if (flex_trits_from_trytes(tf.address, NUM_TRITS_ADDRESS, (tryte_t const *)receiver, NUM_TRYTES_ADDRESS,
                             NUM_TRYTES_ADDRESS) == 0) {
    printf("address flex_trits convertion failed\n");
    goto done;
  }

  // tag
  printf("tag: %s\n", padded_tag);
  if (flex_trits_from_trytes(tf.tag, NUM_TRITS_TAG, (tryte_t const *)padded_tag, NUM_TRYTES_TAG, NUM_TRYTES_TAG) == 0) {
    printf("tag flex_trits convertion failed\n");
    goto done;
  }

  // value
  tf.value = value;

  // message (optional)
  transfer_message_set_string(&tf, msg);

  transfer_array_add(transfers, &tf);

  ret_code = iota_client_send_transfer(cli_ctx.iota_client, has_remainder ? seed : NULL, cli_ctx.security,
                                       cli_ctx.depth, cli_ctx.mwm, false, transfers, NULL, NULL, NULL, bundle);

  printf("send transaction: %s\n", error_2_string(ret_code));
  if (ret_code == RC_OK) {
    flex_trit_t const *bundle_hash = bundle_transactions_bundle_hash(bundle);
    printf("bundle hash: ");
    flex_trit_print(bundle_hash, NUM_TRITS_HASH);
    printf("\n");
  }

done:
  bundle_transactions_free(&bundle);
  transfer_message_free(&tf);
  transfer_array_free(transfers);

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
tryte_t const tryte_chars[27] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
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
  int nerrors = arg_parse(argc, argv, (void **)&get_addresses_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, get_addresses_args.end, argv[0]);
    return CLI_ERR_INVALID_ARG;
  }

  char *endptr = NULL;
  uint64_t start_index = strtoll(get_addresses_args.start_idx->sval[0], &endptr, 10);
  if (endptr == get_addresses_args.start_idx->sval[0]) {
    printf("No digits were found in start index\n");
    return CLI_ERR_INVALID_ARG;
  }
  uint64_t end_index = strtoll(get_addresses_args.end_idx->sval[0], &endptr, 10);
  if (endptr == get_addresses_args.end_idx->sval[0]) {
    printf("No digits were found in end index\n");
    return CLI_ERR_INVALID_ARG;
  }
  if (end_index < start_index) {
    printf("end index[%" PRIu64 "] should bigger or equal to start index[%" PRIu64 "]\n", start_index, end_index);
    return CLI_ERR_INVALID_ARG;
  }

  printf("Security level: %d\n", cli_ctx.security);
  // printf("get address %"PRId64" , %"PRId64"\n", start_index, end_index);
  while (start_index <= end_index) {
    char *addr = iota_sign_address_gen_trytes(cli_ctx.seed, start_index, cli_ctx.security);
    printf("[%" PRIu64 "] %s\n", start_index, addr);
    free(addr);
    start_index++;
  }

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

//==========GET BUNDLE==========
static struct {
  struct arg_str *tail;
  struct arg_end *end;
} get_bundle_args;

static int fn_get_bundle(int argc, char **argv) {
  retcode_t ret_code = RC_OK;
  flex_trit_t tmp_tail[FLEX_TRIT_SIZE_243];
  bundle_status_t bundle_status = BUNDLE_NOT_INITIALIZED;
  bundle_transactions_t *bundle = NULL;

  int nerrors = arg_parse(argc, argv, (void **)&get_bundle_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, get_bundle_args.end, argv[0]);
    return CLI_ERR_CMD_PARSING;
  }

  tryte_t const *tail_ptr = (tryte_t *)get_bundle_args.tail->sval[0];
  if (!is_trytes(tail_ptr, NUM_TRYTES_HASH)) {
    printf("Invalid hash\n");
    return CLI_ERR_INVALID_ARG;
  }

  bundle_transactions_new(&bundle);
  if (flex_trits_from_trytes(tmp_tail, NUM_TRITS_HASH, tail_ptr, NUM_TRYTES_HASH, NUM_TRYTES_HASH) == 0) {
    printf("Error: converting flex_trit failed.\n");
    ret_code = CLI_ERR_FAILED;
  } else {
    if ((ret_code = iota_client_get_bundle(cli_ctx.iota_client, tmp_tail, bundle, &bundle_status)) == RC_OK) {
      if (bundle_status == BUNDLE_VALID) {
        printf("bundle status: %d\n", bundle_status);
        bundle_dump(bundle);
      } else {
        printf("Invalid bundle: %d\n", bundle_status);
      }
    } else {
      printf("Error: %s\n", error_2_string(ret_code));
    }
  }

  bundle_transactions_free(&bundle);
  return ret_code;
}

static void cmd_register_get_bundle() {
  get_bundle_args.tail = arg_str1(NULL, NULL, "<tail>", "A tail hash");
  get_bundle_args.end = arg_end(4);
  cli_cmd_t get_bundle_cmd = {
      .command = "get_bundle",
      .help = "Gets associated transactions from a tail hash",
      .hint = " <tail>",
      .func = &fn_get_bundle,
      .argtable = &get_bundle_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &get_bundle_cmd);
}

//==========REATTACH==========
static struct {
  struct arg_str *tail;
  struct arg_end *end;
} reattach_args;

static int fn_reattach(int argc, char **argv) {
  retcode_t ret_code = RC_OK;
  flex_trit_t tmp_tail[FLEX_TRIT_SIZE_243];
  bundle_transactions_t *bundle = NULL;

  int nerrors = arg_parse(argc, argv, (void **)&reattach_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, reattach_args.end, argv[0]);
    return CLI_ERR_CMD_PARSING;
  }

  tryte_t const *tail_ptr = (tryte_t *)reattach_args.tail->sval[0];
  if (!is_trytes(tail_ptr, NUM_TRYTES_HASH)) {
    printf("Invalid hash\n");
    return CLI_ERR_INVALID_ARG;
  }

  bundle_transactions_new(&bundle);
  if (flex_trits_from_trytes(tmp_tail, NUM_TRITS_HASH, tail_ptr, NUM_TRYTES_HASH, NUM_TRYTES_HASH) == 0) {
    printf("Error: converting flex_trit failed.\n");
    ret_code = CLI_ERR_FAILED;
  } else {
    if ((ret_code = iota_client_replay_bundle(cli_ctx.iota_client, tmp_tail, cli_ctx.depth, cli_ctx.mwm, NULL,
                                              bundle)) == RC_OK) {
      printf("reattach succeeded\n");
      printf("Bundle hash: ");
      flex_trit_print(bundle_transactions_bundle_hash(bundle), NUM_TRITS_HASH);
      printf("\n");
    } else {
      printf("Error: %s\n", error_2_string(ret_code));
    }
  }

  bundle_transactions_free(&bundle);
  return ret_code;
}

static void cmd_register_reattach() {
  reattach_args.tail = arg_str1(NULL, NULL, "<tail>", "A tail transaction hash");
  reattach_args.end = arg_end(4);
  cli_cmd_t reattach_cmd = {
      .command = "reattach",
      .help = "Reattaches a bundle transaction to the Tangle",
      .hint = " <tail>",
      .func = &fn_reattach,
      .argtable = &reattach_args,
  };
  utarray_push_back(cli_ctx.cmd_array, &reattach_cmd);
}

//==========END OF COMMANDS==========

cli_err_t cli_command_init() {
  // init command line paring buffer
  cli_ctx.parsing_buf = calloc(CLI_LINE_BUFFER, 1);
  if (cli_ctx.parsing_buf == NULL) {
    return CLI_ERR_OOM;
  }

  cli_ctx.seed = calloc(NUM_TRYTES_HASH + 1, 1);
  if (cli_ctx.seed == NULL) {
    return CLI_ERR_OOM;
  }
  // init seed hash
  strncpy(cli_ctx.seed, CLIENT_CONFIG_SEED, NUM_TRYTES_HASH);
  cli_ctx.seed[NUM_TRYTES_HASH] = '\0';

  // create cmd list
  utarray_new(cli_ctx.cmd_array, &cli_cmd_icd);
  if (cli_ctx.cmd_array == NULL) {
    return CLI_ERR_OOM;
  }

  cli_ctx.mwm = CLIENT_CONFIG_MWM;
  cli_ctx.depth = CLIENT_CONFIG_DEPTH;
  cli_ctx.security = CLIENT_CONFIG_SECURITY;

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
  cmd_register_get_transactions();
  cmd_register_get_balance();
  cmd_register_send();
  cmd_register_gen_hash();
  cmd_register_get_addresses();
  cmd_register_get_bundle();
  cmd_register_reattach();

  return iota_client_init();
}

cli_err_t cli_command_end() {
  free(cli_ctx.seed);
  free(cli_ctx.parsing_buf);
  utarray_free(cli_ctx.cmd_array);
  iota_client_destroy();
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
