//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "ast/TemplateCollectionPass.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/SourceFile.hpp"
#include "ast/TemplateEngine.hpp"

using namespace eddic;

namespace {

struct Collector : public boost::static_visitor<> {
    ast::TemplateEngine& template_engine;

    std::string parent_struct;

    explicit Collector(ast::TemplateEngine& template_engine) : template_engine(template_engine) {}

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

} //end of anonymous namespace

void ast::TemplateCollectionPass::apply_program(ast::SourceFile& program, bool indicator){
    if(!indicator){
        Collector collector(*template_engine);
        collector(program);
    }
}

void ast::TemplateCollectionPass::apply_struct(ast::struct_definition& struct_, bool indicator){
    if(!indicator){
        Collector collector(*template_engine);
        collector.parent_struct = struct_.mangled_name;
        visit_each(collector, struct_.blocks);
    }
}
