/// \file
/// Source for exporting Verilog models for ILAs

#include "translator/verilog_export.h"
#include "util/log.h"

namespace ila {

// TODO

void foo(const InstrLvlAbsPtr m, const size_t& i) {
  ILA_ASSERT(i < m->state_num()) << "Invalid state idx";

  auto name = m->state(i)->name();
}

}; // namespace ila

