#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include <string>
#include <memory>

using namespace llvm;

static cl::opt<std::string>
  InputFile(cl::Positional, cl::desc("<input IR file (.ll/.bc)>"),
            cl::Required);

enum class Mode {
  NoOpt,
  PrintFunction,
  TransformModule
};
static cl::opt<Mode>
  OpMode("mode", cl::desc("Output mode"),
         cl::values(
           clEnumValN(Mode::NoOpt,           "no-opt",
                      "Don't modify the module"),
           clEnumValN(Mode::PrintFunction,   "print-func",
                      "Only print the modified function" ),
           clEnumValN(Mode::TransformModule, "transform",
                      "Modify the module and generate an output file" )),
         cl::init(Mode::NoOpt));

static cl::opt<std::string>
  OutputFilePath("o", cl::desc("Output file (only applicable in `-mode=transform`)"),
                 cl::init("-"));

static cl::opt<bool>
  DebugAlias("debug-alias",
             cl::desc("Print debug messages regarding alias info"),
             cl::init(false));

static bool analyzeFunction(Function &F, AAResults &AAR) {
  if (llvm::none_of(F.args(), [](const Argument &A) {
                      return A.getType()->isPointerTy();
                    }))
    return false;

  SmallVector<Value*, 2> PtrArgs;
  for (User *U : F.users()) {
    if (auto *CS = dyn_cast<CallInst>(U)) {
      PtrArgs.clear();
      for (Use &Arg : CS->args()) {
        if (Arg->getType()->isPointerTy())
          PtrArgs.push_back(Arg.get());
      }

      // Check alias status with all other arguments
      int i, j;
      for (i = 0; i < PtrArgs.size(); ++i)
        for (j = 0; j < PtrArgs.size(); ++j) {
          if (i == j) continue;
          auto *Arg = PtrArgs[i],
               *OtherArg = PtrArgs[j];
          if (AAR.alias(Arg, OtherArg) != NoAlias) {
            if (DebugAlias) {
              CS->print(errs() << "On");
              errs() << ":\n";
              Arg->printAsOperand(errs() << "\tArgument '");
              OtherArg->printAsOperand(errs() << "' might be aliased with '");
              errs() << "'\n";
            }
            return false;
          }
        }
    }
  }
  return true;
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  cl::ParseCommandLineOptions(argc, argv);

  LLVMContext Ctx;

  // Read the input file
  SMDiagnostic SMD;
  std::unique_ptr<Module> M = parseIRFile(InputFile, SMD, Ctx);
  if (!M) {
    SMD.print(argv[0], errs());
    return 1;
  }

  // Initialize analysis we need...
  PassBuilder PB;
  FunctionAnalysisManager FAM;
  ModuleAnalysisManager MAM;
  CGSCCAnalysisManager CGAM;
  LoopAnalysisManager LAM;
  FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });
  PB.registerFunctionAnalyses(FAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Analyze all functions
  SmallVector<Function*, 2> Candidates;
  for (auto &F : *M) {
    if (analyzeFunction(F, FAM.getResult<AAManager>(F)))
      Candidates.push_back(&F);
  }

  if (OpMode == Mode::NoOpt) return 0;

  // Transform the function
  for (auto *F : Candidates) {
    for (auto &Arg : F->args()) {
      if (Arg.getType()->isPointerTy())
        Arg.addAttr(Attribute::NoAlias);
    }
    if (OpMode == Mode::PrintFunction) {
      F->print(errs());
      errs() << "====\n";
    }
  }

  if (OpMode == Mode::PrintFunction) return 0;

  std::error_code EC;
  ToolOutputFile OutputFile(OutputFilePath, EC, sys::fs::OF_Text);
  if (EC) {
    errs() << "Failed to open the output file " << OutputFilePath;
    errs() << ": " << EC.message() << "\n";
    return 1;
  }

  M->print(OutputFile.os(), nullptr);
  OutputFile.keep();

  return 0;
}
