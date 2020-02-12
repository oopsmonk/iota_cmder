workspace(name = "iota_cmder")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "org_iota_common",
    commit = "f0afbcb6f4ac03982653c9c8350d1ea3b5a8865f",
    remote = "https://github.com/iotaledger/iota_common.git",
)

git_repository(
    name = "org_iota_client",
    commit = "b10def1cfac44bdf8e3103581ee241a7440aca30",
    remote = "https://github.com/iotaledger/iota.c.git",
)

git_repository(
    name = "rules_iota",
    commit = "4fd742195c31b9e2bf859a68cd5de4b2fdba7086",
    remote = "https://github.com/iotaledger/rules_iota.git",
)

load("@rules_iota//:defs.bzl", "iota_deps")

iota_deps()
