//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef FUNCTION_CONTEXT_H
#define FUNCTION_CONTEXT_H

#include <memory>
#include <vector>

#include "Options.hpp"
#include "Context.hpp"
#include "Platform.hpp"

namespace eddic {

class Type;
struct GlobalContext;
class Variable;

typedef std::vector<std::shared_ptr<Variable>> Storage;
typedef boost::variant<int, std::shared_ptr<Variable>> Offset;

/*!
 * \class FunctionContext
 * \brief A symbol table for a function.
 */
class FunctionContext final : public Context, public std::enable_shared_from_this<FunctionContext> {
    private:
     int currentPosition{};
     int currentParameter;
     int temporary = 1;
     int generated = 0;
     Platform platform;

     // Refer all variables that are stored, including temporary
     Storage storage;

    public:
     FunctionContext(std::shared_ptr<Context> parent, std::shared_ptr<GlobalContext> global_context, Platform platform,
                     const std::shared_ptr<Configuration>& configuration);

     int size() const;

     int stack_position() const;
     void set_stack_position(int current);

     std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type) override;
     std::shared_ptr<Variable> addVariable(const std::string& a, std::shared_ptr<const Type> type, ast::Value& value) override;

     std::shared_ptr<Variable> addParameter(const std::string& a, const std::shared_ptr<const Type> & type);

     std::shared_ptr<Variable> newVariable(const std::string& a, const std::shared_ptr<const Type>& type);
     std::shared_ptr<Variable> newVariable(const std::shared_ptr<Variable>& source);
     std::shared_ptr<Variable> newParameter(const std::string& a, const std::shared_ptr<const Type>& type);

     std::shared_ptr<Variable> generate_variable(const std::string& prefix, std::shared_ptr<const Type> type) override;

     std::shared_ptr<Variable> new_temporary(std::shared_ptr<const Type> type) override;

     std::shared_ptr<Variable> new_reference(const std::shared_ptr<const Type>& type,
                                             const std::shared_ptr<Variable>& var, const Offset& offset);

     void removeVariable(std::shared_ptr<Variable> variable) override;

     void allocate_in_param_register(const std::shared_ptr<Variable>& variable, unsigned int register_);

     Storage stored_variables();

     std::shared_ptr<const Type> struct_type = nullptr;

     std::shared_ptr<FunctionContext> function();
};

} //end of eddic

#endif
