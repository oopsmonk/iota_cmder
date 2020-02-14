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

