//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef TAC_FUNCTION_H
#define TAC_FUNCTION_H

#include <memory>
#include <vector>

#include "tac/BasicBlock.hpp"

namespace eddic {

class FunctionContext;

namespace tac {

class Function {
    public:
        tac::BasicBlock& currentBlock();
        tac::BasicBlock& newBlock();

    private:
        std::shared_ptr<FunctionContext> context;

        std::vector<tac::BasicBlock> blocks;
};

} //end of tac

} //end of eddic

#endif
