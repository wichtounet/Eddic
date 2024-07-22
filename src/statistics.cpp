//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "statistics.hpp"

using namespace eddic;

void statistics::inc_counter(const std::string& a){
    ++counters[a];
}

std::size_t statistics::counter(const std::string& a) const {
    return counters.at(a);
}

std::size_t statistics::counter_safe(const std::string& a) const {
    if (counters.contains(a)) {
        return counters.at(a);
    }

    return 0;
}

statistics::iterator statistics::begin() const {
    return counters.cbegin();
}

statistics::iterator statistics::end() const {
    return counters.cend();
}
