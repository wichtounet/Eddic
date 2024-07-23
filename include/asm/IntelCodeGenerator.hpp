//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef INTEL_CODE_GENERATOR_H
#define INTEL_CODE_GENERATOR_H

#include <memory>
#include <string>
#include "asm/CodeGenerator.hpp"

#include "mtac/forward.hpp"

namespace eddic {

class AssemblyFileWriter;
struct GlobalContext;
class Function;

namespace as {

/*!
 * \class IntelCodeGenerator
 * \brief Base class for code generator on Intel platform. 
 */
class IntelCodeGenerator : public CodeGenerator {
    public:
        IntelCodeGenerator(AssemblyFileWriter& writer, mtac::Program& program, GlobalContext & context);
        
        void generate(StringPool& pool, FloatPool& float_pool) override;

    protected:
        GlobalContext & context;

        void addGlobalVariables(StringPool& pool, FloatPool& float_pool);
        
        virtual void writeRuntimeSupport() = 0;
        virtual void addStandardFunctions() = 0;
        virtual void compile(mtac::Function& function) = 0;

        virtual void defineDataSection() = 0;

        virtual void declareIntArray(const std::string& name, unsigned int size) = 0;
        virtual void declareStringArray(const std::string& name, unsigned int size) = 0;
        virtual void declareFloatArray(const std::string& name, unsigned int size) = 0;

        virtual void declareIntVariable(const std::string& name, int value) = 0;
        virtual void declareBoolVariable(const std::string& name, bool value) = 0;
        virtual void declareCharVariable(const std::string& name, char value) = 0;
        virtual void declareStringVariable(const std::string& name, const std::string& label, int size) = 0;
        virtual void declareString(const std::string& label, const std::string& value) = 0;
        virtual void declareFloat(const std::string& label, double value) = 0;

        void output_function(const std::string& function);
};

} //end of as

} //end of eddic

#endif
