#ifndef STRICT_OPT_H
#define STRICT_OPT_H
#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;

struct StrictOpt : public PassInfoMixin<StrictOpt> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end namspace llvm
#endif
