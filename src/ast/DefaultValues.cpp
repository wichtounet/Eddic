//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "variant.hpp"

#include "cpp_utils/assert.hpp"

#include "ast/DefaultValues.hpp"
#include "ast/SourceFile.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/TypeTransformer.hpp"

#include "Type.hpp"
#include "VisitorUtils.hpp"

using namespace eddic;

namespace {

struct SetDefaultValues : public boost::static_visitor<> {
    AUTO_RECURSE_SIMPLE_LOOPS()
    AUTO_RECURSE_FOREACH()
    AUTO_RECURSE_BRANCHES()
    AUTO_RECURSE_SCOPE()

    template<typename T>
    void setDefaultValue(T& declaration){
        if(!declaration.value){
            auto type = visit(ast::TypeTransformer(declaration.context->global(), true), declaration.variableType);

            if(type && type->is_standard_type()){
                if(type == INT){
                    ast::Integer integer;
                    integer.value = 0;

                    declaration.value = ast::Value(integer);
                } else if(type == STRING){
                    ast::Literal literal;
                    literal.value = "";
                    literal.label = "S1";

                    declaration.value = ast::Value(literal);
                } else if(type == FLOAT){
                    ast::Float float_;
                    float_.value = 0.0;

                    declaration.value = ast::Value(float_);
                } else if(type == BOOL){
                    ast::Boolean false_{false};

                    declaration.value = ast::Value(false_);
                } else {
                    cpp_unreachable("Unhandled type");
                }
            }
        }
    }

    void operator()(ast::GlobalVariableDeclaration& declaration){
        setDefaultValue(declaration);
    }

    void operator()(ast::VariableDeclaration& declaration){
        setDefaultValue(declaration);
    }

    AUTO_FORWARD()
    AUTO_IGNORE_OTHERS()
};

} //end of anonymous namespace

void ast::DefaultValuesPass::apply_function(ast::TemplateFunctionDeclaration& function){
    SetDefaultValues visitor;
    visit_each(visitor, function.instructions);
}

void ast::DefaultValuesPass::apply_struct_function(ast::TemplateFunctionDeclaration& function){
    SetDefaultValues visitor;
    visit_each(visitor, function.instructions);
}

void ast::DefaultValuesPass::apply_struct_constructor(ast::Constructor& constructor){
    SetDefaultValues visitor;
    visit_each(visitor, constructor.instructions);
}

void ast::DefaultValuesPass::apply_struct_destructor(ast::Destructor& destructor){
    SetDefaultValues visitor;
    visit_each(visitor, destructor.instructions);
}

void ast::DefaultValuesPass::apply_program(ast::SourceFile& program, bool indicator){
    if(!indicator){
        SetDefaultValues visitor;

        for (auto it = program.begin(); it < program.end(); ++it) {
            if(auto* ptr = boost::get<ast::GlobalArrayDeclaration>(&*it)){
                visit_non_variant(visitor, *ptr);
            } else if(auto* ptr = boost::get<ast::GlobalVariableDeclaration>(&*it)){
                visit_non_variant(visitor, *ptr);
            }
        }
    }
}
