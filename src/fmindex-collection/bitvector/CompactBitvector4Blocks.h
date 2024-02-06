// SPDX-FileCopyrightText: 2006-2023, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2023, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <array>
#include <bitset>
#include <cassert>
#if __has_include(<cereal/archives/binary.hpp>)
#include <cereal/archives/binary.hpp>
#endif
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <span>
#include <vector>

namespace fmindex_collection::bitvector {

/**
 * CompactBitvector with interleaved superblocks, blocks and bits
 *
 * - Each group consist of 256bits, divided into 4 blocks.
 * - Each block uses 8bits to represents a value (4*8bits = 32bits).
 * - Superblock consist of a single 64bit number
 *
 *   For 256bits, we need 352bits, or 1.375bits to save a single bit
 */
struct CompactBitvector4Blocks {
    struct Superblock {
        uint64_t superBlockEntry;
        std::array<uint8_t, 4>  blocks;
        std::array<uint64_t, 4> bits;

        uint64_t rank(size_t idx) const noexcept {
            assert(idx < 256);
            auto blockId = idx >> 6;
            auto bitId   = idx & 63;
            auto maskedBits = bits[blockId] << (63-bitId);
            auto ct = std::bitset<64>{maskedBits}.count();

            auto total = superBlockEntry + blocks[blockId] + ct;
            return total;
        }

        bool value(size_t idx) const noexcept {
            auto blockId = idx >> 6;
            auto bitId   = idx & 63;
            return (bits[blockId] >> bitId) & 1;
        }

        void setBlock(size_t blockId, size_t value) {
            blocks[blockId] = value;
        }

        template <typename Archive>
        void serialize(Archive& ar) {
            ar(superBlockEntry, blocks, bits);
        }
    };

    size_t totalLength;
    std::vector<Superblock> superblocks{};

    CompactBitvector4Blocks(std::span<uint8_t const> _text)
        : CompactBitvector4Blocks{_text.size(), [&](size_t i) {
            return _text[i] != 0;
        }}
    {}

    template <typename CB>
    CompactBitvector4Blocks(size_t length, CB cb) {
        totalLength = length;

        // Next three lines are a reserve call, with zero initialization
        // This is required, so padding bytes will also be zero
        superblocks.resize(length/256+1);
        memset((void*)superblocks.data(), 0, superblocks.size() * sizeof(Superblock));
        superblocks.resize(0);

        superblocks.emplace_back(Superblock{});
        uint64_t sblock_acc{};
        uint16_t block_acc{};

        for (size_t size{1}; size <= length; ++size) {
            if (size % 256 == 0) { // new super block + new block
                superblocks.emplace_back(Superblock{});
                superblocks.back().superBlockEntry = sblock_acc;
                block_acc = 0;
            } else if (size % 64 == 0) { // new block
                superblocks.back().setBlock((size % 256) / 64, block_acc);
            }

            auto blockId      = (size >>  6) % 4;
            auto bitId        = size &  63;

            if (cb(size-1)) {
                auto& bits = superblocks.back().bits[blockId];
                bits = bits | (1ull << bitId);

                block_acc  += 1;
                sblock_acc += 1;
            }
        }
    }

    CompactBitvector4Blocks() {}
    CompactBitvector4Blocks(CompactBitvector4Blocks const&) = default;
    CompactBitvector4Blocks(CompactBitvector4Blocks&&) noexcept = default;
    auto operator=(CompactBitvector4Blocks const&) -> CompactBitvector4Blocks& = default;
    auto operator=(CompactBitvector4Blocks&&) noexcept -> CompactBitvector4Blocks& = default;


    size_t size() const noexcept {
        return totalLength;
    }

    bool symbol(size_t idx) const noexcept {
        idx += 1;
        auto superblockId = idx >> 8;
        auto bitId        = idx % 256;
        return superblocks[superblockId].value(bitId);
    }

    uint64_t rank(size_t idx) const noexcept {
        auto superblockId = idx >> 8;
        auto bitId        = idx % 256;
        return superblocks[superblockId].rank(bitId);
    }


    template <typename Archive>
    void serialize(Archive& ar) {
        // 0 version: slow path
        // 1 version: binary path (fast)
        auto version = []() {
#if __has_include(<cereal/archives/binary.hpp>)
            if constexpr (std::same_as<Archive, cereal::BinaryOutputArchive>
                            || std::same_as<Archive, cereal::BinaryInputArchive>) {
                return 1;
            }
#endif
            return 0;
        }();
        ar(version);

        if (version == 0) {
            ar(superblocks);
        } else if (version == 1) {
#if __has_include(<cereal/archives/binary.hpp>)
            if constexpr (std::same_as<Archive, cereal::BinaryOutputArchive>
                            || std::same_as<Archive, cereal::BinaryInputArchive>) {
                auto l = superblocks.size();
                ar(l);
                superblocks.resize(l);
                ar(cereal::binary_data(superblocks.data(), l * sizeof(Superblock)));
            } else
#endif
            throw std::runtime_error("fmindex-collection - CompactBitvector4Blocks was created with binary data, but this is not available in this app");
        } else {
            throw std::runtime_error("fmindex-collection - CompactBitvector4Blocks was created with legacy format - not readable by this app");
        }
    }
};

}