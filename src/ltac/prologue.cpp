//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#define BOOST_NO_RTTI
#define BOOST_NO_TYPEID
#include <boost/range/adaptors.hpp>

#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "Variable.hpp"

#include "ltac/Utils.hpp"
#include "ltac/Printer.hpp"
#include "ltac/Address.hpp"
#include "ltac/Instruction.hpp"
#include "ltac/prologue.hpp"

using namespace eddic;

namespace {

std::set<ltac::Register> parameter_registers(eddic::Function& function, Platform platform, std::shared_ptr<Configuration> configuration){
    std::set<ltac::Register> overriden_registers;

    auto descriptor = getPlatformDescriptor(platform);

    if(function.standard() || configuration->option_defined("fparameter-allocation")){
        unsigned int maxInt = descriptor->numberOfIntParamRegisters();

        for(auto& parameter : function.parameters()){
            auto type = parameter.type();
            unsigned int position = function.parameter_position_by_type(parameter.name());

            if(mtac::is_single_int_register(type) && position <= maxInt){
                overriden_registers.insert(ltac::Register(descriptor->int_param_register(position)));
            }
        }
    }

    return overriden_registers;
}

std::set<ltac::FloatRegister> float_parameter_registers(eddic::Function& function, Platform platform, std::shared_ptr<Configuration> configuration){
    std::set<ltac::FloatRegister> overriden_float_registers;
    
    auto descriptor = getPlatformDescriptor(platform);

    if(function.standard() || configuration->option_defined("fparameter-allocation")){
        unsigned int maxFloat = descriptor->numberOfFloatParamRegisters();

        for(auto& parameter : function.parameters()){
            auto type = parameter.type();
            unsigned int position = function.parameter_position_by_type(parameter.name());

            if(mtac::is_single_float_register(type) && position <= maxFloat){
                overriden_float_registers.insert(ltac::FloatRegister(descriptor->float_param_register(position)));
            }
        }
    }

    return overriden_float_registers;
}

bool callee_save(Function& definition, ltac::Register reg, Platform platform, std::shared_ptr<Configuration> configuration){
    auto return_type = definition.return_type();
    auto descriptor = getPlatformDescriptor(platform);

    //Do not save the return registers
    if((return_type == INT || return_type == BOOL || return_type == CHAR) && reg.reg == descriptor->int_return_register1()){
        return false;
    } else if(return_type == STRING && (reg.reg == descriptor->int_return_register1() || reg.reg == descriptor->int_return_register2())){
        return false;
    } else if(return_type->is_pointer() && reg.reg == descriptor->int_return_register1()){ 
        return false;
    }

    auto parameters = parameter_registers(definition, platform, configuration);

    //Do not save the parameters registers, they are saved by the caller if necessary
    if(parameters.count(reg)){
        return false;
    }

    return true;
}

bool callee_save(eddic::Function& definition, ltac::FloatRegister reg, Platform platform, std::shared_ptr<Configuration> configuration){
    auto return_type = definition.return_type();
    auto descriptor = getPlatformDescriptor(platform);

    //Do not save the return register
    if(return_type == FLOAT && reg.reg == descriptor->float_return_register()){
        return false;
    } 

    auto parameters = float_parameter_registers(definition, platform, configuration);

    //Do not save the parameters registers, they are saved by the caller if necessary
    if(parameters.count(reg)){
        return false;
    }

    return true;
}

void callee_save_registers(mtac::Function& function, mtac::basic_block_p bb, Platform platform, std::shared_ptr<Configuration> configuration){
    //Save registers for all other functions than main
    if(!function.is_main()){
        bool omit_fp = configuration->option_defined("fomit-frame-pointer");

        auto it = bb->l_statements.begin()+1;
        if(!omit_fp){
            ++it;
        }

        for(const auto& reg : function.use_registers()){
            if(callee_save(function.definition(), reg, platform, configuration)){
                it = bb->l_statements.insert(it, ltac::Instruction(ltac::Operator::PUSH, reg));
                ++it;
            }
        }

        for(const auto& float_reg : function.use_float_registers()){
            if(callee_save(function.definition(), float_reg, platform, configuration)){
                it = bb->l_statements.insert(it, ltac::Instruction(ltac::Operator::SUB, ltac::SP, static_cast<int>(FLOAT->size())));
                ++it;
                it = bb->l_statements.insert(it, ltac::Instruction(ltac::Operator::FMOV, ltac::Address(ltac::SP, 0), float_reg));
                ++it;
            }
        }
    }
}

void callee_restore_registers(mtac::Function& function, mtac::basic_block_p bb, Platform platform, std::shared_ptr<Configuration> configuration){
    //Save registers for all other functions than main
    if(!function.is_main()){
        for(const auto& float_reg : boost::adaptors::reverse(function.use_float_registers())){
            if(callee_save(function.definition(), float_reg, platform, configuration)){
                bb->emplace_back_low(ltac::Operator::FMOV, float_reg, ltac::Address(ltac::SP, 0));
                bb->emplace_back_low(ltac::Operator::ADD, ltac::SP, static_cast<int>(FLOAT->size()));
            }
        }

        for(const auto& reg : boost::adaptors::reverse(function.use_registers())){
            if(callee_save(function.definition(), reg, platform, configuration)){
                bb->emplace_back_low(ltac::Operator::POP, reg);
            }
        }
    }
}

template<typename It>
void callee_restore_registers(mtac::Function& function, It& it, Platform platform, std::shared_ptr<Configuration> configuration){
    //Save registers for all other functions than main
    if(!function.is_main()){
        for(const auto& reg : function.use_registers()){
            if(callee_save(function.definition(), reg, platform, configuration)){
                it.insert(ltac::Instruction(ltac::Operator::POP, reg));
            }
        }

        for(const auto& float_reg : function.use_float_registers()){
            if(callee_save(function.definition(), float_reg, platform, configuration)){
                it.insert(ltac::Instruction(ltac::Operator::ADD, ltac::SP, static_cast<int>(FLOAT->size())));
                it.insert(ltac::Instruction(ltac::Operator::FMOV, float_reg, ltac::Address(ltac::SP, 0)));
            }
        }
    }
}

template<typename T>
bool contains(const std::set<T>& set, const T& value){
    return set.find(value) != set.end();
}

template<typename T>
bool contains(const std::unordered_set<T>& set, const T& value){
    return set.find(value) != set.end();
}

bool caller_save(mtac::Function& source, eddic::Function& target_definition, ltac::Register reg, Platform platform, std::shared_ptr<Configuration> configuration){
    auto source_parameters = parameter_registers(source.definition(), platform, configuration);
    auto target_parameters = parameter_registers(target_definition, platform, configuration);
    auto variable_registers = source.variable_registers();

    //Only saved is used to hold a variable (not bound) or as a parameter in the source function
    if(contains(target_parameters, reg) && (contains(variable_registers, reg) || contains(source_parameters, reg))){
        return true;
    }

    return false;
}

bool caller_save(mtac::Function& source, eddic::Function& target_definition, ltac::FloatRegister reg, Platform platform, std::shared_ptr<Configuration> configuration){
    auto source_parameters = float_parameter_registers(source.definition(), platform, configuration);
    auto target_parameters = float_parameter_registers(target_definition, platform, configuration);
    auto variable_registers = source.variable_float_registers();

    //Only saved is used to hold a variable (not bound) or as a parameter in the source function
    if(contains(target_parameters, reg) && (contains(variable_registers, reg) || contains(source_parameters, reg))){
        return true;
    }

    return false;
}

template<typename It>
void caller_save_registers(mtac::Function& function, eddic::Function& target_function, mtac::basic_block_p bb, It it, Platform platform, std::shared_ptr<Configuration> configuration){
    auto pre_it = it.it;

    while(true){
        while(pre_it != bb->l_statements.begin()){
            --pre_it;
            
            auto& statement = *pre_it;

            if(statement.op == ltac::Operator::PRE_PARAM){
                statement.op = ltac::Operator::NOP;

                for(const auto& float_reg : boost::adaptors::reverse(function.use_float_registers())){
                    if(caller_save(function, target_function, float_reg, platform, configuration)){
                        pre_it = bb->l_statements.insert(pre_it, ltac::Instruction(ltac::Operator::FMOV, ltac::Address(ltac::SP, 0), float_reg));
                        pre_it = bb->l_statements.insert(pre_it, ltac::Instruction(ltac::Operator::SUB, ltac::SP, static_cast<int>(FLOAT->size())));
                    }
                }

                for(const auto& reg : boost::adaptors::reverse(function.use_registers())){
                    if(caller_save(function, target_function, reg, platform, configuration)){
                        pre_it = bb->l_statements.insert(pre_it, ltac::Instruction(ltac::Operator::PUSH, reg));
                    }
                }

                return;
            }
        }

        bb = bb->prev;
        pre_it = bb->l_statements.end();
    }
}

template<typename It>
void find(It& it, std::size_t uid){
    while(true){
        if(it->uid() == uid){
            return;
        }

        ++it;
    }
}

template<typename It>
void caller_cleanup(mtac::Function& function, eddic::Function& target_function, mtac::basic_block_p bb, It it, Platform platform, std::shared_ptr<Configuration> configuration){
    auto call_uid = it->uid();

    caller_save_registers(function, target_function, bb, it, platform, configuration);

    //The iterator has been invalidated by the save, find the call again
    it.restart();
    find(it, call_uid);

    if(it.has_next()){
        ++it;

        bool rewind = true;

        auto& instruction = *it;

        if(instruction.op == ltac::Operator::ADD){
            if(auto* reg_ptr = boost::get<ltac::Register>(&*instruction.arg1)){
                if(*reg_ptr == ltac::SP){
                    rewind = false;
                }
            }
        }

        if(rewind){
            --it;
        }
    }

    for(const auto& float_reg : boost::adaptors::reverse(function.use_float_registers())){
        if(caller_save(function, target_function, float_reg, platform, configuration)){
            it.insert_after(ltac::Instruction(ltac::Operator::FMOV, float_reg, ltac::Address(ltac::SP, 0)));
            it.insert_after(ltac::Instruction(ltac::Operator::ADD, ltac::SP, static_cast<int>(FLOAT->size())));
        }
    }
    
    for(const auto& reg : boost::adaptors::reverse(function.use_registers())){
        if(caller_save(function, target_function, reg, platform, configuration)){
            it.insert_after(ltac::Instruction(ltac::Operator::POP, reg));
        }
    }
}

} //End of anonymous

void ltac::generate_prologue_epilogue(mtac::Program& program, std::shared_ptr<Configuration> configuration){
    timing_timer timer(program.context.timing(), "prologue_generation");

    bool omit_fp = configuration->option_defined("fomit-frame-pointer");
    const auto platform = program.context.target_platform();

    for(auto& function : program.functions){
        auto size = function.context->size();

        //Align stack pointer to the size of an INT

        if(size % INT->size() != 0){
            int padding = INT->size() - (size % INT->size());
            size += padding;
        }

        //1. Generate prologue
        
        auto bb = function.entry_bb();
    
        //Enter stack frame
        if(!omit_fp){
            bb->l_statements.insert(bb->l_statements.begin(), ltac::Instruction(ltac::Operator::ENTER));
            
            //Allocate stack space for locals
            bb->l_statements.insert(bb->l_statements.begin()+1, ltac::Instruction(ltac::Operator::SUB, ltac::SP, size));
        } else {
            //Allocate stack space for locals
            bb->l_statements.insert(bb->l_statements.begin(), ltac::Instruction(ltac::Operator::SUB, ltac::SP, size));
        }

        callee_save_registers(function, bb, platform, configuration);

        //2. Generate epilogue

        bb = function.exit_bb();

        callee_restore_registers(function, bb, platform, configuration);

        bb->emplace_back_low(ltac::Operator::ADD, ltac::SP, size);

        //Leave stack frame
        if(!omit_fp){
            bb->emplace_back_low(ltac::Operator::LEAVE);
        }

        bb->emplace_back_low(ltac::Operator::RET);
        
        //3. Generate epilogue for each unresolved RET
        
        for(auto& bb : function){
            auto it = iterate(bb->l_statements);

            while(it.has_next()){
                auto& statement = *it;

                if(statement.op == ltac::Operator::PRE_RET){
                    statement.op = ltac::Operator::RET;

                    //Leave stack frame
                    if(!omit_fp){
                        it.insert(ltac::Instruction(ltac::Operator::LEAVE));
                    }

                    it.insert(ltac::Instruction(ltac::Operator::ADD, ltac::SP, size));

                    callee_restore_registers(function, it, platform, configuration);
                }

                ++it;
            }
        }

        //4. Generate caller save/restore code
       
        for(auto& bb : function){
            auto it = iterate(bb->l_statements);

            while(it.has_next()){
                auto& statement = *it;
                auto uid = it->uid();

                if(statement.op == ltac::Operator::CALL){
                    caller_cleanup(function, *statement.target_function, bb, it, platform, configuration);

                    //The iterator is invalidated by the cleanup, necessary to find the call again
                    it.restart();
                    find(it, uid);
                }

                ++it;
            }
        }
    }
}
