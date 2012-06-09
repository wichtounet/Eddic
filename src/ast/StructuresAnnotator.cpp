//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "ast/StructuresAnnotator.hpp"
#include "ast/SourceFile.hpp"
#include "ast/ASTVisitor.hpp"

#include "SymbolTable.hpp"
#include "SemanticalException.hpp"
#include "Types.hpp"

using namespace eddic;

struct StructuresCollector : public boost::static_visitor<> {
    AUTO_RECURSE_PROGRAM()

    void operator()(ast::Struct& struct_){
        if(symbols.struct_exists(struct_.Content->name)){
            throw SemanticalException("The structure " + struct_.Content->name + " has already been defined", struct_.Content->position);
        }

        auto signature = std::make_shared<Struct>(struct_.Content->name);

        for(auto& member : struct_.Content->members){
            auto member_type = new_type(member.Content->type);
            signature->members.push_back(std::make_shared<Member>(member.Content->name, member_type));
        }

        symbols.add_struct(signature);
    }

    AUTO_IGNORE_OTHERS()
};

void ast::defineStructures(ast::SourceFile& program){
    StructuresCollector collector;
    collector(program);
}
