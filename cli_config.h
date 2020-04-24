#ifndef __CLI_CONFIG_H__
#define __CLI_CONFIG_H__

#define IOTA_CLIENT_DEBUG

#define CLI_LINE_BUFFER 4096
#define CLI_MAX_ARGC 16

// comment out if using HTTP
#define CLIENT_CONFIG_HTTPS

#define CLIENT_CONFIG_MAINNET

#define CLIENT_CONFIG_SEED "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
#define CLIENT_CONFIG_SECURITY 2  // security level of addresses

#ifdef CLIENT_CONFIG_MAINNET
#define CLIENT_CONFIG_NODE "nodes.iota.cafe"
#define CLIENT_CONFIG_PORT 443
#define CLIENT_CONFIG_MWM 14
#define CLIENT_CONFIG_DEPTH 3
#else  // testnet
#define CLIENT_CONFIG_NODE "nodes.devnet.iota.org"
#define CLIENT_CONFIG_PORT 443
#define CLIENT_CONFIG_MWM 9
#define CLIENT_CONFIG_DEPTH 6
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif  // __CLI_CONFIG_H__
