//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "commons/ByteCode.hpp"
#include "commons/IO.hpp"

#include "ByteCodeFileWriter.hpp"

using std::string;
using std::ios;

void ByteCodeFileWriter::open(const std::string& path) throw (CompilerException) {
	stream.open(path.c_str());

	if(!stream){
		throw CompilerException("Unable to open the output file");
	}
}

void ByteCodeFileWriter::close(){
	stream.close();
}

void ByteCodeFileWriter::writeOneOperandCall(ByteCode bytecode, const std::string& litteral){
	//binary_write(&stream, (int) bytecode);
	
	//writeLitteral(litteral);
}

void ByteCodeFileWriter::writeOneOperandCall(ByteCode bytecode, int value){
	//binary_write(&stream, (int) bytecode);
	//binary_write(&stream, value);
}

void ByteCodeFileWriter::writeSimpleCall(ByteCode bytecode){
	//binary_write(&stream, (int) bytecode);
}

void ByteCodeFileWriter::writeHeader(){
        nativeWrite(".text");
        nativeWrite(".globl main");

        nativeWrite(".main");
}

void writePrintString(std::ofstream& stream){
	stream << "print_string:";
		stream << "pushl %ebp";
		stream << "movl %esp, %ebp";
		stream << "movl $0, %esi";

		stream << "movl $4, %eax";
		stream << "movl $1, %ebx";
		stream << "movl 12(%esp), %ecx";
		stream << "movl 8(%esp), %edx";
		stream << "int $0x80";

		stream << "leave";
		stream << "ret";
}

void ByteCodeFileWriter::writeEnd(){
	nativeWrite("mov $1, %eax");
        nativeWrite("mov $0, %ebx");
        nativeWrite("int $0x80");

	//TODO Write only if necessary
	writePrintString(stream);
}

void ByteCodeFileWriter::writeLitteral(const std::string& litteral){
	//binary_write(&stream, litteral.length() - 2);
	//binary_write(&stream, litteral.substr(1, litteral.length() - 2));
}

void ByteCodeFileWriter::writeInt(int value){
	//binary_write(&stream, value);
}

void ByteCodeFileWriter::nativeWrite(std::string instruction){
	stream << instruction << std::endl;
}
