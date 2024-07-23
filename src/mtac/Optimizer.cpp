//=======================================================================
// Copyright Baptiste Wicht 2011-2016.
// Distributed under the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <memory>
#include <thread>
#include <type_traits>

#include "boost_cfg.hpp"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>

#include "cpp_utils/assert.hpp"
#include "cpp_utils/tmp.hpp"

#include "Options.hpp"
#include "PerfsTimer.hpp"
#include "iterators.hpp"
#include "logging.hpp"
#include "timing.hpp"
#include "GlobalContext.hpp"

#include "mtac/pass_traits.hpp"
#include "mtac/Utils.hpp"
#include "mtac/Pass.hpp"
#include "mtac/Optimizer.hpp"
#include "mtac/Program.hpp"
#include "mtac/ControlFlowGraph.hpp"
#include "mtac/Quadruple.hpp"

//The custom optimizations
#include "mtac/conditional_propagation.hpp"
#include "mtac/remove_aliases.hpp"
#include "mtac/clean_variables.hpp"
#include "mtac/remove_unused_functions.hpp"
#include "mtac/remove_empty_functions.hpp"
#include "mtac/DeadCodeElimination.hpp"
#include "mtac/merge_basic_blocks.hpp"
#include "mtac/remove_dead_basic_blocks.hpp"
#include "mtac/BranchOptimizations.hpp"
#include "mtac/inlining.hpp"
#include "mtac/loop_analysis.hpp"
#include "mtac/induction_variable_optimizations.hpp"
#include "mtac/loop_unrolling.hpp"
#include "mtac/loop_unswitching.hpp"
#include "mtac/complete_loop_peeling.hpp"
#include "mtac/remove_empty_loops.hpp"
#include "mtac/loop_invariant_code_motion.hpp"
#include "mtac/parameter_propagation.hpp"
#include "mtac/pure_analysis.hpp"
#include "mtac/local_cse.hpp"

//The optimization visitors
#include "mtac/ArithmeticIdentities.hpp"
#include "mtac/ReduceInStrength.hpp"
#include "mtac/ConstantFolding.hpp"
#include "mtac/MathPropagation.hpp"
#include "mtac/PointerPropagation.hpp"

//The data-flow problems
#include "mtac/GlobalOptimizations.hpp"
#include "mtac/global_cp.hpp"
#include "mtac/global_offset_cp.hpp"
#include "mtac/global_cse.hpp"

#include "ltac/Register.hpp"
#include "ltac/FloatRegister.hpp"
#include "ltac/PseudoRegister.hpp"
#include "ltac/PseudoFloatRegister.hpp"
#include "ltac/Address.hpp"

using namespace eddic;

namespace eddic {
namespace mtac {

typedef boost::mpl::vector<
        mtac::ConstantFolding*
    > basic_passes;

struct all_basic_optimizations {};

template<>
struct pass_traits<all_basic_optimizations> {
    STATIC_CONSTANT(pass_type, type, pass_type::IPA_SUB);
    STATIC_STRING(name, "all_basic_optimizations");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);

    typedef basic_passes sub_passes;
};

typedef boost::mpl::vector<
        mtac::ArithmeticIdentities*,
        mtac::ReduceInStrength*,
        mtac::ConstantFolding*,
        mtac::conditional_propagation*,
        mtac::ConstantPropagationProblem*,
        mtac::OffsetConstantPropagationProblem*,
        mtac::local_cse*,
        mtac::global_cse*,
        mtac::PointerPropagation*,
        mtac::MathPropagation*,
        mtac::optimize_branches*,
        mtac::remove_dead_basic_blocks*,
        mtac::merge_basic_blocks*,
        mtac::dead_code_elimination*,
        mtac::remove_aliases*,
        mtac::loop_analysis*,
        mtac::loop_invariant_code_motion*,
        mtac::loop_induction_variables_optimization*,
        mtac::remove_empty_loops*,
        mtac::loop_unrolling*,
        mtac::loop_unswitching*,
        mtac::complete_loop_peeling*, //Must be kept last since it can mess up the loop analysis
        mtac::clean_variables*
    > passes;

struct all_optimizations {};

template<>
struct pass_traits<all_optimizations> {
    STATIC_CONSTANT(pass_type, type, pass_type::IPA_SUB);
    STATIC_STRING(name, "all_optimizations");
    STATIC_CONSTANT(unsigned int, property_flags, 0);
    STATIC_CONSTANT(unsigned int, todo_after_flags, 0);

    typedef passes sub_passes;
};

}
}

namespace {

template<typename T, typename Sign>
struct has_gate {
    typedef char yes[1];
    typedef char no [2];
    template <typename U, U> struct type_check;
    template <typename _1> static yes &chk(type_check<Sign, &_1::gate> *);
    template <typename   > static no  &chk(...);
    static bool const value = sizeof(chk<T>(0)) == sizeof(yes);
};

typedef boost::mpl::vector<
        mtac::remove_unused_functions*,
        mtac::all_basic_optimizations*
    > ipa_basic_passes;

typedef boost::mpl::vector<
        mtac::remove_unused_functions*,
        mtac::pure_analysis*,
        mtac::all_optimizations*,
        mtac::remove_empty_functions*,
        mtac::inline_functions*,
        mtac::remove_unused_functions*,
        mtac::parameter_propagation*
    > ipa_passes;

template <typename Pass>
inline constexpr bool need_pool = mtac::pass_traits<Pass>::property_flags & mtac::PROPERTY_POOL;

template <typename Pass>
inline constexpr bool need_platform = mtac::pass_traits<Pass>::property_flags & mtac::PROPERTY_PLATFORM;

template <typename Pass>
inline constexpr bool need_configuration = mtac::pass_traits<Pass>::property_flags & mtac::PROPERTY_CONFIGURATION;

template <typename Pass>
inline constexpr bool need_program = mtac::pass_traits<Pass>::property_flags & mtac::PROPERTY_PROGRAM;

struct pass_runner {
    bool optimized = false;

    mtac::Program& program;
    mtac::Function* function;

    std::shared_ptr<StringPool> pool;
    std::shared_ptr<Configuration> configuration;
    Platform platform;
    timing_system& system;

    pass_runner(mtac::Program& program, std::shared_ptr<StringPool> pool, std::shared_ptr<Configuration> configuration, Platform platform, timing_system& system) :
            program(program), pool(pool), configuration(configuration), platform(platform), system(system) {};

    template<typename Pass>
    void apply_todo(){
        //No todo are implemented
    }

    template<typename Pass>
    void set_pool(Pass& pass){
        if constexpr (need_pool<Pass>) {
            pass.set_pool(pool);
        }
    }

    template<typename Pass>
    void set_platform(Pass& pass){
        if constexpr (need_platform<Pass>) {
            pass.set_platform(platform);
        }
    }

    template<typename Pass>
    void set_configuration(Pass& pass){
        if constexpr (need_configuration<Pass>) {
            pass.set_configuration(configuration);
        }
    }

    template<typename Pass>
    Pass construct(){
        if constexpr (need_program<Pass>) {
            return Pass(program);
        } else {
            return Pass();
        }
    }

    template<typename Pass>
    Pass make_pass(){
        auto pass = construct<Pass>();

        set_pool(pass);
        set_platform(pass);
        set_configuration(pass);

        return pass;
    }

    template <typename Pass>
    bool has_to_be_run(Pass & pass) {
        if constexpr (has_gate<Pass, bool (Pass::*)(std::shared_ptr<Configuration>)>::value) {
            return pass.gate(configuration);
        } else {
            return true;
        }
    }

    template <typename Pass>
    bool apply(Pass & pass) {
        constexpr auto type = mtac::pass_traits<Pass>::type;

        if constexpr (type == mtac::pass_type::IPA) {
            return pass(program);
        } else if constexpr (type == mtac::pass_type::IPA_SUB) {
            for (auto & function : program.functions) {
                this->function = &function;

                if (log::enabled<Debug>()) {
                    LOG<Debug>("Optimizer") << "Start optimizations on " << function.get_name() << log::endl;

                    std::cout << function << std::endl;
                }

                boost::mpl::for_each<typename mtac::pass_traits<Pass>::sub_passes>(boost::ref(*this));
            }

            return false;
        } else if constexpr (type == mtac::pass_type::CUSTOM) {
            return pass(*function);
        } else if constexpr (type == mtac::pass_type::LOCAL) {
            for (auto & block : *function) {
                for (auto & quadruple : block->statements) {
                    pass(quadruple);
                }
            }

            return pass.optimized;
        } else if constexpr (type == mtac::pass_type::DATA_FLOW) {
            auto results = mtac::data_flow(*function, pass);

            // Once the data-flow problem is fixed, statements can be optimized
            return pass.optimize(*function, results);
        } else if constexpr (type == mtac::pass_type::BB) {
            bool optimized = false;

            for (auto & block : *function) {
                pass.clear();

                for (auto & quadruple : block->statements) {
                    pass(quadruple);
                }

                optimized |= pass.optimized;
            }

            return optimized;
        } else if constexpr (type == mtac::pass_type::BB_TWO_PASS) {
            bool optimized = false;

            for (auto & block : *function) {
                pass.clear();

                pass.pass = mtac::Pass::DATA_MINING;
                for (auto & quadruple : block->statements) {
                    pass(quadruple);
                }

                pass.pass = mtac::Pass::OPTIMIZE;
                for (auto & quadruple : block->statements) {
                    pass(quadruple);
                }

                optimized |= pass.optimized;
            }

            return optimized;
        } else {
            cpp_unreachable("Invalid pass type");
        }
    }

    template <typename Pass>
    void debug_local(bool local) {
        if constexpr (mtac::pass_traits<Pass>::type == mtac::pass_type::IPA || mtac::pass_traits<Pass>::type == mtac::pass_type::IPA_SUB) {
            LOG<Debug>("Optimizer") << mtac::pass_traits<Pass>::name() << " returned " << local << log::endl;
        } else {
            if (log::enabled<Debug>()) {
                if (local) {
                    LOG<Debug>("Optimizer") << mtac::pass_traits<Pass>::name() << " returned true" << log::endl;

                    // Print the function
                    std::cout << *function << std::endl;
                } else {
                    LOG<Debug>("Optimizer") << mtac::pass_traits<Pass>::name() << " returned false" << log::endl;
                }
            }
        }
    }

    template<typename Pass>
    inline void operator()(Pass*){
        auto pass = make_pass<Pass>();

        if(has_to_be_run(pass)){
            timing_timer timer(system, mtac::pass_traits<Pass>::name());

            bool local = apply<Pass>(pass);
            if(local){
                program.context.stats().inc_counter(std::string(mtac::pass_traits<Pass>::name()) + "_true");
                apply_todo<Pass>();
            }

            debug_local<Pass>(local);

            optimized |= local;
        }
    }
};

} //end of anonymous namespace

void mtac::Optimizer::optimize(mtac::Program& program, std::shared_ptr<StringPool> string_pool, Platform platform, std::shared_ptr<Configuration> configuration) const {
    timing_timer timer(program.context.timing(), "whole_optimizations");

    //Build the CFG of each functions (also needed for register allocation)
    for(auto& function : program.functions){
        timing_timer timer(program.context.timing(), "build_cfg");
        mtac::build_control_flow_graph(function);
    }

    if(configuration->option_defined("fglobal-optimization")){
        //Apply Interprocedural Optimizations
        pass_runner runner(program, string_pool, configuration, platform, program.context.timing());
        do{
            runner.optimized = false;
            boost::mpl::for_each<ipa_passes>(boost::ref(runner));
        } while(runner.optimized);
    } else {
        //Even if global optimizations are disabled, perform basic optimization (only constant folding)
        pass_runner runner(program, string_pool, configuration, platform, program.context.timing());
        boost::mpl::for_each<ipa_basic_passes>(boost::ref(runner));
    }
}
