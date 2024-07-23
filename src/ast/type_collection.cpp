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
#include "logging.hpp"
#include "VisitorUtils.hpp"

#include "ast/type_collection.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TypeTransformer.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

struct TemplateCollector : public boost::static_visitor<> {
    ast::TemplateEngine& template_engine;

    ast::struct_definition * parent_struct = nullptr;

    explicit TemplateCollector(ast::TemplateEngine& template_engine) : template_engine(template_engine) {}

    AUTO_RECURSE_PROGRAM()

    void operator()(ast::TemplateFunctionDeclaration& declaration){
        if(declaration.is_template()){
            if (parent_struct) {
                template_engine.add_template_member_function(declaration.functionName, *parent_struct, declaration);
            } else {
                template_engine.add_template_function(declaration.functionName, declaration);
            }
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
    GlobalContext &                                                       context;
    std::unordered_map<std::string, std::shared_ptr<const eddic::Type>> & fully_resolved;
    IsResolved(GlobalContext & context, std::unordered_map<std::string, std::shared_ptr<const eddic::Type>> & fully_resolved) :
            context(context), fully_resolved(fully_resolved) {}

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
        // Pointer types can be resolved directly through incomplete pointers
        return true;
    }

    bool operator()(const ast::TemplateType& type) const {
        // All template types must be resolved
        if (!std::ranges::all_of(type.template_types, [this](auto & tmp_type) { return visit(*this, tmp_type); })) {
            return false;
        }

        std::vector<std::shared_ptr<const eddic::Type>> template_types;

        ast::TypeTransformer transformer(context);

        for(auto& type : type.template_types){
            template_types.push_back(visit(transformer, type));
        }

        return fully_resolved.contains(mangle_template_type(type.type, template_types));
    }
};

void ast::TypeCollectionPass::apply_struct(ast::struct_definition& structure, bool){
    // If the structure is already annotated, we skip over (this must be a template instantiation)
    if (structure.mangled_name.empty()) {
        // 0. Sanity check: validate no double members

        std::vector<std::string> names;

        for (auto & block : structure.blocks) {
            if (auto * ptr = boost::get<ast::MemberDeclaration>(&block)) {
                auto & member = *ptr;

                if (std::find(names.begin(), names.end(), member.name) != names.end()) {
                    context.error_handler.semantical_exception("The member " + member.name + " has already been defined", member);
                }

                names.push_back(member.name);
            } else if (auto * ptr = boost::get<ast::ArrayDeclaration>(&block)) {
                auto & member = *ptr;
                auto & name   = member.arrayName;

                if (std::find(names.begin(), names.end(), name) != names.end()) {
                    context.error_handler.semantical_exception("The member " + name + " has already been defined", member);
                }

                names.push_back(name);
            }
        }

        // 1. Prepare the mangled name

        std::vector<std::shared_ptr<const eddic::Type>> template_types;
        std::string                                     mangled_name;
        if (structure.is_template_instantation()) {
            ast::TypeTransformer transformer(context);

            for (auto & type : structure.inst_template_types) {
                template_types.push_back(visit(transformer, type));
            }

            mangled_name = mangle_template_type(structure.name, template_types);
        } else {
            mangled_name = mangle_custom_type(structure.name);
        }

        cpp_assert(!mangled_name.empty(), "Invalid type mangling");

        structure.mangled_name = mangled_name;

        // 2. Register the structure signature

        if (context.struct_exists(mangled_name)) {
            context.error_handler.semantical_exception("The structure " + mangled_name + " has already been defined", structure);
        }

        context.add_struct(std::make_shared<eddic::Struct>(mangled_name));

        // Collect the function templates
        TemplateCollector template_collector(*template_engine);
        template_collector.parent_struct = &structure;
        visit_each(template_collector, structure.blocks);

        pending.insert(&structure);
    }

    // 3. We try to fully resolve all types

    while (true) {
        bool progress = false;

        IsResolved is_resolved(context, fully_resolved);

        auto current_pass = pending;

        for (auto * structure  :current_pass) {
            cpp_assert(!structure->mangled_name.empty(), "mangled_name is empty");

            if (structure->struct_type) {
                // This is already resolved
                continue;
            }

            bool can_resolve = true;
            if (structure->parent_type) {
                template_engine->check_type(*structure->parent_type, *structure);

                if (!visit(is_resolved, *structure->parent_type)) {
                    LOG<Trace>("Types") << "Structure \"" << structure->mangled_name << "\" cannot be resolved (parent)" << log::endl;
                    can_resolve = false;
                }
            }

            for (auto & block : structure->blocks) {
                if (auto * member = boost::get<ast::MemberDeclaration>(&block)) {
                    template_engine->check_type(member->type, *member);

                    if (can_resolve && !visit(is_resolved, member->type)) {
                        can_resolve = false;
                    }
                } else if (auto * member = boost::get<ast::ArrayDeclaration>(&block)) {
                    template_engine->check_type(member->arrayType, *member);

                    if (can_resolve && !visit(is_resolved, member->arrayType)) {
                        LOG<Trace>("Types") << "Structure \"" << structure->mangled_name << "\" cannot be resolved (member)" << log::endl;
                        can_resolve = false;
                    }
                }
            }

            if (structure->is_template_instantation()) {
                for (auto & type : structure->inst_template_types) {
                    template_engine->check_type(type, *structure);

                    if (can_resolve && !visit(is_resolved, type)) {
                        LOG<Trace>("Types") << "Structure \"" << structure->mangled_name << "\" cannot be resolved (template)" << log::endl;
                        can_resolve = false;
                    }
                }

            }

            if (!can_resolve) {
                continue;
            }

            // At this point, the structure is entirely resolvable

            auto signature = context.get_struct_safe(structure->mangled_name);

            // Resolve the parent type if any
            if (structure->parent_type) {
                signature->parent_type = visit(ast::TypeTransformer(context), *structure->parent_type);
            }

            // Resolve and collect all members
            for (auto & block : structure->blocks) {
                if (auto * member = boost::get<ast::MemberDeclaration>(&block)) {
                    if (boost::smart_get<ast::PointerType>(&member->type)) {
                        signature->members.emplace_back(member->name, POINTER);
                    } else {
                        signature->members.emplace_back(member->name, visit(ast::TypeTransformer(context), member->type));
                    }
                } else if (auto * member = boost::get<ast::ArrayDeclaration>(&block)) {
                    auto data_member_type = visit(ast::TypeTransformer(context), member->arrayType);

                    if (data_member_type->is_array()) {
                        context.error_handler.semantical_exception("Multidimensional arrays are not permitted", *member);
                    }

                    if (auto * ptr = boost::get<ast::Integer>(&member->size)) {
                        signature->members.emplace_back(member->arrayName, new_array_type(data_member_type, ptr->value));
                    } else {
                        context.error_handler.semantical_exception("Only arrays of fixed size are supported", *member);
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
                std::vector<std::shared_ptr<const eddic::Type>> template_types;

                ast::TypeTransformer transformer(context);

                for(auto& type : structure->inst_template_types){
                    template_types.push_back(visit(transformer, type));
                }

                structure->struct_type = new_template_type(context, structure->name, template_types);
            } else {
                structure->struct_type = new_type(context, structure->name, false);
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

            LOG<Trace>("Types") << "Structure \"" << structure->mangled_name << "\" is fully resolved (" << fully_resolved.size() << ")" << log::endl;

            // Mark the structure as resolved
            fully_resolved[structure->mangled_name] = structure->struct_type;
            pending.erase(structure);

            // We have made some progress
            progress = true;
        }

        // We exit if we have not made any progress or if there is nothing left to do
        if (!progress || pending.empty()) {
            break;
        }
    }
}

void ast::TypeCollectionPass::apply_program(ast::SourceFile& program, bool indicator){
    if (!indicator) { // We only collect template declarations once
        TemplateCollector template_collector(*template_engine);
        template_collector(program);
    }
}
