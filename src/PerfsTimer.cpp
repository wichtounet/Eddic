//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "PerfsTimer.hpp"

using namespace eddic;

        PerfsTimer::PerfsTimer(const std::string& n) : name(n) {}
        
        PerfsTimer::~PerfsTimer(){
            if(option_defined("perfs")){
                std::cout << name << " took " << timer.elapsed() << "ms" << std::endl;
            }
        }
