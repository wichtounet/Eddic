//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef AST_PASS_H
#define AST_PASS_H

#include <memory>
#include <string>

#include "Platform.hpp"

#include "ast/source_def.hpp"

namespace eddic {

struct Configuration;
struct StringPool;

namespace ast {

struct TemplateEngine;

struct Pass {
    virtual void apply_program(ast::SourceFile &, bool) {}
    virtual void apply_program_post(ast::SourceFile &, bool) {}
    virtual void apply_function(ast::TemplateFunctionDeclaration &) {}
    virtual void apply_struct(ast::struct_definition &, bool) {}
    virtual void apply_struct_function(ast::TemplateFunctionDeclaration &) {}
    virtual void apply_struct_constructor(ast::Constructor &) {}
    virtual void apply_struct_destructor(ast::Destructor &) {}

    void        set_string_pool(std::shared_ptr<StringPool> pool);
    void        set_template_engine(std::shared_ptr<ast::TemplateEngine> template_engine);
    void        set_platform(Platform platform);
    void        set_configuration(std::shared_ptr<Configuration> configuration);
    void        set_current_pass(unsigned int i);
    void        set_name(const std::string & name);
    std::string name();

    virtual unsigned int passes();
    virtual bool         is_simple();

protected:
    unsigned int pass = 0;
    std::string  pass_name;

    std::shared_ptr<StringPool>          pool;
    std::shared_ptr<ast::TemplateEngine> template_engine;
    Platform                             platform;
    std::shared_ptr<Configuration>       configuration;
};

} // namespace ast

} // namespace eddic

#endif
