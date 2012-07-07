//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>

#include "assert.hpp"
#include "AssemblyFileWriter.hpp"
#include "FunctionContext.hpp"
#include "SymbolTable.hpp"
#include "Labels.hpp"
#include "VisitorUtils.hpp"

#include "asm/StringConverter.hpp"
#include "asm/IntelX86CodeGenerator.hpp"
#include "asm/IntelAssemblyUtils.hpp"

using namespace eddic;

as::IntelX86CodeGenerator::IntelX86CodeGenerator(AssemblyFileWriter& w) : IntelCodeGenerator(w) {}

struct X86_32StringConverter : public as::StringConverter {
    std::string to_string(eddic::ltac::Register reg) const {
        static std::string registers[6] = {"eax", "ebx", "ecx", "edx", "esi", "edi"};

        if(static_cast<int>(reg) == 1000){
            return "esp"; 
        } else if(static_cast<int>(reg) == 1001){
            return "ebp"; 
        }

        return registers[static_cast<int>(reg)];
    }

    std::string to_string(eddic::ltac::FloatRegister reg) const {
        static std::string registers[8] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};

        return registers[static_cast<int>(reg)];
    }
    
    std::string to_string(eddic::ltac::Argument& arg) const {
        if(auto* ptr = boost::get<int>(&arg)){
            return ::toString(*ptr);
        } else if(auto* ptr = boost::get<double>(&arg)){
            std::stringstream ss;
            ss << "__float32__(" << std::fixed << *ptr << ")";
            return ss.str();
        } else if(auto* ptr = boost::get<ltac::Register>(&arg)){
            return to_string(*ptr); 
        } else if(auto* ptr = boost::get<ltac::FloatRegister>(&arg)){
            return to_string(*ptr); 
        } else if(auto* ptr = boost::get<ltac::Address>(&arg)){
            return address_to_string(*ptr);
        } else if(auto* ptr = boost::get<std::string>(&arg)){
            return *ptr;
        }

        ASSERT_PATH_NOT_TAKEN("Unhandled variant type");
    }
};

namespace x86 {

std::ostream& operator<<(std::ostream& os, eddic::ltac::Argument& arg){
    X86_32StringConverter converter;
    return os << converter.to_string(arg);
}

void enterFunction(AssemblyFileWriter& writer){
    writer.stream() << "push ebp" << std::endl;
    writer.stream() << "mov ebp, esp" << std::endl;
}

void defineFunction(AssemblyFileWriter& writer, const std::string& function){
    writer.stream() << std::endl << function << ":" << std::endl;
    
    enterFunction(writer);
}

void leaveFunction(AssemblyFileWriter& writer){
}

} //end of x86 namespace

using namespace x86;

namespace eddic { namespace as {

struct X86StatementCompiler : public boost::static_visitor<> {
    AssemblyFileWriter& writer;

    X86StatementCompiler(AssemblyFileWriter& writer) : writer(writer) {
        //Nothing else to init
    }

    void operator()(std::shared_ptr<ltac::Instruction> instruction){
        switch(instruction->op){
            case ltac::Operator::MOV:
                if(boost::get<ltac::FloatRegister>(&*instruction->arg1) && boost::get<ltac::Register>(&*instruction->arg2)){
                    writer.stream() << "movd " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                } else if(boost::get<ltac::Register>(&*instruction->arg1) && boost::get<ltac::FloatRegister>(&*instruction->arg2)){
                    writer.stream() << "movd " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                } else if(boost::get<ltac::Address>(&*instruction->arg1)){
                    writer.stream() << "mov dword " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                } else {
                    writer.stream() << "mov " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                }

                break;
            case ltac::Operator::FMOV:
                if(boost::get<ltac::FloatRegister>(&*instruction->arg1) && boost::get<ltac::Register>(&*instruction->arg2)){
                    writer.stream() << "movd " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                } else {
                    writer.stream() << "movss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                }

                break;
            case ltac::Operator::MEMSET:
                writer.stream() << "mov ecx, " << *instruction->arg2 << std::endl;
                writer.stream() << "xor eax, eax" << std::endl;
                writer.stream() << "lea edi, " << *instruction->arg1 << std::endl;
                writer.stream() << "std" << std::endl;
                writer.stream() << "rep stosw" << std::endl;
                writer.stream() << "cld" << std::endl;

                break;
            case ltac::Operator::ALLOC_STACK:
                writer.stream() << "sub esp, " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::FREE_STACK:
                writer.stream() << "add esp, " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::LEAVE:
                writer.stream() << "leave" << std::endl;
                break;
            case ltac::Operator::RET:
                writer.stream() << "ret" << std::endl;
                break;
            case ltac::Operator::CMP_INT:
                writer.stream() << "cmp " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMP_FLOAT:
                writer.stream() << "ucomiss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::OR:
                writer.stream() << "or " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::XOR:
                writer.stream() << "xor " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::PUSH:
                if(boost::get<ltac::Address>(&*instruction->arg1)){
                    writer.stream() << "push dword " << *instruction->arg1 << std::endl;
                } else {
                    writer.stream() << "push " << *instruction->arg1 << std::endl;
                }

                break;
            case ltac::Operator::POP:
                writer.stream() << "pop " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::LEA:
                writer.stream() << "lea " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::SHIFT_LEFT:
                writer.stream() << "sal " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::SHIFT_RIGHT:
                writer.stream() << "sar " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::ADD:
                writer.stream() << "add " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::SUB:
                writer.stream() << "sub " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::MUL:
                if(instruction->arg3){
                    writer.stream() << "imul " << *instruction->arg1 << ", " << *instruction->arg2 << ", " << *instruction->arg3 << std::endl;
                } else {
                    writer.stream() << "imul " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                }

                break;
            case ltac::Operator::DIV:
                writer.stream() << "idiv " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::FADD:
                writer.stream() << "addss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::FSUB:
                writer.stream() << "subss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::FMUL:
                writer.stream() << "mulss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::FDIV:
                writer.stream() << "divss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::INC:
                writer.stream() << "inc " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::DEC:
                writer.stream() << "dec " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::NEG:
                writer.stream() << "neg " << *instruction->arg1 << std::endl;
                break;
            case ltac::Operator::I2F:
                writer.stream() << "cvtsi2ss " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::F2I:
                writer.stream() << "cvttss2si " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVE:
                writer.stream() << "cmove " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVNE:
                writer.stream() << "cmovne " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVA:
                writer.stream() << "cmova " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVAE:
                writer.stream() << "cmovae " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVB:
                writer.stream() << "cmovb " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVBE:
                writer.stream() << "cmovbe " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVG:
                writer.stream() << "cmovg " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVGE:
                writer.stream() << "cmovge " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVL:
                writer.stream() << "cmovl " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::CMOVLE:
                writer.stream() << "cmovle " << *instruction->arg1 << ", " << *instruction->arg2 << std::endl;
                break;
            case ltac::Operator::NOP:
                //Nothing to output for a nop
                break;
            default:
                ASSERT_PATH_NOT_TAKEN("The instruction operator is not supported");
        }
    }
    
    void operator()(std::shared_ptr<ltac::Jump> jump){
        switch(jump->type){
            case ltac::JumpType::CALL:
                writer.stream() << "call " << jump->label << std::endl;
                break;
            case ltac::JumpType::ALWAYS:
                writer.stream() << "jmp " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::NE:
                writer.stream() << "jne " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::E:
                writer.stream() << "je " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::GE:
                writer.stream() << "jge " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::G:
                writer.stream() << "jg " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::LE:
                writer.stream() << "jle " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::L:
                writer.stream() << "jl " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::AE:
                writer.stream() << "jae " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::A:
                writer.stream() << "ja" << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::BE:
                writer.stream() << "jbe " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::B:
                writer.stream() << "jb " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::P:
                writer.stream() << "jp " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::Z:
                writer.stream() << "jz " << "." << jump->label << std::endl;
                break;
            case ltac::JumpType::NZ:
                writer.stream() << "jnz " << "." << jump->label << std::endl;
                break;
            default:
                ASSERT_PATH_NOT_TAKEN("The jump type is not supported");
        }
    }

    void operator()(std::string& label){
        writer.stream() << "." << label << ":" << std::endl;
    }
};

void IntelX86CodeGenerator::compile(std::shared_ptr<ltac::Function> function){
    defineFunction(writer, function->getName());
    //TODO In the future, it is possible that it is up to the ltac compiler to generate the preamble of functions

    X86StatementCompiler compiler(writer);

    visit_each(compiler, function->getStatements());
}

void IntelX86CodeGenerator::writeRuntimeSupport(){
    writer.stream() << "section .text" << std::endl << std::endl;

    writer.stream() << "global _start" << std::endl << std::endl;

    writer.stream() << "_start:" << std::endl;

    //If the user wants the args, we add support for them
    if(symbols.getFunction("main")->parameters.size() == 1){
        writer.stream() << "pop ebx" << std::endl;                          //ebx = number of args
        writer.stream() << "lea ecx, [4 + ebx * 8]" << std::endl;           //ecx = size of the array
        writer.stream() << "push ecx" << std::endl;
        writer.stream() << "call eddi_alloc" << std::endl;                  //eax = start address of the array
        writer.stream() << "add esp, 4" << std::endl;

        writer.stream() << "lea esi, [eax + ecx - 4]" << std::endl;         //esi = last address of the array
        writer.stream() << "mov edx, esi" << std::endl;                     //edx = last address of the array
        
        writer.stream() << "mov [esi], ebx" << std::endl;                   //Set the length of the array
        writer.stream() << "sub esi, 8" << std::endl;                       //Move to the destination address of the first arg

        writer.stream() << ".copy_args:" << std::endl;
        writer.stream() << "pop edi" << std::endl;                          //edi = address of current args
        writer.stream() << "mov [esi+4], edi" << std::endl;                 //set the address of the string

        /* Calculate the length of the string  */
        writer.stream() << "xor eax, eax" << std::endl;
        writer.stream() << "xor ecx, ecx" << std::endl;
        writer.stream() << "not ecx" << std::endl;
        writer.stream() << "repne scasb" << std::endl;
        writer.stream() << "not ecx" << std::endl;
        writer.stream() << "dec ecx" << std::endl;
        /* End of the calculation */

        writer.stream() << "mov dword [esi], ecx" << std::endl;               //set the length of the string
        writer.stream() << "sub esi, 8" << std::endl;
        writer.stream() << "dec ebx" << std::endl;
        writer.stream() << "jnz .copy_args" << std::endl;

        writer.stream() << "push edx" << std::endl;
    }

    writer.stream() << "call main" << std::endl;
    writer.stream() << "mov eax, 1" << std::endl;
    writer.stream() << "xor ebx, ebx" << std::endl;
    writer.stream() << "int 80h" << std::endl;
}

void IntelX86CodeGenerator::defineDataSection(){
    writer.stream() << std::endl << "section .data" << std::endl;
}

void IntelX86CodeGenerator::declareIntArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "times " << size << " dd 0" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareFloatArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "times " << size << " dd __float32__(0.0)" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareStringArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "%rep " << size << std::endl;
    writer.stream() << "dd S3" << std::endl;
    writer.stream() << "dd 0" << std::endl;
    writer.stream() << "%endrep" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareIntVariable(const std::string& name, int value){
    writer.stream() << "V" << name << " dd " << value << std::endl;
}

void IntelX86CodeGenerator::declareStringVariable(const std::string& name, const std::string& label, int size){
    writer.stream() << "V" << name << " dd " << label << ", " << size << std::endl;
}

void IntelX86CodeGenerator::declareString(const std::string& label, const std::string& value){
    writer.stream() << label << " dd " << value << std::endl;
}

void IntelX86CodeGenerator::declareFloat(const std::string& label, double value){
    writer.stream() << std::fixed << label << " dd __float32__(" << value << ")" << std::endl;
}

}} //end of eddic::as

void as::IntelX86CodeGenerator::addStandardFunctions(){
    if(as::is_enabled_printI()){
        output_function("x86_32_printI");
    }
    
    if(symbols.referenceCount("_F7printlnI")){
        output_function("x86_32_printlnI");
    }

    if(symbols.referenceCount("_F5printF")){
        output_function("x86_32_printF");
    }
    
    if(symbols.referenceCount("_F7printlnF")){
        output_function("x86_32_printlnF");
    }
    
    if(symbols.referenceCount("_F5printB")){
        output_function("x86_32_printB");
    }
    
    if(symbols.referenceCount("_F7printlnB")){
        output_function("x86_32_printlnB");
    }
    
    if(as::is_enabled_println()){
        output_function("x86_32_println");
    }
    
    if(symbols.referenceCount("_F5printS") || as::is_enabled_printI() || as::is_enabled_println()){ 
        output_function("x86_32_printS");
    }
    
    if(symbols.referenceCount("_F7printlnS")){ 
        output_function("x86_32_printlnS");
    }
    
    if(symbols.referenceCount("_F6concatSS")){
        output_function("x86_32_concat");
    }
    
    if(symbols.getFunction("main")->parameters.size() == 1 || symbols.referenceCount("_F6concatSS")){
        output_function("x86_32_eddi_alloc");
    }
    
    if(symbols.referenceCount("_F4timeAI")){
        output_function("x86_32_time");
    }
    
    if(symbols.referenceCount("_F8durationAIAI")){
        output_function("x86_32_duration");
    }
}
