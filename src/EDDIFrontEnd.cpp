//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "EDDIFrontEnd.hpp"
#include "SemanticalException.hpp"
#include "Options.hpp"
#include "StringPool.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "PerfsTimer.hpp"

#include "parser_x3/SpiritParser.hpp"

#include "ast/SourceFile.hpp"
#include "ast/PassManager.hpp"
#include "ast/DependenciesResolver.hpp"
#include "ast/Printer.hpp"

#include "mtac/Compiler.hpp"

using namespace eddic;

void check_for_main(GlobalContext & context);
void generate_program(ast::SourceFile& program, std::shared_ptr<Configuration> configuration, Platform platform, std::shared_ptr<StringPool> pool);

std::unique_ptr<mtac::Program> EDDIFrontEnd::compile(const std::string& file, Platform platform, GlobalContext & context){

    //The AST program to build
    ast::SourceFile source(context);

    //Parse the file into the program
    parser_x3::SpiritParser parser;
    bool parsing = parser.parse(file, source, context);

    //If the parsing was successfully
    if(parsing){
        set_string_pool(std::make_shared<StringPool>());

        //Read dependencies
        resolveDependencies(source, parser);

        //If the user asked for it, print the Abstract Syntax Tree coming from the parser
        if(configuration->option_defined("ast-raw")){
            ast::Printer printer;
            printer.print(source);
        }

        //AST Passes
        generate_program(source, configuration, platform, pool);

        //If the user asked for it, print the Abstract Syntax Tree
        if(configuration->option_defined("ast") || configuration->option_defined("ast-only")){
            ast::Printer printer;
            printer.print(source);
        }

        //If the user wants only the AST prints, it is not necessary to compile the AST
        if(configuration->option_defined("ast-only")){
            return nullptr;
        }

        auto program = std::make_unique<mtac::Program>(context);

        //Generate Three-Address-Code language
        mtac::Compiler compiler;
        compiler.compile(source, pool, *program);

        return program;
    }

    //If the parsing fails, the error is already printed to the console
    return nullptr;
}

void generate_program(ast::SourceFile& source, std::shared_ptr<Configuration> configuration, Platform platform, std::shared_ptr<StringPool> pool){
    PerfsTimer timer("AST Passes");

    //Initialize the passes
    ast::PassManager pass_manager(platform, configuration, source, pool);
    pass_manager.init_passes();

    //Run all the passes on the program
    pass_manager.run_passes();

    //Check that there is a main in the program
    check_for_main(source.context);
}

void check_for_main(GlobalContext & context){
    if(!context.exists("_F4main") && !context.exists("_F4mainAS")){
        throw SemanticalException("The program does not contain a valid main function");
    }
}
