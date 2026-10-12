/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/kvs/internal/kvs_config_parser_factory.hpp"
#include "score/kvs/internal/kvs_config_parser_mock.hpp"
#include <gmock/gmock.h>

/*
 * Test-only definition of `create_kvs_config_parser()`, linked into `:unit_tests` through the
 * `kvs_config_parser_mock` Bazel target. Returns a plain `KvsConfigParserMock` whose default
 * action reports "no configuration file applies" (`std::nullopt`), so existing
 * `KvsBuilder::build()` unit tests behave as if no configuration-by-file document exists, without
 * depending on the real `KvsConfigParser` or any filesystem/environment state. Tests that need a
 * non-default `load_entry()` result are expected to inject their own `KvsConfigParserMock`
 * (future work, tracked for the `cpp-unit-test-writer` agent) rather than relying on the instance
 * `KvsBuilder::build()` creates internally. The production target (`kvs_config_parser`) defines
 * the same symbol for `:kvs`; the two targets are never linked into the same binary.
 */
namespace score::mw::per::kvs
{

std::unique_ptr<IKvsConfigParser> create_kvs_config_parser()
{
    auto mock = std::make_unique<KvsConfigParserMock>();
    ON_CALL(*mock, load_entry(::testing::_))
        .WillByDefault(::testing::Return(score::Result<std::optional<KvsConfigEntry>>(std::nullopt)));
    return mock;
}

} /* namespace score::mw::per::kvs */
