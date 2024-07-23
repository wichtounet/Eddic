//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "Platform.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "Variable.hpp"

#include "mtac/RegisterAllocation.hpp"
#include "mtac/Program.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

void mtac::register_param_allocation(mtac::Program& program, Platform platform){
    timing_timer timer(program.context.timing(), "param_register_allocation");

    const auto *descriptor = getPlatformDescriptor(platform);

    auto maxInt = descriptor->numberOfIntParamRegisters();
    auto maxFloat = descriptor->numberOfFloatParamRegisters();

    for(auto& function_info : program.context.functions()){
        const auto& function = function_info.second;

        //Only custom functions have a context
        if(function.context()){
            for(unsigned int i = 0; i < function.parameters().size(); ++i){
                const auto& parameter = function.parameter(i);
                auto type = parameter.type();
                unsigned int position = function.parameter_position_by_type(parameter.name());
                auto param = function.context()->getVariable(parameter.name());

                if((mtac::is_single_int_register(type) && position <= maxInt) || (mtac::is_single_float_register(type) && position <= maxFloat)){
                    Position oldPosition = param->position();

                    function.context()->allocate_in_param_register(param, position);
                    
                    //We have to change the position of the all the following parameters
                    for(unsigned int j = i + 1; j < function.parameters().size(); ++j){
                        auto p = function.context()->getVariable(function.parameter(j).name());
                        Position paramPosition = p->position();
                        p->setPosition(oldPosition); 
                        oldPosition = paramPosition;
                    }
                }
            }
        }
    }
}
