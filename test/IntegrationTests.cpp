//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <string>

#include "Options.hpp"
#include "Compiler.hpp"
#include "Utils.hpp"
#include "Platform.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Eddic Tests
#include <boost/test/unit_test.hpp>

#define TEST_SAMPLE(file)\
BOOST_AUTO_TEST_CASE( samples_##file ){\
    assertCompiles("samples/" #file ".eddi");\
}

void assertCompiles(const std::string& file){
    eddic::parseOptions(0, {});

    eddic::Compiler compiler;

    eddic::Platform platform = eddic::Platform::INTEL_X86;
    int code = compiler.compileOnly(file, platform);

    BOOST_CHECK_EQUAL (code, 0);
}

void assertOutputEquals(const std::string& file, const std::string& output){
    assertCompiles("test/cases/" + file);

    std::string out = eddic::execCommand("./a.out"); 
    
    BOOST_CHECK_EQUAL (output, out);
}

/* Compiles all the samples */

TEST_SAMPLE(arrays)
TEST_SAMPLE(asm)
TEST_SAMPLE(assembly)
TEST_SAMPLE(bool)
TEST_SAMPLE(compound)
TEST_SAMPLE(concat)
TEST_SAMPLE(const)
TEST_SAMPLE(functions)
TEST_SAMPLE(inc)
TEST_SAMPLE(includes)
TEST_SAMPLE(optimize)
TEST_SAMPLE(problem)
TEST_SAMPLE(sort)
TEST_SAMPLE(identifiers)

/* Specific tests */ 

BOOST_AUTO_TEST_CASE( if_ ){
    assertOutputEquals("if.eddi", "Cool");
}

BOOST_AUTO_TEST_CASE( while_ ){
    assertOutputEquals("while.eddi", "01234");
}

BOOST_AUTO_TEST_CASE( for_ ){
    assertOutputEquals("for.eddi", "01234");
}

BOOST_AUTO_TEST_CASE( foreach_ ){
    assertOutputEquals("foreach.eddi", "012345");
}

BOOST_AUTO_TEST_CASE( globals_ ){
    assertOutputEquals("globals.eddi", "1000a2000aa");
}

BOOST_AUTO_TEST_CASE( void_functions ){
    assertOutputEquals("void.eddi", "4445");
}

BOOST_AUTO_TEST_CASE( string_functions ){
    assertOutputEquals("return_string.eddi", "abcdef");
}

BOOST_AUTO_TEST_CASE( int_functions ){
    assertOutputEquals("return_int.eddi", "484");
}

BOOST_AUTO_TEST_CASE( recursive_functions ){
    assertOutputEquals("recursive.eddi", "362880");
}

BOOST_AUTO_TEST_CASE( math ){
    assertOutputEquals("math.eddi", "333|111|-111|0|24642|2|-2|-1|1|2|0|-111|");
}

BOOST_AUTO_TEST_CASE( builtin ){
    assertOutputEquals("builtin.eddi", "10|11|12|13|12|13|10|11|4|8|13|8|0|3|");
}

BOOST_AUTO_TEST_CASE( args ){
    assertCompiles("test/cases/args.eddi");

    std::string out = eddic::execCommand("./a.out"); 
    BOOST_CHECK_EQUAL ("./a.out|", out);
    
    out = eddic::execCommand("./a.out arg1 arg2 arg3"); 
    BOOST_CHECK_EQUAL ("./a.out|arg1|arg2|arg3|", out);
}
    
/* Unit test for bug fixes regression */
BOOST_AUTO_TEST_CASE( while_bug ){
    assertOutputEquals("while_bug.eddi", "W1W2W3W4W5");
}
