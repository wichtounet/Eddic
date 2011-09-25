//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <map>
#include <vector>

#include <unordered_map>
#include <unordered_set>

#include "Types.hpp"

#include "AssemblyFileWriter.hpp"

namespace eddic {

enum PositionType {
    STACK, 
    GLOBAL
};

class Position {
    private:
        const PositionType m_type;
        const int m_offset;
        const std::string m_name;

    public:
        Position() : m_type(STACK), m_offset(0), m_name("") {} //TODO Delete that later, just here for compilation
        Position(PositionType type, int offset) : m_type(type), m_offset(offset), m_name("") {}
        Position(PositionType type, std::string name) : m_type(type), m_offset(0), m_name(name) {}

        bool isStack(){
            return m_type == STACK;
        }

        bool isGlobal(){
            return m_type == GLOBAL;
        }

        int offset(){
            return m_offset;
        }

        std::string name(){
            return m_name;
        }
};

class Variable {
    private:
        const std::string m_name;
        const Type m_type;
        int m_index;
        Position position;

    public:
        Variable(const std::string& name, Type type, int index) : m_name(name), m_type(type), m_index(index) {} //TODO Remove
        Variable(const std::string& name, Type type) : m_name(name), m_type(type) {} //TODO Add position
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

class Context {
    private:
        static std::vector<Context*> contexts;
        static unsigned int currentVariable;

        std::map<std::string, Variable*> variables;
        Context* m_parent;

    public:
        Context() : m_parent(NULL) {
            contexts.push_back(this);
        }
        Context(Context* parent) : m_parent(parent) {
            contexts.push_back(this);
        }
        ~Context();

        bool exists(const std::string& variable) const;
        unsigned int index(const std::string& variable) const;
        Variable* create(const std::string& variable, Type type);
        Variable* find(const std::string& variable);
        void write(AssemblyFileWriter& writer);

        Context* parent() {
            return m_parent;
        }

        static void writeAll(AssemblyFileWriter& writer);
        static void cleanup();
};

//TODO Improve the way to manage memory of context
//TODO Rename to Context when finished the implementation
class TempContext {
    private:
        TempContext* m_parent;
        
        static std::vector<TempContext*> contexts;

    protected:
        std::unordered_map<std::string, Variable*> m_stored;
        std::unordered_set<std::string> m_visibles;

    public:
        TempContext(TempContext* parent) : m_parent(parent) {
            contexts.push_back(this);
        }
        virtual ~TempContext();
        
        virtual void addVariable(const std::string& a, Type type);
        virtual bool exists(const std::string& a) const;
        virtual Variable* getVariable(const std::string& variable) const;
        
        void storeVariable(const std::string& name, Variable* variable);

        TempContext* parent() const  {
            return m_parent;
        }
        
        static void cleanup();
};

class GlobalContext : public TempContext {
    public:
        GlobalContext() : TempContext(NULL) {}
};

class FunctionContext : public TempContext {
    public:
        FunctionContext(TempContext* parent) : TempContext(parent) {}
};

class BlockContext : public TempContext {
    private:
        FunctionContext* m_functionContext;

    public:
        BlockContext(TempContext* parent, FunctionContext* functionContext) : TempContext(parent), m_functionContext(functionContext) {} 
        
        void addVariable(const std::string a, Type type);
};

} //end of eddic

#endif
