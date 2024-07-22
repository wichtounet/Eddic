//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "cpp_utils/assert.hpp"

#include "Function.hpp"
#include "Type.hpp"

#include "mtac/Utils.hpp"

using namespace eddic;

Function::Function(std::shared_ptr<const Type> return_type, std::string name, std::string mangled_name) 
        : _return_type(return_type), _name(std::move(name)), _mangled_name(std::move(mangled_name)) {
    //Nothing to do
}

Function::Function(Function&& rhs) : 
        _struct_type(std::move(rhs._struct_type)), _return_type(std::move(rhs._return_type)),  
        _name(std::move(rhs._name)), _mangled_name(std::move(rhs._mangled_name)),
        _standard(std::move(rhs._standard)),
        _parameters(std::move(rhs._parameters)) {

   //Reset rhs
   rhs._standard = false;
}

//TODO Why is context not moved ?

Function& Function::operator=(Function&& rhs){
    _struct_type = std::move(rhs._struct_type);
    _return_type = std::move(rhs._return_type);
    _name = std::move(rhs._name);
    _mangled_name = std::move(rhs._mangled_name);
    _standard = std::move(rhs._standard);
    _parameters = std::move(rhs._parameters);

   //Reset rhs
   rhs._standard = false;

   return *this;
}
        
const Parameter& Function::parameter(std::size_t i) const {
    return _parameters.at(i);
}

const Parameter& Function::parameter(const std::string& name) const {
    for(auto& p : _parameters){
        if(p.name() == name){
            return p;
        }
    }

    cpp_unreachable(("There are no \"" + name + "\" parameter in the function").c_str());
}

std::vector<Parameter>& Function::parameters(){
    return _parameters;
}

const std::vector<Parameter>& Function::parameters() const {
    return _parameters;
}

unsigned int Function::parameter_position_by_type(const std::string& name) const {
    auto type = parameter(name).type();

    if(mtac::is_single_int_register(type)){
        unsigned int position = 0;
        
        for(auto& p : _parameters){
            if(mtac::is_single_int_register(p.type())){
                ++position; 
            }

            if(p.name() == name){
                return position;
            }
        }
        
        cpp_unreachable("This parameter does not exists in the given function");
    } else if(mtac::is_single_float_register(type)){
        unsigned int position = 0;
        
        for(auto& p : _parameters){
            if(mtac::is_single_float_register(p.type())){
                ++position; 
            }

            if(p.name() == name){
                return position;
            }
        }
        
        cpp_unreachable("This parameter does not exists in the given function");
    } else {
        return 0;
    }
}

bool eddic::operator==(const eddic::Function& lhs, const eddic::Function& rhs){
    return lhs.mangled_name() == rhs.mangled_name();
}

bool eddic::operator!=(const eddic::Function& lhs, const eddic::Function& rhs){
    return lhs.mangled_name() != rhs.mangled_name();
}

void Function::set_context(FunctionContext * context){
    _context = context;
}

FunctionContext* Function::context() const {
    return _context;
}

std::shared_ptr<const Type>& Function::struct_type(){
    return _struct_type;
}

bool Function::standard() const {
    return _standard;
}

bool& Function::standard(){
    return _standard;
}

const std::shared_ptr<const Type>& Function::return_type() const {
    return _return_type;
}

const std::string& Function::name() const {
    return _name;
}

const std::string& Function::mangled_name() const {
    return _mangled_name;
}
