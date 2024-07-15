//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "cpp_utils/assert.hpp"

#include "logging.hpp"
#include "variant.hpp"
#include "Utils.hpp"
#include "VisitorUtils.hpp"
#include "Variable.hpp"
#include "iterators.hpp"
#include "Type.hpp"
#include "SemanticalException.hpp"
#include "FunctionContext.hpp"

#include "ast/TemplateEngine.hpp"
#include "ast/SourceFile.hpp"
#include "ast/ASTVisitor.hpp"
#include "ast/GetTypeVisitor.hpp"
#include "ast/PassManager.hpp"

using namespace eddic;

namespace {

//TODO Most of this shit can be easily optimized...

struct ValueCopier : public boost::static_visitor<ast::Value> {
    ast::Value operator()(const ast::Integer& source) const {
        return ast::Value{ast::Integer(source)};
    }

    ast::Value operator()(const ast::IntegerSuffix& source) const {
        return ast::Value{ast::IntegerSuffix(source)};
    }

    ast::Value operator()(const ast::Float& source) const {
        return ast::Value{ast::Float(source)};
    }

    ast::Value operator()(const ast::Literal& source) const {
        return ast::Value{ast::Literal(source)};
    }

    ast::Value operator()(const ast::CharLiteral& source) const {
        return ast::Value(ast::CharLiteral(source));
    }

    ast::Value operator()(const ast::VariableValue& source) const {
        return ast::Value{ast::VariableValue(source)};
    }

    ast::Value operator()(const ast::Expression& source) const {
        ast::Expression copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.first = visit(*this, source.first);

        for(auto& operation : source.operations){
            if(ast::has_operation_value(operation)){
                if(auto* ptr = boost::smart_get<ast::FunctionCall>(&operation.get<1>())){
                    ast::FunctionCall value_copy;

                    value_copy.function_name = ptr->function_name;
                    value_copy.template_types = ptr->template_types;
                    value_copy.mangled_name = ptr->mangled_name;
                    value_copy.left_type = ptr->left_type;

                    std::vector<ast::Value> values;

                    for(auto& v : ptr->values){
                        values.push_back(visit(*this, v));
                    }

                    value_copy.values = values;

                    copy.operations.push_back(boost::make_tuple(operation.get<0>(), value_copy));
                } else {
                    copy.operations.push_back(boost::make_tuple(operation.get<0>(), operation.get<1>()));
                }
            } else {
                copy.operations.push_back(operation);
            }
        }

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::Null& source) const {
        return ast::Value(source);
    }

    ast::Value operator()(const ast::Boolean& source) const {
        return ast::Value(source);
    }

    ast::Value operator()(const ast::FunctionCall& source) const {
        ast::FunctionCall copy;

        copy.mangled_name = source.mangled_name;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.function_name = source.function_name;
        copy.template_types = source.template_types;

        for(auto& value : source.values){
            copy.values.push_back(visit(*this, value));
        }

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::Cast& source) const {
        ast::Cast copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.type = source.type;
        copy.value = visit(*this, source.value);

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::BuiltinOperator& source) const {
        ast::BuiltinOperator copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.type = source.type;

        for(auto& value : source.values){
            copy.values.push_back(visit(*this, value));
        }

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::Assignment& source) const {
        ast::Assignment copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.op = source.op;
        copy.value = visit(*this, source.value);
        copy.left_value = visit(*this, source.left_value);

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::PrefixOperation& source) const {
        ast::PrefixOperation copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.op = source.op;
        copy.left_value = visit(*this, source.left_value);

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::Ternary& source) const {
        ast::Ternary copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.condition = visit(*this, source.condition);
        copy.false_value = visit(*this, source.false_value);
        copy.true_value = visit(*this, source.true_value);

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::New& source) const {
        ast::New copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.context = source.context;
        copy.mangled_name = source.mangled_name;
        copy.type = source.type;

        for(auto& value : source.values){
            copy.values.push_back(visit(*this, value));
        }

        return ast::Value(copy);
    }

    ast::Value operator()(const ast::NewArray& source) const {
        ast::NewArray copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.context = source.context;
        copy.type = source.type;
        copy.size = visit(*this, source.size);

        return ast::Value{copy};
    }

    template<typename T>
    ast::Value operator()(const x3::forward_ast<T>& value) const {
        return (*this)(value.get());
    }
};

struct InstructionCopier : public boost::static_visitor<ast::Instruction> {
    ast::Instruction operator()(const ast::Expression& expression) const {
        return ast::Instruction{boost::smart_get<ast::Expression>(visit_non_variant(ValueCopier(), expression))};
    }

    ast::Instruction operator()(const ast::FunctionCall& source) const {
        ast::FunctionCall copy;

        copy.mangled_name = source.mangled_name;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.template_types = source.template_types;
        copy.function_name = source.function_name;

        for(auto& value : source.values){
            copy.values.push_back(visit(ValueCopier(), value));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::VariableDeclaration& source) const {
        ast::VariableDeclaration copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.variableType = source.variableType;
        copy.variableName = source.variableName;

        if(source.value){
            copy.value = visit(ValueCopier(), *source.value);
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::StructDeclaration& source) const {
        ast::StructDeclaration copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.variableType = source.variableType;
        copy.variableName = source.variableName;

        for(auto& value : source.values){
            copy.values.push_back(visit(ValueCopier(), value));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::ArrayDeclaration& source) const {
        ast::ArrayDeclaration copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.arrayType = source.arrayType;
        copy.arrayName = source.arrayName;
        copy.size = source.size;

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Assignment& source) const {
        ast::Assignment copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.left_value = visit(ValueCopier(), source.left_value);
        copy.value = visit(ValueCopier(), source.value);
        copy.op = source.op;

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Return& source) const {
        ast::Return copy;

        copy.mangled_name = source.mangled_name;
        copy.context = source.context;
        (x3::file_position_tagged&) copy = source;
        copy.value = visit(ValueCopier(), source.value);

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::If& source) const {
        ast::If copy;

        copy.context = source.context;
        copy.condition = visit(ValueCopier(), source.condition);

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        for(auto& else_if : source.elseIfs){
            ast::ElseIf else_if_copy;

            else_if_copy.context = else_if.context;
            else_if_copy.condition = visit(ValueCopier(), else_if.condition);

            for(auto& instruction : else_if.instructions){
                else_if_copy.instructions.push_back(visit(*this, instruction));
            }

            copy.elseIfs.push_back(else_if_copy);
        }

        if(source.else_){
            ast::Else else_copy;

            else_copy.context = source.else_->context;

            for(auto& instruction : (*source.else_).instructions){
                else_copy.instructions.push_back(visit(*this, instruction));
            }

            copy.else_ = else_copy;
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Scope& source) const {
        ast::Scope copy;

        copy.instructions.reserve(source.instructions.size());

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::While& source) const {
        ast::While copy;

        copy.context = source.context;
        copy.condition = visit(ValueCopier(), source.condition);

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::DoWhile& source) const {
        ast::DoWhile copy;

        copy.context = source.context;
        copy.condition = visit(ValueCopier(), source.condition);

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Foreach& source) const {
        ast::Foreach copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.variableType = source.variableType;
        copy.variableName = source.variableName;
        copy.from = source.from;
        copy.to = source.to;

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::ForeachIn& source) const {
        ast::ForeachIn copy;

        copy.context = source.context;
        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.variableType = source.variableType;
        copy.variableName = source.variableName;
        copy.arrayName = source.arrayName;
        copy.var = source.var;
        copy.arrayVar = source.arrayVar;
        copy.iterVar = source.iterVar;

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::PrefixOperation& source) const {
        ast::PrefixOperation copy;

        (x3::file_position_tagged&) copy = (x3::file_position_tagged&) source;
        copy.left_value = visit(ValueCopier(), source.left_value);
        copy.op = source.op;

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::For& source) const {
        ast::For copy;

        copy.context = source.context;

        if(source.start){
            copy.start = visit(*this, *source.start);
        }

        if(source.condition){
            copy.condition = visit(ValueCopier(), *source.condition);
        }

        if(source.repeat){
            copy.repeat = visit(*this, *source.repeat);
        }

        for(auto& instruction : source.instructions){
            copy.instructions.push_back(visit(*this, instruction));
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Switch& source) const {
        ast::Switch copy;

        (x3::file_position_tagged&) copy = source;
        copy.context = source.context;
        copy.value = visit(ValueCopier(), source.value);

        for(auto& switch_case : source.cases){
            ast::SwitchCase case_;

            (x3::file_position_tagged&) case_ = switch_case;
            case_.value = visit(ValueCopier(), switch_case.value);
            case_.context = switch_case.context;

            for(auto& instruction : switch_case.instructions){
                case_.instructions.push_back(visit(*this, instruction));
            }

            copy.cases.push_back(case_);
        }

        if(source.default_case){
            ast::DefaultCase default_;

            default_.context = source.default_case->context;

            for(auto& instruction : source.default_case->instructions){
                default_.instructions.push_back(visit(*this, instruction));
            }

            copy.default_case = default_;
        }

        return ast::Instruction{copy};
    }

    ast::Instruction operator()(const ast::Delete& source) const {
        ast::Delete copy;

        (x3::file_position_tagged&) copy = source;
        copy.value = visit(ValueCopier(), source.value);

        return ast::Instruction{copy};
    }
};

struct Adaptor : public boost::static_visitor<> {
    const std::unordered_map<std::string, ast::Type>& replacements;

    Adaptor(const std::unordered_map<std::string, ast::Type>& replacements) : replacements(replacements) {}

    AUTO_RECURSE_DELETE()
    AUTO_RECURSE_DESTRUCTOR()
    AUTO_RECURSE_RETURN_VALUES()
    AUTO_RECURSE_BRANCHES()
    AUTO_RECURSE_SIMPLE_LOOPS()
    AUTO_RECURSE_PREFIX()
    AUTO_RECURSE_SWITCH()

    void operator()(ast::struct_definition& struct_){
        if(!struct_.is_template_declaration()){
            visit_each(*this, struct_.blocks);
        }
    }

    bool has_to_be_replaced(const std::string& type){
        return replacements.find(type) != replacements.end();
    }

    ast::Type replace(ast::Type& type){
        if(auto* ptr = boost::smart_get<ast::SimpleType>(&type)){
            if(has_to_be_replaced(ptr->type)){
                return replacements.at(ptr->type);
            } else {
                return ast::Type(*ptr);
            }
        } else if(auto* ptr = boost::smart_get<ast::ArrayType>(&type)){
            ptr->type = replace(ptr->type);

            return ast::Type(*ptr);
        } else if(auto* ptr = boost::smart_get<ast::PointerType>(&type)){
            ptr->type = replace(ptr->type);

            return ast::Type(*ptr);
        } else if(auto* ptr = boost::smart_get<ast::TemplateType>(&type)){
            if(has_to_be_replaced(ptr->type)){
                auto replacement = replacements.at(ptr->type);

                decltype(auto) simple = boost::smart_get<ast::SimpleType>(replacement);
                ptr->type = simple.type;
            }

            for(std::size_t i = 0; i < ptr->template_types.size(); ++i){
                ptr->template_types[i] = replace(ptr->template_types[i]);
            }

            return ast::Type(*ptr);
        } else {
            cpp_unreachable("Unhandled type");
        }
    }

    void operator()(ast::MemberDeclaration& declaration){
        declaration.type = replace(declaration.type);
    }

    void operator()(ast::Constructor& declaration){
        for(auto& param : declaration.parameters){
            param.parameterType = replace(param.parameterType);
        }

        visit_each(*this, declaration.instructions);
    }

    void operator()(ast::TemplateFunctionDeclaration& declaration){
        if(!declaration.is_template()){
            declaration.returnType = replace(declaration.returnType);

            for(auto& param : declaration.parameters){
                param.parameterType = replace(param.parameterType);
            }

            visit_each(*this, declaration.instructions);
        }
    }

    void operator()(ast::Assignment& assignment){
        visit(*this, assignment.left_value);
        visit(*this, assignment.value);
    }

    void operator()(ast::Expression& expression){
        for(auto& op : expression.operations){
            if(op.get<0>() == ast::Operator::CALL){
                auto& value = boost::smart_get<ast::FunctionCall>(op.get<1>());

                for(auto& type : value.template_types){
                    type = replace(type);
                }
            }
        }

        VISIT_COMPOSED_VALUE(expression);
    }

    void operator()(ast::FunctionCall& source){
        for(std::size_t i = 0; i < source.template_types.size(); ++i){
            source.template_types[i] = replace(source.template_types[i]);
        }

        visit_each(*this, source.values);
    }

    void operator()(ast::Cast& source){
        source.type = replace(source.type);
        visit(*this, source.value);
    }

    void operator()(ast::VariableDeclaration& source){
        source.variableType = replace(source.variableType);

        visit_optional(*this, source.value);
    }

    void operator()(ast::StructDeclaration& source){
        source.variableType = replace(source.variableType);

        visit_each(*this, source.values);
    }

    void operator()(ast::ArrayDeclaration& source){
        source.arrayType = replace(source.arrayType);
    }

    void operator()(ast::Foreach& source){
        source.variableType = replace(source.variableType);

        visit_each(*this, source.instructions);
    }

    void operator()(ast::ForeachIn& source){
        source.variableType = replace(source.variableType);

        visit_each(*this, source.instructions);
    }

    void operator()(ast::New& source){
        source.type = replace(source.type);

        visit_each(*this, source.values);
    }

    void operator()(ast::NewArray& source){
        source.type = replace(source.type);

        visit(*this, source.size);
    }

    AUTO_FORWARD()
    AUTO_RECURSE_SCOPE()
    AUTO_IGNORE_OTHERS()
};

std::vector<ast::Instruction> copy(const std::vector<ast::Instruction>& source){
    std::vector<ast::Instruction> destination;
    destination.reserve(source.size());

    InstructionCopier copier;

    for(auto& instruction : source){
        destination.emplace_back(visit(copier, instruction));
    }

    return destination;
}

std::vector<ast::StructBlock> copy(const std::vector<ast::StructBlock>& blocks){
    std::vector<ast::StructBlock> destination;
    destination.reserve(blocks.size());

    for(auto& block : blocks){
        if(auto* ptr = boost::smart_get<ast::Destructor>(&block)){
            auto& destructor = *ptr;

            ast::Destructor d;
            d.context = destructor.context;
            (x3::file_position_tagged&) d = destructor;
            d.parameters = destructor.parameters;
            d.instructions = copy(destructor.instructions);

            destination.emplace_back(d);
        } else if(auto* ptr = boost::smart_get<ast::Constructor>(&block)){
            auto& constructor = *ptr;

            ast::Constructor c;
            c.context = constructor.context;
            (x3::file_position_tagged&) c = constructor;
            c.parameters = constructor.parameters;
            c.instructions = copy(constructor.instructions);

            destination.emplace_back(c);
        } else if(auto* ptr = boost::smart_get<ast::ArrayDeclaration>(&block)){
            auto& array_declaration = *ptr;

            ast::ArrayDeclaration copy;
            copy.context = array_declaration.context;
            (x3::file_position_tagged&) copy = array_declaration;
            copy.arrayType = array_declaration.arrayType;
            copy.arrayName = array_declaration.arrayName;
            copy.size = array_declaration.size;

            destination.emplace_back(copy);
        } else if(auto* ptr = boost::smart_get<ast::MemberDeclaration>(&block)){
            destination.emplace_back(*ptr);
        } else if(auto* ptr = boost::smart_get<ast::TemplateFunctionDeclaration>(&block)){
            if(ptr->is_template()){
                auto& function = *ptr;

                ast::TemplateFunctionDeclaration f;
                f.context = function.context;
                (x3::file_position_tagged&) f = function;
                f.struct_name = function.struct_name;
                f.template_types = function.template_types;
                f.returnType = function.returnType;
                f.functionName = function.functionName;
                f.instructions = copy(function.instructions);
                f.parameters = function.parameters;

                destination.emplace_back(f);
            } else {
                auto& function = *ptr;

                ast::TemplateFunctionDeclaration f;
                f.context = function.context;
                (x3::file_position_tagged&) f = function;
                f.returnType = function.returnType;
                f.functionName = function.functionName;
                f.instructions = copy(function.instructions);
                f.parameters = function.parameters;

                destination.emplace_back(f);
            }
        }
    }

    return destination;
}

template<typename Container>
bool is_instantiated(const Container& container, const std::string& name, const std::vector<ast::Type>& template_types){
    auto it_pair = container.equal_range(name);

    if(it_pair.first == it_pair.second && it_pair.second == container.end()){
        return false;
    }

    do {
        auto types = it_pair.first->second;

        if(are_equals(types, template_types)){
            return true;
        }

        ++it_pair.first;
    } while(it_pair.first != it_pair.second);

    return false;
}

} //end of anonymous namespace

ast::TemplateEngine::TemplateEngine(ast::PassManager& pass_manager) : pass_manager(pass_manager) {}

bool ast::TemplateEngine::is_instantiated(const std::string& name, const std::string& context, const std::vector<ast::Type>& template_types){
    return ::is_instantiated(function_template_instantiations[context], name, template_types);
}

bool ast::TemplateEngine::is_class_instantiated(const std::string& name, const std::vector<ast::Type>& template_types){
    return ::is_instantiated(class_template_instantiations, name, template_types);
}

void ast::TemplateEngine::check_function(ast::FunctionCall& function_call){
   if(function_call.template_types.size() > 0){
        check_function(function_call.template_types, function_call.function_name, function_call, "");
   }
}

void ast::TemplateEngine::check_member_function(std::shared_ptr<const eddic::Type> type, ast::Operation& op, x3::file_position_tagged& position){
    if(op.get<0>() == ast::Operator::CALL){
        auto& value = boost::smart_get<ast::FunctionCall>(op.get<1>());

        if(!value.template_types.empty()){
            auto object_type = type->is_pointer() ? type->data_type() : type;

            check_function( value.template_types,
                    value.function_name,
                    position,
                    object_type->mangle());
        }
    }
}

void ast::TemplateEngine::instantiate_function(ast::TemplateFunctionDeclaration& function, const std::string& context, const std::string& name, std::vector<ast::Type>& template_types){
    if(!is_instantiated(name, context, template_types)){
        LOG<Info>("Template") << "Instantiate function template " << name << log::endl;

        //Instantiate the function
        ast::TemplateFunctionDeclaration declaration;
        (x3::file_position_tagged&) declaration = function;
        declaration.returnType = function.returnType;
        declaration.functionName = function.functionName;
        declaration.parameters = function.parameters;
        declaration.instructions = copy(function.instructions);

        std::unordered_map<std::string, ast::Type> replacements;

        auto& source_types = function.template_types;
        for(std::size_t i = 0; i < template_types.size(); ++i){
            replacements[source_types[i]] = template_types[i];
        }

        Adaptor adaptor(replacements);
        visit_non_variant(adaptor, declaration);

        //Mark it as instantiated
        function_template_instantiations[context].insert(ast::TemplateEngine::LocalFunctionInstantiationMap::value_type(name, template_types));

        pass_manager.function_instantiated(declaration, context);
    }

    return;
}

void ast::TemplateEngine::check_function(std::vector<ast::Type>& template_types, const std::string& name, x3::file_position_tagged& position, const std::string& context){
    LOG<Info>("Template") << "Look for function template " << name << " in " << context << log::endl;

    if(!function_templates.count(context) || !function_templates[context].count(name)){
        pass_manager.program().context->error_handler.semantical_exception("There are no registered template function named " + name, position);
    }

    if(context.empty()){
        for(auto& block : pass_manager.program()){
            if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&block)){
                auto& function = *ptr;

                if(!function.is_template()){
                    continue;
                }

                if(function.functionName != name){
                    continue;
                }

                auto& source_types = function.template_types;

                if(source_types.size() == template_types.size()){
                    instantiate_function(function, context, name, template_types);

                    return;
                }
            }
        }
    } else {
        for(auto& block : pass_manager.program()){
            if(auto* ptr = boost::get<ast::struct_definition>(&block)){
                auto& struct_ = *ptr;

                if(!struct_.struct_type || struct_.struct_type->mangle() != context){
                    continue;
                }

                for(auto& struct_block : struct_.blocks){
                    if(auto* ptr = boost::get<ast::TemplateFunctionDeclaration>(&struct_block)){
                        auto& function = *ptr;

                        if(!function.is_template()){
                            continue;
                        }

                        if(function.functionName != name){
                            continue;
                        }

                        auto& source_types = function.template_types;

                        if(source_types.size() == template_types.size()){
                            instantiate_function(function, context, name, template_types);

                            return;
                        }
                    }
                }

                // struct names are unique
                break;
            }
        }
    }

    pass_manager.program().context->error_handler.semantical_exception("No matching template function " + name, position);
}


void ast::TemplateEngine::check_type(ast::Type& type, x3::file_position_tagged& position){
    if(auto* ptr = boost::smart_get<ast::TemplateType>(&type)){
        auto& template_types = ptr->template_types;
        auto& name = ptr->type;

        auto it = class_templates.find(name);

        if(it == class_templates.end()){
            pass_manager.program().context->error_handler.semantical_exception("There are no class template named " + name, position);
        }

        while(it != class_templates.end()){
            auto& struct_declaration = it->second;
            auto& source_types = struct_declaration.decl_template_types;

            if(source_types.size() == template_types.size()){
                if(!is_class_instantiated(name, template_types)){
                    LOG<Info>("Template") << "Instantiate class template " << name << log::endl;

                    //Instantiate the struct
                    ast::struct_definition declaration;

                    (x3::file_position_tagged&) declaration = struct_declaration;
                    declaration.name = struct_declaration.name;
                    declaration.inst_template_types = template_types;
                    declaration.header = struct_declaration.header;
                    declaration.standard = struct_declaration.standard;

                    declaration.blocks = copy(struct_declaration.blocks);

                    std::unordered_map<std::string, ast::Type> replacements;

                    for(std::size_t i = 0; i < template_types.size(); ++i){
                        replacements[source_types[i]] = template_types[i];
                    }

                    Adaptor adaptor(replacements);
                    visit_non_variant(adaptor, declaration);

                    //Mark it as instantiated
                    class_template_instantiations.insert(ast::TemplateEngine::ClassInstantiationMap::value_type(name, template_types));

                    pass_manager.struct_instantiated(declaration);
                }

                return;
            }

            ++it;
        }
    } else if(auto* ptr = boost::smart_get<ast::ArrayType>(&type)){
        check_type(ptr->type, position);
    } else if(auto* ptr = boost::smart_get<ast::PointerType>(&type)){
        check_type(ptr->type, position);
    } else if(auto* ptr = boost::smart_get<ast::TemplateType>(&type)){
        for(auto& type : ptr->template_types){
            check_type(type, position);
        }
    }
}

void ast::TemplateEngine::add_template_struct(const std::string& struct_, ast::struct_definition& declaration){
    LOG<Trace>("Template") << "Collected class template " << struct_ << log::endl;

    class_templates.insert(ast::TemplateEngine::ClassTemplateMap::value_type(struct_, declaration));
}

void ast::TemplateEngine::add_template_function(const std::string& context, const std::string& function, ast::TemplateFunctionDeclaration&){
    LOG<Trace>("Template") << "Collected function template " << function <<" in context " << context << log::endl;

    function_templates[context].insert(function);
}
