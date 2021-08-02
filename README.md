# IOTA Commander  

![github actions](https://github.com/oopsmonk/iota_cmder/actions/workflows/push_build.yml/badge.svg)

A terminal-based application for interacting with the Tangle through [iota.c](https://github.com/iotaledger/iota.c). It is designed to run on PCs and embedded devices that POSIX compliant.  

## Commands  

**System**

* `help`: Show support commands.
* `version`: Show version info.
* `node_set`: Set connected node
* `node_info_conf`: Display connected node.

**Client APIs**

* `node_info`: Display node info.
* `api_msg_index`: Find messages from a given Index.
* `api_get_balance`: Get balance value from a given address.
* `api_msg_children`: Get children from a given message ID.
* `api_msg_meta`: Get metadata from a given message ID.
* `api_address_outputs`: Get output IDs from a given address.
* `api_get_output`: Get the output data from a given output ID.
* `api_tips`: Get tips from the connected node.
* `api_send_msg`: Send out a data message to the Tangle.
* `api_get_msg`: Get a message data from a given message ID.

**Wallet APIs**

* `seed`: Display wallet seed.
* `seed_set`: Set wallet seed.
* `address`: Display addresses from an index.
* `balance`: Display balance from an index.
* `send`: Send a value transaction to the Tangle.

## How to Use  

### CMake  

```bash
git clone https://github.com/oopsmonk/iota_cmder.git
cd iota_cmder
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$PWD ..
make -j8 && ./iota_cmder
```
