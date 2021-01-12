#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include "MacroGuardValidator.h"

using namespace clang;
using namespace macro_guard;

void MacroGuardValidator::MacroDefined(const Token &MacroNameTok,
                                       const MacroDirective *MD) {
  const auto *MI = MD->getMacroInfo();
  if (MI->tokens_empty()) return;

  for (const auto *ArgII : ArgsToEnclosed) {
    // First, check if this macro really has this argument
    if (llvm::none_of(MI->params(), [ArgII](const IdentifierInfo *II) {
                                      return ArgII == II;
                                    })) {
      auto *MacroNameII = MacroNameTok.getIdentifierInfo();
      assert(MacroNameII);
      auto NameTokLoc = MacroNameTok.getLocation();
      llvm::errs() << "[WARNING] Can't find argument '" << ArgII->getName() << "' ";
      llvm::errs() << "at macro '" << MacroNameII->getName() << "'(";
      llvm::errs() << NameTokLoc.printToString(SM) << ")\n";
      continue;
    }

    for (auto TokIdx = 0U, TokSize = MI->getNumTokens();
         TokIdx < TokSize; ++TokIdx) {
      auto CurTok = *(MI->tokens_begin() + TokIdx);
      if (CurTok.getIdentifierInfo() == ArgII) {
        // Check if previous and successor Tokens are parenthesis
        if (TokIdx > 0 && TokIdx < TokSize - 1) {
          auto PrevTok = *(MI->tokens_begin() + TokIdx - 1),
               NextTok = *(MI->tokens_begin() + TokIdx + 1);
          if (PrevTok.is(tok::l_paren) && NextTok.is(tok::r_paren))
            continue;
        }
        // The argument is not enclosed
        auto TokLoc = CurTok.getLocation();
        llvm::errs() << "[WARNING] In " << TokLoc.printToString(SM) << ": ";
        llvm::errs() << "macro argument '" << ArgII->getName()
                     << "' is not enclosed by parenthesis\n";
      }
    }
  }

  // Also clear the storage after one check
  ArgsToEnclosed.clear();
}
