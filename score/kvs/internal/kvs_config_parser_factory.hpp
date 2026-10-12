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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_FACTORY_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_FACTORY_HPP

#include "score/kvs/internal/ikvs_config_parser.hpp"
#include <memory>

/*
 * Declares the single factory symbol `create_kvs_config_parser()` that `KvsBuilder::build()` calls
 * to obtain an `IKvsConfigParser`. Exactly one definition of this symbol is linked into any given
 * binary: the `kvs_config_parser` Bazel target (returning `KvsConfigParser`) for production code
 * linked into `:kvs`, or the `kvs_config_parser_mock` target (returning `KvsConfigParserMock`) for
 * `:unit_tests`. `:kvs` never depends on the mock target.
 */
namespace score::mw::per::kvs
{

std::unique_ptr<IKvsConfigParser> create_kvs_config_parser();

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_FACTORY_HPP */
