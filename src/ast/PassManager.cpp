//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "cpp_utils/assert.hpp"

#include "logging.hpp"
#include "Options.hpp"
#include "SemanticalException.hpp"
#include "TerminationException.hpp"
#include "GlobalContext.hpp"

#include "ast/PassManager.hpp"
#include "ast/Pass.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TemplateEngine.hpp"

//The passes
#include "ast/TransformerEngine.hpp"
#include "ast/ContextAnnotator.hpp"
#include "ast/structure_check.hpp"
#include "ast/DefaultValues.hpp"
#include "ast/member_function_collection.hpp"
#include "ast/function_collection.hpp"
#include "ast/function_generation.hpp"
#include "ast/function_check.hpp"
#include "ast/type_collection.hpp"
#include "ast/type_finalization.hpp"
#include "ast/VariablesAnnotator.hpp"
#include "ast/StringChecker.hpp"
#include "ast/TypeChecker.hpp"
#include "ast/WarningsEngine.hpp"
#include "ast/TemplateFunctionDeclaration.hpp"

using namespace eddic;

namespace {

// This applies the pass to a structure
void apply_pass(ast::Pass & pass, ast::struct_definition& struct_){
    pass.apply_struct(struct_, false);

    for(auto& block : struct_.blocks){
        if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&block)){
            if(!ptr->is_template()){
                pass.apply_struct_function(*ptr);
            }
        } else if(auto* ptr = boost::get<ast::Destructor>(&block)){
            pass.apply_struct_destructor(*ptr);
        } else if(auto* ptr = boost::get<ast::Constructor>(&block)){
            pass.apply_struct_constructor(*ptr);
        }
    }
}

// This applies the pass to the entire program
void apply_pass(ast::Pass & pass, ast::SourceFile& program, Configuration & configuration){
    LOG<Info>("Passes") << "Run (standard) pass \"" << pass.name() << "\"" << log::endl;

    program.context->stats().inc_counter("passes");

    for(unsigned int i = 0; i < pass.passes(); ++i){
        pass.set_current_pass(i);
        pass.apply_program(program, false);

        bool valid = true;

        for (auto it = program.begin(); it < program.end(); ++it) {
            try {
                if (auto * ptr = boost::get<ast::struct_definition>(&*it)) {
                    if(!ptr->is_template_declaration()){
                        apply_pass(pass, *ptr);
                    }
                } else if (auto * ptr = boost::get<ast::TemplateFunctionDeclaration>(&*it)) {
                    if(!ptr->is_template()){
                        pass.apply_function(*ptr);
                    }
                }
            } catch (const SemanticalException& e){
                if(!configuration.option_defined("quiet")){
                    output_exception(e, program.context);
                }

                valid = false;
            }
        }

        pass.apply_program_post(program, false);

        if(!valid){
            throw TerminationException();
        }
    }

    LOG<Info>("Passes") << "Finished running (standard) pass \"" << pass.name() << "\"" << log::endl;
}

template<typename Pass>
std::shared_ptr<Pass> make_pass(const std::string& name, std::shared_ptr<ast::TemplateEngine> template_engine,
            Platform platform, std::shared_ptr<Configuration> configuration, std::shared_ptr<StringPool> pool){
    auto pass = std::make_shared<Pass>();

    pass->set_name(name);
    pass->set_template_engine(template_engine);
    pass->set_platform(platform);
    pass->set_configuration(configuration);
    pass->set_string_pool(pool);

    return pass;
}

} //end of anonymous namespace

ast::PassManager::PassManager(Platform platform, std::shared_ptr<Configuration> configuration, ast::SourceFile& program, std::shared_ptr<StringPool> pool) :
        platform(platform), configuration(configuration), program_(program), pool(pool) {
    template_engine = std::make_shared<ast::TemplateEngine>(*this);
}

void ast::PassManager::init_passes(){
    //Clean pass
    passes.push_back(make_pass<ast::CleanPass>("clean", template_engine, platform, configuration, pool));

    //Context annotation pass
    passes.push_back(make_pass<ast::ContextAnnotationPass>("context annotation", template_engine, platform, configuration, pool));

    //Type collection pass
    passes.push_back(make_pass<ast::TypeCollectionPass>("type collection", template_engine, platform, configuration, pool));

    //Type finalization pass
    passes.push_back(make_pass<ast::TypeFinalizationPass>("type finalization", template_engine, platform, configuration, pool));

    //Function Generation Pass
    passes.push_back(make_pass<ast::FunctionGenerationPass>("function generation", template_engine, platform, configuration, pool));

    //Structures check pass
    passes.push_back(make_pass<ast::StructureCheckPass>("structure check", template_engine, platform, configuration, pool));

    //Add default values to declarations
    passes.push_back(make_pass<ast::DefaultValuesPass>("default values", template_engine, platform, configuration, pool));

    //Member function collection pass
    passes.push_back(make_pass<ast::MemberFunctionCollectionPass>("member function collection", template_engine, platform, configuration, pool));

    //Function collection pass
    passes.push_back(make_pass<ast::FunctionCollectionPass>("function collection", template_engine, platform, configuration, pool));

    //Variables annotation pass
    passes.push_back(make_pass<ast::VariableAnnotationPass>("variables annotation", template_engine, platform, configuration, pool));

    //Function check pass
    passes.push_back(make_pass<ast::FunctionCheckPass>("function check", template_engine, platform, configuration, pool));

    //String collection pass
    passes.push_back(make_pass<ast::StringCollectionPass>("string collection", template_engine, platform, configuration, pool));

    //Type checking pass
    passes.push_back(make_pass<ast::TypeCheckingPass>("Type checking", template_engine, platform, configuration, pool));

    //Transform pass
    passes.push_back(make_pass<ast::TransformPass>("Transform", template_engine, platform, configuration, pool));

    //Transform pass
    passes.push_back(make_pass<ast::WarningsPass>("Warnings", template_engine, platform, configuration, pool));
}

void ast::PassManager::apply_function_instantiated(Pass & pass, ast::TemplateFunctionDeclaration & function) {
    for (unsigned int i = 0; i < pass.passes(); ++i) {
        LOG<Info>("Passes") << "Run (template) pass \"" << pass.name() << "\":" << i << log::endl;

        program_.context->stats().inc_counter("passes");

        pass.set_current_pass(i);
        pass.apply_program(program_, true);
        pass.apply_function(function);
        pass.apply_program_post(program_, true);

        LOG<Info>("Passes") << "Finished running (template) pass \"" << pass.name() << "\":" << i << log::endl;
    }
}

void ast::PassManager::apply_member_function_instantiated(Pass & pass, ast::struct_definition & struct_, ast::TemplateFunctionDeclaration & function) {
    for (unsigned int i = 0; i < pass.passes(); ++i) {
        LOG<Info>("Passes") << "Run (template) pass \"" << pass.name() << "\":" << i << log::endl;

        program_.context->stats().inc_counter("passes");

        pass.set_current_pass(i);
        pass.apply_program(program_, true);
        pass.apply_struct(struct_, true);
        pass.apply_struct_function(function);
        pass.apply_program_post(program_, true);

        LOG<Info>("Passes") << "Finished running (template) pass \"" << pass.name() << "\":" << i << log::endl;
    }
}

void ast::PassManager::apply_struct_instantiated(Pass & pass, ast::struct_definition & struct_) {
    for (unsigned int i = 0; i < pass.passes(); ++i) {
        LOG<Info>("Passes") << "Run (template) pass \"" << pass.name() << "\":" << i << log::endl;

        program_.context->stats().inc_counter("passes");

        pass.set_current_pass(i);
        pass.apply_program(program_, true);
        apply_pass(pass, struct_);
        pass.apply_program_post(program_, true);

        LOG<Info>("Passes") << "Finished running (template) pass \"" << pass.name() << "\":" << i << log::endl;
    }
}

void ast::PassManager::function_instantiated(ast::TemplateFunctionDeclaration& function){
    LOG<Info>("Passes") << "Apply passes to instantiated function \"" << function.functionName << log::endl;

    for(auto& pass : applied_passes){
        apply_function_instantiated(*pass, function);
    }

    LOG<Info>("Passes") << "Passes applied to instantiated function \"" << function.functionName << log::endl;

    functions_instantiated.emplace_back(nullptr, function);
}

void ast::PassManager::member_function_instantiated(ast::struct_definition & struct_, ast::TemplateFunctionDeclaration & function) {
    LOG<Info>("Passes") << "Apply passes to instantiated member function \"" << function.functionName << "\"" << " in " << struct_.mangled_name << log::endl;

    for(auto& pass : applied_passes){
        apply_member_function_instantiated(*pass, struct_, function);
    }

    LOG<Info>("Passes") << "Passes applied to instantiated member function \"" << function.functionName << "\"" << " in " << struct_.mangled_name << log::endl;

    functions_instantiated.emplace_back(&struct_, function);
}

void ast::PassManager::struct_instantiated(ast::struct_definition& struct_){
    cpp_assert(struct_.is_template_instantation(), "Must be called with a template instantiation");

    LOG<Info>("Passes") << "Apply passes to instantiated struct \"" << struct_.name << "\"" << log::endl;

    inc_depth();

    ast::SourceFileBlock block(struct_);
    auto & new_structure = program_.emplace_back(block);
    auto * new_struct = boost::get<struct_definition>(&new_structure);

    cpp_assert(new_struct, "Problem with program insertion");

    for (auto & pass : applied_passes) {
        apply_struct_instantiated(*pass, *new_struct);
    }

    dec_depth();

    LOG<Info>("Passes") << "Passes applied to instantiated struct \"" << new_struct->name << "\"" << log::endl;

    class_instantiated.push_back(new_struct);
}

void ast::PassManager::inc_depth(){
    ++template_depth;

    if(template_depth > static_cast<unsigned int>(configuration->option_int_value("template-depth"))){
        throw SemanticalException("Recursive template-instantiation depth limit reached");
    }
}

void ast::PassManager::dec_depth(){
    --template_depth;
}

void ast::PassManager::run_passes(){
    timing_timer timer(program_.context->timing(), "ast_passes");

    for(auto& pass : passes){
        //A simple pass is only applied once to the whole program
        //They won't be applied on later instantiated function templates and class templates
        if(pass->is_simple()){
            LOG<Info>("Passes") << "Run simple pass \"" << pass->name() << "\"" << log::endl;

            for(unsigned int i = 0; i < pass->passes(); ++i){
                pass->set_current_pass(i);

                //It is up to the simple pass to recurse into the program
                pass->apply_program(program_, false);
            }

            LOG<Info>("Passes") << "Finished running simple pass \"" << pass->name() << "\"" << log::endl;
        } else {
            // Normal pass are applied until all function and structures have been handled
            // Any instantiation will go through all previous passes
            apply_pass(*pass, program_, *configuration);

            while (!class_instantiated.empty() || !functions_instantiated.empty()) {
                auto new_classes = class_instantiated;
                auto new_functions = functions_instantiated;

                class_instantiated.clear();
                functions_instantiated.clear();

                // Add structures to the program and apply passes

                for (auto * struct_ : new_classes) {
                    apply_struct_instantiated(*pass, *struct_);
                }

                // Add functions to the program and apply passes

                for (auto & [struct_, function] : new_functions) {
                    if (struct_) {
                        auto & new_function = struct_->blocks.emplace_back(function);
                        apply_member_function_instantiated(*pass, *struct_, boost::get<ast::TemplateFunctionDeclaration>(new_function));
                    } else {
                        auto & new_function = program_.emplace_back(function);
                        apply_function_instantiated(*pass, boost::get<ast::TemplateFunctionDeclaration>(new_function));
                    }
                }
            }

            //The next passes will have to apply it again to fresh functions
            applied_passes.push_back(pass);
        }
    }
}
