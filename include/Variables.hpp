//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef VARIABLES_H
#define VARIABLES_H

#include <string>
#include <map>

#include "Types.hpp"

#include "AssemblyFileWriter.hpp"

namespace eddic {

class Variable {
    private:
        const std::string m_name;
        const Type m_type;
        const int m_index;
    public:
        Variable(const std::string& name, Type type, int index) : m_name(name), m_type(type), m_index(index) {}
        std::string name() const  {
            return m_name;
        }
        int index() const {
            return m_index;
        }
        Type type() const {
            return m_type;
        }
};

class Variables {
    private:
        std::map<std::string, Variable*> variables;
        unsigned int currentVariable;
    public:
        Variables() {
            currentVariable = 0;
        };
        ~Variables();
        bool exists(const std::string& variable) const;
        unsigned int index(const std::string& variable) const;
        Variable* create(const std::string& variable, Type type);
        Variable* find(const std::string& variable);
        void write(AssemblyFileWriter& writer);
};

} //end of eddic

#endif
