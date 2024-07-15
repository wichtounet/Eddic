//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "cpp_utils/assert.hpp"

#include "mangling.hpp"
#include "Type.hpp"
#include "Platform.hpp"
#include "GlobalContext.hpp"

using namespace eddic;

/* Standard Types */

std::shared_ptr<const Type> eddic::BOOL;
std::shared_ptr<const Type> eddic::INT;
std::shared_ptr<const Type> eddic::CHAR;
std::shared_ptr<const Type> eddic::FLOAT;
std::shared_ptr<const Type> eddic::STRING;
std::shared_ptr<const Type> eddic::VOID;
std::shared_ptr<const Type> eddic::POINTER;

/* Const versions */

std::shared_ptr<const Type> CBOOL;
std::shared_ptr<const Type> CINT;
std::shared_ptr<const Type> CCHAR;
std::shared_ptr<const Type> CFLOAT;
std::shared_ptr<const Type> CSTRING;
std::shared_ptr<const Type> CVOID;

void eddic::init_global_types(Platform platform) {
    BOOL   = std::make_shared<StandardType>(platform, BaseType::BOOL, false);
    INT    = std::make_shared<StandardType>(platform, BaseType::INT, false);
    CHAR   = std::make_shared<StandardType>(platform, BaseType::CHAR, false);
    FLOAT  = std::make_shared<StandardType>(platform, BaseType::FLOAT, false);
    STRING = std::make_shared<StandardType>(platform, BaseType::STRING, false);
    VOID   = std::make_shared<StandardType>(platform, BaseType::VOID, false);

    /* Const versions */

    CBOOL   = std::make_shared<StandardType>(platform, BaseType::BOOL, true);
    CINT    = std::make_shared<StandardType>(platform, BaseType::INT, true);
    CCHAR   = std::make_shared<StandardType>(platform, BaseType::CHAR, true);
    CFLOAT  = std::make_shared<StandardType>(platform, BaseType::FLOAT, true);
    CSTRING = std::make_shared<StandardType>(platform, BaseType::STRING, true);
    CVOID   = std::make_shared<StandardType>(platform, BaseType::VOID, true);

    // Incomplete pointer type
    POINTER    = std::make_shared<PointerType>();
}

/* Implementation of Type */

bool Type::is_array() const {
    return false;
}

bool Type::is_dynamic_array() const {
    return is_array() && !has_elements();
}

bool Type::is_structure() const {
    return is_custom_type() || is_template_type();
}

bool Type::is_pointer() const {
    return false;
}

bool Type::is_incomplete() const {
    return false;
}

bool Type::is_custom_type() const {
    return false;
}

bool Type::is_standard_type() const {
    return false;
}

bool Type::is_const() const {
    return false;
}

bool Type::is_template_type() const {
    return false;
}

unsigned int Type::elements() const {
    cpp_unreachable("Not an array type");
}

bool Type::has_elements() const {
    cpp_unreachable("Not an array type");
}

std::string Type::type() const {
    cpp_unreachable("Not a custom type");
}

std::shared_ptr<const Type> Type::data_type() const {
    cpp_unreachable("No data type");
}

std::vector<std::shared_ptr<const Type>> Type::template_types() const {
    cpp_unreachable("No template types");
}

BaseType Type::base() const {
    cpp_unreachable("Not a standard type");
}

std::string Type::mangle() const {
    return ::mangle(shared_from_this());
}

bool eddic::operator==(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs){
    if(lhs->is_array()){
        return rhs->is_array() && lhs->data_type() == rhs->data_type() ;//&& lhs->elements() == rhs->elements();
    }

    if(lhs->is_pointer()){
        return rhs->is_pointer() && lhs->data_type() == rhs->data_type();
    }

    if(lhs->is_custom_type()){
        return rhs->is_custom_type() && lhs->type() == rhs->type();
    }

    if(lhs->is_standard_type()){
        return rhs->is_standard_type() && lhs->base() == rhs->base();
    }

    if(lhs->is_template_type()){
        if(rhs->is_template_type() && lhs->type() == rhs->type()){
            return lhs->template_types() == rhs->template_types();
        }
    }

    return false;
}

bool eddic::operator!=(std::shared_ptr<const Type> lhs, std::shared_ptr<const Type> rhs){
    return !(lhs == rhs);
}

/* Implementation of StandardType  */

StandardType::StandardType(Platform platform, BaseType type, bool const_) :
        Type(getPlatformDescriptor(platform)->size_of(type)), base_type(type), const_(const_) {}

BaseType StandardType::base() const {
    return base_type;
}

bool StandardType::is_standard_type() const {
    return true;
}

bool StandardType::is_const() const {
    return const_;
}

/* Implementation of CustomType */

CustomType::CustomType(const GlobalContext & context, const std::string & type) :
        Type(context.total_size_of_struct(context.get_struct_safe(mangle_custom_type(type)))), m_type(type) {}

std::string CustomType::type() const {
    return m_type;
}

bool CustomType::is_custom_type() const {
    return true;
}

/* Implementation of ArrayType  */

ArrayType::ArrayType(const std::shared_ptr<const Type> & sub_type) : Type(INT->size()), sub_type(sub_type) {}
ArrayType::ArrayType(const std::shared_ptr<const Type> & sub_type, int size) : Type(sub_type->size() * size + INT->size()), sub_type(sub_type), m_elements(size) {}

unsigned int ArrayType::elements() const {
    assert(has_elements());

    return *m_elements;
}

bool ArrayType::has_elements() const {
    return static_cast<bool>(m_elements);
}

std::shared_ptr<const Type> ArrayType::data_type() const {
    return sub_type;
}

bool ArrayType::is_array() const {
    return true;
}

/* Implementation of PointerType  */

PointerType::PointerType() : Type(INT->size()), incomplete(true) {}
PointerType::PointerType(const std::shared_ptr<const Type> & sub_type) : Type(INT->size()), sub_type(sub_type) {}

std::shared_ptr<const Type> PointerType::data_type() const {
    return sub_type;
}

bool PointerType::is_pointer() const {
    return true;
}

bool PointerType::is_incomplete() const {
    return true;
}

/* Implementation of TemplateType  */

TemplateType::TemplateType(const GlobalContext &           context,
                           const std::string &                              main_type,
                           const std::vector<std::shared_ptr<const Type>> & sub_types) :
        Type(context.total_size_of_struct(context.get_struct_safe(mangle_template_type(main_type, sub_types)))), main_type(main_type), sub_types(sub_types) {}

std::string TemplateType::type() const {
    return main_type;
}

std::vector<std::shared_ptr<const Type>> TemplateType::template_types() const {
    return sub_types;
}

bool TemplateType::is_template_type() const {
    return true;
}

/* Implementation of factories  */

std::shared_ptr<const Type> eddic::new_type(const GlobalContext & context, const std::string& type, bool const_){
    //Parse standard and custom types
    if(is_standard_type(type)){
        if(const_){
            if (type == "int") {
                return CINT;
            } else if (type == "char") {
                return CCHAR;
            } else if (type == "bool") {
                return CBOOL;
            } else if (type == "float"){
                return CFLOAT;
            } else if (type == "str"){
                return CSTRING;
            } else {
                return CVOID;
            }
        } else {
            if (type == "int") {
                return INT;
            } else if (type == "char") {
                return CHAR;
            } else if (type == "bool") {
                return BOOL;
            } else if (type == "float"){
                return FLOAT;
            } else if (type == "str"){
                return STRING;
            } else {
                return VOID;
            }
        }
    } else {
        cpp_assert(!const_, "Only standard type can be const");
        return std::make_shared<CustomType>(context, type);
    }
}

std::shared_ptr<const Type> eddic::new_array_type(std::shared_ptr<const Type> data_type){
    return std::make_shared<ArrayType>(data_type);
}

std::shared_ptr<const Type> eddic::new_array_type(std::shared_ptr<const Type> data_type, int size){
    return std::make_shared<ArrayType>(data_type, size);
}

std::shared_ptr<const Type> eddic::new_pointer_type(std::shared_ptr<const Type> data_type){
    return std::make_shared<PointerType>(data_type);
}

std::shared_ptr<const Type> eddic::new_template_type(const GlobalContext & context, std::string data_type, std::vector<std::shared_ptr<const Type>> template_types){
    return std::make_shared<TemplateType>(context, data_type, template_types);
}

bool eddic::is_standard_type(const std::string& type){
    return type == "int" || type == "char" || type == "void" || type == "str" || type == "bool" || type == "float";
}
