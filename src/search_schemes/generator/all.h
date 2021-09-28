#pragma once

#include "backtracking.h"
#include "bestKnown.h"
#include "h2.h"
#include "kianfar.h"
#include "kucherov.h"
#include "optimum.h"
#include "pigeon.h"
#include "suffixFilter.h"
#include "zeroOnesZero.h"

#include <functional>
#include <string>
#include <tuple>
#include <vector>
#include <map>

namespace search_schemes::generator {

inline auto all = std::map<std::string, std::function<Scheme(int, int, int, int)>>{
    { "backtracking",   [](int minError, int maxError, int sigma, int dbSize) { return backtracking(1, minError, maxError); }},
    { "optimum",        [](int minError, int maxError, int sigma, int dbSize) { return optimum(minError, maxError); }},
    { "01*0",           [](int minError, int maxError, int sigma, int dbSize) { return zeroOnesZero_trivial(minError, maxError); }},
    { "01*0_opt",       [](int minError, int maxError, int sigma, int dbSize) { return zeroOnesZero_opt(minError, maxError); }},
    { "pigeon",         [](int minError, int maxError, int sigma, int dbSize) { return pigeon_trivial(minError, maxError); }},
    { "pigeon_opt",     [](int minError, int maxError, int sigma, int dbSize) { return pigeon_opt(minError, maxError); }},
    { "suffix",         [](int minError, int maxError, int sigma, int dbSize) { return suffixFilter(maxError+1, minError, maxError); }},
    { "kianfar",        [](int minError, int maxError, int sigma, int dbSize) { return kianfar(maxError); }},
    { "kucherov-k1",    [](int minError, int maxError, int sigma, int dbSize) { return kucherov(maxError+1, maxError); }},
    { "kucherov-k2",    [](int minError, int maxError, int sigma, int dbSize) { return kucherov(maxError+2, maxError); }},
    { "h2-k1",          [](int minError, int maxError, int sigma, int dbSize) { return h2(maxError+1, minError, maxError); }},
    { "h2-k2",          [](int minError, int maxError, int sigma, int dbSize) { return h2(maxError+2, minError, maxError); }},
    { "h2-k3",          [](int minError, int maxError, int sigma, int dbSize) { return h2(maxError+3, minError, maxError); }},
};

}
