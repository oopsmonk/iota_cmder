# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_binary
cc_binary(
    name = "iota_cmder",
    srcs = [
        "cli_cmd.c",
        "cli_cmd.h",
        "cli_config.h",
        "iota_cmder.c",
        "split_argv.c",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "@argtable3",
        "@linenoise",
        "@org_iota_client//cclient/api",
        "@org_iota_common//utils:input_validators",
    ],
)
