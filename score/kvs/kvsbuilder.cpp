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
#include "score/kvs/kvsbuilder.hpp"
#include "score/kvs/internal/kvs_config_parser_factory.hpp"

namespace score::mw::per::kvs
{

/*********************** KVS Builder Implementation *********************/
KvsBuilder::KvsBuilder(const InstanceId& instance_id)
    : instance_id(instance_id),
      snapshot_id(SnapshotId{0}),
      need_defaults(OpenNeedDefaults::Optional),
      need_kvs(OpenNeedKvs::Optional),
      directory("./data_folder/") /* Default Directory */,
      snapshot_max_count_setting(KVS_MAX_SNAPSHOTS),
      read_only_setting(false),
      max_storage_size_setting(std::nullopt)
{
}

KvsBuilder& KvsBuilder::snapshot(const SnapshotId& snapshot_id)
{
    this->snapshot_id = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::need_defaults_flag(const OpenNeedDefaults& flag)
{
    need_defaults = flag;
    return *this;
}

KvsBuilder& KvsBuilder::need_kvs_flag(const OpenNeedKvs& flag)
{
    need_kvs = flag;
    return *this;
}

KvsBuilder& KvsBuilder::dir(std::string&& dir_path)
{
    this->directory = std::move(dir_path);
    return *this;
}

KvsBuilder& KvsBuilder::snapshot_max_count(std::size_t count)
{
    snapshot_max_count_setting = count;
    return *this;
}

KvsBuilder& KvsBuilder::read_only_flag(bool flag)
{
    read_only_setting = flag;
    return *this;
}

KvsBuilder& KvsBuilder::max_storage_size(std::size_t size_bytes)
{
    max_storage_size_setting = size_bytes;
    return *this;
}

/*
 * input: none (reads the builder's own members and, through `IKvsConfigParser::load_entry()`,
 *        the configuration-by-file document applicable to `instance_id`, if any).
 * behavior: Validates the builder-set `snapshot_max_count`/`max_storage_size` values, loads the
 *           configuration-by-file entry matching `instance_id`, and merges it over local copies
 *           of the builder settings (file entry > builder setter > built-in default). The
 *           builder's own members are never moved or cleared, so `build()` may be called more
 *           than once. An empty directory resolves to the current directory ("./"). Finally
 *           calls `Kvs::open()` with the merged settings.
 * output: the opened `Kvs` on success; `ErrorCode::InvalidConfiguration` if a builder setting or
 *         the resolved file entry is out of range, the error propagated from
 *         `IKvsConfigParser::load_entry()` (e.g. `ErrorCode::FileNotFound`,
 *         `ErrorCode::JsonParserError`), or any error from `Kvs::open()`.
 */
score::Result<Kvs> KvsBuilder::build()
{
    if (snapshot_max_count_setting > 3U)
    {
        return score::MakeUnexpected(ErrorCode::InvalidConfiguration, "snapshot_max_count must be in 0..3");
    }
    if (max_storage_size_setting.has_value() && (max_storage_size_setting.value() == 0U))
    {
        return score::MakeUnexpected(ErrorCode::InvalidConfiguration, "max_storage_size must be greater than zero");
    }

    auto parser = create_kvs_config_parser();
    auto entry_res = parser->load_entry(instance_id.id);
    if (!entry_res)
    {
        return score::MakeUnexpected<Kvs>(entry_res.error());
    }

    /* Local copies: the builder's own members are never consumed, so build() stays repeatable. */
    std::string local_directory = directory;
    OpenNeedDefaults local_need_defaults = need_defaults;
    OpenNeedKvs local_need_kvs = need_kvs;
    std::size_t local_snapshot_max_count = snapshot_max_count_setting;
    bool local_read_only = read_only_setting;
    std::optional<std::size_t> local_max_storage_size = max_storage_size_setting;

    if (entry_res.value().has_value())
    {
        const KvsConfigEntry& entry = entry_res.value().value();
        if (entry.storage_path.has_value())
        {
            local_directory = entry.storage_path.value();
        }
        if (entry.need_defaults.has_value())
        {
            local_need_defaults = (entry.need_defaults.value() == KvsConfigNeedMode::Required)
                                      ? OpenNeedDefaults::Required
                                      : OpenNeedDefaults::Optional;
        }
        if (entry.need_kvs.has_value())
        {
            local_need_kvs = (entry.need_kvs.value() == KvsConfigNeedMode::Required) ? OpenNeedKvs::Required
                                                                                      : OpenNeedKvs::Optional;
        }
        if (entry.snapshot_max_count.has_value())
        {
            local_snapshot_max_count = entry.snapshot_max_count.value();
        }
        if (entry.read_only.has_value())
        {
            local_read_only = entry.read_only.value();
        }
        if (entry.max_storage_size_bytes.has_value())
        {
            local_max_storage_size = entry.max_storage_size_bytes.value();
        }
    }

    /* Use current directory if empty */
    if (local_directory.empty())
    {
        local_directory = "./";
    }

    return Kvs::open(instance_id,
                     snapshot_id,
                     local_need_defaults,
                     local_need_kvs,
                     std::move(local_directory),
                     local_snapshot_max_count,
                     local_read_only,
                     local_max_storage_size);
}

} /* namespace score::mw::per::kvs */
