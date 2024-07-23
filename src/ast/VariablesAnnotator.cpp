//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <algorithm>
#include <memory>

#include "variant.hpp"
#include "SemanticalException.hpp"
#include "Context.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "Variable.hpp"
#include "Utils.hpp"
#include "VisitorUtils.hpp"
#include "mangling.hpp"

#include "ast/VariablesAnnotator.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/GetTypeVisitor.hpp"
#include "ast/VariableType.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

namespace {

struct VariablesVisitor : public boost::static_visitor<> {
    GlobalContext & context;
    std::shared_ptr<ast::TemplateEngine> template_engine;

    VariablesVisitor(GlobalContext & context, std::shared_ptr<ast::TemplateEngine> template_engine) :
            context(context), template_engine(template_engine) {
                //NOP
    }

    AUTO_RECURSE_RETURN_VALUES()
    AUTO_RECURSE_SIMPLE_LOOPS()
    AUTO_RECURSE_BRANCHES()
    AUTO_RECURSE_SWITCH()
    AUTO_RECURSE_SWITCH_CASE()
    AUTO_RECURSE_DEFAULT_CASE()
    AUTO_RECURSE_NEW_ARRAY()
    AUTO_RECURSE_PREFIX()
    AUTO_RECURSE_COMPOSED_VALUES()
    AUTO_RECURSE_BUILTIN_OPERATORS()
    AUTO_RECURSE_TERNARY()
    AUTO_RECURSE_CAST_VALUES()
    AUTO_RECURSE_NEW()
    AUTO_RECURSE_DELETE()
    AUTO_RECURSE_FUNCTION_CALLS()

    void operator()(ast::Assignment& assignment){
        visit(*this, assignment.left_value);
        visit(*this, assignment.value);
    }

    void visit_function(ast::TemplateFunctionDeclaration& declaration){
        // TODO Check whether this is necessary or not
        /*if(!declaration.is_template()){
            template_engine->check_type(declaration.returnType, declaration);

            for(auto& parameter : declaration.parameters){
                template_engine->check_type(parameter.parameterType, declaration);
            }

            visit_each(*this, declaration.instructions);
        }*/
    }

    void visit_function(ast::Constructor& declaration){
        for(auto& parameter : declaration.parameters){
            template_engine->check_type(parameter.parameterType, declaration);
        }

        visit_each(*this, declaration.instructions);
    }

    void visit_function(ast::Destructor& declaration){
        visit_each(*this, declaration.instructions);
    }

    void operator()(ast::GlobalVariableDeclaration& declaration){
        template_engine->check_type(declaration.variableType, declaration);
    }

    void operator()(ast::GlobalArrayDeclaration& declaration){
        template_engine->check_type(declaration.arrayType, declaration);
    }

    void operator()(ast::ArrayDeclaration& declaration){
        template_engine->check_type(declaration.arrayType, declaration);
    }

    void operator()(ast::Foreach& foreach){
        template_engine->check_type(foreach.variableType, foreach);

        visit_each(*this, foreach.instructions);
    }

    void operator()(ast::ForeachIn& foreach){
        template_engine->check_type(foreach.variableType, foreach);

        visit_each(*this, foreach.instructions);
    }

    void operator()(ast::StructDeclaration& declaration){
        template_engine->check_type(declaration.variableType, declaration);
    }

    void operator()(ast::VariableDeclaration& declaration){
        template_engine->check_type(declaration.variableType, declaration);
    }

    AUTO_FORWARD()
    AUTO_RECURSE_SCOPE()
    AUTO_IGNORE_OTHERS()
};

} //end of anonymous namespace

void ast::VariableAnnotationPass::apply_function(ast::TemplateFunctionDeclaration& function){
    VariablesVisitor visitor(context, template_engine);
    visitor.visit_function(function);
}

void ast::VariableAnnotationPass::apply_struct_function(ast::TemplateFunctionDeclaration& function){
    VariablesVisitor visitor(context, template_engine);
    visitor.visit_function(function);
}

void ast::VariableAnnotationPass::apply_struct_constructor(ast::Constructor& constructor){
    VariablesVisitor visitor(context, template_engine);
    visitor.visit_function(constructor);
}

void ast::VariableAnnotationPass::apply_struct_destructor(ast::Destructor& destructor){
    VariablesVisitor visitor(context, template_engine);
    visitor.visit_function(destructor);
}

void ast::VariableAnnotationPass::apply_program(ast::SourceFile& program, bool indicator){
    if(!indicator){
        VariablesVisitor visitor(context, template_engine);

        for(auto it = program.begin(); it < program.end(); ++it){
            if(auto* ptr = boost::get<ast::GlobalArrayDeclaration>(&*it)){
                visit_non_variant(visitor, *ptr);
            } else if(auto* ptr = boost::get<ast::GlobalVariableDeclaration>(&*it)){
                visit_non_variant(visitor, *ptr);
            }
        }
    }
}
