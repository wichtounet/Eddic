//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "GlobalContext.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "Variable.hpp"

#include "ltac/stack_space.hpp"
#include "ltac/Utils.hpp"
#include "ltac/Address.hpp"
#include "ltac/Instruction.hpp"

using namespace eddic;

namespace {

void optimize_ranges(std::vector<std::pair<int, int>>& memset_ranges){
    std::sort(memset_ranges.begin(), memset_ranges.end(), [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs){ return lhs.first < rhs.first; });

    auto it = memset_ranges.begin();

    while(it != memset_ranges.end()){
        auto& range = *it;

        auto second_it = std::next(it);

        while(second_it != memset_ranges.end()){
            auto& second_range = *second_it;

            if(second_range.first == range.first + range.second){
                range.second += second_range.second;

                second_it = memset_ranges.erase(second_it);
            } else {
                break;
            }
        }

        it = second_it;
    }
}

} //end of anonymous namespace

void ltac::alloc_stack_space(mtac::Program& program){
    timing_timer timer(program.context->timing(), "stack_space");

    for(auto& function : program.functions){
        auto bb = function.entry_bb();
        auto int_size = INT->size();

        //Clear the stack variables

        std::vector<std::pair<int, int>> memset_ranges;

        for(const auto& var_pair : *function.context){
            const auto& var = var_pair.second;

            if(var->position().isStack()){
                auto type = var->type();
                int position = var->position().offset();

                if(type->is_array() && type->has_elements()){
                    memset_ranges.emplace_back(position + int_size, type->data_type()->size() * type->elements());
                } else if(type->is_custom_type()){
                    memset_ranges.emplace_back(position, type->size());
                }
            }
        }

        optimize_ranges(memset_ranges);

        for(auto& range : memset_ranges){
            int size = range.second / int_size;

            if(size < 8){
                for(int i = 0; i < size; ++i){
                    bb->emplace_back_low(ltac::Operator::MOV, ltac::Address(ltac::BP, range.first + i * int_size), 0);
                }
            } else {
                int int_in_sse = 16 / int_size;

                ltac::PseudoFloatRegister reg(function.pseudo_float_registers());
                function.set_pseudo_float_registers(function.pseudo_float_registers() + 1);

                bb->emplace_back_low(ltac::Operator::XORPS, reg, reg);

                int normal = size % int_in_sse; 

                for(int i = 0; i < normal; ++i){
                    bb->emplace_back_low(ltac::Operator::MOV, ltac::Address(ltac::BP, range.first + i * int_size), 0);
                }

                for(int i = 0; i < size - normal; i += int_in_sse){
                    bb->emplace_back_low(ltac::Operator::MOVDQU, ltac::Address(ltac::BP, range.first + (i +    normal) * int_size), reg);
                }
            }
        }

        auto function_context = function.context;

        //Set the sizes of arrays

        for(const auto& var_pair : *function_context){
            const auto& var = var_pair.second;

            if(var->position().isStack()){
                auto type = var->type();
                int position = var->position().offset();

                if(type->is_array() && type->has_elements()){
                    bb->emplace_back_low(ltac::Operator::MOV, ltac::Address(ltac::BP, position), static_cast<int>(type->elements()));
                } else if(type->is_custom_type()){
                    //Set lengths of arrays inside structures
                    auto struct_type = function_context->global().get_struct(type);
                    auto offset = 0;

                    while(struct_type){
                        for(auto& member : struct_type->members){
                            if(member.type->is_array() && !member.type->is_dynamic_array()){
                                bb->emplace_back_low(ltac::Operator::MOV, 
                                        ltac::Address(ltac::BP, position + offset + function_context->global().member_offset(struct_type, member.name)),
                                        static_cast<int>(member.type->elements()));
                            }
                        }

                        struct_type = function_context->global().get_struct(struct_type->parent_type);
                    }
                }
            }
        }
    }
}
