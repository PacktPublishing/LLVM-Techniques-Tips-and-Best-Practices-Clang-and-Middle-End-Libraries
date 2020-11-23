#include "llvm/Support/CommandLine.h"
using namespace llvm;

static cl::opt<bool> Foo("foo", cl::init(false));

int main(int argc, char** argv) {
  cl::ParseCommandLineOptions(argc, argv);
  return 0;
}
