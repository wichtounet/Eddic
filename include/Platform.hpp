//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef PLATFORM_H
#define PLATFORM_H

#include "PlatformDescriptor.hpp"

namespace eddic {

/*!
 * \enum Platform
 * \brief A type of platform. 
 */
enum class Platform : unsigned int {
    INTEL_X86 = 0,
    INTEL_X86_64 = 1
};

constexpr inline unsigned int number_of_platform = 2;

/*!
 * \brief Return the platform descriptor of the specified platform. 
 * \param platform The platform identifier
 * \return The platform descriptor of the given platform. 
 */
const PlatformDescriptor* getPlatformDescriptor(Platform platform);

} //end of eddic

#endif
