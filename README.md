# IOTA Commander  

A terminal-based application for interacting with the Tangle through [iota.c](https://github.com/iotaledger/iota.c). It is designed to run on PCs and embedded devices that POSIX compliant.  

It is inspired by [IOTA ESP32 Wallet](https://github.com/oopsmonk/iota_esp32_wallet)  

## How to Use  

### CMake  

```bash
git clone https://github.com/oopsmonk/iota_cmder.git
cd iota_cmder
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$PWD ..
make -j8 && ./iota_cmder
```

### Bazel  

[Use Bazel](https://docs.bazel.build/versions/master/install.html)  

```bash
git clone https://github.com/oopsmonk/iota_cmder.git
cd iota_cmder
bazel run iota_cmder
```

[Use bazelisk](https://github.com/bazelbuild/bazelisk)  

```bash
git clone https://github.com/oopsmonk/iota_cmder.git
cd iota_cmder
USE_BAZEL_VERSION=2.1.0 bazelisk run iota_cmder
```

### Client Configurations  

You can config client by:
* node_info_set: a node you want to use
* client_conf_set: set MWW, depth, security level
* seed_set: set the SEED (optional, please make sure you understand the risk before sending a value tx)  
* help: list commands

Default SEED is "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"  

```bash
# setup node url and port
IOTA> node_info_set nodes.thetangle.org 443 1
nodes.thetangle.org, port 443, https 1

# setup mwm, depth, security level
IOTA> client_conf_set 14 3 2
Set: MWM 14, Depth 3, Security 2

# check your node configuration
IOTA> node_info
appName IRI 
appVersion 1.8.4 
latestMilestone: RSVYL9SZJLFM99DDVCNSASYYBT9ASEETP9BTAEFVTVVLHQUMXHNOSIZMVWORQRVRRVSUMQBTKGXRA9999
latestMilestoneIndex 1341705 
latestSolidSubtangleMilestone: RSVYL9SZJLFM99DDVCNSASYYBT9ASEETP9BTAEFVTVVLHQUMXHNOSIZMVWORQRVRRVSUMQBTKGXRA9999
latestSolidSubtangleMilestoneIndex 1341705 
neighbors 24 
packetsQueueSize 0 
time 1581669480289 
tips 5790 
transactionsToRequest 0 
features: RemotePOW, snapshotPruning, loadBalancer, 
coordinatorAddress: EQSAUZXULTTYZCLNJNTXQTQHOMOFZERHTCGTXOLTVAHKSA9OGAZDEKECURBRIXIJWNPFCQIOVFVVXJVD9
Client Configuration: MWM 14, Depth 3, Security 2
```
