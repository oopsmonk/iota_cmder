# IOTA Commander  

![github actions](https://github.com/oopsmonk/iota_cmder/actions/workflows/push_build.yml/badge.svg)

A terminal-based application for interacting with the Tangle through [iota.c](https://github.com/iotaledger/iota.c). It is designed to run on PCs and embedded devices that POSIX compliant.  

It is inspired by [IOTA ESP32 Wallet](https://github.com/oopsmonk/iota_esp32_wallet)  

## Commands  

* `help`: Show support commands.
* `version`: Show version info.
* `node_info`: Display node info.
* `node_info_set`: Set iota node endpoint.
* `client_config`: Display MWM, Depth, and Security Level.
* `client_config_set`: Set MWM, Depth, Security Level.
* `seed`: Display wallet seed.
* `seed_set`: Set wallet seed.
* `account`: Display addresses, balances, and unused address.
* `send`: Send transaction
* `get_addresses`: Get addresses from given starting and ending index.

## How to Use  

### CMake  

```bash
git clone https://github.com/oopsmonk/iota_cmder.git
cd iota_cmder
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$PWD ..
make -j8 && ./iota_cmder
```
