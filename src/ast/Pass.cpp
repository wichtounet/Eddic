//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "ast/Pass.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

void ast::Pass::set_template_engine(std::shared_ptr<ast::TemplateEngine> template_engine){
    this->template_engine = template_engine;
}

void ast::Pass::set_name(const std::string& name){
    this->pass_name = name;
}

std::string ast::Pass::name(){
    return pass_name;
}

void ast::Pass::set_string_pool(std::shared_ptr<StringPool> pool){
    this->pool = pool;
}

void ast::Pass::set_platform(Platform platform){
    this->platform = platform;
}

void ast::Pass::set_configuration(std::shared_ptr<Configuration> configuration){
    this->configuration = configuration;
}

unsigned int ast::Pass::passes(){
    return 1;
}

void ast::Pass::set_current_pass(unsigned int i){
    pass = i;
}

bool ast::Pass::is_simple(){
    return false;
}
