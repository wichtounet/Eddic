//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "FunctionChecker.hpp"

#include "ast/Program.hpp"
#include "FunctionTable.hpp"

#include "SemanticalException.hpp"
#include "ASTVisitor.hpp"
#include "VisitorUtils.hpp"

#include "mangling.hpp"

#include "Options.hpp"
#include "Compiler.hpp"

using namespace eddic;

class FunctionInserterVisitor : public boost::static_visitor<> {
    private:
        FunctionTable& functionTable;

    public:
        FunctionInserterVisitor(FunctionTable& table) : functionTable(table) {}

        AUTO_RECURSE_PROGRAM()
         
        void operator()(ast::FunctionDeclaration& declaration){
            auto signature = std::make_shared<Function>();

            signature->name = declaration.Content->functionName;

            for(auto& param : declaration.Content->parameters){
                signature->parameters.push_back(
                    std::make_shared<ParameterType>(param.parameterName, stringToType(param.parameterType)));
            }
            
            declaration.Content->mangledName = signature->mangledName = mangle(declaration.Content->functionName, signature->parameters);

            //TODO Verifiy that the function has not been previously defined

            functionTable.addFunction(signature);

            //Stop recursion here
        }

        void operator()(ast::GlobalVariableDeclaration&){
            //Stop recursion here
        }
        
        void operator()(ast::GlobalArrayDeclaration&){
            //Stop recursion here
        }
};

class FunctionCheckerVisitor : public boost::static_visitor<> {
    private:
        FunctionTable& functionTable;

    public:
        FunctionCheckerVisitor(FunctionTable& table) : functionTable(table) {}

        AUTO_RECURSE_PROGRAM()
        AUTO_RECURSE_FUNCTION_DECLARATION() 
        AUTO_RECURSE_GLOBAL_DECLARATION() 
        AUTO_RECURSE_SIMPLE_LOOPS()
        AUTO_RECURSE_FOREACH()
        AUTO_RECURSE_BRANCHES()
        AUTO_RECURSE_BINARY_CONDITION()
        AUTO_RECURSE_COMPOSED_VALUES()
        AUTO_RECURSE_VARIABLE_OPERATIONS()

        void operator()(ast::FunctionCall& functionCall){
            std::string name = functionCall.Content->functionName;
            
            if(name == "println" || name == "print"){
                return;
            }

            std::string mangled = mangle(name, functionCall.Content->values);

            if(!functionTable.exists(mangled)){
                throw SemanticalException("The function \"" + functionCall.Content->functionName + "()\" does not exists");
            } else {
                functionTable.addReference(mangled);
            }
        }

        template<typename T>        
        void operator()(T&){
            //No function calls there
        }
};

class FunctionInspector : public boost::static_visitor<> {
    private:
        FunctionTable& functionTable;

    public:
        FunctionInspector(FunctionTable& table) : functionTable(table) {}

        void operator()(ast::Program& program){
            visit_each(*this, program.Content->blocks);
        }

        void operator()(ast::FunctionDeclaration& declaration){
            int references = functionTable.referenceCount(declaration.Content->mangledName);

            if(declaration.Content->functionName != "main" && references == 0){
                warn("unused function '" + declaration.Content->functionName + "'");
            }
        }

        void operator()(ast::GlobalVariableDeclaration&){
            //Nothing to warn about there
        }

        void operator()(ast::GlobalArrayDeclaration&){
            //Nothing to warn about there
        }
};

void FunctionChecker::check(ast::Program& program, FunctionTable& functionTable){
    //First phase : Collect functions
    FunctionInserterVisitor inserterVisitor(functionTable);
    inserterVisitor(program);

    //Second phase : Verify calls
    FunctionCheckerVisitor checkerVisitor(functionTable);
    checkerVisitor(program);

    if(WarningUnused){
        //Third phase, warnings
        FunctionInspector inspector(functionTable);
        inspector(program);
    }
}
