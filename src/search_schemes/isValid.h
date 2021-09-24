#pragma once

#include "Scheme.h"

#include <algorithm>
#include <cassert>

namespace search_schemes {

/* checks if Scheme is valid
 *
 */
auto isValid(Search const& s) -> bool;

// check if all schemes are valid and have same size
auto isValid(Scheme const& ss) -> bool;

}
