#pragma once

#include "concepts.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

#include <iostream>

namespace occtable {
namespace compactWavelet {

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


template <size_t TSigma>
struct Bitvector {
    static constexpr auto bitct = bits_count(TSigma);
    static constexpr auto bvct = pow(2, bitct);

    struct Block {

        std::array<uint32_t, TSigma> blocks{};
        std::array<uint64_t, bitct>  bits{};

        uint64_t rank(uint8_t idx, uint8_t symb) const {
            auto which_bv = [](uint8_t symb, auto cb) {
                size_t id{0};
                uint8_t mask = 1u << (bitct-1);
                for (size_t b{0}; b < bitct; ++b) {
                    auto bit = (symb & mask) != 0;
                    cb(id, bit);
                    id = id * 2 +1 + bit;
                    mask = mask >> 1;
                }
            };

            size_t s = 0;
            size_t l1 = 64;
            size_t l2 = idx;
            auto const& bbits = bits;
            size_t depth = 0;
            which_bv(symb, [&](size_t id, size_t bit) {
/*                if ((idx == 1 or idx == 2) and symb == 0) {
                    std::cout << "accessing " << id << " " << bit << "\n";
                }*/
                auto bits = std::bitset<64>(bbits[depth]) >> s;
                auto c1 = (bits << (64-l1)).count();
                auto d1 = (bits << (64-l2)).count();


                if (bit == 0) {
                    s = s;
                    l2 = l2 - d1;
                    l1 = l1 - c1;
                } else {
                    s = s + l1 - c1;
                    l2 = d1;
                    l1 = c1;
                }
                depth += 1;
            });
            return l2 + blocks[symb];
        }

        uint64_t prefix_rank(uint8_t idx, uint8_t symb) const {
            auto which_bv = [](uint8_t symb, auto cb) {
                size_t id{0};
                uint8_t mask = 1u << (bitct-1);
                for (size_t b{0}; b < bitct; ++b) {
                    auto bit = (symb & mask) != 0;
                    cb(id, bit);
                    id = id * 2 +1 + bit;
                    mask = mask >> 1;
                }
            };

            size_t s = 0;
            size_t l1 = 64;
            size_t l2 = idx;
            size_t a{};
            size_t depth = 0;
            auto const& bbits = bits;
            which_bv(symb+1, [&](size_t id, size_t bit) {
                auto bits = std::bitset<64>(bbits[depth]) >> s;
                auto c1 = (bits << (64-l1)).count();
                auto d1 = (bits << (64-l2)).count();

                auto c0 = l1 - c1;
                auto d0 = l2 - d1;
                if (bit == 0) {
                    s = s;
                    l2 = d0;
                    l1 = c0;
                } else {
                    a += d0;
                    s = s + c0;
                    l2 = d1;
                    l1 = c1;
                }
                depth += 1;
            });
            for (size_t i{0}; i <= symb; ++i) {
                a += blocks[i];
            }
            return a;
        }

        template <typename CB>
        void bvAccess(size_t idx, CB cb) const {
            bvAccess(0, 64, idx, cb);
/*            auto bv = std::bitset<64>(bits[0]);
            auto d1 = (bv << (64-idx)).count();
            auto d0 = idx - d1;
            auto c1 = bv.count();
            auto c0 = 64 - c1;
            bvAccess<1, 0>(0,  c0, d0, cb);
            bvAccess<1, 1>(c0, c1, d1, cb);*/
        }




        template <size_t Depth=0, size_t I=0, typename CB>
        void bvAccess(size_t s, size_t l1, size_t idx, CB cb) const {
/*            if (s == 64 or l1 == 0 or idx == 0) {
                bvAccess0<Depth, I>(cb);
                return;
            }*/
            auto bv = std::bitset<64>(bits[Depth]) >> s;
            auto d1 = (bv << (64-idx)).count();
            auto d0 = idx - d1;
            if constexpr (Depth < bitct-1) {
                auto c1 = (bv << (64-l1)).count();
                auto c0 = l1 - c1;
                bvAccess<Depth+1, (I << 1)>  (s,    c0, d0, cb);
                bvAccess<Depth+1, (I << 1)+1>(s+c0, c1, d1, cb);
            } else {
                cb(d0, d1, std::index_sequence<(I << 1)>{});
            }
        }
        template <size_t Depth, size_t I, typename CB>
        void bvAccess0(CB cb) const {
            if constexpr (Depth < bitct-1) {
                bvAccess0<Depth+1, (I << 1)>(cb);
                bvAccess0<Depth+1, (I << 1)+1>(cb);
            } else {
                cb(0, 0, std::index_sequence<(I << 1)>{});
            }
        }

        auto all_ranks(uint8_t idx) const -> std::array<uint64_t, TSigma> {
            std::array<uint64_t, TSigma> rs{0};
//            if (idx > 0) {
                bvAccess(idx, [&]<size_t I>(size_t d0, size_t d1, std::index_sequence<I>) {
                    if constexpr (I+1 < TSigma) {;
                        rs[I]    = d0 + blocks[I];
                        rs[I+1]  = d1 + blocks[I+1];
                    }
                });
  /*          } else {
                for (size_t i{0}; i < TSigma; ++i) {
                    rs[i] = blocks[i];
                }
            }*/

            return rs;
        }
    };


    std::vector<Block> blocks;
    std::vector<std::array<uint64_t, TSigma>> superBlocks;
    std::array<uint64_t, TSigma+1> C;

    template <typename CB>
    Bitvector(size_t length, CB cb) {
        blocks.reserve(length/64+2);

        std::array<uint64_t, bvct> sblock_acc{0};
        std::array<uint32_t, bvct> block_acc{0};
        std::array<uint64_t, bvct> bv_size{};
        for (auto& c : bv_size) {
            c = 0;
        }


        auto which_bv = [](uint8_t symb, auto cb) {
            size_t id{0};
            uint8_t mask = 1u << (bitct-1);
            for (size_t b{0}; b < bitct; ++b) {
                auto bit = (symb & mask) != 0;
                cb(id, bit);
                id = id * 2 +1 + bit;
                mask = mask >> 1;
            }
        };

        std::array<std::vector<uint8_t>, bvct> waveletcount{};
        auto insertCount = [&]() {
            if (!blocks.empty()) {
                auto& bits = blocks.back().bits;
                size_t idx{0};
                for (size_t i{0}; i < waveletcount.size(); ++i) {
    //                std::cout << "id3: " << i << " " << waveletcount.size() << "\n";
                    auto t = waveletcount.at(i);
                    for (uint64_t bit : t) {
                        bits[idx / 64] = bits[idx / 64] | (bit << (idx % 64));
                        ++idx;
                    }
                }
            }
            if (blocks.size() % (1ul<<26) == 0) { // new super block + new block
                superBlocks.emplace_back();
                for (size_t j{0}; j < TSigma; ++j) {
                    superBlocks.back()[j] = sblock_acc[j];
                }
                block_acc = {};
            }

            blocks.emplace_back();
            for (size_t i{0}; i < TSigma; ++i) {
                blocks.back().blocks[i] = block_acc[i];
            }
            for (auto& c : waveletcount) {
                c.clear();
            }
        };

/*        which_bv(bvct-1, [&](size_t id, size_t bit) {
            waveletcount[id].push_back(bit);
        });
        sblock_acc[bvct-1] += 1;
        block_acc[bvct-1]  += 1;*/

        for (size_t size{0}; size < length; ++size) {
            if (size % 64 == 0) { // new block
               insertCount();
            }

            auto blockId      = size >>  6;
            auto superBlockId = size >> 32;
//            auto bitId        = size &  63;

            auto symb = cb(size);
//            std::cout << "symb: " << int(symb) << " " << size << "\n";

            which_bv(symb, [&](size_t id, size_t bit) {
//               std::cout << "id: " << id << " " << waveletcount.size() << "\n";
//               std::cout << waveletcount[id].size() << "\n";
               uint8_t bit2 = bit;
                waveletcount[id].push_back(bit2);
            });
//            std::cout << "done\n";


//            auto& bits = blocks[blockId].bits[symb];
//            bits = bits | (1ul << bitId);
            block_acc[symb] += 1;
            sblock_acc[symb] += 1;
        }
        while (waveletcount[0].size() < 64) {
            which_bv(bvct-1, [&](size_t id, size_t bit) {
//                     std::cout << "id2: " << id << "\n";
                waveletcount[id].push_back(bit);
            });
            block_acc[bvct-1] += 1;
            sblock_acc[bvct-1] += 1;
        }
        insertCount();

        C[0] = 0;
        for (size_t i{0}; i < TSigma; ++i) {
            C[i+1] = sblock_acc[i] + C[i];
        }
    };


    uint64_t rank(uint64_t idx, uint8_t symb) const {
        auto blockId      = idx >>  6;
        auto superBlockId = idx >> 32;
        auto bitId        = idx &  63;
        return blocks[blockId].rank(bitId, symb) + superBlocks[superBlockId][symb] + C[symb];
    }

    uint64_t prefix_rank(uint64_t idx, uint8_t symb) const {
        auto blockId      = idx >>  6;
        auto superBlockId = idx >> 32;
        auto bitId        = idx &  63;
        uint64_t a={};
        for (size_t i{0}; i<= symb; ++i) {
            a += superBlocks[superBlockId][i];
        }
        return blocks[blockId].prefix_rank(bitId, symb) + a;
    }

    auto all_ranks(uint64_t idx) const -> std::tuple<std::array<uint64_t, TSigma>, std::array<uint64_t, TSigma>> {
        auto blockId      = idx >>  6;
        auto superBlockId = idx >> 32;
        auto bitId        = idx &  63;

        auto rs = blocks[blockId].all_ranks(bitId);
        std::array<uint64_t, TSigma> prs{0};

        rs[0] += superBlocks[superBlockId][0];
        prs[0] = rs[0];
        for (size_t i{1}; i < TSigma; ++i) {
            rs[i] += superBlocks[superBlockId][i];
            prs[i] = prs[i-1] + rs[i];
            rs[i] += C[i];
        }
        rs[0] += C[0];

        return {rs, prs};
    }

};


template <size_t TSigma>
struct OccTable {
    static constexpr size_t Sigma = TSigma;

    Bitvector<Sigma> bitvector;
    size_t size_{};

    static size_t expectedMemoryUsage(size_t length) {
        using Block = typename Bitvector<TSigma>::Block;
        auto blockSize = std::max(alignof(Block), sizeof(Block));

        size_t C           = sizeof(uint64_t) * (Sigma+1);
        size_t blocks      = blockSize        * (length+1) / 64;
        size_t superblocks = sizeof(uint64_t) * (length+1) / (1ul << 32);
        return C + blocks + superblocks;
    }


    OccTable(std::vector<uint8_t> const& _bwt)
        : bitvector(_bwt.size(), [&](size_t i) -> uint8_t {
            return _bwt[i];
        })
        , size_{_bwt.size()}
    {}

    uint64_t size() const {
        return size_;
    }


    uint64_t rank(uint64_t idx, uint8_t symb) const {
        return bitvector.rank(idx, symb);
    }

    uint64_t prefix_rank(uint64_t idx, uint8_t symb) const {
        return bitvector.prefix_rank(idx, symb);
    }

    auto all_ranks(uint64_t idx) const -> std::tuple<std::array<uint64_t, Sigma>, std::array<uint64_t, Sigma>> {
        return bitvector.all_ranks(idx);
    }

};
static_assert(checkOccTable<OccTable>);

}
}
