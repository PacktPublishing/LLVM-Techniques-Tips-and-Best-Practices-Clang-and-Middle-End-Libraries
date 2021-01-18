#include "TernaryConverter.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace ast_matchers;

bool TernaryConverterAction::ParseArgs(const CompilerInstance &CI,
                                       const std::vector<std::string> &Args) {
  for (const auto &Arg : Args) {
    if (Arg == "-no-detect-assignment") NoAssignment = true;
    if (Arg == "-no-detect-return") NoReturn = true;
  }
  return true;
}

static StatementMatcher buildReturnMatcher(StringRef Suffix) {
  auto Tag = ("return" + Suffix).str();
  return compoundStmt(statementCountIs(1),
                      hasAnySubstatement(
                        returnStmt(
                          hasReturnValue(expr().bind(Tag)))));
}

static StatementMatcher buildAssignmentMatcher(StringRef Suffix) {
  auto TagDest = ("dest" + Suffix).str();
  auto TagVal = ("val" + Suffix).str();
  return compoundStmt(statementCountIs(1),
                      hasAnySubstatement(
                        binaryOperator(hasOperatorName("="),
                                       hasLHS(declRefExpr().bind(TagDest)),
                                       hasRHS(expr().bind(TagVal))
                                       )
                      ));
}

static StatementMatcher buildIfStmtMatcher(StatementMatcher trueBodyMatcher,
                                           StatementMatcher falseBodyMatcher) {
  return ifStmt(hasThen(trueBodyMatcher),
                hasElse(falseBodyMatcher)).bind("if_stmt");
}

namespace {
struct MatchCallbackBase : public MatchFinder::MatchCallback {
  virtual void run(const MatchFinder::MatchResult &Result) = 0;

protected:
  unsigned diag_warn_potential_ternary,
           diag_note_true_expr,
           diag_note_false_expr;

  MatchCallbackBase(unsigned DiagMain,
                    unsigned DiagTrueBr, unsigned DiagFalseBr)
    : diag_warn_potential_ternary(DiagMain),
      diag_note_true_expr(DiagTrueBr),
      diag_note_false_expr(DiagFalseBr) {}
};

struct MatchReturnCallback : public MatchCallbackBase {
  MatchReturnCallback(unsigned DiagMain,
                      unsigned DiagTrueBr, unsigned DiagFalseBr)
    : MatchCallbackBase(DiagMain, DiagTrueBr, DiagFalseBr) {}

  void run(const MatchFinder::MatchResult &Result) override {
    const auto& Nodes = Result.Nodes;
    auto& Diag = Result.Context->getDiagnostics();

    const auto* If = Nodes.getNodeAs<IfStmt>("if_stmt");
    if (If)
      Diag.Report(If->getBeginLoc(), diag_warn_potential_ternary);
    else
      Diag.Report(diag_warn_potential_ternary);

    const auto* TrueRetExpr = Nodes.getNodeAs<Expr>("return.true");
    const auto* FalseRetExpr = Nodes.getNodeAs<Expr>("return.false");
    if (TrueRetExpr && FalseRetExpr) {
      Diag.Report(TrueRetExpr->getBeginLoc(), diag_note_true_expr);
      Diag.Report(FalseRetExpr->getBeginLoc(), diag_note_false_expr);
    }
  }
};

struct MatchAssignmentCallback : public MatchCallbackBase {
  MatchAssignmentCallback(unsigned DiagMain,
                          unsigned DiagTrueBr, unsigned DiagFalseBr)
    : MatchCallbackBase(DiagMain, DiagTrueBr, DiagFalseBr) {}

  void run(const MatchFinder::MatchResult &Result) override {
    const auto& Nodes = Result.Nodes;
    auto& Diag = Result.Context->getDiagnostics();

    // Check if destination of both assignments are the same
    const auto *DestTrue = Nodes.getNodeAs<DeclRefExpr>("dest.true"),
               *DestFalse = Nodes.getNodeAs<DeclRefExpr>("dest.false");
    if (DestTrue && DestFalse) {
      if (DestTrue->getDecl() == DestFalse->getDecl()) {
        // Can be converted to ternary!
        const auto* If = Nodes.getNodeAs<IfStmt>("if_stmt");
        if (If)
          Diag.Report(If->getBeginLoc(), diag_warn_potential_ternary);
        else
          Diag.Report(diag_warn_potential_ternary);

        const auto* TrueValExpr = Nodes.getNodeAs<Expr>("val.true");
        const auto* FalseValExpr = Nodes.getNodeAs<Expr>("val.false");
        if (TrueValExpr && FalseValExpr) {
          Diag.Report(TrueValExpr->getBeginLoc(), diag_note_true_expr);
          Diag.Report(FalseValExpr->getBeginLoc(), diag_note_false_expr);
        }
      }
    }
  }
};
} // end anonymous namespace

std::unique_ptr<ASTConsumer>
TernaryConverterAction::CreateASTConsumer(CompilerInstance &CI,
                                          StringRef InFile) {
  // Create custom diagnostic message
  assert(CI.hasASTContext() && "No ASTContext??");
  auto& Diag = CI.getASTContext().getDiagnostics();
  auto DiagWarnMain = Diag.getCustomDiagID(
    DiagnosticsEngine::Warning,
    "this if statement can be converted to ternary operator:");
  auto DiagNoteTrueExpr = Diag.getCustomDiagID(
    DiagnosticsEngine::Note,
    "with true expression being this:");
  auto DiagNoteFalseExpr = Diag.getCustomDiagID(
    DiagnosticsEngine::Note,
    "with false expression being this:");

  ASTFinder = std::make_unique<MatchFinder>();

  // Return matcher
  if (!NoReturn) {
    ReturnMatchCB = std::make_unique<MatchReturnCallback>(DiagWarnMain,
                                                          DiagNoteTrueExpr,
                                                          DiagNoteFalseExpr);
    ASTFinder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource,
                                   buildIfStmtMatcher(buildReturnMatcher(".true"),
                                                      buildReturnMatcher(".false"))),
                          ReturnMatchCB.get());
  }

  // Assignment matcher
  if (!NoAssignment) {
    AssignMatchCB = std::make_unique<MatchAssignmentCallback>(DiagWarnMain,
                                                              DiagNoteTrueExpr,
                                                              DiagNoteFalseExpr);
    ASTFinder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource,
                                   buildIfStmtMatcher(buildAssignmentMatcher(".true"),
                                                      buildAssignmentMatcher(".false"))),
                          AssignMatchCB.get());
  }

  return std::move(ASTFinder->newASTConsumer());
}

static FrontendPluginRegistry::Add<TernaryConverterAction>
  X("ternary-converter", "Prompt messages to hint branches that can be"
                         " converted to ternary operators");
