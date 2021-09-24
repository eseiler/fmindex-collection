#include "isValid.h"

#include <cassert>
#include <algorithm>

namespace search_schemes {

namespace {
    /* check if pi is
     * - contiguous
     * - holds the connectivity property
     * - starts with 0
     */
    bool checkPiContiguous(decltype(Search::pi) pi) {
        assert(not pi.empty());
        auto minV = pi.front();
        auto maxV = pi.front();

        for (auto iter = ++pi.begin(); iter != pi.end(); ++iter) {
            if (*iter == maxV+1) {
                maxV = *iter;
            } else if (*iter+1 == minV) {
                minV = *iter;
            } else {
                return false;
            }
        }
        return minV == 0;
    }

    // check that if is monotonically increasing
    template <typename Collection>
    auto checkMonotonicallyIncreasing(Collection const& col) {
        assert(not col.empty());
        if (col.size() == 1) {
            return true;
        }
        auto iter = begin(col);
        for (auto lastIter = iter++; iter != end(col); lastIter = iter++) {
            if (*lastIter > *iter) {
                return false;
            }
        }
        return true;
    }
}

auto isValid(Search const& s) -> bool {
    // check if has valid size
    if (s.pi.empty()) {
        return false;
    }

    // check if all entries have same length
    if (s.pi.size() != s.u.size() or s.pi.size() != s.l.size()) {
        return false;
    }

    // check if pi is contiguous and start with 0
    if (not checkPiContiguous(s.pi)) {
        return false;
    }
    // check that l and u are monotonically increasing
    if (not checkMonotonicallyIncreasing(s.l)) {
        return false;
    }
    if (not checkMonotonicallyIncreasing(s.u)) {
        return false;
    }
    // check that l is always equal or smaller than u
    for (size_t i{0}; i < s.pi.size(); ++i) {
        if (s.l[i] > s.u[i]) {
            return false;
        }
    }
    return true;
}

auto isValid(Scheme const& ss) -> bool {
    // searching for an invalid scheme
    auto iter = std::find_if(begin(ss), end(ss), [&](auto const& s) {
        return not isValid(s) or s.pi.size() != ss.front().pi.size();
    });
    return iter == end(ss);
}

}
