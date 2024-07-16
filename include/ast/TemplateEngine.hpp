//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef TEMPLATE_ENGINE_H
#define TEMPLATE_ENGINE_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "ast/VariableType.hpp"
#include "ast/source_def.hpp"
#include "parser_x3/error_reporting.hpp"

namespace eddic::ast {

class PassManager;

struct TemplateEngine {
    public:
        TemplateEngine(ast::PassManager& pass_manager);

        using function_template_map = std::unordered_map<std::string, std::unordered_map<std::string, std::pair<ast::struct_definition *, ast::TemplateFunctionDeclaration>>>;

        using LocalFunctionInstantiationMap = std::unordered_multimap<std::string, std::vector<ast::Type>>;
        using FunctionInstantiationMap      = std::unordered_map<std::string, LocalFunctionInstantiationMap>;

        using ClassTemplateMap      = std::unordered_multimap<std::string, ast::struct_definition>;
        using ClassInstantiationMap = std::unordered_multimap<std::string, std::vector<ast::Type>>;

        void check_function(ast::FunctionCall& function_call);
        void check_member_function(std::shared_ptr<const eddic::Type> left, ast::Operation& operation, x3::file_position_tagged& position);
        void check_type(ast::Type& type, x3::file_position_tagged& position);

        void add_template_struct(const std::string& struct_, ast::struct_definition& declaration);
        void add_template_function(const std::string& function, ast::TemplateFunctionDeclaration& declaration);
        void add_template_member_function(const std::string & function, ast::struct_definition & struct_, ast::TemplateFunctionDeclaration & declaration);

        function_template_map function_templates;
        FunctionInstantiationMap function_template_instantiations;

        ClassTemplateMap class_templates;
        ClassInstantiationMap class_template_instantiations;

    private:
        ast::PassManager& pass_manager;

        void check_function(std::vector<ast::Type>& template_types, const std::string& function, x3::file_position_tagged& position, const std::string& context);

        void instantiate_function(ast::struct_definition *,
                                  ast::TemplateFunctionDeclaration & function,
                                  const std::string &                context,
                                  const std::string &                name,
                                  std::vector<ast::Type> &           template_types);

        bool is_instantiated(const std::string& name, const std::string& context, const std::vector<ast::Type>& template_types);
        bool is_class_instantiated(const std::string& name, const std::vector<ast::Type>& template_types);
};

} // namespace eddic::ast

#endif
