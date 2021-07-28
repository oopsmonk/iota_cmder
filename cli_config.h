#ifndef __CLI_CONFIG_H__
#define __CLI_CONFIG_H__

#define IOTA_CLIENT_DEBUG

#define CLI_LINE_BUFFER 4096
#define CLI_MAX_ARGC 16

// comment out if using HTTP
#define CLIENT_CONFIG_HTTPS

// #define CLIENT_CONFIG_MAINNET

#define WALLET_CONFIG_SEED "RANDOM" // RANDOM or a string with 64 characters
#define WALLET_CONFIG_PATH "m/44'/4218'/0'/0'"

#ifdef CLIENT_CONFIG_MAINNET
#define CLIENT_CONFIG_NODE "chrysalis-nodes.iota.org"
#define CLIENT_CONFIG_PORT 443
#define NODE_USE_TLS 1
#else  // testnet
#define CLIENT_CONFIG_NODE "api.lb-0.testnet.chrysalis2.com"
#define CLIENT_CONFIG_PORT 443
#define NODE_USE_TLS 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif  // __CLI_CONFIG_H__
