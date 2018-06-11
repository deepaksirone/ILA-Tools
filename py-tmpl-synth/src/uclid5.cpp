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
      , S(c_)
    {
    }

    Uclid5Translator::Uclid5Translator(const Uclid5Translator& ut)
      : name(ut.name)
      , abs(ut.abs)
      , toZ3(c_)
      , S(c_)
    {
    }

    Uclid5Translator::~Uclid5Translator()
    {
    }


    void Uclid5Translator::initVar(const std::string& name)
    {
        const npair_t* vpair = abs->getMapEntry(name);
        toZ3.addConstant(name, vpair->init.get());
    }

    py::list Uclid5Translator::getExprValues(const NodeRef* node)
    {
        static const int MAX_ITER = 64;
        int iter = 0;

        nptr_t nptr(node->node);
        z3::expr e = toZ3.getExpr(nptr.get());
        z3::expr c = toZ3.getCnst(nptr.get());
        z3::expr v = toZ3.getVar(nptr->getName(), nptr->type);

        S.push();
        S.add(c);
        S.add(e == v);

        py::list result;

        for (iter = 0; iter < MAX_ITER && S.check() == z3::sat; iter++) {
            z3::model m = S.get_model();
            z3::expr v_expr = m.eval(v);
            if (nptr->type.isBool()) {
                bool value;
                Z3_lbool vz = v_expr.bool_value();
                if (vz == Z3_L_FALSE) {
                    value = false;
                } else if (vz == Z3_L_TRUE) {
                    value = true;
                } else {
                    throw PyILAException(PyExc_RuntimeError, "Undefined boolean value.");
                }
                result.append(value);
            } else if(nptr->type.isBitvector()) {
                using namespace py;
                std::string v_str = v_expr.get_decimal_string(0);
                PyObject* l_e = PyInt_FromString((char*) v_str.c_str(), NULL, 0);
                object o_e(handle<>(borrowed(l_e)));
                result.append(o_e);
            } else {
                throw PyILAException(PyExc_RuntimeError, "Unsupported type.");
            }
            S.add(v != v_expr);
        }
        S.pop();
        return result;
    }
}
