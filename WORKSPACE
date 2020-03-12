workspace(name = "iota_cmder")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "org_iota_common",
    commit = "1b56a5282933fb674181001630e7b2e2c33b5eea",
    remote = "https://github.com/iotaledger/iota_common.git",
)

git_repository(
    name = "org_iota_client",
    commit = "d632a84272dab322e9fbcbe71a5e6ac7e8168900",
    remote = "https://github.com/iotaledger/iota.c.git",
)

git_repository(
    name = "rules_iota",
    commit = "2d15c55f12cff0db106f45866312f61314c583cd",
    remote = "https://github.com/iotaledger/rules_iota.git",
)

load("@rules_iota//:defs.bzl", "iota_deps")

iota_deps()
