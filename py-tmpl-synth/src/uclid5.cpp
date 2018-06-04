#include <uclid5.hpp>
#include <abstraction.hpp>

namespace ila
{
    void LiftingZ3Adapter::addConstant(const std::string& name, const Node* init)
    {
        z3::expr e = getExpr(init);
        constants.erase(name);
        constants.insert(map_pair_t(name, e));
    }

    void LiftingZ3Adapter::addConstant(const std::string& name, const z3::expr& val)
    {
        constants.erase(name);
        constants.insert(map_pair_t(name, val));
    }

    void LiftingZ3Adapter::_dumpConstants()
    {
        for(auto pos = constants.begin(); pos != constants.end(); pos++) {
            std::cout << pos->first << "-> " << (pos->second) << std::endl;
        }
    }

    z3::expr LiftingZ3Adapter::getBoolVarExpr(const BoolVar* b)
    {
        expr_map_t::iterator pos = constants.find(b->getName());
        if (pos != constants.end()) {
            return pos->second;
        } else {
            return c.bool_const(b->getName().c_str());
        }
    }

    z3::expr LiftingZ3Adapter::getBitvectorVarExpr(const BitvectorVar* bv)
    {
        expr_map_t::iterator pos = constants.find(bv->getName());
        if (pos != constants.end()) {
            return pos->second;
        } else {
            return c.bv_const(bv->getName().c_str(), bv->type.bitWidth);
        }
    }

    z3::expr LiftingZ3Adapter::getMemVarExpr(const MemVar* memv)
    {
        expr_map_t::iterator pos = constants.find(memv->getName());
        if (pos != constants.end()) {
            return pos->second;
        } else {
            auto addrsort = c.bv_sort(memv->type.addrWidth);
            auto datasort = c.bv_sort(memv->type.dataWidth);
            auto memsort = c.array_sort(addrsort, datasort);
            return c.constant(memv->getName().c_str(), memsort);
        }
    }

    z3::expr LiftingZ3Adapter::getVar(const std::string& name, const NodeType& type)
    {
        if (type.isBool()) {
            return c.bool_const(name.c_str());
        } else if (type.isBitvector()) {
            return c.bv_const(name.c_str(), type.bitWidth);
        } else if (type.isMem()) {
            auto addrsort = c.bv_sort(type.addrWidth);
            auto datasort = c.bv_sort(type.dataWidth);
            auto memsort  = c.array_sort(addrsort, datasort);
            return c.constant(name.c_str(), memsort);
        } else {
            ILA_ASSERT(false, "Unexpected type.");
        }
        return c.bool_val(false);
    }

    Uclid5Translator::Uclid5Translator(Abstraction* a)
      : abs(a)
      , toZ3(c_)
    {
        nodeset_t fetchVars;
        abs->fetchExpr->getSupportVars(fetchVars);
        for (auto n : fetchVars) {
            const npair_t* pair = abs->getMapEntry(n->getName());
            ILA_ASSERT(pair != NULL, "Variable not in map.");
            if (!abs->isInput(n->getName())) {
                if (isConstant(pair)) {
                    std::cout << "cnst : " << n->getName() << std::endl;
                    toZ3.addConstant(n->getName(), pair->init.get());
                } else {
                    std::cout << "var  : " << n->getName() << std::endl;
                    searchName.push_back(n->getName());
                    searchVars.push_back(pair->next.get());
                    searchInit.push_back(pair->init.get());
                }
            }
        }
    }


    Uclid5Translator::~Uclid5Translator()
    {
    }

    bool Uclid5Translator::isConstant(const npair_t* obj)
    {
        return obj->next->equal(obj->var.get());
    }

    void Uclid5Translator::search(
            const std::string& name, 
            const Node* init, const Node* next)
    {
        z3::solver S(c_);

        stack_t states;
        z3::expr initExpr = toZ3.getExpr(init);
        uint64_t initVal = -1;
        ILA_ASSERT(initExpr.is_numeral_u64(initVal), "Initial value must be a numeral representable as u64.");
        z3::expr var = toZ3.getVar(name + ":value", init->type);

        std::cout << "next: " << *next << std::endl;

        states.push(initExpr);
        std::set<uint64_t> visited;
        visited.insert(initVal);

        while(states.size() > 0) {
            z3::expr stackExpr = states.top();
            states.pop();
            std::cout << "[pop]: " << stackExpr << std::endl;

            // Clear the memo.
            toZ3.clear();
            // Set PC = value from stack.
            toZ3.addConstant(name, stackExpr);

            // Get the next expression.
            z3::expr nxtExpr = toZ3.getExpr(next);
            std::cout << "nxtExpr: " << nxtExpr << std::endl;

            // Create the comparison.
            z3::expr cmp = (var == toZ3.getExpr(next));
            S.push();
            S.add(var == toZ3.getExpr(next));
           
            // ALL-SAT
            while (S.check() == z3::sat) {
                z3::model m = S.get_model();
                z3::expr e = m.eval(var);
                uint64_t n = -1;
                ILA_ASSERT(e.is_numeral_u64(n), "Must be a numeral representable as u64.");
                if (visited.find(n) == visited.end()) {
                    std::cout << "[nxt]: " << std::hex << n << std::dec << std::endl;
                    visited.insert(n);
                    states.push(e);
                }
                S.add(var != e);
            }
            S.pop();
            // std::cout << "result: " << (bool)(r == z3::sat) << std::endl;
        }
    }

    void Uclid5Translator::translate()
    {
        if (searchVars.size() != 1) {
            throw PyILAException(
                    PyExc_RuntimeError, 
                    "Must have exactly one state variable to drive the DFS.");
        } else {
            search(searchName[0], searchInit[0], searchVars[0]);
        }
    }
}
