/// \file
/// Header for wrapping Expr.

#ifndef __WRAP_EXPR_H__
#define __WRAP_EXPR_H__

#include "ila/expr_fuse.h"

namespace ila {
namespace pyapi {

/// \brief The Expr wrapper for Boost.Python API.
class ExprWrap {
  // ------------------------- MEMBERS -------------------------------------- //
  /// Wrapped data.
  ExprPtr ptr_;

public:
  // ------------------------- CONSTRUCTOR/DESTRUCTOR ----------------------- //
  /// Default constructor for Boost.Python wrapper.
  ExprWrap() : ptr_(NULL) {}
  /// Constructor for copying from C++ API.
  ExprWrap(const ExprPtr e) : ptr_(e) {}

  // ------------------------- ACCESSORS/MUTATORS --------------------------- //
  const ExprPtr get() const;

  // ------------------------- METHODS -------------------------------------- //
  std::string Name() const;

  ExprWrap* Complement() const;

  ExprWrap* Negate() const;

  ExprWrap* And(ExprWrap* rhs) const;

  // ------------------------- STATIC METHODS ------------------------------- //
  static ExprWrap* Load(ExprWrap* mem, ExprWrap* addr);

}; // class ExprWrap

} // namespace pyapi
} // namespace ila

#endif // __WRAP_EXPR_H__

