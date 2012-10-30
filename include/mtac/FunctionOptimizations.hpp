//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef MTAC_FUNCTION_OPTIMIZATIONS_H
#define MTAC_FUNCTION_OPTIMIZATIONS_H

#include <memory>

#include "mtac/pass_traits.hpp"

namespace eddic {

namespace mtac {

class Program;

struct remove_unused_functions {
    bool operator()(std::shared_ptr<mtac::Program> program);
};

template<>
struct pass_traits<remove_unused_functions> {
    STATIC_CONSTANT(pass_type, type, pass_type::IPA);
    STATIC_STRING(name, "remove_unused_functions");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);
};

struct remove_empty_functions {
    bool operator()(std::shared_ptr<mtac::Program> program);
};

template<>
struct pass_traits<remove_empty_functions> {
    STATIC_CONSTANT(pass_type, type, pass_type::IPA);
    STATIC_STRING(name, "remove_empty_functions");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);
};

} //end of mtac

} //end of eddic

#endif
