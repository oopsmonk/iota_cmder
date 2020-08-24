workspace(name = "iota_cmder")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "org_iota_common",
    commit = "12d54bfadead696a4ffac40ce2ba01d62e9dc35d",
    remote = "https://github.com/iotaledger/iota_common.git",
)

git_repository(
    name = "org_iota_client",
    commit = "7743c91a02a32001a1158bdf4cee3580ce973991",
    remote = "https://github.com/iotaledger/iota.c.git",
)

git_repository(
    name = "rules_iota",
    commit = "2d15c55f12cff0db106f45866312f61314c583cd",
    remote = "https://github.com/iotaledger/rules_iota.git",
)

load("@rules_iota//:defs.bzl", "iota_deps")

iota_deps()
