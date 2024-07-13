//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef TYPE_TRANSFORMER_H
#define TYPE_TRANSFORMER_H

#include <memory>

#include "variant.hpp"

#include "ast/VariableType.hpp"

namespace eddic {

struct GlobalContext;
class Type;

namespace ast {

/*!
 * \class TypeTransformer
 * \brief AST visitor to transform an AST type into a type descriptor.   
 */
class TypeTransformer : public boost::static_visitor<std::shared_ptr<const eddic::Type>> {
    public:
        explicit TypeTransformer(GlobalContext & context, bool standard_only = false) : context(context), standard_only(standard_only) {}

        std::shared_ptr<const eddic::Type> operator()(const ast::SimpleType& type) const;
        std::shared_ptr<const eddic::Type> operator()(const ast::ArrayType& type) const;
        std::shared_ptr<const eddic::Type> operator()(const ast::PointerType& type) const;
        std::shared_ptr<const eddic::Type> operator()(const ast::TemplateType& type) const;

    private:
        GlobalContext & context;
        bool standard_only = false;
};

} //end of ast

} //end of eddic

#endif
