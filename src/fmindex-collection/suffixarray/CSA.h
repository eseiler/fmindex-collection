// SPDX-FileCopyrightText: 2006-2023, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2023, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause
#pragma once


#include "../BitStack.h"
#include "../bitvector/Bitvector.h"
#include "../bitvector/CompactBitvector.h"
#include "concepts.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <tuple>


namespace fmindex_collection {

struct CSA {
    std::vector<uint64_t> ssa;
    bitvector::CompactBitvector bv;
    size_t samplingRate;      // distance between two samples (inside one sequence)
    size_t bitsForPosition;   // bits reserved for position
    size_t bitPositionMask;   // Bit mask, to extract the position from ssa
    size_t seqCount;          // Number of sequences


    static auto createJoinedCSA(CSA const& lhs, CSA const& rhs) -> CSA {
        auto csa = CSA{};
        size_t bitsForPosition = std::max(lhs.bitsForPosition, rhs.bitsForPosition);
        auto seqBits = size_t(std::ceil(std::log2(lhs.seqCount + rhs.seqCount)));
        if (seqBits + bitsForPosition > 64) {
            throw std::runtime_error{"Can't merge indices, requires more than 64bit"};
        }

        //!TODO sampling Rate is only required for FMTree localization, we should remove it
        csa.samplingRate    = std::max(lhs.samplingRate, rhs.samplingRate);
        csa.bitsForPosition = bitsForPosition;
        csa.bitPositionMask = (1ull<<bitsForPosition)-1;
        return csa;
    }


    CSA() = default;
    CSA(CSA const&) = delete;
    CSA(CSA&&) noexcept = default;

    template <std::ranges::range Range>
        requires requires(Range r) {
            {*(r.begin())} -> std::same_as<std::optional<std::tuple<size_t, size_t>>>;
        }
    CSA(Range _ssa, size_t sequencesCount, size_t longestSequence, size_t _samplingRate)
        : seqCount{sequencesCount}
    {

        samplingRate = _samplingRate; //!TODO samplingRate has to go

        bitsForPosition = size_t(std::ceil(std::log2(longestSequence)));
        size_t bitsForSeqId = longestSequence;
        if (bitsForPosition + bitsForSeqId > 64) {
            throw std::runtime_error{"requires more than 64bit to encode sequence length and number of sequence"};
        }
        bitPositionMask = (1ull<<bitsForPosition)-1;

        for (auto o : _ssa) {
            push_back(o);
        }
    }

    CSA(std::vector<uint64_t> _ssa, BitStack const& bitstack, size_t _samplingRate, size_t _bitsForPosition, size_t _seqCount)
        : ssa{std::move(_ssa)}
        , bv{bitstack.size, [&](size_t idx) {
            return bitstack.value(idx);
        }}
        , samplingRate{_samplingRate}
        , bitsForPosition{_bitsForPosition}
        , bitPositionMask{(1ull<<bitsForPosition)-1}
        , seqCount{_seqCount}
    {}
    CSA(std::vector<uint64_t> sa, size_t _samplingRate, std::span<std::tuple<size_t, size_t> const> _inputSizes, bool reverse=false)
        : bv {sa.size(), [&](size_t idx) {
            return (sa[idx] % _samplingRate) == 0;
        }}
        , samplingRate{_samplingRate}
        , seqCount{_inputSizes.size()}
    {
        size_t longestSequence = std::accumulate(_inputSizes.begin(), _inputSizes.end(), size_t{}, [](size_t lhs, auto rhs) {
            auto [len, delCt] = rhs;
            return std::max(lhs, len + delCt);
        });
        bitsForPosition = size_t(std::ceil(std::log2(longestSequence)));
        size_t bitsForSeqId = std::max(size_t{1}, size_t(std::ceil(std::log2(_inputSizes.size()))));
        if (bitsForPosition + bitsForSeqId > 64) {
            throw std::runtime_error{"requires more than 64bit to encode sequence length and number of sequence"};
        }
        bitPositionMask = (1ull<<bitsForPosition)-1;

        // Generate accumulated input
        auto accInputSizes = std::vector<uint64_t>{};
        accInputSizes.reserve(_inputSizes.size()+1);
        accInputSizes.emplace_back(0);
        for (size_t i{0}; i < _inputSizes.size(); ++i) {
            auto [len, delCt] = _inputSizes[i];
            accInputSizes.emplace_back(accInputSizes.back() + len + delCt);
        }

        // Construct sampled suffix array
        size_t ssaI{}; // Index of the ssa that is inside of sa
        for (size_t i{0}; i < sa.size(); ++i) {
            bool sample = (sa[i] % samplingRate) == 0;
            if (sample) {
                // find subject id
                auto iter = std::upper_bound(accInputSizes.begin(), accInputSizes.end(), sa[i]);
                size_t subjId = std::distance(accInputSizes.begin(), iter) - 1;
                auto subjPos = sa[i] - accInputSizes[subjId];
                if (reverse) {
                    auto [len, delCt] = _inputSizes[subjId];
                    if (subjPos < len) {
                        subjPos = len - subjPos;
                    } else {
                        subjPos = len+1;
                    }
                }
                sa[ssaI] = subjPos | (subjId << bitsForPosition);
                ++ssaI;
            }
        }
        sa.resize(ssaI);
        ssa = std::move(sa);
    }

    auto operator=(CSA const&) -> CSA& = delete;
    auto operator=(CSA&&) noexcept -> CSA& = default;

    size_t memoryUsage() const {
        return sizeof(ssa) + ssa.size() * sizeof(ssa.back());
    }

    auto value(size_t idx) const -> std::optional<std::tuple<uint64_t, uint64_t>> {
        if (!bv.symbol(idx)) {
            return std::nullopt;
        }
        auto v = ssa[bv.rank(idx)];
        auto chr = v >> bitsForPosition;
        auto pos = v & bitPositionMask;

        return std::make_tuple(chr, pos);
    }

    void push_back(std::optional<std::tuple<size_t, size_t>> value) {
        bv.push_back(value.has_value());
        if (value) {
            auto [seqNr, pos] = *value;
            ssa.push_back((seqNr << bitsForPosition) + pos);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(ssa, bv, samplingRate, bitsForPosition, bitPositionMask, seqCount);
    }
};
static_assert(SuffixArray_c<CSA>);

}
