#ifndef TERNARY_CONVERTER_H
#define TERNARY_CONVERTER_H
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendAction.h"
#include <string>
#include <vector>

namespace clang {
struct TernaryConverterAction : public PluginASTAction {
  std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override;

  bool ParseArgs(const CompilerInstance&,
                 const std::vector<std::string>&) override;

  ActionType getActionType() override {
    return Cmdline;
  }

private:
  /// By default this plugin will detect whether return statement
  /// and simple assignment in an if statement can be converted into
  /// ternary operator. The following two flags are used for disabling
  /// either of the feature.
  bool NoAssignment = false;
  bool NoReturn = false;

  std::unique_ptr<ast_matchers::MatchFinder> ASTFinder;
  std::unique_ptr<ast_matchers::MatchFinder::MatchCallback> ReturnMatchCB,
                                                            AssignMatchCB;
};
} // end namespace clang
#endif
