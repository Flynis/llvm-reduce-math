#ifndef LLVM_TRANSFORMS_UTILS_REDUCEMATH_H
#define LLVM_TRANSFORMS_UTILS_REDUCEMATH_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ReduceMathPass : public PassInfoMixin<ReduceMathPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_REDUCEMATH_H