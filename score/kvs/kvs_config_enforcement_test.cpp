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

/* Unit tests for the enforcement of configuration-by-file settings inside Kvs: read-only mode,
 * the storage size limit (max_storage_size_bytes), and the snapshot count limit (snapshot_max_count).
 * Design reference: others/config_by_file.md, section "As implemented (issue #411)". */

#include "kvs_config_general_test.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

using namespace score::mw::per::kvs;
using score::mw::per::kvs::test::KvsConfigFixture;
using score::mw::per::kvs::test::kTestInstanceId;

namespace
{

/* Path of the `_<id>` KVS payload file for the test instance inside `dir`. */
std::string KvsFile(const std::string& dir, std::size_t id)
{
    return (std::filesystem::path(dir) / ("kvs_" + std::to_string(kTestInstanceId) + "_" + std::to_string(id) + ".json"))
        .string();
}

bool Exists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

/* Reads the string value of `key`; fails the calling test if the key is missing or not a string. */
std::string GetString(Kvs& kvs, const std::string& key)
{
    auto value = kvs.get_value(key);
    if (!value)
    {
        ADD_FAILURE() << "get_value(" << key << ") failed";
        return {};
    }
    return std::get<std::string>(value.value().getValue());
}

template <typename TResult>
void ExpectErrorCode(const TResult& result, ErrorCode expected)
{
    ASSERT_FALSE(result) << "expected error code " << static_cast<int>(expected) << " but call succeeded";
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), expected);
}

/* Sets key "k" to each value in turn and flushes after every write. */
void FlushSequence(Kvs& kvs, const std::vector<std::string>& values)
{
    for (const auto& value : values)
    {
        ASSERT_TRUE(kvs.set_value("k", KvsValue(value)));
        ASSERT_TRUE(kvs.flush());
    }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/* Read-only enforcement (builder read_only_flag / file access_mode "read_only") */
////////////////////////////////////////////////////////////////////////////////

using KvsReadOnly = KvsConfigFixture;

TEST_F(KvsReadOnly, EveryMutatorIsRejectedAndLeavesFilesByteIdentical)
{
    /* Create a populated storage with a writable instance first. */
    const std::string dir = SubDir("data");
    {
        KvsBuilder writable{InstanceId{kTestInstanceId}};
        auto kvs = writable.dir(std::string(dir)).build();
        ASSERT_TRUE(kvs);
        FlushSequence(kvs.value(), {"first", "second"});
        ASSERT_TRUE(kvs.value().set_value("other", KvsValue("pending")));
        ASSERT_TRUE(kvs.value().flush());
    }
    const auto before = Snapshot(dir);
    ASSERT_FALSE(before.empty());

    KvsBuilder reader{InstanceId{kTestInstanceId}};
    auto kvs = reader.dir(std::string(dir)).read_only_flag(true).build();
    ASSERT_TRUE(kvs);

    ExpectErrorCode(kvs.value().set_value("k", KvsValue("changed")), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().remove_key("k"), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().remove_all_keys(), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().reset(), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().reset_key("k"), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().flush(), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{1}), ErrorCode::ReadOnlyViolation);

    EXPECT_EQ(Snapshot(dir), before);
}

TEST_F(KvsReadOnly, ReadsStillWorkOnPersistedData)
{
    const std::string dir = SubDir("data");
    {
        KvsBuilder writable{InstanceId{kTestInstanceId}};
        auto kvs = writable.dir(std::string(dir)).build();
        ASSERT_TRUE(kvs);
        FlushSequence(kvs.value(), {"persisted"});
    }

    KvsBuilder reader{InstanceId{kTestInstanceId}};
    auto kvs = reader.dir(std::string(dir)).read_only_flag(true).build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(GetString(kvs.value(), "k"), "persisted");
    auto keys = kvs.value().get_all_keys();
    EXPECT_TRUE(keys);
}

TEST_F(KvsReadOnly, ReadOnlyOpenOfEmptyDirectoryCreatesNothing)
{
    const std::string dir = SubDir("empty");

    KvsBuilder reader{InstanceId{kTestInstanceId}};
    auto kvs = reader.dir(std::string(dir)).read_only_flag(true).build();
    ASSERT_TRUE(kvs);

    ExpectErrorCode(kvs.value().set_value("k", KvsValue("v")), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().flush(), ErrorCode::ReadOnlyViolation);
    EXPECT_TRUE(ListDir(dir).empty());
}

TEST_F(KvsReadOnly, ReadOnlyFlagFalseAllowsWrites)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).read_only_flag(false).build();
    ASSERT_TRUE(kvs);
    EXPECT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(Exists(KvsFile(dir, 0)));
}

////////////////////////////////////////////////////////////////////////////////
/* Storage size limit (max_storage_size_bytes) */
////////////////////////////////////////////////////////////////////////////////

using KvsStorageSize = KvsConfigFixture;

TEST_F(KvsStorageSize, FirstFlushOverLimitLeavesDirectoryEmpty)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).max_storage_size(1).build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("a value that certainly exceeds one byte")));
    ExpectErrorCode(kvs.value().flush(), ErrorCode::StorageSizeExceeded);
    EXPECT_TRUE(ListDir(dir).empty());
}

TEST_F(KvsStorageSize, OverLimitFlushLeavesExistingFilesUntouched)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    /* Limit comfortably above the small payload but below the big one. */
    auto kvs = builder.dir(std::string(dir)).max_storage_size(200).build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("small")));
    ASSERT_TRUE(kvs.value().flush());
    const auto before = Snapshot(dir);
    ASSERT_FALSE(before.empty());

    ASSERT_TRUE(kvs.value().set_value("k", KvsValue(std::string(1000, 'x'))));
    ExpectErrorCode(kvs.value().flush(), ErrorCode::StorageSizeExceeded);

    EXPECT_EQ(Snapshot(dir), before);
    /* Rejected flush must not rotate snapshots either. */
    EXPECT_FALSE(Exists(KvsFile(dir, 1)));
}

TEST_F(KvsStorageSize, PayloadExactlyAtLimitIsAccepted)
{
    /* Learn the exact payload size of the file written for a known value. */
    const std::string reference_dir = SubDir("reference");
    {
        KvsBuilder builder{InstanceId{kTestInstanceId}};
        auto kvs = builder.dir(std::string(reference_dir)).build();
        ASSERT_TRUE(kvs);
        ASSERT_TRUE(kvs.value().set_value("k", KvsValue("boundary")));
        ASSERT_TRUE(kvs.value().flush());
    }
    const auto payload_size = std::filesystem::file_size(KvsFile(reference_dir, 0));
    ASSERT_GT(payload_size, 0u);

    const std::string dir = SubDir("exact");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).max_storage_size(payload_size).build();
    ASSERT_TRUE(kvs);
    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("boundary")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(Exists(KvsFile(dir, 0)));
}

TEST_F(KvsStorageSize, PayloadOneByteOverLimitIsRejected)
{
    const std::string reference_dir = SubDir("reference");
    {
        KvsBuilder builder{InstanceId{kTestInstanceId}};
        auto kvs = builder.dir(std::string(reference_dir)).build();
        ASSERT_TRUE(kvs);
        ASSERT_TRUE(kvs.value().set_value("k", KvsValue("boundary")));
        ASSERT_TRUE(kvs.value().flush());
    }
    const auto payload_size = std::filesystem::file_size(KvsFile(reference_dir, 0));

    const std::string dir = SubDir("one_over");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).max_storage_size(payload_size - 1).build();
    ASSERT_TRUE(kvs);
    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("boundary")));
    ExpectErrorCode(kvs.value().flush(), ErrorCode::StorageSizeExceeded);
    EXPECT_TRUE(ListDir(dir).empty());
}

TEST_F(KvsStorageSize, UnlimitedByDefault)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).build();
    ASSERT_TRUE(kvs);
    ASSERT_TRUE(kvs.value().set_value("k", KvsValue(std::string(100000, 'y'))));
    EXPECT_TRUE(kvs.value().flush());
}

////////////////////////////////////////////////////////////////////////////////
/* Snapshot count limit (snapshot_max_count) */
////////////////////////////////////////////////////////////////////////////////

using KvsSnapshotLimit = KvsConfigFixture;

TEST_F(KvsSnapshotLimit, ZeroKeepsOnlyCurrentFile)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).snapshot_max_count(0).build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), 0u);

    FlushSequence(kvs.value(), {"a", "b", "c", "d"});

    EXPECT_TRUE(Exists(KvsFile(dir, 0)));
    EXPECT_FALSE(Exists(KvsFile(dir, 1)));
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{1}), ErrorCode::InvalidSnapshotId);
}

TEST_F(KvsSnapshotLimit, ZeroFromFileEntryIsEnforcedEndToEnd)
{
    const std::string dir = SubDir("data");
    const std::string config = WriteConfig(
        std::string(R"({"format_version": 1, "key_value_storage_list": [{"instance_id": 7, "storage_path": ")") +
        dir + R"(", "snapshot_max_count": 0}]})");
    SetConfigEnv(config);

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), 0u);

    FlushSequence(kvs.value(), {"a", "b", "c"});
    EXPECT_FALSE(Exists(KvsFile(dir, 1)));
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{1}), ErrorCode::InvalidSnapshotId);
}

TEST_F(KvsSnapshotLimit, OneKeepsOneRotatedSnapshot)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).snapshot_max_count(1).build();
    ASSERT_TRUE(kvs);

    FlushSequence(kvs.value(), {"a", "b", "c", "d"});

    EXPECT_TRUE(Exists(KvsFile(dir, 0)));
    EXPECT_TRUE(Exists(KvsFile(dir, 1)));
    EXPECT_FALSE(Exists(KvsFile(dir, 2)));
    EXPECT_EQ(GetString(kvs.value(), "k"), "d");
    EXPECT_TRUE(kvs.value().snapshot_restore(SnapshotId{1}));
    EXPECT_EQ(GetString(kvs.value(), "k"), "c");
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{2}), ErrorCode::InvalidSnapshotId);
}

TEST_F(KvsSnapshotLimit, TwoKeepsTwoRotatedSnapshots)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).snapshot_max_count(2).build();
    ASSERT_TRUE(kvs);

    FlushSequence(kvs.value(), {"a", "b", "c", "d", "e"});

    EXPECT_TRUE(Exists(KvsFile(dir, 0)));
    EXPECT_TRUE(Exists(KvsFile(dir, 1)));
    EXPECT_TRUE(Exists(KvsFile(dir, 2)));
    EXPECT_FALSE(Exists(KvsFile(dir, 3)));
    EXPECT_TRUE(kvs.value().snapshot_restore(SnapshotId{2}));
    EXPECT_EQ(GetString(kvs.value(), "k"), "c");
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{3}), ErrorCode::InvalidSnapshotId);
}

TEST_F(KvsSnapshotLimit, DefaultLimitIsThree)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), 3u);

    FlushSequence(kvs.value(), {"a", "b", "c", "d", "e"});

    EXPECT_TRUE(Exists(KvsFile(dir, 3)));
    EXPECT_FALSE(Exists(KvsFile(dir, 4)));
    EXPECT_TRUE(kvs.value().snapshot_restore(SnapshotId{3}));
    EXPECT_EQ(GetString(kvs.value(), "k"), "b");
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{4}), ErrorCode::InvalidSnapshotId);
}

TEST_F(KvsSnapshotLimit, RejectedRestoreLeavesFilesUntouched)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).snapshot_max_count(1).build();
    ASSERT_TRUE(kvs);
    FlushSequence(kvs.value(), {"a", "b"});

    const auto before = Snapshot(dir);
    ExpectErrorCode(kvs.value().snapshot_restore(SnapshotId{5}), ErrorCode::InvalidSnapshotId);
    EXPECT_EQ(Snapshot(dir), before);
}
