//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>

#include "Assembler.hpp"
#include "DebugStopWatch.hpp"
#include "Utils.hpp"

#ifdef DEBUG
static const bool debug = true;
#else
static const bool debug = false;
#endif

using namespace eddic;

namespace {

void exec(const std::string& command) {
    DebugStopWatch<debug> timer("Exec " + command);
    
    if(debug){
        std::cout << "eddic : exec command : " << command << std::endl;
    }

    std::string result = execCommand(command);

    if(result.size() > 0){
        std::cout << result << std::endl;
    }
}

void assembleWithoutDebug(Platform platform, const std::string& output){
    switch(platform){
        case Platform::INTEL_X86:
            exec("nasm -f elf32 -o output.o output.asm");
            exec("ld -S -m elf_i386 output.o -o " + output);

            break;
        case Platform::INTEL_X86_64:
            exec("nasm -f elf64 -o output.o output.asm");
            exec("ld -S -m elf_x86_64 output.o -o " + output);

            break;
   } 
}

void assembleWithDebug(Platform platform, const std::string& output){
    switch(platform){
        case Platform::INTEL_X86:
            exec("nasm -g -f elf32 -o output.o output.asm");
            exec("ld -m elf_i386 output.o -o " + output);

            break;
        case Platform::INTEL_X86_64:
            exec("nasm -g -f elf64 -o output.o output.asm");
            exec("ld -m elf_x86_64 output.o -o " + output);

        break;
   } 
}

} //end of anonymous namespace

void eddic::assemble(Platform platform, const std::string& output, bool debug){
   if(debug){
       assembleWithDebug(platform, output);
   } else {
       assembleWithoutDebug(platform, output);
   }
}
