//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <memory>
#include <utility>

#include "Platform.hpp"

namespace eddic {

class FrontEnd;

namespace mtac {
struct Program;
};

struct Configuration;

/*!
 * \class Compiler
 * \brief The EDDI compiler.
 *
 * This class is used to launch the compilation of a source file. It will then launch each phases of the compilation on this phase
 * and produce either an executable or an assembly file depending on the provided options. 
 */
struct Compiler {
    /*!
     * Compile the given file. 
     * \param file The file to compile. 
     * \return Return code of the compilation process. Numbers other than 0 indicates an error. 
     */
    int compile(const std::string& file, const std::shared_ptr<Configuration> & configuration);
    
    /*!
     * Compile the given file. The compilation is not timed and the platform is not modified.  
     * \param file The file to compile. 
     * \param platform The platform to compile for. 
     * \return Return code of the compilation process. Numbers other than 0 indicates an error. 
     */
    int compile_only(const std::string& file, Platform platform, const std::shared_ptr<Configuration> & configuration);

    std::unique_ptr<mtac::Program> compile_mtac(const std::string& file, Platform platform, const std::shared_ptr<Configuration> & configuration, FrontEnd& front_end);
    
    void compile_ltac(mtac::Program& program, Platform platform, const std::shared_ptr<Configuration> & configuration, FrontEnd& front_end);
};

} //end of eddic

#endif
