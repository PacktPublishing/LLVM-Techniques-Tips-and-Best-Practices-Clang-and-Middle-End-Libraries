#ifndef MACRO_GUARD_VALIDATOR_H
#define MACRO_GUARD_VALIDATOR_H
#include "clang/Lex/PPCallbacks.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {
// Forward declarations
class Token;
class MacroDirective;
class IdentifierInfo;
class SourceManager;
} // end namespace clang

namespace macro_guard {
extern llvm::SmallVector<const clang::IdentifierInfo*, 2> ArgsToEnclosed;

class MacroGuardValidator : public clang::PPCallbacks {
  clang::SourceManager &SM;

public:
  explicit MacroGuardValidator(clang::SourceManager &SM) : SM(SM) {}

  void MacroDefined(const clang::Token &MacroNameToke,
                    const clang::MacroDirective *MD) override;
};
} // end namespace macro_guard
#endif
