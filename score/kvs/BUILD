# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@score_docs_as_code//:docs.bzl", "docs_bundle")

docs_bundle(
    name = "kvs_docs",
    code_targets = [
        ":kvs",
        "//score/kvs/rust_kvs:rust_kvs",
    ],
    primary_need_id = "comp__persistency_kvs",
    source_dir = "docs",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "error",
    srcs = ["error.cpp"],
    hdrs = ["error.hpp"],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "kvs_config_parser",
    srcs = [
        "internal/kvs_config_parser.cpp",
        "internal/kvs_config_parser_factory.cpp",
    ],
    hdrs = [
        "internal/ikvs_config_parser.hpp",
        "internal/kvs_config_parser.hpp",
        "internal/kvs_config_parser_factory.hpp",
        "internal/kvs_config_types.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":error",
        "@score_baselibs//score/json",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "kvs_config_parser_mock",
    testonly = True,
    srcs = ["internal/kvs_config_parser_mock_factory.cpp"],
    hdrs = [
        "internal/ikvs_config_parser.hpp",
        "internal/kvs_config_parser_factory.hpp",
        "internal/kvs_config_parser_mock.hpp",
        "internal/kvs_config_types.hpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":error",
        "@googletest//:gtest",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "kvs",
    srcs = [
        "internal/kvs_helper.cpp",
        "internal/kvs_helper.hpp",
        "kvs.cpp",
        "kvsbuilder.cpp",
        "kvsvalue.cpp",
    ],
    hdrs = [
        "kvs.hpp",
        "kvsbuilder.hpp",
        "kvsvalue.hpp",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":error",
        ":kvs_config_parser",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)

alias(
    name = "kvs_cpp",
    actual = "//score/kvs",
    deprecation = "//score/kvs:kvs_cpp has moved to //score/kvs " +
                  "Update your dependency to the //score/kvs label.",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "unit_tests",
    size = "small",
    srcs = [
        "kvs_builder_test.cpp",
        "kvs_error_test.cpp",
        "kvs_general_test.cpp",
        "kvs_general_test.hpp",
        "kvs_helper_test.cpp",
        "kvs_test.cpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":kvs_config_parser_mock",
        ":kvs",
        "@googletest//:gtest_main",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/filesystem:mock",
        "@score_baselibs//score/json:mock",
        "@score_baselibs//score/mw/log:backend_stub_testutil",
        "@score_baselibs//score/result",
    ],
)

cc_test(
    name = "kvs_config_real_test",
    size = "small",
    testonly = True,
    srcs = [
        "kvs_config_enforcement_test.cpp",
        "kvs_config_general_test.hpp",
        "kvs_config_test.cpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":kvs",
        "@googletest//:gtest_main",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/result",
    ],
)
