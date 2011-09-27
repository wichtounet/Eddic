//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "Nodes.hpp"
#include "StringPool.hpp"
#include "Options.hpp"
#include "AssemblyFileWriter.hpp"
#include "Context.hpp"
#include "Functions.hpp"

#include <cassert>

using std::string;
using std::endl;
using std::map;

using namespace eddic;

void Program::addFunction(Function* function){
    functions[function->name()] = function;

    addLast(function);
}

bool Program::exists(string function){
    return functions.find(function) != functions.end();
}

static void writePrintString(std::ofstream& m_stream) {
    m_stream << std::endl;
    m_stream << "print_string:" << std::endl;
    m_stream << "pushl %ebp" << std::endl;
    m_stream << "movl %esp, %ebp" << std::endl;
    m_stream << "movl $0, %esi" << std::endl;

    m_stream << "movl $4, %eax" << std::endl;
    m_stream << "movl $1, %ebx" << std::endl;
    m_stream << "movl 12(%ebp), %ecx" << std::endl;
    m_stream << "movl 8(%ebp), %edx" << std::endl;
    m_stream << "int $0x80" << std::endl;

    m_stream << "leave" << std::endl;
    m_stream << "ret" << std::endl;
}

static void writePrintLine(std::ofstream& m_stream) {
    m_stream << std::endl;
    m_stream << "print_line:" << std::endl;
    m_stream << "pushl %ebp" << std::endl;
    m_stream << "movl %esp, %ebp" << std::endl;

    m_stream << "pushl $S1" << endl;
    m_stream << "pushl $1" << endl;
    m_stream << "call print_string" << endl;
    m_stream << "addl $8, %esp" << endl;

    m_stream << "leave" << std::endl;
    m_stream << "ret" << std::endl;
}

static void writePrintInteger(std::ofstream& m_stream) {
    m_stream << std::endl;
    m_stream << "print_integer:" << std::endl
             << "pushl %ebp" << std::endl
             << "movl %esp, %ebp" << std::endl
             << "movl 8(%ebp), %eax" << std::endl
             << "xorl %esi, %esi" << std::endl

             << "loop:" << std::endl
             << "movl $0, %edx" << std::endl
             << "movl $10, %ebx" << std::endl
             << "divl %ebx" << std::endl
             << "addl $48, %edx" << std::endl
             << "pushl %edx" << std::endl
             << "incl %esi" << std::endl
             << "cmpl $0, %eax" << std::endl
             << "jz   next" << std::endl
             << "jmp loop" << std::endl

             << "next:" << std::endl
             << "cmpl $0, %esi" << std::endl
             << "jz   exit" << std::endl
             << "decl %esi" << std::endl

             << "movl $4, %eax" << std::endl
             << "movl %esp, %ecx" << std::endl
             << "movl $1, %ebx" << std::endl
             << "movl $1, %edx" << std::endl
             << "int  $0x80" << std::endl

             << "addl $4, %esp" << std::endl

             << "jmp  next" << std::endl

             << "exit:" << std::endl
             << "leave" << std::endl
             << "ret" << std::endl;
}

static void writeConcat(std::ofstream& m_stream){
    m_stream << std::endl;
    m_stream << "concat:" << std::endl
             << "pushl %ebp" << std::endl
             << "movl %esp, %ebp" << std::endl
			
			 << "movl 16(%ebp), %edx" << std::endl
			 << "movl 8(%ebp), %ecx" << std::endl
			 << "addl %ecx, %edx" << std::endl

			 << "pushl %edx" << std::endl
			 << "call malloc" << std::endl
			 << "addl $4, %esp" << std::endl

			 << "movl %eax, -4(%ebp)" << std::endl
			 << "movl %eax, %ecx" << std::endl
			 << "movl $0, %eax" << std::endl

			 << "movl 16(%ebp), %ebx" << std::endl
			 << "movl 20(%ebp), %edx" << std::endl

			 << "copy_concat_1:" << std::endl
			 << "cmpl $0, %ebx" << std::endl
			 << "je end_concat_1"  << std::endl
			 << "movb (%edx), %al" << std::endl
			 << "movb %al, (%ecx)" << std::endl
			 << "addl $1, %ecx" << std::endl
			 << "addl $1, %edx" << std::endl
			 << "subl $1, %ebx" << std::endl
			 << "jmp copy_concat_1" << std::endl
			 << "end_concat_1" << ":" << std::endl

			 << "movl 8(%ebp), %ebx" << std::endl
			 << "movl 12(%ebp), %edx" << std::endl

			 << "copy_concat_2:" << std::endl
			 << "cmpl $0, %ebx" << std::endl
			 << "je end_concat_2"  << std::endl
			 << "movb (%edx), %al" << std::endl
			 << "movb %al, (%ecx)" << std::endl
			 << "addl $1, %ecx" << std::endl
			 << "addl $1, %edx" << std::endl
			 << "subl $1, %ebx" << std::endl
			 << "jmp copy_concat_2" << std::endl
			 << "end_concat_2:" << std::endl

			 << "movl 16(%ebp), %edx" << std::endl
			 << "movl 8(%ebp), %ecx" << std::endl
			 << "addl %ecx, %edx" << std::endl

			 << "movl -4(%ebp), %eax" << std::endl

             << "leave" << std::endl
             << "ret" << std::endl;
}

void Methods::write(AssemblyFileWriter& writer) {
    writePrintString(writer.stream());
    writePrintInteger(writer.stream());
    writePrintLine(writer.stream());
	writeConcat(writer.stream());
}

void Declaration::checkVariables() {
    if (context()->exists(m_variable)) {
        throw CompilerException("Variable has already been declared", token());
    }

    Variable* var = context()->create(m_variable, m_type);

    m_index = var->index();

    value->checkVariables();

    if (value->type() != m_type) {
        throw CompilerException("Incompatible type", token());
    }
}

void VariableOperation::checkStrings(StringPool& pool) {
    value->checkStrings(pool);
}

void Assignment::checkVariables() {
    if (!context()->exists(m_variable)) {
        throw CompilerException("Variable has not  been declared", token());
    }

    Variable* var = context()->find(m_variable);

    m_index = var->index();
    m_type = var->type();

    value->checkVariables();

    if (value->type() != var->type()) {
        throw CompilerException("Incompatible type", token());
    }
}

void Swap::checkVariables() {
    if (m_lhs == m_rhs) {
        throw CompilerException("Cannot swap a variable with itself", token());
    }

    if (!context()->exists(m_lhs) || !context()->exists(m_rhs)) {
        throw CompilerException("Variable has not been declared", token());
    }

    Variable* lhs_var = context()->find(m_lhs);
    Variable* rhs_var = context()->find(m_rhs);

    m_lhs_index = lhs_var->index();
    m_rhs_index = rhs_var->index();

    if (lhs_var->type() != rhs_var->type()) {
        throw CompilerException("Incompatible type", token());
    }

    m_type = lhs_var->type();
}

void VariableValue::checkVariables() {
    if (!context()->exists(m_variable)) {
        throw CompilerException("Variable has not been declared", token());
    }

    Variable* variable = context()->find(m_variable);

    m_type = variable->type();
    m_index = variable->index();
}

void Litteral::checkStrings(StringPool& pool) {
    m_label = pool.label(m_litteral);
}

void VariableOperation::write(AssemblyFileWriter& writer) {
    value->write(writer);

    switch (m_type) {
        case INT:
            writer.stream() << "movl (%esp), %eax" << endl;
            writer.stream() << "movl %eax, VI" << m_index << endl;
            writer.stream() << "addl $4, %esp" << endl;

            break;
        case STRING:
            writer.stream() << "movl (%esp), %eax" << endl;
            writer.stream() << "movl 4(%esp), %ebx" << endl;
            writer.stream() << "addl $8, %esp" << endl;

            writer.stream() << "movl %eax, VS" << m_index << "+4" << endl;
            writer.stream() << "movl %ebx, VS" << m_index << endl;

            break;
    }
}

void Swap::write(AssemblyFileWriter& writer) {
    switch (m_type) {
        case INT:
            writer.stream() << "movl VI" << m_lhs_index << ", %eax" << endl;
            writer.stream() << "movl VI" << m_rhs_index << ", %ebx" << endl;
            writer.stream() << "movl %eax, VI" << m_rhs_index << endl;
            writer.stream() << "movl %ebx, VI" << m_lhs_index << endl;

            break;
        case STRING:
            writer.stream() << "movl VS" << m_lhs_index << ", %eax" << endl;
            writer.stream() << "movl VS" << m_rhs_index << ", %ebx" << endl;
            writer.stream() << "movl %eax, VS" << m_rhs_index << endl;
            writer.stream() << "movl %ebx, VS" << m_lhs_index << endl;

            writer.stream() << "movl VS" << m_lhs_index << "+4, %eax" << endl;
            writer.stream() << "movl VS" << m_rhs_index << "+4, %ebx" << endl;
            writer.stream() << "movl %eax, VS" << m_rhs_index << "+4" << endl;
            writer.stream() << "movl %ebx, VS" << m_lhs_index << "+4" << endl;

            break;
    }
}

void Print::write(AssemblyFileWriter& writer) {
    value->write(writer);

    switch (value->type()) {
        case INT:
            writer.stream() << "call print_integer" << endl;
            writer.stream() << "addl $4, %esp" << endl;

            break;
        case STRING:
            writer.stream() << "call print_string" << endl;
            writer.stream() << "addl $8, %esp" << endl;

            break;
    }
}

void Println::write(AssemblyFileWriter& writer) {
    Print::write(writer);

    writer.stream() << "call print_line" << endl;
}

void Print::checkStrings(StringPool& pool) {
    value->checkStrings(pool);
}

void Print::checkVariables() {
    value->checkVariables();
}

void Integer::write(AssemblyFileWriter& writer) {
    writer.stream() << "pushl $" << m_value << std::endl;
}

void VariableValue::write(AssemblyFileWriter& writer) {
    switch (m_type) {
        case INT:
            writer.stream() << "pushl VI" << m_index << std::endl;

            break;
        case STRING:
            writer.stream() << "pushl VS" << m_index << endl;
            writer.stream() << "pushl VS" << m_index << "+4" << std::endl;

            break;
    }
}

void Litteral::write(AssemblyFileWriter& writer) {
    writer.stream() << "pushl $" << m_label << std::endl;
    writer.stream() << "pushl $" << (m_litteral.size() - 2) << std::endl;
}

//Constantness

bool Value::isConstant() {
    return false;
}

bool Litteral::isConstant() {
    return true;
}

bool Integer::isConstant() {
    return true;
}

bool VariableValue::isConstant() {
    return false;
}

//Values

string Value::getStringValue() {
    throw "Not constant";
}

int Value::getIntValue() {
    throw "Not constant";
}

int Integer::getIntValue() {
    return m_value;
}

string Litteral::getStringValue() {
    return m_litteral;
}
