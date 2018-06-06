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

    Uclid5Translator::Uclid5Translator(const std::string& name_, boost::shared_ptr<Abstraction>& a)
      : name(name_)
      , abs(a)
      , toZ3(c_)
    {
    }

    Uclid5Translator::Uclid5Translator(const Uclid5Translator& ut)
      : name(ut.name)
      , abs(ut.abs)
      , toZ3(c_)
    {
    }

    Uclid5Translator::~Uclid5Translator()
    {
    }
}
