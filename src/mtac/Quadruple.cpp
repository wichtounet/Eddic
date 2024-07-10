//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <atomic>
#include <iomanip>

#include "cpp_utils/assert.hpp"

#include "Function.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include "mtac/Quadruple.hpp"
#include "mtac/basic_block.hpp"

using namespace eddic;

static std::atomic<std::size_t> uid_counter(0);

mtac::Quadruple::Quadruple(mtac::Operator o, tac::Size size) : 
        _uid(++uid_counter), op(o), size(size) {
    //Nothing to init    
}

mtac::Quadruple::Quadruple(std::shared_ptr<Variable> result, mtac::Argument a1, mtac::Operator o, tac::Size size) : 
        _uid(++uid_counter), result(result), arg1(std::move(a1)), op(o), size(size) {
    //Nothing to init    
}

mtac::Quadruple::Quadruple(std::shared_ptr<Variable> result, mtac::Argument a1, mtac::Operator o, mtac::Argument a2, tac::Size size) : 
        _uid(++uid_counter), result(result), arg1(std::move(a1)), arg2(std::move(a2)), op(o), size(size) {
    //Nothing to init    
}

mtac::Quadruple::Quadruple(mtac::Operator o, mtac::Argument a1, tac::Size size) : 
        _uid(++uid_counter), arg1(std::move(a1)), op(o), size(size) {
    //Nothing to init    
}

mtac::Quadruple::Quadruple(mtac::Operator o, mtac::Argument a1, mtac::Argument a2, tac::Size size) : 
        _uid(++uid_counter), arg1(std::move(a1)), arg2(std::move(a2)), op(o), size(size) {
    //Nothing to init    
}
    
mtac::Quadruple::Quadruple(std::string param, mtac::Operator op, tac::Size size) : 
        _uid(++uid_counter), op(op), size(size), m_param(std::move(param)) {
    //Nothing to init
}

mtac::Quadruple::Quadruple(mtac::Operator op, mtac::Argument arg, std::shared_ptr<Variable> param, eddic::Function& function, tac::Size size) : 
        _uid(++uid_counter), result(param), arg1(std::move(arg)), op(op), size(size), m_function(&function) {
    //Nothing to init
}

mtac::Quadruple::Quadruple(mtac::Operator op, mtac::Argument arg, std::string param, eddic::Function& function, tac::Size size) : 
        _uid(++uid_counter), arg1(std::move(arg)), op(op), size(size), m_function(&function), m_param(std::move(param)) {
    //Nothing to init
}

mtac::Quadruple::Quadruple(mtac::Operator op, eddic::Function& function, std::shared_ptr<Variable> return1, std::shared_ptr<Variable> return2, tac::Size size) : 
        _uid(++uid_counter), result(return1), op(op), size(size), secondary(return2), m_function(&function) {
    cpp_assert(m_function, "Function is mandatory for calls");
}

mtac::Quadruple::Quadruple(mtac::Operator op, mtac::Argument arg, std::string label, tac::Size size) : 
        _uid(++uid_counter), arg1(std::move(arg)), op(op), size(size), m_param(std::move(label)) {
    //Nothing to init
}

mtac::Quadruple::Quadruple(const mtac::Quadruple& rhs) : 
        _uid(++uid_counter), 
        result(rhs.result), 
        arg1(rhs.arg1),
        arg2(rhs.arg2),
        op(rhs.op),
        size(rhs.size),
        secondary(rhs.secondary),
        m_function(rhs.m_function),
        m_param(rhs.m_param),
        block(rhs.block)
{
    //Nothing
}

mtac::Quadruple& mtac::Quadruple::operator=(const mtac::Quadruple& rhs){
    //No need to assign this into this
    if(this == &rhs){
        return *this;
    }
    
    _uid = ++uid_counter;
    result = rhs.result;
    arg1 = rhs.arg1;
    arg2 = rhs.arg2;
    op = rhs.op;
    size = rhs.size;
    block = rhs.block;
    m_function = rhs.m_function;
    m_param = rhs.m_param;
    secondary = rhs.secondary;

    return *this;
}

mtac::Quadruple::Quadruple(mtac::Quadruple&& rhs) noexcept : 
        _uid(std::move(rhs._uid)), 
        result(std::move(rhs.result)), 
        arg1(std::move(rhs.arg1)),
        arg2(std::move(rhs.arg2)),
        op(std::move(rhs.op)),
        size(std::move(rhs.size)),
        secondary(std::move(rhs.secondary)),
        m_function(std::move(rhs.m_function)),
        m_param(std::move(rhs.m_param)),
        block(std::move(rhs.block))
{
    rhs._uid = 0;
}

mtac::Quadruple& mtac::Quadruple::operator=(mtac::Quadruple&& rhs) noexcept {
    //No need to assign this into this
    if(this == &rhs){
        return *this;
    }
    
    _uid = std::move(rhs._uid);
    result = std::move(rhs.result);
    arg1 = std::move(rhs.arg1);
    arg2 = std::move(rhs.arg2);
    op = std::move(rhs.op);
    size = std::move(rhs.size);
    block = std::move(rhs.block);
    m_function = std::move(rhs.m_function);
    m_param = std::move(rhs.m_param);
    secondary = std::move(rhs.secondary);

    rhs._uid = 0;

    return *this;
}

const std::string& mtac::Quadruple::label() const {
    return m_param;
}

const std::string& mtac::Quadruple::std_param() const {
    return m_param;
}

eddic::Function& mtac::Quadruple::function(){
    cpp_assert(m_function, "function() can only be called on operations that support it");

    return *m_function;
}

const eddic::Function& mtac::Quadruple::function() const {
    cpp_assert(m_function, "function() can only be called on operations that support it");

    return *m_function;
}

const std::shared_ptr<Variable>& mtac::Quadruple::param() const {
    return result;
}

const std::shared_ptr<Variable>& mtac::Quadruple::return1() const {
    return result;
}

const std::shared_ptr<Variable>& mtac::Quadruple::return2() const {
    return secondary;
}

bool mtac::Quadruple::is_if(){
    return op >= mtac::Operator::IF_UNARY && op <= mtac::Operator::IF_FL;
}

bool mtac::Quadruple::is_if_false(){
    return op >= mtac::Operator::IF_FALSE_UNARY && op <= mtac::Operator::IF_FALSE_FL;
}

bool mtac::Quadruple::operator==(const mtac::Quadruple& rhs) const {
    return _uid == rhs._uid;
}

bool mtac::Quadruple::operator!=(const mtac::Quadruple& rhs) const {
    return !(*this == rhs);
}

namespace {

std::string printTarget(const mtac::Quadruple& quadruple){
    if(quadruple.block){
        return "B" + toString(quadruple.block->index);   
    } else {
        return quadruple.label();
    }
}

std::string size(tac::Size size){
    switch(size){
        case tac::Size::DEFAULT:
            return "";
        case tac::Size::BYTE:
            return " (byte)";
        case tac::Size::WORD:
            return " (word)";
        case tac::Size::DOUBLE_WORD:
            return " (dword)";
        case tac::Size::QUAD_WORD:
            return " (qword)";
        default:
            cpp_unreachable("Invalid TAC Size");
    }
}

}

void mtac::transform_to_nop(mtac::Quadruple& quadruple){
    //Transform it to a NOP
    quadruple.op = mtac::Operator::NOP;

    //Clear all informations
    quadruple.arg1.reset();
    quadruple.arg2.reset();
    quadruple.result = nullptr;
    quadruple.secondary = nullptr;
    quadruple.block = nullptr;
    quadruple.m_function = nullptr;
    quadruple.m_param.clear();
}

std::ostream& eddic::mtac::operator<<(std::ostream& stream, const mtac::Quadruple& quadruple){
    auto op = quadruple.op;

    stream << "\t" << std::setw(3) << std::setfill('0') << quadruple.uid() << ":";

    switch(op){
        case mtac::Operator::ASSIGN:
            stream << "\t" << quadruple.result << " = (normal) " << size(quadruple.size) << *quadruple.arg1 ;
            break;
        case mtac::Operator::FASSIGN:
            stream << "\t" << quadruple.result << " = (float) " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::PASSIGN:
            stream << "\t" << quadruple.result << " = (pointer) " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::ADD:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " + " << *quadruple.arg2;
            break;
        case mtac::Operator::FADD:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " + (float) " << *quadruple.arg2;
            break;
        case mtac::Operator::SUB:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " - " << *quadruple.arg2;
            break;
        case mtac::Operator::FSUB:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " - (float) " << *quadruple.arg2;
            break;
        case mtac::Operator::MUL:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " * " << *quadruple.arg2;
            break;
        case mtac::Operator::FMUL:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " * (float) " << *quadruple.arg2;
            break;
        case mtac::Operator::DIV:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " / " << *quadruple.arg2;
            break;
        case mtac::Operator::FDIV:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " / (float) " << *quadruple.arg2;
            break;
        case mtac::Operator::MOD:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " % " << *quadruple.arg2;
            break;
        case mtac::Operator::AND:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " & " << *quadruple.arg2;
            break;
        case mtac::Operator::EQUALS:
        case mtac::Operator::FE:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " == " << *quadruple.arg2;
            break;
        case mtac::Operator::NOT_EQUALS:
        case mtac::Operator::FNE:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " != " << *quadruple.arg2;
            break;
        case mtac::Operator::GREATER:
        case mtac::Operator::FG:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " > " << *quadruple.arg2;
            break;
        case mtac::Operator::GREATER_EQUALS:
        case mtac::Operator::FGE:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " >= " << *quadruple.arg2;
            break;
        case mtac::Operator::LESS:
        case mtac::Operator::FL:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " < " << *quadruple.arg2;
            break;
        case mtac::Operator::LESS_EQUALS:
        case mtac::Operator::FLE:
            stream << "\t" << quadruple.result << " = " << size(quadruple.size) << *quadruple.arg1 << " <= " << *quadruple.arg2;
            break;
        case mtac::Operator::MINUS:
            stream << "\t" << quadruple.result << " = - " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::FMINUS:
            stream << "\t" << quadruple.result << " = - (float) " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::NOT:
            stream << "\t" << quadruple.result << " = ! " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::I2F:
            stream << "\t" << quadruple.result << " = (cast float) " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::F2I:
            stream << "\t" << quadruple.result << " = (cast int) " << size(quadruple.size) << *quadruple.arg1;
            break;
        case mtac::Operator::DOT:
            stream << "\t" << quadruple.result << " = (normal)" << size(quadruple.size) << " (" << *quadruple.arg1 << ")" << *quadruple.arg2;
            break;
        case mtac::Operator::FDOT:
            stream << "\t" << quadruple.result << " = (float)" << size(quadruple.size) << " (" << *quadruple.arg1 << ")" << *quadruple.arg2;
            break;
        case mtac::Operator::PDOT:
            stream << "\t" << quadruple.result << " = (pointer)" << size(quadruple.size) << " (" << *quadruple.arg1 << ")" << *quadruple.arg2;
            break;
        case mtac::Operator::DOT_ASSIGN:
            stream << "\t" << "(" << quadruple.result << ")" << *quadruple.arg1 << " = (normal)" << size(quadruple.size) << " " << *quadruple.arg2;
            break;
        case mtac::Operator::DOT_FASSIGN:
            stream << "\t" << "(" << quadruple.result << ")" << *quadruple.arg1 << " = (float) " << size(quadruple.size) << *quadruple.arg2;
            break;
        case mtac::Operator::DOT_PASSIGN:
            stream << "\t" << "(" << quadruple.result << ")" << *quadruple.arg1 << " = (pointer) " << size(quadruple.size) << *quadruple.arg2;
            break;
        case mtac::Operator::RETURN:
            stream << "\t" << "return";

            if(quadruple.arg1){
                stream << " " << *quadruple.arg1;
            }

            if(quadruple.arg2){
                stream << ", " << *quadruple.arg2;
            }

            break;
        case  mtac::Operator::NOP:
            stream << "\t" << "nop";
            break;
        case  mtac::Operator::LABEL:
            stream << "\t" << quadruple.label() << ":";
            break;
        case  mtac::Operator::GOTO:
            if(quadruple.block){
                stream << "\t " << "goto " << "B" + toString(quadruple.block->index);
            } else {
                stream << "\t" << "goto " << quadruple.label();
            }

            break;
        case mtac::Operator::PARAM:
            if(quadruple.param()){
                stream << "\t" << "param " << size(quadruple.size) << "(" << quadruple.param() << ") " << *quadruple.arg1;
            } else {
                if(quadruple.std_param().length() > 0){
                    stream << "\t" << "param " << size(quadruple.size) << "(std::" << quadruple.std_param() << ") " << *quadruple.arg1;
                } else {
                    stream << "\t" << "param " << size(quadruple.size) << *quadruple.arg1;
                }
            }

            break;
        case  mtac::Operator::PPARAM:
            if(quadruple.param()){
                stream << "\t" << "param address " << size(quadruple.size) << "(" << quadruple.param() << ") " << *quadruple.arg1;
            } else {
                if(quadruple.std_param().length() > 0){
                    stream << "\t" << "param address " << size(quadruple.size) << "(std::" << quadruple.std_param() << ") " << *quadruple.arg1;
                } else {
                    stream << "\t" << "param address " << size(quadruple.size) << *quadruple.arg1;
                }
            }

            break;
        case  mtac::Operator::CALL:
            stream << "\t";

            if(quadruple.return1()){
                stream << quadruple.return1();
            }

            if(quadruple.return2()){
                stream << ", " << quadruple.return2();
            }

            if(quadruple.return1() || quadruple.return2()){
                stream << " = ";
            }

            stream << "call " << quadruple.function().mangled_name();
            break;
        case mtac::Operator::IF_FALSE_UNARY:
            stream << "\t" << "if_false " << *quadruple.arg1 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_EQUALS:
        case mtac::Operator::IF_FALSE_FE:
            stream << "\t" << "if_false " << *quadruple.arg1 << " == " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_NOT_EQUALS:
        case mtac::Operator::IF_FALSE_FNE:
            stream << "\t" << "if_false " << *quadruple.arg1 << " != " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_LESS:
        case mtac::Operator::IF_FALSE_FL:
            stream << "\t" << "if_false " << *quadruple.arg1 << " < " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_LESS_EQUALS:
        case mtac::Operator::IF_FALSE_FLE:
            stream << "\t" << "if_false " << *quadruple.arg1 << " <= " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_GREATER:
        case mtac::Operator::IF_FALSE_FG:
            stream << "\t" << "if_false " << *quadruple.arg1 << " > " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_FALSE_GREATER_EQUALS:
        case mtac::Operator::IF_FALSE_FGE:
            stream << "\t" << "if_false " << *quadruple.arg1 << " >= " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_UNARY:
            stream << "\t" << "if " << *quadruple.arg1 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_EQUALS:
        case mtac::Operator::IF_FE:
            stream << "\t" << "if " << *quadruple.arg1 << " == " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_NOT_EQUALS:
        case mtac::Operator::IF_FNE:
            stream << "\t" << "if " << *quadruple.arg1 << " != " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_LESS:
        case mtac::Operator::IF_FL:
            stream << "\t" << "if " << *quadruple.arg1 << " < " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_LESS_EQUALS:
        case mtac::Operator::IF_FLE:
            stream << "\t" << "if " << *quadruple.arg1 << " <= " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_GREATER:
        case mtac::Operator::IF_FG:
            stream << "\t" << "if " << *quadruple.arg1 << " > " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
        case mtac::Operator::IF_GREATER_EQUALS:
        case mtac::Operator::IF_FGE:
            stream << "\t" << "if " << *quadruple.arg1 << " >= " << *quadruple.arg2 << " goto " << printTarget(quadruple);
            break;
    }

    return stream;
}
