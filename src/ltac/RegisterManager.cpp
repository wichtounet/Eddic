//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <set>

#include "cpp_utils/assert.hpp"

#include "FunctionContext.hpp"
#include "Type.hpp"
#include "Options.hpp"
#include "logging.hpp"
#include "GlobalContext.hpp"
#include "Variable.hpp"

#include "ltac/Utils.hpp"
#include "ltac/RegisterManager.hpp"
#include "ltac/StatementCompiler.hpp"
#include "ltac/Register.hpp"
#include "ltac/FloatRegister.hpp"
#include "ltac/PseudoRegister.hpp"
#include "ltac/PseudoFloatRegister.hpp"

#include "mtac/Utils.hpp"

using namespace eddic;

namespace {

template<typename Reg> 
Reg get_pseudo_reg(as::PseudoRegisters<Reg>& registers, std::shared_ptr<Variable> variable){
    //The variable is already in a register
    if(registers.inRegister(variable)){
        return registers[variable];
    }

    return registers.get_new_reg();
}

} //end of anonymous namespace
    
ltac::RegisterManager::RegisterManager(FloatPool& float_pool) : float_pool(float_pool){
        //Nothing else to init
}

void ltac::RegisterManager::reset(){
    written.clear();
    local.clear();
}

void ltac::RegisterManager::copy(mtac::Argument argument, ltac::PseudoFloatRegister reg){
    assert(ltac::is_variable(argument) || mtac::isFloat(argument));

    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;
        
        //If the variable is hold in a register, just move the register value
        if(pseudo_float_registers.inRegister(variable)){
            auto old_reg = pseudo_float_registers[variable];
            bb->emplace_back_low(ltac::Operator::FMOV, reg, old_reg);
        } else {
            auto position = variable->position();

            assert(position.isStack() || position.isGlobal() || position.isParameter());

            if(position.isParameter() || position.isStack()){
                bb->emplace_back_low(ltac::Operator::FMOV, reg, ltac::Address(ltac::BP, position.offset()));
            } else if(position.isGlobal()){
                bb->emplace_back_low(ltac::Operator::FMOV, reg, ltac::Address("V" + position.name()));
            } 
        }
    } else if(auto* ptr = boost::get<double>(&argument)){
        auto label = float_pool.label(*ptr);
        bb->emplace_back_low(ltac::Operator::FMOV, reg, ltac::Address(label));
    } else {
        auto label = float_pool.label(static_cast<double>(boost::get<int>(argument)));
        bb->emplace_back_low(ltac::Operator::FMOV, reg, ltac::Address(label));
    }
}

void ltac::RegisterManager::copy(mtac::Argument argument, ltac::PseudoRegister reg, tac::Size size){
    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;
        
        //If the variable is hold in a register, just move the register value
        if(pseudo_registers.inRegister(variable)){
            auto old_reg = pseudo_registers[variable];
            
            bb->emplace_back_low(ltac::Operator::MOV, reg, old_reg);
        } else {
            auto position = variable->position();

            cpp_assert(position.isStack() || position.isGlobal() || position.isParameter(), (variable->name() + " is not in a register").c_str());

            if(position.isParameter() || position.isStack()){
                if(variable->type() == CHAR || variable->type() == BOOL){
                    bb->emplace_back_low(ltac::Operator::MOV, reg, ltac::Address(ltac::BP, position.offset()), tac::Size::BYTE);
                } else {
                    bb->emplace_back_low(ltac::Operator::MOV, reg, ltac::Address(ltac::BP, position.offset()), size);
                }
            } else if(position.isGlobal()){
                bb->emplace_back_low(ltac::Operator::MOV, reg, ltac::Address("V" + position.name()), size);
            }
        }
    } else {
        //If it's a constant (int, double, string), just move it
        bb->emplace_back_low(ltac::Operator::MOV, reg, to_arg(argument, *this));
    }
}

void ltac::RegisterManager::move(mtac::Argument argument, ltac::PseudoRegister reg){
    copy(argument, reg);

    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        //The variable is now held in the new register
        pseudo_registers.setLocation(*ptr, reg);
    }
}

void ltac::RegisterManager::move(mtac::Argument argument, ltac::PseudoFloatRegister reg){
    assert(ltac::is_variable(argument) || mtac::isFloat(argument));
    
    copy(argument, reg);

    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        //The variable is now held in the new register
        pseudo_float_registers.setLocation(*ptr, reg);
    } 
}

bool is_local(std::shared_ptr<Variable> var, ltac::RegisterManager& manager){
    return var->position().isParameter() || (manager.is_escaped(var) && !var->position().isParamRegister()) || var->position().isStack();
}
        
ltac::PseudoRegister ltac::RegisterManager::get_pseudo_reg(std::shared_ptr<Variable> var){
    auto reg = ::get_pseudo_reg(pseudo_registers, var);
    move(var, reg);
    LOG<Trace>("Registers") << "Get pseudo reg for " << var->name() << " => " << reg << log::endl;

    if(is_local(var, *this)){
        local.insert(var);
    }

    return reg;
}

ltac::PseudoRegister ltac::RegisterManager::get_pseudo_reg_no_move(std::shared_ptr<Variable> var){
    auto reg = ::get_pseudo_reg(pseudo_registers, var);
    pseudo_registers.setLocation(var, reg);
    
    LOG<Trace>("Registers") << "Get pseudo reg for " << var->name() << " => " << reg << log::endl;

    if(is_local(var, *this)){
        local.insert(var);
    }

    return reg;
}

ltac::PseudoFloatRegister ltac::RegisterManager::get_pseudo_float_reg(std::shared_ptr<Variable> var){
    auto reg = ::get_pseudo_reg(pseudo_float_registers, var);
    move(var, reg);
    LOG<Trace>("Registers") << "Get pseudo reg for " << var->name() << " => " << reg << log::endl;

    if(is_local(var, *this)){
        local.insert(var);
    }

    return reg;
}

ltac::PseudoFloatRegister ltac::RegisterManager::get_pseudo_float_reg_no_move(std::shared_ptr<Variable> var){
    auto reg = ::get_pseudo_reg(pseudo_float_registers, var);
    pseudo_float_registers.setLocation(var, reg);
    
    LOG<Trace>("Registers") << "Get pseudo reg for " << var->name() << " => " << reg << log::endl;

    if(is_local(var, *this)){
        local.insert(var);
    }

    return reg;
}

ltac::PseudoRegister ltac::RegisterManager::get_bound_pseudo_reg(unsigned short hard){
    return pseudo_registers.get_bound_reg(hard);
}

ltac::PseudoFloatRegister ltac::RegisterManager::get_bound_pseudo_float_reg(unsigned short hard){
    return pseudo_float_registers.get_bound_reg(hard);
}

ltac::PseudoRegister ltac::RegisterManager::get_free_pseudo_reg(){
    return pseudo_registers.get_new_reg();
}

ltac::PseudoFloatRegister ltac::RegisterManager::get_free_pseudo_float_reg(){
    return pseudo_float_registers.get_new_reg();
}

bool ltac::RegisterManager::is_escaped(const std::shared_ptr<Variable> & variable){
    if(pointer_escaped->count(variable)){
        LOG<Trace>("Registers") << variable->name() << " is escaped " << log::endl;

        return true;
    }

    LOG<Trace>("Registers") << variable->name() << " is not escaped " << log::endl;

    return false;
}
    
void ltac::RegisterManager::collect_parameters(eddic::Function& definition, const PlatformDescriptor* descriptor){
    for(auto& parameter : definition.parameters()){
        auto param = definition.context()->getVariable(parameter.name());

        if(param->position().isParamRegister()){
            if(mtac::is_single_int_register(param->type())){
                auto reg = get_bound_pseudo_reg(descriptor->int_param_register(param->position().offset()));
                pseudo_registers.setLocation(param, reg);
            } else if(mtac::is_single_float_register(param->type())){
                auto reg = get_bound_pseudo_float_reg(descriptor->float_param_register(param->position().offset()));
                pseudo_float_registers.setLocation(param, reg);
            }
        }
    }
}

bool ltac::RegisterManager::is_written(const std::shared_ptr<Variable> & variable){
    return written.contains(variable);
}

void ltac::RegisterManager::set_written(const std::shared_ptr<Variable> & variable){
    written.insert(variable);
}

int ltac::RegisterManager::last_pseudo_reg(){
    return pseudo_registers.last_reg();
}
 
int ltac::RegisterManager::last_float_pseudo_reg(){
    return pseudo_float_registers.last_reg();
}

void ltac::RegisterManager::remove_from_pseudo_reg(const std::shared_ptr<Variable> & variable){
    return pseudo_registers.remove_from_reg(variable);
}

void ltac::RegisterManager::remove_from_pseudo_float_reg(const std::shared_ptr<Variable> & variable){
    return pseudo_float_registers.remove_from_reg(variable);
}
