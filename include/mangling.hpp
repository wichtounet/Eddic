//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MANGLING_H
#define MANGLING_H

#include <vector>
#include <string>
#include <memory>

#include "Type.hpp"

#include "ast/Value.hpp"

namespace eddic {

class Parameter;

/*!
 * \brief Return the mangled representation of the given type. 
 * \param type The type to mangle. 
 * \return The mangled type. 
 */
std::string mangle(std::shared_ptr<const Type> type);
std::string mangle_custom_type(const std::string & type);
std::string mangle_template_type(const std::string & type, const std::vector<std::shared_ptr<const Type>> & sub_types);

/*!
 * \brief Return the signature of the function from the mangled representation. 
 * \param mangled The mangled representation of the function. 
 * \return The function signature. 
 */
std::string unmangle(std::string mangled);

/*!
 * \brief Return the mangled representation of the given function based on the parameters.
 *
 * If struct_type is not null, this mangles a member function, and a normal function otherwise. 
 *
 * \param name The name of the function. 
 * \param parameters The parameters. 
 * \param struct_type = The sr
 * \return The mangled function name. 
 */
std::string mangle(const std::string& name, const std::vector<Parameter>& parameters, std::shared_ptr<const Type> struct_type = nullptr);
std::string mangle_ctor(const std::vector<Parameter>& parameters, std::shared_ptr<const Type> struct_type);

/*!
 * \brief Return the mangled representation of the given function. Used for function calls.  
 * \param functionName The name of the function. 
 * \param values The values used for the call. 
 * \param struct_type The struct of the member function.
 * \return The mangled function name. 
 */
std::string mangle(const std::string& functionName, const std::vector<ast::Value>& values, std::shared_ptr<const Type> struct_type = nullptr);
std::string mangle_ctor(const std::vector<ast::Value>& values, std::shared_ptr<const Type> struct_type);
std::string mangle_dtor(std::shared_ptr<const Type> struct_type);

/*!
 * \brief Return the mangled representation of the given function. Generic use with a list of types. 
 * \param functionName The name of the function. 
 * \param types The parameters types. 
 * \param struct_type The struct of the member function.
 * \return The mangled function name. 
 */
std::string mangle(const std::string& functionName, const std::vector<std::shared_ptr<const Type>>& types, std::shared_ptr<const Type> struct_type = nullptr);
std::string mangle_ctor(const std::vector<std::shared_ptr<const Type>>& types, std::shared_ptr<const Type> struct_type);

} //end of eddic

#endif
