//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>

#include "SemanticalException.hpp"
#include "Type.hpp"
#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "mangling.hpp"
#include "VisitorUtils.hpp"

#include "ast/type_collection.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

struct TemplateCollector : public boost::static_visitor<> {
    ast::TemplateEngine& template_engine;

    std::string parent_struct;

    explicit TemplateCollector(ast::TemplateEngine& template_engine) : template_engine(template_engine) {}

    AUTO_RECURSE_PROGRAM()

    void operator()(ast::TemplateFunctionDeclaration& declaration){
        if(declaration.is_template()){
            template_engine.add_template_function(parent_struct, declaration.functionName, declaration);
        }
    }

    void operator()(ast::struct_definition& template_struct){
        if(template_struct.is_template_declaration()){
            template_engine.add_template_struct(template_struct.name, template_struct);
        }
    }

    AUTO_FORWARD()
    AUTO_IGNORE_OTHERS()
};

struct IsResolved : public boost::static_visitor<bool> {
    std::unordered_map<std::string, std::shared_ptr<const eddic::Type>> & fully_resolved;
    explicit IsResolved(std::unordered_map<std::string, std::shared_ptr<const eddic::Type>> & fully_resolved) : fully_resolved(fully_resolved) {}

    bool operator()(const ast::SimpleType& type) const {
        if (is_standard_type(type.type)) {
            return true;
        }

        return fully_resolved.contains(mangle_custom_type(type.type));
    }

    bool operator()(const ast::ArrayType& type) const {
        return visit(*this, type.type.get());
    }

    bool operator()(const ast::PointerType& type) const {
        return visit(*this, type.type.get());
    }

    bool operator()(const ast::TemplateType& type) const {
        return std::ranges::all_of(type.template_types, [this](auto & tmp_type) { return visit(*this, tmp_type); });
    }
};

void ast::TypeCollectionPass::apply_struct(ast::struct_definition& structure, bool){
    // 0. Sanity check: validate no double members
    if (structure.is_template_declaration()) {
        return;
    }

    // If the structure is already annotated, we skip over (this must be a template instantiation)
    if (!structure.mangled_name.empty()) {
        return;
    }

    std::vector<std::string> names;

    for (auto & block : structure.blocks) {
        if (auto * ptr = boost::get<ast::MemberDeclaration>(&block)) {
            auto & member = *ptr;

            if (std::find(names.begin(), names.end(), member.name) != names.end()) {
                context->error_handler.semantical_exception("The member " + member.name + " has already been defined", member);
            }

            names.push_back(member.name);
        } else if (auto * ptr = boost::get<ast::ArrayDeclaration>(&block)) {
            auto & member = *ptr;
            auto & name   = member.arrayName;

            if (std::find(names.begin(), names.end(), name) != names.end()) {
                context->error_handler.semantical_exception("The member " + name + " has already been defined", member);
            }

            names.push_back(name);
        }
    }

    // Prepare the mangled name

    std::vector<std::shared_ptr<const eddic::Type>> template_types;
    std::string                                     mangled_name;
    if (structure.is_template_instantation()) {
        ast::TypeTransformer transformer(*context);

        for (auto & type : structure.inst_template_types) {
            template_types.push_back(visit(transformer, type));
        }

        mangled_name = mangle_template_type(structure.name, template_types);
    } else {
        mangled_name = mangle_custom_type(structure.name);
    }

    structure.mangled_name = mangled_name;

    // Register the structure signature

    if (context->struct_exists(mangled_name)) {
        context->error_handler.semantical_exception("The structure " + mangled_name + " has already been defined", structure);
    }

    context->add_struct(std::make_shared<eddic::Struct>(mangled_name));

    // Collect the function templates
    TemplateCollector template_collector(*template_engine);
    template_collector.parent_struct = structure.mangled_name;
    visit_each(template_collector, structure.blocks);
}

void ast::TypeCollectionPass::apply_program_post(ast::SourceFile& program, bool indicator){
    // TODO Implement more sanity checks for members

    // 2. We can collect all the templates

    if (!indicator) { // We only collect template declarations once
        TemplateCollector template_collector(*template_engine);
        template_collector(program);
    }

    // 3. We try to fully resolve all types
    // TODO, we should only work on the pending list

    while (true) {
        bool progress = false;

        IsResolved is_resolved(fully_resolved);

        for (auto & block : program) {
            if (auto * structure = boost::get<ast::struct_definition>(&block)) {
                if (structure->is_template_declaration()) {
                    continue;
                }

                if (structure->struct_type) {
                    // This is already resolved
                    continue;
                }

                if (structure->parent_type) {
                    if (!visit(is_resolved, *structure->parent_type)) {
                        pending.insert(structure->mangled_name);
                        continue;
                    }
                }

                bool resolved_members = true;
                for (auto & block : structure->blocks) {
                    if (auto * member = boost::get<ast::MemberDeclaration>(&block)) {
                        if (!visit(is_resolved, member->type)) {
                            resolved_members = false;
                            break;
                        }
                    } else if (auto * member = boost::get<ast::ArrayDeclaration>(&block)) {
                        if (!visit(is_resolved, member->arrayType)) {
                            resolved_members = false;
                            break;
                        }
                    }
                }

                if (!resolved_members) {
                    pending.insert(structure->mangled_name);
                    continue;
                }

                // TODO Check if template types can be resolved

                // At this point, the structure is entirely resolvable

                auto signature = context->get_struct(structure->mangled_name);

                // Resolve the parent type if any
                if (structure->parent_type) {
                    signature->parent_type = visit(ast::TypeTransformer(*context), *structure->parent_type);
                }

                // Resolve and collect all members
                for (auto & block : structure->blocks) {
                    if (auto * member = boost::get<ast::MemberDeclaration>(&block)) {
                        signature->members.emplace_back(member->name, visit(ast::TypeTransformer(*context), member->type));
                    } else if (auto * member = boost::get<ast::ArrayDeclaration>(&block)) {
                        auto data_member_type = visit(ast::TypeTransformer(*context), member->arrayType);
                        if (auto * ptr = boost::get<ast::Integer>(&member->size)) {
                            signature->members.emplace_back(member->arrayName, new_array_type(data_member_type, ptr->value));
                        }
                    }
                }

                // Put small types first
                std::sort(signature->members.begin(), signature->members.end(), [](const Member & lhs, const Member & rhs) {
                    if (lhs.type == CHAR || lhs.type == BOOL) {
                        return false;
                    }

                    return rhs.type == CHAR || rhs.type == BOOL;
                });

                // Create the type itself

                if (structure->is_template_instantation()){
                    // TODO structure->struct_type = new_template_type(*context, structure->name, template_types);
                } else {
                    structure->struct_type = new_type(*context, structure->name, false);
                }

                // Annotate functions with the parent struct

                for (auto & block : structure->blocks) {
                    if (auto * function = boost::get<ast::TemplateFunctionDeclaration>(&block)) {
                        if (!function->is_template()) {
                            if (function->context) {
                                function->context->struct_type = structure->struct_type;
                            }
                        }
                    } else if (auto * function = boost::get<ast::Constructor>(&block)) {
                        if (function->context) {
                            function->context->struct_type = structure->struct_type;
                        }
                    } else if (auto * function = boost::get<ast::Destructor>(&block)) {
                        if (function->context) {
                            function->context->struct_type = structure->struct_type;
                        }
                    }
                }

                // Mark the structure as resolved
                fully_resolved[structure->mangled_name] = structure->struct_type;
                pending.erase(structure->mangled_name);

                // We have made some progress
                progress = true;
            }
        }

        // We exit if we have not made any progress or if there is nothing left to do
        if (!progress || pending.empty()) {
            break;
        }
    }
}
