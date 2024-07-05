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
#include "ast/TemplateCollectionPass.hpp"
#include "ast/ContextAnnotator.hpp"
#include "ast/structure_check.hpp"
#include "ast/structure_collection.hpp"
#include "ast/structure_inheritance.hpp"
#include "ast/structure_member_collection.hpp"
#include "ast/DefaultValues.hpp"
#include "ast/member_function_collection.hpp"
#include "ast/function_collection.hpp"
#include "ast/function_generation.hpp"
#include "ast/function_check.hpp"
#include "ast/VariablesAnnotator.hpp"
#include "ast/StringChecker.hpp"
#include "ast/TypeChecker.hpp"
#include "ast/WarningsEngine.hpp"

using namespace eddic;

namespace {

void apply_pass(std::shared_ptr<ast::Pass> pass, ast::struct_definition& struct_){
    pass->apply_struct(struct_, false);

    for(auto& block : struct_.blocks){
        if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&block)){
            if(!ptr->is_template()){
                pass->apply_struct_function(*ptr);
            }
        } else if(auto* ptr = boost::get<ast::Destructor>(&block)){
            pass->apply_struct_destructor(*ptr);
        } else if(auto* ptr = boost::get<ast::Constructor>(&block)){
            pass->apply_struct_constructor(*ptr);
        }
    }
}

void apply_pass(std::shared_ptr<ast::Pass> pass, ast::SourceFile& program, std::shared_ptr<Configuration> configuration){
    LOG<Info>("Passes") << "Run pass \"" << pass->name() << "\"" << log::endl;
    program.context->stats().inc_counter("passes");

    for(unsigned int i = 0; i < pass->passes(); ++i){
        pass->set_current_pass(i);
        pass->apply_program(program, false);

        bool valid = true;

        for(auto& block : program){
            try {
                if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&block)){
                    if(!ptr->is_template()){
                        pass->apply_function(*ptr);
                    }
                } else if(auto* ptr = boost::get<ast::struct_definition>(&block)){
                    if(!ptr->is_template_declaration()){
                        apply_pass(pass, *ptr);
                    }
                }
            } catch (const SemanticalException& e){
                if(!configuration->option_defined("quiet")){
                    output_exception(e, program.context);
                }

                valid = false;
            }
        }

        if(!valid){
            throw TerminationException();
        }
    }
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

    //Structures collection pass
    passes.push_back(make_pass<ast::StructureCollectionPass>("structures collection", template_engine, platform, configuration, pool));

    //Structures inheritance pass
    passes.push_back(make_pass<ast::StructureInheritancePass>("structures inheritance", template_engine, platform, configuration, pool));

    //Template Collection pass
    passes.push_back(make_pass<ast::TemplateCollectionPass>("templates collection", template_engine, platform, configuration, pool));

    //Structures member collection pass
    passes.push_back(make_pass<ast::StructureMemberCollectionPass>("structures member collection", template_engine, platform, configuration, pool));

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

void ast::PassManager::function_instantiated(ast::TemplateFunctionDeclaration& function, const std::string& context){
    LOG<Info>("Passes") << "Apply passes to instantiated function \"" << function.functionName << "\"" << " in context " << context << log::endl;

    for(auto& pass : applied_passes){
        for(unsigned int i = 0; i < pass->passes(); ++i){
            LOG<Info>("Passes") << "Run pass \"" << pass->name() << "\":" << i << log::endl;
            program_.context->stats().inc_counter("passes");

            pass->set_current_pass(i);
            pass->apply_program(program_, true);

            if(context.empty()){
                pass->apply_function(function);
            } else {
                for(auto& block : program_){
                    if(auto* struct_type = boost::get<ast::struct_definition>(&block)){
                        if(!struct_type->is_template_declaration() && struct_type->struct_type->mangle() == context){
                            pass->apply_struct(*struct_type, true);
                            pass->apply_struct_function(function);

                            break;
                        }
                    }
                }
            }
        }
    }

    LOG<Info>("Passes") << "Passes applied to instantiated function \"" << function.functionName << "\"" << " in context " << context << log::endl;

    functions_instantiated.push_back(std::make_pair(context, function));
}

void ast::PassManager::struct_instantiated(ast::struct_definition& struct_){
    cpp_assert(struct_.is_template_instantation(), "Must be called with a template instantiation");

    LOG<Info>("Passes") << "Apply passes to instantiated struct \"" << struct_.name << "\"" << log::endl;

    inc_depth();

    for(auto& pass : applied_passes){
        for(unsigned int i = 0; i < pass->passes(); ++i){
            LOG<Info>("Passes") << "Run pass \"" << pass->name() << "\":" << i << log::endl;
            program_.context->stats().inc_counter("passes");

            pass->set_current_pass(i);
            pass->apply_program(program_, true);
            apply_pass(pass, struct_);
        }
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
        }
        //Normal pass are applied until all function and structures have been handled
        else {
            //The next passes will have to apply it again to fresh functions
            applied_passes.push_back(pass);

            apply_pass(pass, program_, configuration);

            //Add the instantiated class and function templates to the actual program

            for(auto& struct_ : class_instantiated){
                program_.emplace_back(struct_);
            }

            for(auto& function_pair : functions_instantiated){
                auto& context = function_pair.first;
                auto& function = function_pair.second;

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

            class_instantiated.clear();
            functions_instantiated.clear();
        }
    }
}
