#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Token.h"
#include "clang/Lex/Pragma.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/raw_ostream.h"

#include "MacroGuardValidator.h"

using namespace clang;

namespace macro_guard {
llvm::SmallVector<const IdentifierInfo*, 2> ArgsToEnclosed;
} // end namespace macro_guard

using namespace macro_guard;

namespace {
class MacroGuardPragma : public PragmaHandler {
  bool IsValidatorRegistered;

public:
  MacroGuardPragma() : PragmaHandler("macro_arg_guard"),
                       IsValidatorRegistered(false) {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) override;
};
} // end anonymous namespace

void MacroGuardPragma::HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                                    Token &PragmaTok) {
  // Reset the to-be-enclosed argument list
  ArgsToEnclosed.clear();

  Token Tok;
  PP.Lex(Tok);
  while (Tok.isNot(tok::eod)) {
    if (auto *II = Tok.getIdentifierInfo()) {
      ArgsToEnclosed.push_back(II);
    }
    PP.Lex(Tok);
  }

  if (!IsValidatorRegistered) {
    // Register the validator PPCallbacks
    auto Validator = std::make_unique<MacroGuardValidator>(PP.getSourceManager());
    PP.addPPCallbacks(std::move(Validator));
    IsValidatorRegistered = true;
  }
}

static PragmaHandlerRegistry::Add<MacroGuardPragma> X("macro_arg_guard", "");
