#pragma once

#include "concepts.h"

#include <array>
#include <cstdint>
#include <vector>

namespace fmindex_collection {
namespace occtable {
namespace naive {

template <size_t TSigma>
struct OccTable {
    static constexpr size_t Sigma = TSigma;

    std::vector<std::array<uint64_t, Sigma>> occ{};
    std::array<uint64_t, Sigma+1> C{};

    static size_t expectedMemoryUsage(size_t length) {
        size_t C       = sizeof(uint64_t) * (Sigma+1);
        size_t entries = sizeof(uint64_t) * Sigma * (length+1);
        return C + entries;
    }

    OccTable(std::vector<uint8_t> const& _bwt) {
        occ.reserve(_bwt.size()+1);
        occ.push_back(std::array<uint64_t, Sigma>{});

        for (size_t i{0}; i < _bwt.size(); ++i) {
            occ.push_back(occ.back());
            occ.back()[_bwt[i]] += 1;
        }
        C[0] = 0;
        for (size_t i{0}; i < Sigma; ++i) {
            C[i+1] = occ.back()[i] + C[i];
        }
    }

    OccTable(cereal_tag) {}

    static auto name() -> std::string {
        return "Naive";
    }

    static auto extension() -> std::string {
        return "n";
    }

    size_t memoryUsage() const {
        return occ.size() * sizeof(occ.back()) + sizeof(OccTable);
    }

    uint64_t size() const {
        return C.back();
    }

    uint64_t rank(uint64_t idx, uint8_t symb) const {
        return occ[idx][symb] + C[symb];
    }

    uint64_t prefix_rank(uint64_t idx, uint8_t symb) const {
        uint64_t a{};
        for (size_t i{0}; i <= symb; ++i) {
            a += occ[idx][i];
        }
        return a;
    }

    size_t symbol(uint64_t idx) const {
        idx += 1;
        for (size_t i{0}; i < Sigma-1; ++i) {
            if (occ[idx][i] > occ[idx-1][i]) {
                return i;
            }
        }
        return Sigma-1;
    }

    auto all_ranks(uint64_t idx) const -> std::tuple<std::array<uint64_t, Sigma>, std::array<uint64_t, Sigma>> {
        std::array<uint64_t, Sigma> rs{0};
        std::array<uint64_t, Sigma> prs{0};
        uint64_t a{0};
        for (size_t i{0}; i < Sigma; ++i) {
            auto r = occ[idx][i];
            rs[i] = r + C[i];
            a += r;
            prs[i] = a;
        }
        return {rs, prs};
    }

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(occ, C);
    }
};
static_assert(checkOccTable<OccTable>);

}
}
}
