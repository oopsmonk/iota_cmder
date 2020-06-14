workspace(name = "iota_cmder")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "org_iota_common",
    commit = "cf649803757abf48432d4fa60e9f27ac119bae5f",
    remote = "https://github.com/iotaledger/iota_common.git",
)

git_repository(
    name = "org_iota_client",
    commit = "b0564c029942aee006d2c0d20e2663af234d5397",
    remote = "https://github.com/iotaledger/iota.c.git",
)

git_repository(
    name = "rules_iota",
    commit = "2d15c55f12cff0db106f45866312f61314c583cd",
    remote = "https://github.com/iotaledger/rules_iota.git",
)

load("@rules_iota//:defs.bzl", "iota_deps")

iota_deps()
