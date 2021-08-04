#pragma once

#include "concepts.h"

#include <array>
#include <cstdint>
#include <vector>
#include <filesystem>

#include <sdsl/suffix_trees.hpp>
#include <sdsl/construct.hpp>

#include <iostream>

namespace occtable {
namespace sdsl_wt_bldc {

constexpr inline size_t bits_count(size_t y) {
    if (y == 0) return 1;
    size_t i{0};
    while (y != 0) {
        y = y >> 1;
        ++i;
    }
    return i;
}
constexpr inline size_t pow(size_t b, size_t y) {
    if (y == 0) return 1;
    return pow(b, (y-1)) * b;
}


inline void writeFile(std::filesystem::path const& file, std::vector<uint8_t> const& buffer) {
    auto ofs = std::ofstream{file, std::ios::binary};
    ofs.write(reinterpret_cast<char const*>(buffer.data()), buffer.size());
}


using sdsl_wt_index_type =
    sdsl::wt_blcd<sdsl::bit_vector, // Wavelet tree type
                    sdsl::rank_support_v<1>,
                    sdsl::select_support_scan<>,
                    sdsl::select_support_scan<0>>;

template <size_t TSigma>
struct OccTable {


    sdsl_wt_index_type index;
    static constexpr size_t Sigma = TSigma;

    std::array<uint64_t, Sigma+1> C{};

    static size_t expectedMemoryUsage(size_t length) {
        size_t C       = sizeof(uint64_t) * (Sigma+1);
        size_t bits    = bits_count(Sigma+1);
        size_t entries = bits * (length+1) / 8;

        return C + entries;
    }

    OccTable(std::vector<uint8_t> _bwt) {
        for (auto& c : _bwt) {
            c += 1;
        }
        writeFile("tmp.sdsl.tmp", _bwt);
        sdsl::construct(index, "tmp.sdsl.tmp", 1);

/*        for (size_t i{0}; i < index.size(); ++i) {
            std::cout << i << ": " << int (_bwt[i]) << "   ";
            for (size_t j{0}; j <= Sigma; ++j) {
                std::cout << index.rank(i, j) << " ";
            }
            std::cout << "\n";
        }*/


        C[0] = 0;
        for (size_t i{0}; i < Sigma; ++i) {
            auto r = index.rank(_bwt.size(), i+1);
            C[i+1] = r + C[i];
        }
    }

    uint64_t size() const {
        return C.back();
    }

    uint64_t rank(uint64_t idx, uint8_t symb) const {
        return index.rank(idx, symb+1) + C[symb];
    }

    uint64_t prefix_rank(uint64_t idx, uint8_t symb) const {
        uint64_t a{};
        for (size_t i{0}; i <= symb; ++i) {
            a += index.rank(idx, i+1);
        }
        return a;
    }

    auto all_ranks(uint64_t idx) const -> std::tuple<std::array<uint64_t, Sigma>, std::array<uint64_t, Sigma>> {
        std::array<uint64_t, Sigma> rs{0};
        std::array<uint64_t, Sigma> prs{0};
        for (size_t i{0}; i < Sigma; ++i) {
            rs[i] = rank(idx, i);
            prs[i] = prefix_rank(idx, i);
        }
        return {rs, prs};
    }

};

static_assert(checkOccTable<OccTable>);

}
}