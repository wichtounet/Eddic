//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_COMPLETE_LOOP_PEELING_H
#define MTAC_COMPLETE_LOOP_PEELING_H

#include <memory>

#include "mtac/pass_traits.hpp"
#include "mtac/forward.hpp"

namespace eddic {

namespace mtac {

struct complete_loop_peeling {
    mtac::Program& program;

    complete_loop_peeling(mtac::Program& program) : program(program){}

    bool gate(std::shared_ptr<Configuration> configuration);
    bool operator()(mtac::Function& function);
};

template<>
struct pass_traits<complete_loop_peeling> {
    STATIC_CONSTANT(pass_type, type, pass_type::CUSTOM);
    STATIC_STRING(name, "complete_loop_peeling");
    STATIC_CONSTANT(unsigned int, property_flags, PROPERTY_PROGRAM);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);
};

} //end of mtac

} //end of eddic

#endif
