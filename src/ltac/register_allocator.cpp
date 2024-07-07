//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <list>
#include <ranges>

#include "cpp_utils/assert.hpp"

#include "PerfsTimer.hpp"
#include "logging.hpp"
#include "FunctionContext.hpp"
#include "GlobalContext.hpp"
#include "Type.hpp"

#include "mtac/GlobalOptimizations.hpp"

#include "ltac/LiveRegistersProblem.hpp"
#include "ltac/register_allocator.hpp"
#include "ltac/interference_graph.hpp"
#include "ltac/Utils.hpp"
#include "ltac/Printer.hpp"

/*
 * Register allocation using Chaitin-style graph coloring allocation. 
 *
 * The renumber and coalescing are simplified by renumbering coalescing 
 * only pseudo registers that are local to a basic block. 
 *
 * TODO:
 *  - Use Chaitin-Briggs optimistic coloring
 *  - Implement rematerialization
 *  - Use UD-chains and make renumber and coalescing complete
 */

using namespace eddic;

namespace {

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoRegister>::value, std::size_t>::type last_register(mtac::Function& function){
    return function.pseudo_registers();
}

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoFloatRegister>::value, std::size_t>::type last_register(mtac::Function& function){
    return function.pseudo_float_registers();
}

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoRegister>::value, void>::type set_last_reg(mtac::Function& function, std::size_t reg){
    function.set_pseudo_registers(reg);
}

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoFloatRegister>::value, void>::type set_last_reg(mtac::Function& function, std::size_t reg){
    function.set_pseudo_float_registers(reg);
}

template<typename Source, typename Target>
void update_uses(ltac::Instruction&, std::unordered_map<Source, Target>&){
    //Nothing by default
}

template<>
void update_uses(ltac::Instruction& statement, std::unordered_map<ltac::PseudoRegister, ltac::Register>& register_allocation){
    for(auto& reg : statement.uses){
        statement.hard_uses.push_back(register_allocation[reg]);
    }

    for(auto& reg : statement.kills){
        statement.hard_kills.push_back(register_allocation[reg]);
    }
}

template<typename Source, typename Target, typename Opt>
void update_reg(Opt& reg, std::unordered_map<Source, Target>& register_allocation){
    if(reg){
        if(auto* ptr = boost::get<Source>(&*reg)){
            if(register_allocation.count(*ptr)){
                reg = register_allocation[*ptr];
            }
        }
    }
}

template<typename Source, typename Target, typename Opt>
void update(Opt& arg, std::unordered_map<Source, Target>& register_allocation){
    if(arg){
        if(auto* ptr = boost::get<Source>(&*arg)){
            if(register_allocation.count(*ptr)){
                arg = register_allocation[*ptr];
            }
        } else if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            update_reg(ptr->base_register, register_allocation);
            update_reg(ptr->scaled_register, register_allocation);
        }
    }
}

template<typename Source, typename Target>
void replace_registers(mtac::Function& function, std::unordered_map<Source, Target>& register_allocation){
    for(auto& bb : function){
        for(auto& statement : bb->l_statements){
            update(statement.arg1, register_allocation);
            update(statement.arg2, register_allocation);
            update(statement.arg3, register_allocation);

            update_uses(statement, register_allocation);
        }
    }
}

template<typename Opt, typename Pseudo>
bool contains_reg_addr(Opt& reg, Pseudo search_reg){
    if(reg){
        if(auto* ptr = boost::get<Pseudo>(&*reg)){
            return search_reg == *ptr;
        }
    }

    return false;
}

template<typename Opt, typename Pseudo>
bool contains_reg(Opt& arg, Pseudo reg){
    if(arg){
        if(auto* ptr = boost::get<Pseudo>(&*arg)){
            return reg == *ptr;
        }

        if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            return contains_reg_addr(ptr->base_register, reg)
                || contains_reg_addr(ptr->scaled_register, reg);
        }
    }

    return false;
}

//Must be called after is_store

template<typename Pseudo>
bool is_load(ltac::Instruction& statement, const Pseudo& reg){
    return contains_reg(statement.arg1, reg) 
        || contains_reg(statement.arg2, reg)
        || contains_reg(statement.arg3, reg);
}

template<typename Pseudo>
bool is_store_complete(ltac::Instruction& statement, const Pseudo& reg){
    if(ltac::erase_result_complete(statement.op)){
        if(auto* reg_ptr = boost::get<Pseudo>(&*statement.arg1)){
            return *reg_ptr == reg;
        }
    }

    return false;
}

template<typename Pseudo>
bool is_store(ltac::Instruction& statement, Pseudo reg){
    if(ltac::erase_result(statement.op)){
        if(auto* reg_ptr = boost::get<Pseudo>(&*statement.arg1)){
            return *reg_ptr == reg;
        }
    }

    return false;
}

template<typename Pseudo>
void replace_reg_addr(boost::optional<ltac::AddressRegister>& reg, Pseudo source, Pseudo target){
    if(reg){
        if(auto* ptr = boost::get<Pseudo>(&*reg)){
            if(*ptr == source){
                reg = target;
            }
        }
    }
}

template<typename Pseudo>
void replace_register(boost::optional<ltac::Argument>& arg, Pseudo source, Pseudo target){
    if(arg){
        if(auto* ptr = boost::get<Pseudo>(&*arg)){
            if(*ptr == source){
                arg = target;
            }
        } else if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            replace_reg_addr(ptr->base_register, source, target);
            replace_reg_addr(ptr->scaled_register, source, target);
        }
    }
}

template<typename Pseudo>
void replace_register(ltac::Instruction& statement, Pseudo source, Pseudo target){
    replace_register(statement.arg1, source, target);
    replace_register(statement.arg2, source, target);
    replace_register(statement.arg3, source, target);
}

//1. Renumber

template<typename Pseudo>
using local_reg = std::unordered_map<mtac::basic_block_p, std::unordered_set<Pseudo>>;

template<typename Opt, typename Pseudo>
void find_reg_addr(Opt& reg, std::unordered_set<Pseudo>& registers){
    if(reg){
        if(auto* ptr = boost::get<Pseudo>(&*reg)){
            registers.insert(*ptr);
        }
    }
}

template<typename Opt, typename Pseudo>
void find_reg(Opt& arg, std::unordered_set<Pseudo>& registers){
    if(arg){
        if(auto* ptr = boost::get<Pseudo>(&*arg)){
            registers.insert(*ptr);
        } else if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            find_reg_addr(ptr->base_register, registers);
            find_reg_addr(ptr->scaled_register, registers);
        }
    }
}

template<typename Stmt, typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoRegister>::value, void>::type 
get_special_uses(Stmt& instruction, std::unordered_set<Pseudo>& local_pseudo_registers){
    for(auto& reg : instruction.uses){
        local_pseudo_registers.insert(reg);
    }
}

template<typename Stmt, typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoFloatRegister>::value, void>::type 
get_special_uses(Stmt& instruction, std::unordered_set<Pseudo>& local_pseudo_registers){
    for(auto& reg : instruction.float_uses){
        local_pseudo_registers.insert(reg);
    }
}

template<typename Pseudo>
void find_local_registers(mtac::Function& function, local_reg<Pseudo>& local_pseudo_registers){
    local_reg<Pseudo> pseudo_registers;

    for(auto& bb : function){
        for(auto& statement : bb->l_statements){
            if(!statement.is_jump() && !statement.is_label()){
                find_reg(statement.arg1, pseudo_registers[bb]);
                find_reg(statement.arg2, pseudo_registers[bb]);
                find_reg(statement.arg3, pseudo_registers[bb]);
            }

            get_special_uses(statement, pseudo_registers[bb]);
        }
    }

    //TODO Find a more efficient way to prune the results...

    for(const auto& bb : function){
        for(auto& reg : pseudo_registers[bb]){
            bool found = false;

            for(const auto& bb2 : function){
                if(bb != bb2 && pseudo_registers[bb2].contains(reg)){
                    found = true;
                    break;
                }
            }

            if(!found){
                local_pseudo_registers[bb].insert(reg);
            }
        }
    }
}

template <typename Pseudo>
std::size_t count_store_complete(const mtac::basic_block_p & bb, Pseudo & reg) {
    std::size_t count = 0;
    for (auto & statement : bb->l_statements) {
        if (is_store_complete<Pseudo>(statement, reg)) {
            ++count;
        }
    }

    return count;
}

template<typename Pseudo>
void renumber(mtac::Function& function){
    local_reg<Pseudo> local_pseudo_registers;
    find_local_registers(function, local_pseudo_registers);

    auto current_reg = last_register<Pseudo>(function);

    for(auto& bb :function){
        for(auto& reg : local_pseudo_registers[bb]){
            //No need to renumber bound registers
            if(reg.bound){
                continue;
            }

            auto count = count_store_complete(bb, reg);

            if(count > 1){
                Pseudo target;
                bool start = false;

                for(auto& statement : bb->l_statements){
                    if(is_store_complete<Pseudo>(statement, reg)){
                        target = Pseudo(++current_reg);
                        start = true;

                        //Make sure to replace only the first argument
                        statement.arg1 = target;
                    } else {
                        if(start){
                            replace_register(statement, reg, target);
                        }
                    }
                }
            }
        }
    }

    set_last_reg<Pseudo>(function, current_reg);
}

//2. Build

template<typename Opt, typename Pseudo>
void gather_reg(Opt& reg, ltac::interference_graph<Pseudo>& graph){
    if(reg){
        if(auto* ptr = boost::get<Pseudo>(&*reg)){
            graph.gather(*ptr);
        }
    }
}

template<typename Opt, typename Pseudo>
void gather(Opt& arg, ltac::interference_graph<Pseudo>& graph){
    if(arg){
        if(auto* ptr = boost::get<Pseudo>(&*arg)){
            graph.gather(*ptr);
        } else if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            gather_reg(ptr->base_register, graph);
            gather_reg(ptr->scaled_register, graph);
        }
    }
}

template<typename Pseudo>
void gather_pseudo_regs(mtac::Function& function, ltac::interference_graph<Pseudo>& graph){
    for(auto& bb : function){
        for(auto& statement : bb->l_statements){
            gather(statement.arg1, graph);
            gather(statement.arg2, graph);
            gather(statement.arg3, graph);
        }
    }

    LOG<Trace>("registers") << "Found " << graph.size() << " pseudo registers" << log::endl;
}

template<typename Pseudo, typename Results>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoRegister>::value, std::unordered_set<Pseudo>&>::type get_live_results(Results& results){
    return results.registers;
}

template<typename Pseudo, typename Results>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoFloatRegister>::value, std::unordered_set<Pseudo>&>::type get_live_results(Results& results){
    return results.float_registers;
}

template<typename Pseudo>
void build_interference_graph(ltac::interference_graph<Pseudo>& graph, mtac::Function& function){
    //Init the graph structure with the current size
    gather_pseudo_regs(function, graph);
    graph.build_graph();

    //Return quickly
    if(!graph.size()){
        return;
    }
    
    ltac::LivePseudoRegistersProblem problem;
    auto live_results = mtac::data_flow(function, problem);

    for(auto& bb : function){
        for(auto& statement : bb->l_statements){
            auto& results = live_results->OUT_S[statement.uid()];

            if(results.top()){
               continue; 
            }

            auto& live_registers = get_live_results<Pseudo>(results.values());

            if(live_registers.size() > 1){
                auto it = live_registers.begin();
                auto end = live_registers.end();

                while(it != end){
                    auto next = it;
                    ++next;

                    while(next != end){
                        graph.add_edge(graph.convert(*it), graph.convert(*next));

                        ++next;
                    }

                    ++it;
                }
            }
        }
    }

    graph.build_adjacency_vectors();
}

//3. Coalesce

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoRegister>::value, bool>::type is_copy(ltac::Instruction& statement){
    return statement.op == ltac::Operator::MOV 
        && boost::get<Pseudo>(&*statement.arg1) 
        && boost::get<Pseudo>(&*statement.arg2);
}

template<typename Pseudo>
typename std::enable_if<std::is_same<Pseudo, ltac::PseudoFloatRegister>::value, bool>::type is_copy(ltac::Instruction& statement){
    return statement.op == ltac::Operator::FMOV 
        && boost::get<Pseudo>(&*statement.arg1) 
        && boost::get<Pseudo>(&*statement.arg2);
}

template<typename Pseudo>
bool coalesce(ltac::interference_graph<Pseudo>& graph, mtac::Function& function){
    local_reg<Pseudo> local_pseudo_registers;
    find_local_registers(function, local_pseudo_registers);
    
    std::unordered_set<Pseudo> prune;
    std::unordered_map<Pseudo, Pseudo> replaces;

    //TODO Instead of pruning dependent registers pairs, update the graph 
    //and continue to avoid too many rebuilding of the graph

    for(const auto& bb : function){
        for(auto& instruction : bb->l_statements){
            if(is_copy<Pseudo>(instruction)){
                auto reg1 = boost::get<Pseudo>(*instruction.arg1);
                auto reg2 = boost::get<Pseudo>(*instruction.arg2);

                if(
                           reg1 != reg2
                        && !reg1.bound && !reg2.bound 
                        && local_pseudo_registers[bb].contains(reg1) && local_pseudo_registers[bb].contains(reg2) 
                        && !graph.connected(graph.convert(reg1), graph.convert(reg2))
                        && !prune.contains(reg1) && !prune.contains(reg2))
                {
                    LOG<Debug>("registers") << "Coalesce " << reg1 << " and " << reg2 << log::endl;

                    replaces[reg1] = reg2;
                    prune.insert(reg1);
                    prune.insert(reg2);

                    ltac::transform_to_nop(instruction);            
                }
            }
        }
    }

    replace_registers(function, replaces);

    return !replaces.empty();
}

//4. Spill costs

constexpr std::size_t store_cost = 5;
constexpr std::size_t load_cost = 3;

constexpr std::size_t depth_cost(unsigned int depth){
    unsigned int cost = 1;

    while(depth > 0){
        cost *= 10;

        --depth;
    }

    return cost;
}

template<typename Opt, typename Pseudo>
void update_cost_reg(Opt& reg, ltac::interference_graph<Pseudo>& graph, unsigned int depth){
    if(reg){
        if(auto* ptr = boost::get<Pseudo>(&*reg)){
            graph.spill_cost(graph.convert(*ptr)) += load_cost * depth_cost(depth);
        }
    }
}

template<typename Opt, typename Pseudo>
void update_cost(Opt& arg, ltac::interference_graph<Pseudo>& graph, unsigned int depth){
    if(arg){
        if(auto* ptr = boost::get<Pseudo>(&*arg)){
            graph.spill_cost(graph.convert(*ptr)) += load_cost * depth_cost(depth);
        } else if(auto* ptr = boost::get<ltac::Address>(&*arg)){
            update_cost_reg(ptr->base_register, graph, depth);
            update_cost_reg(ptr->scaled_register, graph, depth);
        }
    }
}

template<typename Pseudo>
void estimate_spill_costs(mtac::Function& function, ltac::interference_graph<Pseudo>& graph){
    for(const auto& bb : function){
        for(auto& statement : bb->l_statements){
            if(ltac::erase_result(statement.op)){
                if(auto* reg_ptr = boost::get<Pseudo>(&*statement.arg1)){
                    graph.spill_cost(graph.convert(*reg_ptr)) += store_cost * depth_cost(bb->depth);
                }
            } else {
                update_cost(statement.arg1, graph, bb->depth);
            }

            update_cost(statement.arg2, graph, bb->depth);
            update_cost(statement.arg3, graph, bb->depth);
        }
    }
}

//5. Simplify

template<typename Pseudo>
unsigned int number_of_registers(Platform platform){
    if constexpr (std::is_same_v<Pseudo, ltac::PseudoFloatRegister>) {
        return getPlatformDescriptor(platform)->number_of_float_registers();
    } else {
        return getPlatformDescriptor(platform)->number_of_registers();
    }
}

template<typename Pseudo>
std::vector<unsigned short> hard_registers(Platform platform){
    if constexpr (std::is_same_v<Pseudo, ltac::PseudoFloatRegister>) {
        return getPlatformDescriptor(platform)->symbolic_float_registers();
    } else {
        return getPlatformDescriptor(platform)->symbolic_registers();
    }
}

//TODO Find a way to get rid of that and use graph.degree() instead
//graph.degree() is constant-time...

template<typename Pseudo>
std::size_t degree(ltac::interference_graph<Pseudo>& graph, std::size_t candidate, const std::list<std::size_t>& order){
    std::size_t count = 0;

    const auto& neighbors = graph.neighbors(candidate);

    std::unordered_set<std::size_t> bound;

    for(auto neighbor : neighbors){
        auto n_reg = graph.convert(neighbor);

        if (!std::ranges::contains(order, neighbor)) {
            ++count;

            if(n_reg.bound){
                bound.insert(n_reg.binding);
            }
        } else {
            if(n_reg.bound && !bound.contains(n_reg.binding)){
                ++count;
                bound.insert(n_reg.binding);
            }
        }
    }

    return count;
}

template<typename Pseudo>
double spill_heuristic(ltac::interference_graph<Pseudo>& graph, std::size_t reg, std::list<std::size_t>& order){
    return static_cast<double>(graph.spill_cost(reg)) / static_cast<double>(degree(graph, reg, order));
}

template<typename Pseudo>
void simplify(ltac::interference_graph<Pseudo>& graph, Platform platform, std::vector<std::size_t>& spilled, std::list<std::size_t>& order){
    std::set<std::size_t> n;
    for(std::size_t r = 0; r < graph.size(); ++r){
        if(graph.convert(r).bound){
            order.push_back(r);
            graph.remove_node(r);
        } else {
            n.insert(r);
        }
    }

    auto K = number_of_registers<Pseudo>(platform);
    LOG<Trace>("registers") << "Attempt a " << K << "-coloring of the graph" << log::endl;

    while(!n.empty()){
        std::size_t node;
        bool found = false;

        for(auto candidate : n){
            LOG<Dev>("registers") << "Degree(" << graph.convert(candidate) << ") = " << degree(graph, candidate, order) << log::endl;
            if(degree(graph, candidate, order) < K){
                node = candidate;        
                found = true;
                break;
            }
        }
        
        if(!found){
            node = *n.begin();
            auto min_cost = spill_heuristic(graph, node, order);

            for(auto candidate : n){
                if(!graph.convert(candidate).bound && spill_heuristic(graph, candidate, order) < min_cost){
                    min_cost = spill_heuristic(graph, candidate, order);
                    node = candidate;
                }
            }

            LOG<Trace>("registers") << "Mark pseudo " << node << "(" << graph.convert(node) << ") to be spilled" << log::endl;

            spilled.push_back(node);
        } else {
            LOG<Trace>("registers") << "Put pseudo " << graph.convert(node) << " on the stack" << log::endl;

            order.push_back(node);
        }

        n.erase(node);
        graph.remove_node(node);
    }

    LOG<Trace>("registers") << "Graph simplified" << log::endl;
}

//6. Select

template<typename Pseudo, typename Hard>
void replace_registers(mtac::Function& function, std::unordered_map<std::size_t, std::size_t>& allocation, ltac::interference_graph<Pseudo>& graph){
    std::unordered_map<Pseudo, Hard> replacement;

    for(auto& pair : allocation){
        replacement[graph.convert(pair.first)] = Hard{static_cast<unsigned short>(pair.second)};
    }

    replace_registers(function, replacement);
}

template<typename Pseudo, typename Hard>
void select(ltac::interference_graph<Pseudo>& graph, mtac::Function& function, Platform platform, std::list<std::size_t>& order){
    std::unordered_map<std::size_t, std::size_t> allocation;
    std::set<std::size_t> variable_allocated;
    
    auto colors = hard_registers<Pseudo>(platform);

    //Handle bound registers
    order.erase(std::remove_if(order.begin(), order.end(), [&graph, &allocation](auto& reg){
        //Handle bound registers
        if(graph.convert(reg).bound){
            LOG<Trace>("registers") << "Alloc " << graph.convert(reg).binding << " to pseudo " << graph.convert(reg) << " (bound)" << log::endl;
            allocation[reg] = graph.convert(reg).binding;
            return true;
        }

        return false;
    }), order.end());

    while(!order.empty()){
        std::size_t reg = order.back();
        order.pop_back();
        
        for(auto color : colors){
            bool found = false;

            for(auto neighbor : graph.neighbors(reg)){
                if((allocation.contains(neighbor) && allocation[neighbor] == color)){
                    found = true;
                    break;
                }
            }

            if(!found){
                LOG<Trace>("registers") << "Alloc " << color << " to pseudo " << graph.convert(reg) << log::endl;
                allocation[reg] = color;
                variable_allocated.insert(color);
                break;
            }
        }

        if(!allocation.contains(reg)){
            std::cout << "Error allocating " << graph.convert(reg) << std::endl;

            for(auto neighbor : graph.neighbors(reg)){
                if(allocation.contains(neighbor)){
                    std::cout << "neighbor " << graph.convert(neighbor) << " of color " << allocation[neighbor] << std::endl;
                } else {
                    std::cout << "uncolored neighbor " << graph.convert(neighbor) << " of color " << std::endl;
                }
            }
        }

        cpp_assert(allocation.count(reg), "The register must have been allocated a color");
    }

    for(const auto& alloc : allocation){
        function.use(Hard(alloc.second));
    }

    for(const auto& alloc : variable_allocated){
        function.variable_use(Hard(alloc));
    }

    replace_registers<Pseudo, Hard>(function, allocation, graph);
}

//7. Spill code

template<typename It>
void spill_load(ltac::PseudoRegister& pseudo, unsigned int position, It& it){
    it.insert(ltac::Instruction(ltac::Operator::MOV, pseudo, ltac::Address(ltac::BP, position)));
}

template<typename It>
void spill_load(ltac::PseudoFloatRegister& pseudo, unsigned int position, It& it){
    it.insert(ltac::Instruction(ltac::Operator::FMOV, pseudo, ltac::Address(ltac::BP, position)));
}

template<typename It>
void spill_store(ltac::PseudoRegister& pseudo, unsigned int position, It& it){
    it.insert_after(ltac::Instruction(ltac::Operator::MOV, ltac::Address(ltac::BP, position), pseudo));
}

template<typename It>
void spill_store(ltac::PseudoFloatRegister& pseudo, unsigned int position, It& it){
    it.insert_after(ltac::Instruction(ltac::Operator::FMOV, ltac::Address(ltac::BP, position), pseudo));
}

template<typename Pseudo>
void spill_code(ltac::interference_graph<Pseudo>& graph, mtac::Function& function, std::vector<std::size_t>& spilled){
    auto current_reg = last_register<Pseudo>(function);
    
    for(auto reg : spilled){
        auto pseudo_reg = graph.convert(reg);

        //Allocate stack space for the pseudo reg
        auto position = function.context->stack_position();
        position -= INT->size(function.context->global()->target_platform());
        function.context->set_stack_position(position);

        for(auto& bb : function){
            auto it = iterate(bb->l_statements);
            
            //TODO Add support for spilling float registers

            while(it.has_next()){
                auto& statement = *it;

                if(is_store_complete(statement, pseudo_reg)){
                    Pseudo new_pseudo_reg(++current_reg);

                    replace_register(statement, pseudo_reg, new_pseudo_reg);

                    spill_store(new_pseudo_reg, position, it);
                } else if(is_store(statement, pseudo_reg)){
                    Pseudo new_pseudo_reg(++current_reg);
                    
                    replace_register(statement, pseudo_reg, new_pseudo_reg);
                    
                    spill_load(new_pseudo_reg, position, it);

                    ++it;
                    
                    spill_store(new_pseudo_reg, position, it);
                } else if(is_load(statement, pseudo_reg)){
                    Pseudo new_pseudo_reg(++current_reg);

                    replace_register(statement, pseudo_reg, new_pseudo_reg);

                    spill_load(new_pseudo_reg, position, it);

                    ++it;
                } 
                
                ++it;
            }
        }
    }

    set_last_reg<Pseudo>(function, current_reg);
}

//Register allocation

template<typename Pseudo, typename Hard>
void register_allocation(mtac::Function& function, Platform platform){
    bool coalesced = false;

    while(true){
        if(!coalesced){
            //1. Renumber
            renumber<Pseudo>(function);
        }

        //2. Build
        ltac::interference_graph<Pseudo> graph;
        build_interference_graph(graph, function);

        //No pseudo registers, return quickly
        if(!graph.size()){
            return;
        }

        //3. Coalesce
        coalesced = coalesce(graph, function);

        if(coalesced){
            continue;
        }

        //4. Spill costs
        estimate_spill_costs(function, graph);

        //5. Simplify
        std::vector<std::size_t> spilled;
        std::list<std::size_t> order;
        simplify(graph, platform, spilled, order);

        if(!spilled.empty()){
            //6. Spill code
            spill_code(graph, function, spilled);
        } else {
            //7. Select
            select<Pseudo, Hard>(graph, function, platform, order);

            return;
        }
    }
}

} //end of anonymous namespace

void ltac::register_allocation(mtac::Program& program, Platform platform){
    timing_timer timer(program.context->timing(), "register_allocation");

    for(auto& function : program.functions){
        LOG<Trace>("registers") << "Allocate integer registers for function " << function.get_name() << log::endl;
        ::register_allocation<ltac::PseudoRegister, ltac::Register>(function, platform);
        
        LOG<Trace>("registers") << "Allocate float registers for function " << function.get_name() << log::endl;
        ::register_allocation<ltac::PseudoFloatRegister, ltac::FloatRegister>(function, platform);
    }
}
