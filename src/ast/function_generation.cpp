//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "VisitorUtils.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "mangling.hpp"
#include "FunctionContext.hpp"

#include "ast/function_generation.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/struct_definition.hpp"

using namespace eddic;

void ast::FunctionGenerationPass::apply_struct(ast::struct_definition& struct_, bool indicator){
    if(indicator){
        return;
    }

    cpp_assert(struct_.struct_type, "The struct type should be set earlier");

    auto platform = context->target_platform();

    bool default_constructor = false;
    bool constructor = false;
    bool destructor = false;
    bool copy_constructor = false;

    for(auto& block : struct_.blocks){
        if(boost::get<ast::Destructor>(&block)){
            destructor = true;
        } else if(auto* ptr = boost::get<ast::Constructor>(&block)){
            constructor = true;

            auto& c = *ptr;

            if(c.parameters.empty()){
                default_constructor = true;
            } else if(c.parameters.size() == 1){
                auto& parameter = c.parameters.front();
                auto parameter_type = visit(ast::TypeTransformer(*context), parameter.parameterType);

                if(parameter_type == new_pointer_type(struct_.struct_type)){
                    copy_constructor = true;
                }
            }
        }
    }

    //Generate default constructor if necessary
    if(!default_constructor && !constructor){
        ast::Constructor c;
        c.context = std::make_shared<FunctionContext>(context, context, platform, configuration);
        c.struct_type = struct_.struct_type;

        std::vector<std::shared_ptr<const eddic::Type>> types;
        c.mangledName = mangle_ctor(types, c.struct_type);

        struct_.blocks.emplace_back(c);
    }

    //Generate destructor if necessary
    if(!destructor){
        ast::Destructor d;
        d.context = std::make_shared<FunctionContext>(context, context, platform, configuration);
        d.struct_type = struct_.struct_type;
        d.mangledName = mangle_dtor(d.struct_type);

        struct_.blocks.emplace_back(d);
    }

    //Generate copy constructor if necessary
    if(!copy_constructor){
        auto type = struct_.struct_type;
        auto struct_type = context->get_struct_safe(type->mangle());

        bool possible = true;
        for(auto& member : struct_type->members){
            if(member.type->is_array()){
                possible = false;
                break;
            }
        }

        if(possible){
            auto function_context = std::make_shared<FunctionContext>(context, context, platform, configuration);

            function_context->addParameter("this", new_pointer_type(type));
            function_context->addParameter("rhs", new_pointer_type(type));

            ast::Constructor c;
            c.context = function_context;
            c.struct_type = type;

            std::vector<std::shared_ptr<const eddic::Type>> types;
            types.push_back(new_pointer_type(type));
            c.mangledName = mangle_ctor(types, type);

            FunctionParameter parameter;
            parameter.parameterName = "rhs";

            ast::PointerType ptr_type;

            if (struct_.is_template_instantation()) {
                ast::TemplateType struct_type;
                struct_type.type = struct_.name;
                struct_type.template_types = struct_.inst_template_types;

                ptr_type.type = struct_type;
            } else {
                ast::SimpleType type;
                type.const_ = false;
                type.type   = c.struct_type->type();

                ptr_type.type = type;
            }

            parameter.parameterType = ptr_type;

            c.parameters.push_back(parameter);

            for(auto& member : struct_type->members){
                auto& name = member.name;

                ast::Assignment assignment;
                assignment.context = function_context;

                ast::VariableValue this_value;
                this_value.context = function_context;
                this_value.variableName = "this";
                this_value.var = function_context->getVariable("this");

                ast::Expression left_expression;
                left_expression.context = function_context;
                left_expression.first = this_value;
                left_expression.operations.push_back(boost::make_tuple(ast::Operator::DOT, ast::Literal{name}));

                ast::VariableValue rhs_value;
                rhs_value.context = function_context;
                rhs_value.variableName = "rhs";
                rhs_value.var = function_context->getVariable("rhs");

                ast::Expression right_expression;
                right_expression.context = function_context;
                right_expression.first = rhs_value;
                right_expression.operations.push_back(boost::make_tuple(ast::Operator::DOT, ast::Literal{name}));

                assignment.left_value = left_expression;
                assignment.value = right_expression;

                c.instructions.emplace_back(std::move(assignment));
            }

            struct_.blocks.emplace_back(std::move(c));
        }
    }
}
