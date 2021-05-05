#ifndef __CLI_CONFIG_H__
#define __CLI_CONFIG_H__

#define IOTA_CLIENT_DEBUG

#define CLI_LINE_BUFFER 4096
#define CLI_MAX_ARGC 16

// comment out if using HTTP
#define CLIENT_CONFIG_HTTPS

// #define CLIENT_CONFIG_MAINNET

#define WALLET_CONFIG_SEED "SEED_IS_A_STRING_WITH_64_CHAR"
#define WALLET_CONFIG_PATH "m/44'/4218'/0'/0'"

#ifdef CLIENT_CONFIG_MAINNET
#define CLIENT_CONFIG_NODE "https://chrysalis-nodes.iota.org/"
#define CLIENT_CONFIG_PORT 443
#else  // testnet
#define CLIENT_CONFIG_NODE "https://api.lb-0.testnet.chrysalis2.com/"
#define CLIENT_CONFIG_PORT 443
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif  // __CLI_CONFIG_H__
