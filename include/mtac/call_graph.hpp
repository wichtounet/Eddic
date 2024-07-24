//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef MTAC_CALL_GRAPH_H
#define MTAC_CALL_GRAPH_H

#include<vector>
#include<memory>
#include<unordered_map>
#include<unordered_set>

#include "../Function.hpp"

namespace std {
    template<>
    class hash<std::reference_wrapper<eddic::Function>> {
        private:
            std::hash<std::string> hasher;

        public:
            size_t operator()(const std::reference_wrapper<eddic::Function>& val) const {
                return hasher(val.get().mangled_name());
            }
    };
}

namespace eddic {

class Function;

namespace mtac {

using Reachable = std::unordered_set<std::reference_wrapper<eddic::Function>>;

struct Program;

struct call_graph_node;
using call_graph_node_p = std::shared_ptr<call_graph_node>;

struct call_graph_edge {
    call_graph_node_p source;
    call_graph_node_p target;
    std::size_t count;

    call_graph_edge(call_graph_node_p source, call_graph_node_p target) : source(source), target(target), count(0){
        //Nothing to init
    }
};

using call_graph_edge_p = std::shared_ptr<call_graph_edge>;

struct call_graph_node {
    eddic::Function& function;
    std::vector<call_graph_edge_p> out_edges;
    std::vector<call_graph_edge_p> in_edges;

    call_graph_node(eddic::Function& function) : function(function){
        //Nothing to init
    }
};

class call_graph {
    public:
        ~call_graph();

        call_graph_node_p entry = nullptr;

        call_graph_node_p node(eddic::Function& function);

        void add_edge(eddic::Function& source, eddic::Function& target);
        call_graph_edge_p edge(eddic::Function& source, eddic::Function& target);

        void compute_reachable();
        void release_reachable();

        void clear();

        /*!
         * \brief Generate the list of functions in topological order from
         * the call graph.
         *
         * The topological sort is performed in O(|E|).
         *
         * \return A list of functions in topological order.
         */
        std::vector<std::reference_wrapper<eddic::Function>> topological_order();

        bool is_reachable(eddic::Function& function);

    private:
        std::unordered_map<std::string, call_graph_node_p> nodes;
        Reachable reachable;
};

void build_call_graph(mtac::Program& program);

} //end of mtac

} //end of eddic

#endif

