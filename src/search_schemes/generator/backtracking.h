#pragma once

#include "../Scheme.h"
#include <numeric>


namespace search_schemes::generator {

inline auto backtracking(int N, int minK, int K) -> Scheme {
    assert(N > 0);
    assert(K >= minK);
    auto s = Search{std::vector<int>(N, 0), std::vector<int>(N, 0), std::vector<int>(N, K)};
    s.l.back() = minK;
    std::iota(begin(s.pi), end(s.pi), 0);
    return Scheme{s};
}

}
