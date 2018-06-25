#ifndef __SIMPLIFY_HPP_DEFINED__
#define __SIMPLIFY_HPP_DEFINED__

#include <ast.hpp>
#include <smt.hpp>
#include <stack>
#include <smt.hpp>
#include <rewriter.hpp>
#include <boost/logic/tribool.hpp>

namespace ila
{
    class ITESimplifier
    {
        std::shared_ptr<z3::context> ctx;
        std::shared_ptr<z3::solver> S;
        std::shared_ptr<Z3ExprAdapter> adapter;

        rwmap_t rwmap;

        void _add(const nptr_t& a, bool negate=false);
        boost::logic::tribool _isConstant(const nptr_t& c);

        void getNewArgs(const Node* op, nptr_vec_t& args);
        nptr_t getRepl(const Node* n) const;
        nptr_t rewriteITE(const Node* n);
        void dfs(const Node* n);
    public:
        // constructor.
        ITESimplifier(const nptr_t& assump);
        // constructor with existing contexts, solvers and expr adapters.
        ITESimplifier(
                std::shared_ptr<z3::context> ctx_, 
                std::shared_ptr<z3::solver> S_,
                std::shared_ptr<Z3ExprAdapter> adapter_);

        // destructor.
        virtual ~ITESimplifier();
        // add a vector of assumption.
        void addAssumptions(const nptr_vec_t& assumps);
        // add exactly one assumption.
        void addAssumption(const nptr_t& a);
        // create clone
        void createClone(const Node* n);

        void reset() { rwmap.clear(); }
        nptr_t simplify(Node* n);

    };
}
#endif
