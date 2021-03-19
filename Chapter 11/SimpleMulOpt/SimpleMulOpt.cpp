#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

#define DEBUG_TYPE "simple-mul-opt"

using namespace llvm;

STATISTIC(NumMul, "Number of multiplications processed");

namespace {
struct SimpleMulOpt : public PassInfoMixin<SimpleMulOpt> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end anonymous namespace

PreservedAnalyses
SimpleMulOpt::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &ORE = FAM.getResult<OptimizationRemarkEmitterAnalysis>(F);

  struct Replacement {
    Instruction *I;
    Value *Base;
    APInt ShiftAmt;
  };
  SmallVector<Replacement, 2> Worklist;

  for (auto &I : instructions(F)) {
    if (auto *BinOp = dyn_cast<BinaryOperator>(&I))
      if (BinOp->getOpcode() == Instruction::Mul) {
        NumMul++;
        auto *LHS = BinOp->getOperand(0),
             *RHS = BinOp->getOperand(1);
        LLVM_DEBUG(dbgs() << "Found a multiplication instruction ");
        LLVM_DEBUG(LHS->printAsOperand(dbgs() << " with LHS: "));
        LLVM_DEBUG(RHS->printAsOperand(dbgs() << " and RHS: "));
        LLVM_DEBUG(dbgs() << "\n");

        // Neither of them is constant
        if (!isa<ConstantInt>(LHS) && !isa<ConstantInt>(RHS)) {
          ORE.emit([&]() {
            std::string InstStr;
            raw_string_ostream SS(InstStr);
            I.print(SS);
            return OptimizationRemarkMissed(DEBUG_TYPE, "NoConstOperand", &I)
                   << "Instruction"
                   << ore::NV("Inst", SS.str())
                   << " does not contain any constant operand";
          });
          continue;
        }
        auto *ConstV = isa<ConstantInt>(LHS)? LHS : RHS;
        auto *Base = ConstV == LHS? RHS : LHS;
        const APInt &Const = cast<ConstantInt>(ConstV)->getValue();
        // Not power of two
        if (!Const.isPowerOf2()) {
          ORE.emit([&]() {
            return OptimizationRemarkMissed(DEBUG_TYPE, "ConstNotPowerOf2", &I)
                   << "Constant operand "
                   << Const.toString(10, false)
                   << " is not power of two";
          });
          continue;
        }
        APInt ShiftAmt(Const.getBitWidth(), Const.logBase2());
        Worklist.push_back({&I, Base, ShiftAmt});
      }
  }

  // Replacing multiplication instructions
  for (auto &R : Worklist) {
    // Create new left-shifting instruction
    IRBuilder<> Builder(R.I);
    auto *Shl = Builder.CreateShl(R.Base, R.ShiftAmt);

    ORE.emit([&]() {
      std::string OrigInstStr, NewInstStr;
      raw_string_ostream OrigSS(OrigInstStr),
                         NewSS(NewInstStr);
      R.I->print(OrigSS);
      Shl->print(NewSS);
      return OptimizationRemark(DEBUG_TYPE, "Replacement", R.I)
             << "Replacing" << ore::NV("Original", OrigSS.str())
             << " with" << ore::NV("New", NewSS.str());
    });

    R.I->replaceAllUsesWith(Shl);
    R.I->eraseFromParent();
  }

  return Worklist.empty()? PreservedAnalyses::all() : PreservedAnalyses::none();
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "SimpleMulOpt", "v0.1",
    [](PassBuilder &PB) {
      using OptimizationLevel= typename PassBuilder::OptimizationLevel;
      using PipelineElement = typename PassBuilder::PipelineElement;
      PB.registerPipelineStartEPCallback(
        [](ModulePassManager &MPM, OptimizationLevel OL) {
          if (OL.getSpeedupLevel() > 2)
            MPM.addPass(createModuleToFunctionPassAdaptor(SimpleMulOpt()));
        });
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PipelineElement> Elements) {
          if (Name == "simple-mul-opt") {
            FPM.addPass(SimpleMulOpt());
            return true;
          }
          return false;
        });
    }
  };
}
