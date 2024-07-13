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
#include "ast/VariablesAnnotator.hpp"
#include "ast/StringChecker.hpp"
#include "ast/TypeChecker.hpp"
#include "ast/WarningsEngine.hpp"

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

        for(auto& block : program){
            try {
                if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&block)){
                    if(!ptr->is_template()){
                        pass.apply_function(*ptr);
                    }
                } else if(auto* ptr = boost::get<ast::struct_definition>(&block)){
                    if(!ptr->is_template_declaration()){
                        apply_pass(pass, *ptr);
                    }
                }
            } catch (const SemanticalException& e){
                if(!configuration.option_defined("quiet")){
                    output_exception(e, program.context);
                }

                valid = false;
            }
        }

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

    // TODO Need a validation pair for the resolving

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

    //Function check pass
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

void ast::PassManager::apply_function_instantiated(Pass & pass, ast::TemplateFunctionDeclaration & function, const std::string & context) {
    for (unsigned int i = 0; i < pass.passes(); ++i) {
        LOG<Info>("Passes") << "Run (template) pass \"" << pass.name() << "\":" << i << log::endl;

        program_.context->stats().inc_counter("passes");

        pass.set_current_pass(i);
        pass.apply_program(program_, true);

        if (context.empty()) {
            pass.apply_function(function);
        } else {
            for (auto & block : program_) {
                if (auto * struct_type = boost::get<ast::struct_definition>(&block)) {
                    if (!struct_type->is_template_declaration() && struct_type->struct_type->mangle() == context) {
                        pass.apply_struct(*struct_type, true);
                        pass.apply_struct_function(function);

                        break;
                    }
                }
            }
        }

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

        LOG<Info>("Passes") << "Finished running (template) pass \"" << pass.name() << "\":" << i << log::endl;
    }
}

void ast::PassManager::function_instantiated(ast::TemplateFunctionDeclaration& function, const std::string& context){
    LOG<Info>("Passes") << "Apply passes to instantiated function \"" << function.functionName << "\"" << " in context " << context << log::endl;

    for(auto& pass : applied_passes){
        apply_function_instantiated(*pass, function, context);
    }

    LOG<Info>("Passes") << "Passes applied to instantiated function \"" << function.functionName << "\"" << " in context " << context << log::endl;

    functions_instantiated.emplace_back(context, function);
}

void ast::PassManager::struct_instantiated(ast::struct_definition& struct_){
    cpp_assert(struct_.is_template_instantation(), "Must be called with a template instantiation");

    LOG<Info>("Passes") << "Apply passes to instantiated struct \"" << struct_.name << "\"" << log::endl;

    inc_depth();

    for(auto& pass : applied_passes){
        apply_struct_instantiated(*pass, struct_);
    }

    dec_depth();

    LOG<Info>("Passes") << "Passes applied to instantiated struct \"" << struct_.name << "\"" << log::endl;

    class_instantiated.push_back(struct_);
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

                // Apply the current pass to instantiated structures
                for(auto& struct_ : new_classes){
                    apply_struct_instantiated(*pass, struct_);
                }

                // Apply the current pass to instantiated functions
                for(auto& [context, function] : new_functions){
                    apply_function_instantiated(*pass, function, context);
                }

                //Add the instantiated class and function templates to the actual program

                for(auto& struct_ : new_classes){
                    program_.emplace_back(struct_);
                }

                for(auto& [context, function] : new_functions){
                    if(context.empty()){
                        program_.emplace_back(function);
                    } else {
                        for(auto& block : program_){
                            if(auto* struct_type = boost::get<ast::struct_definition>(&block)){
                                if(!struct_type->is_template_declaration() && struct_type->struct_type->mangle() == context){
                                    struct_type->blocks.emplace_back(function);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            //The next passes will have to apply it again to fresh functions
            applied_passes.push_back(pass);

        }
    }
}
