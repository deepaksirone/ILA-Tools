#ifndef __UCLID5_HPP_DEFINED__
#define __UCLID5_HPP_DEFINED__

#include <cassert>

#include <unordered_map>
#include <vector>
#include <memory>

#include "boost/foreach.hpp"
#include "boost/dynamic_bitset.hpp"
#include "boost/logic/tribool.hpp"
#include <z3++.h>
#include <util.hpp>
#include <stack>
#include <smt.hpp>

#include <ast.hpp>

namespace ila
{
    class Abstraction;

    struct LiftingZ3Adapter : public Z3ExprAdapter {
        LiftingZ3Adapter(z3::context& c_) : Z3ExprAdapter(c_, "") {}

        // List of constants.
        typedef std::map<std::string, z3::expr> expr_map_t;
        typedef std::pair<std::string, z3::expr> map_pair_t;
        expr_map_t constants;
        
        void addConstant(const std::string& name, const Node* init);
        // Convert a boolean variable into a Z3 expression.
        virtual z3::expr getBoolVarExpr(const BoolVar* bv);
        // Convert a bitvector variable into a Z3 expression.
        virtual z3::expr getBitvectorVarExpr(const BitvectorVar* bvv);
        // Convert a mem var into Z3.
        virtual z3::expr getMemVarExpr(const MemVar* mv);
        // convert a variable into an expression.
        z3::expr getVar(const std::string& name, const Node* node);
    };

    class Uclid5Translator
    {
    public:
        typedef std::vector<z3::expr> exprvec_t;
        typedef std::stack<z3::expr> stack_t;
    private:
        // pointer to the abstraction we are translating.
        Abstraction* abs;  
        std::vector<std::string> searchName;
        nodevec_t searchVars;
        nodevec_t searchInit;
        exprvec_t constEx;

        // SMT.
        z3::context c_;
        LiftingZ3Adapter toZ3;

        // is constant.
        bool isConstant(const npair_t* obj);

        // depth-first search.
        void search(const std::string& name, const Node* init, const Node* next);

    public:
        // constructor.
        Uclid5Translator(Abstraction* a);
        // destructor.
        virtual ~Uclid5Translator();
        // convert to boogie.
        void translate();
    };
}


#endif /* __BOOGIE_HPP_DEFINED__ */

