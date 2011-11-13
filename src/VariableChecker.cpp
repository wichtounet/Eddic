//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <algorithm>

#include <memory>
#include <boost/variant/variant.hpp>

#include "VariableChecker.hpp"

#include "ast/Program.hpp"

#include "IsConstantVisitor.hpp"
#include "GetTypeVisitor.hpp"
#include "SemanticalException.hpp"
#include "Context.hpp"
#include "FunctionContext.hpp"
#include "Types.hpp"
#include "Variable.hpp"

#include "Compiler.hpp"
#include "Options.hpp"

#include "VisitorUtils.hpp"
#include "ASTVisitor.hpp"

using namespace eddic;

struct CheckerVisitor : public boost::static_visitor<> {
    AUTO_RECURSE_PROGRAM()
    AUTO_RECURSE_FUNCTION_CALLS()
    AUTO_RECURSE_SIMPLE_LOOPS()
    AUTO_RECURSE_BRANCHES()
    AUTO_RECURSE_BINARY_CONDITION()
   
    void operator()(ast::FunctionDeclaration& declaration){
        //Add all the parameters to the function context
        for(auto& parameter : declaration.Content->parameters){
            Type type = stringToType(parameter.parameterType);
            
            declaration.Content->context->addParameter(parameter.parameterName, type);    
        }

        visit_each(*this, declaration.Content->instructions);
    }
    
    void operator()(ast::GlobalVariableDeclaration& declaration){
        if (declaration.Content->context->exists(declaration.Content->variableName)) {
            throw SemanticalException("The global Variable " + declaration.Content->variableName + " has already been declared");
        }
    
        if(!boost::apply_visitor(IsConstantVisitor(), *declaration.Content->value)){
            throw SemanticalException("The value must be constant");
        }

        Type type = stringToType(declaration.Content->variableType); 

        declaration.Content->context->addVariable(declaration.Content->variableName, type, *declaration.Content->value);

        Type valueType = boost::apply_visitor(GetTypeVisitor(), *declaration.Content->value);
        if (valueType != type) {
            throw SemanticalException("Incompatible type for global variable " + declaration.Content->variableName);
        }
    }
    
    void operator()(ast::Foreach& foreach){
        if(foreach.Content->context->exists(foreach.Content->variableName)){
            throw SemanticalException("The foreach variable " + foreach.Content->variableName  + " has already been declared");
        }

        foreach.Content->context->addVariable(foreach.Content->variableName, stringToType(foreach.Content->variableType));

        visit_each(*this, foreach.Content->instructions);
    }

    void operator()(ast::Assignment& assignment){
        if (!assignment.Content->context->exists(assignment.Content->variableName)) {
            throw SemanticalException("Variable " + assignment.Content->variableName + " has not  been declared");
        }

        visit(*this, assignment.Content->value);

        auto var = assignment.Content->context->getVariable(assignment.Content->variableName);

        Type valueType = boost::apply_visitor(GetTypeVisitor(), assignment.Content->value);
        if (valueType != var->type()) {
            throw SemanticalException("Incompatible type in assignment of variable " + assignment.Content->variableName);
        }

        var->addReference();
    }
    
    void operator()(ast::Declaration& declaration){
        if (declaration.Content->context->exists(declaration.Content->variableName)) {
            throw SemanticalException("Variable " + declaration.Content->variableName + " has already been declared");
        }

        Type variableType = stringToType(declaration.Content->variableType);
        declaration.Content->context->addVariable(declaration.Content->variableName, variableType);

        visit(*this, *declaration.Content->value);

        Type valueType = boost::apply_visitor(GetTypeVisitor(), *declaration.Content->value);
        if (valueType != variableType) {
            throw SemanticalException("Incompatible type in declaration of variable " + declaration.Content->variableName);
        }
    }
    
    void operator()(ast::Swap& swap){
        if (swap.Content->lhs == swap.Content->rhs) {
            throw SemanticalException("Cannot swap a variable with itself");
        }

        if (!swap.Content->context->exists(swap.Content->lhs) || !swap.Content->context->exists(swap.Content->rhs)) {
            throw SemanticalException("Variable has not been declared in the swap");
        }

        swap.Content->lhs_var = swap.Content->context->getVariable(swap.Content->lhs);
        swap.Content->rhs_var = swap.Content->context->getVariable(swap.Content->rhs);

        if (swap.Content->lhs_var->type() != swap.Content->rhs_var->type()) {
            throw SemanticalException("Swap of variables of incompatible type");
        }

        //Reference both variables
        swap.Content->lhs_var->addReference();
        swap.Content->rhs_var->addReference();
    }

    void operator()(ast::VariableValue& variable){
        if (!variable.Content->context->exists(variable.Content->variableName)) {
            throw SemanticalException("Variable " + variable.Content->variableName + " has not been declared");
        }

        variable.Content->var = variable.Content->context->getVariable(variable.Content->variableName);

        //Reference the variable
        variable.Content->var->addReference();
    }

    void operator()(ast::ComposedValue& value){
        visit(*this, value.Content->first);
        
        for_each(value.Content->operations.begin(), value.Content->operations.end(), 
            [&](boost::tuple<char, ast::Value>& operation){ visit(*this, operation.get<1>()); });

        GetTypeVisitor visitor;

        Type type = boost::apply_visitor(visitor, value.Content->first);

        for(auto& operation : value.Content->operations){
            Type operationType = boost::apply_visitor(visitor, operation.get<1>());

            if(type != operationType){
                throw SemanticalException("Incompatible type");
            }
        }
    }

    void operator()(ast::TerminalNode&){
        //Terminal nodes have no need for variable checking    
    }
};

struct UnusedInspector : public boost::static_visitor<> {
    void check(std::shared_ptr<Context> context){
        auto iter = context->begin();
        auto end = context->end();

        for(; iter != end; iter++){
            auto var = iter->second;

            if(var->referenceCount() == 0){
                if(var->position().isStack()){
                    warn("unused variable '" + var->name() + "'");
                } else if(var->position().isGlobal()){
                    warn("unused global variable '" + var->name() + "'");
                } else if(var->position().isParameter()){
                    warn("unused parameter '" + var->name() + "'");
                }
            }
        }
    }

    void operator()(ast::Program& program){
        check(program.Content->context);
        
        visit_each(*this, program.Content->blocks);
    }

    void operator()(ast::FunctionDeclaration& function){
        check(function.Content->context);
    }

    void operator()(ast::GlobalVariableDeclaration&){
        //Nothing to check there
    }
};

void VariableChecker::check(ast::Program& program){
    CheckerVisitor visitor;
    visit_non_variant(visitor, program);

    if(WarningUnused){
        UnusedInspector inspector;
        visit_non_variant(inspector, program);
    }
}
