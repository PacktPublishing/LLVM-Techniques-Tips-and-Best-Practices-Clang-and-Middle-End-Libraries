#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class HaltAnalyzer : public PassInfoMixin<HaltAnalyzer> {
  static constexpr const char* HaltFuncName = "my_halt";

  SmallVector<Instruction*, 2> Calls;
  void findHaltCalls(Function &F);

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end anonymous namespace

PreservedAnalyses
HaltAnalyzer::run(Function &F, FunctionAnalysisManager &FAM) {
  auto PA = PreservedAnalyses::all();

  findHaltCalls(F);
  if (Calls.size() == 0) return PA;

  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  SmallVector<BasicBlock*, 4> DomBBs;
  for (auto *I : Calls) {
    auto *BB = I->getParent();
    DomBBs.clear();
    DT.getDescendants(BB, DomBBs);

    for (auto *DomBB : DomBBs) {
      // exclude self
      if (DomBB != BB) {
        DomBB->printAsOperand(errs() << "Unreachable: ");
        errs() << "\n";
      }
    }
  }

  return PA;
}

void HaltAnalyzer::findHaltCalls(Function &F) {
  Calls.clear();
  for (auto &I : instructions(F)) {
    if (auto *CI = dyn_cast<CallInst>(&I)) {
      if (CI->getCalledFunction()->getName() == HaltFuncName)
        Calls.push_back(&I);
    }
  }
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "HaltAnalyzer", "v0.1",
    [](PassBuilder &PB) {
      using OptimizationLevel= typename PassBuilder::OptimizationLevel;
      PB.registerOptimizerLastEPCallback(
        [](ModulePassManager &MPM, OptimizationLevel OL) {
          MPM.addPass(createModuleToFunctionPassAdaptor(HaltAnalyzer()));
        });
    }
  };
}
