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
#ifndef SCORE_LIB_KVS_KVSBUILDER_HPP
#define SCORE_LIB_KVS_KVSBUILDER_HPP

#include "score/kvs/kvs.hpp"
#include <string>

namespace score::mw::per::kvs
{

/**
 * @class KvsBuilder
 * @brief Builder for opening a KVS object.
 * This class allows configuration of various options for opening a KVS instance,
 * such as whether default values are required, whether the KVS data must already exist.
 *
 * Important: You don't need to include any other header files to use the KVS.
 * For documentation of the KVS Functions, refer to the kvs.hpp documentation.
 *
 * \brief Example Usage
 * \code
 *  #include <iostream>
 *  #include "score/kvs/kvsbuilder.hpp"
 *
 *  using namespace score::mw::per::kvs;
 *
 *  int main() {
 *    // Open kvs
 *    auto open_res = KvsBuilder(0)
 *                        .need_defaults_flag(OpenNeedDefaults::Required)
 *                        .need_kvs_flag(OpenNeedKvs::Required)
 *                        .build();
 *    if (!open_res) return 1;
 *    Kvs kvs = std::move(open_res.value());
 *
 *    // Set and get a value
 *    kvs.set_value("pi", KvsValue(3.14));
 *    auto get_res = kvs.get_value("pi");
 *
 *    // Delete a key
 *    kvs.remove_key("pi");
 *    std::cout << "has pi? " << (kvs.key_exists("pi").value_or(false) ? "yes" : "no") << "\n";
 *
 *
 *    return 0;
 *  }
 * \endcode
 */
class KvsBuilder final
{
  public:
    /**
     * @brief Constructs a KvsBuilder for the given KVS instance.
     * @param instance_id Unique identifier for the KVS instance.
     */
    explicit KvsBuilder(const InstanceId& instance_id);

    /**
     * @brief Specify the snapshot identifier to be loaded.
     * @param snapshot_id Unique identifier for the KVS snapshot.
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& snapshot(const SnapshotId& snapshot_id);

    /**
     * @brief Specify whether default values must be loaded.
     * @param flag True to require default values; false to make them optional.
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& need_defaults_flag(const OpenNeedDefaults& flag);

    /**
     * @brief Configure if KVS must exist when opening the KVS.
     * @param flag True to require an existing store; false to allow starting empty.
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& need_kvs_flag(const OpenNeedKvs& flag);

    /**
     * @brief Specify the directory where KVS files are stored.
     * @param dir The directory path as a string.
     * Use "" or "." for the current directory.
     *
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& dir(std::string&& dir_path);

    /**
     * @brief Specify the maximum number of rotated snapshots to keep in addition to the
     * current (`_0`) file. Must be in the range 0..3 (validated by `build()`); `0` disables
     * rotation entirely.
     * @param count The maximum snapshot count.
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& snapshot_max_count(std::size_t count);

    /**
     * @brief Configure whether the opened KVS rejects every mutating operation.
     * @param flag True to open read-only; false (default) for read-write.
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& read_only_flag(bool flag);

    /**
     * @brief Limit the serialized JSON payload size `flush()` is allowed to write.
     * @param size_bytes The maximum payload size in bytes; must be greater than zero
     * (validated by `build()`).
     * @return Reference to this builder (for chaining).
     */
    KvsBuilder& max_storage_size(std::size_t size_bytes);

    /**
     * @brief Builds and opens the Kvs instance with the configured options.
     *
     * Internally resolves the configuration-by-file entry (if any) for this builder's
     * `instance_id`, merges it over the configured settings with precedence
     * file entry > builder setter > built-in default, and calls `Kvs::open()` with the result.
     * The builder's own settings are read, never consumed, so `build()` may be called more than
     * once on the same instance.
     *
     * @return A score::Result<Kvs> containing the opened store or an ErrorCode. In particular,
     *         `ErrorCode::InvalidConfiguration` if a builder setting (`snapshot_max_count`,
     *         `max_storage_size`) or the resolved configuration-by-file entry is out of range.
     */
    score::Result<Kvs> build();

  private:
    InstanceId instance_id;          ///< ID of the KVS instance
    SnapshotId snapshot_id;          ///< ID of the KVS snapshot
    OpenNeedDefaults need_defaults;  ///< Whether default values are required
    OpenNeedKvs need_kvs;            ///< Whether an existing KVS is required
    std::string directory;           ///< Directory where to store the KVS Files
    std::size_t snapshot_max_count_setting;           ///< Maximum number of rotated snapshots
    bool read_only_setting;                           ///< Whether mutating operations are rejected
    std::optional<std::size_t> max_storage_size_setting;  ///< Maximum serialized payload size, if any
};

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_KVSBUILDER_HPP */
