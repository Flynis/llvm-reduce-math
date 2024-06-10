#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/Utils/ReduceMath.h"

using namespace llvm;
using namespace llvm::PatternMatch;

using BuilderTy = IRBuilder<TargetFolder>;

// match variation of a^2 - 2ab + b^2
static bool matchesSquareDif(BinaryOperator &I, Value *&A, Value *&B) {
  constexpr unsigned MulOp = Instruction::Mul;
  constexpr unsigned AddOp = Instruction::Add;
  constexpr unsigned Mul2Op = Instruction::Shl;
  constexpr unsigned SubOp = Instruction::Sub;

  // (2*a - b) * b + a*a 
  return match(
      &I, m_BinOp(
              AddOp,
              m_BinOp(
                  MulOp, 
                  m_BinOp(
                      SubOp,
                      m_Value(B),
                      m_c_BinOp(Mul2Op, m_Value(A), m_SpecificInt(1))
                  ),
                  m_Deferred(B)
              ),
              m_BinOp(MulOp, m_Deferred(A), m_Deferred(A))
          )
  );
}

// Fold variation of a^2 - 2ab + b^2 -> (a - b)^2
static Instruction* foldSquareDif(BinaryOperator &I, BuilderTy &Builder) {
  Value *A, *B;
  if (matchesSquareDif(I, A, B)) {
    Value *AB = Builder.CreateSub(A, B);
    return BinaryOperator::CreateMul(AB, AB);
  }
  return nullptr;
}

// match variation of a^2 + b^2 + c^2 + 2ab + 2ac + 2bc
static bool matchesSquareSum3(BinaryOperator &I, Value *&A, Value *&B, Value *&C) {
  constexpr unsigned MulOp = Instruction::Mul;
  constexpr unsigned AddOp = Instruction::Add;
  constexpr unsigned Mul2Op = Instruction::Shl;

  // (a + b) * (a + b) + (((a + b) * 2 + c) * c) 
  return match(
      &I, m_BinOp(
              AddOp,
              m_BinOp(
                  MulOp, 
                  m_BinOp(AddOp, m_Value(A), m_Value(B)),
                  m_BinOp(AddOp, m_Deferred(A), m_Deferred(B))
              ),
              m_BinOp(
                  MulOp,
                  m_BinOp(
                      AddOp, 
                      m_c_BinOp(
                          Mul2Op,
                          m_BinOp(AddOp, m_Deferred(A), m_Deferred(B)),
                          m_SpecificInt(1)
                      ),
                      m_Value(C)
                  ),
                  m_Deferred(C)
              )
          )
  );
}

// Fold variation of a^2 + b^2 + c^2 + 2ab + 2ac + 2bc -> (a + b + c)^2
static Instruction* foldSquareSum3(BinaryOperator &I, BuilderTy &Builder) {
  Value *A, *B, *C;
  if (matchesSquareSum3(I, A, B, C)) {
    Value *AB = Builder.CreateAdd(A, B);
    Value *ABC = Builder.CreateAdd(AB, C);
    return BinaryOperator::CreateMul(ABC, ABC);
  }
  return nullptr;
}

// match variations of a^3 - 3*a^2*b + 3*a*b^2 - b^3
static bool matchesCubeDif(BinaryOperator &I, Value *&A, Value *&B) {
  constexpr unsigned MulOp = Instruction::Mul;
  constexpr unsigned AddOp = Instruction::Add;
  constexpr unsigned SubOp = Instruction::Sub;
  APInt three(32, 3);
  const APInt *th = &three;
  // ((b*a*3 - b*b) + a*a*(-3)) * b + a*a*a
  // ((3*a*b - b) * b - a*a*3) * b + a*a*a
  // ((a * 3) * b - ((a * 3) * a + b*b)) * b + a*a*a 
  // ((b-a)*a*3 - b*b) * b + a*a*a
  return match(
      &I, m_BinOp(
              AddOp,
              m_BinOp(
                  MulOp, 
                  m_CombineOr(
                      m_CombineOr(
                          m_BinOp(
                              AddOp,
                              m_BinOp(
                                  SubOp,
                                  m_BinOp(
                                      MulOp,
                                      m_BinOp(MulOp, m_Value(B), m_Value(A)),
                                      m_SpecificInt(3)
                                  ),
                                  m_BinOp(MulOp, m_Deferred(B), m_Deferred(B))
                              ),
                              m_BinOp(
                                  MulOp,
                                  m_BinOp(MulOp, m_Deferred(A), m_Deferred(A)),
                                  m_Negative(th)
                              )
                          ),
                          m_BinOp(
                              SubOp,
                              m_BinOp(
                                  MulOp,
                                  m_BinOp(
                                      SubOp,
                                      m_BinOp(MulOp, m_Value(A), m_SpecificInt(3)),
                                      m_Value(B)
                                  ),
                                  m_Deferred(B)
                              ),
                              m_BinOp(
                                  MulOp,
                                  m_BinOp(MulOp, m_Deferred(A), m_SpecificInt(3)),
                                  m_Deferred(A)
                              )
                          )
                      ),
                      m_CombineOr(
                          m_BinOp(
                              SubOp, 
                              m_BinOp(
                                  MulOp,
                                  m_BinOp(MulOp, m_Value(A), m_SpecificInt(3)),
                                  m_Value(B)
                              ),
                              m_BinOp(
                                  AddOp,
                                  m_BinOp(
                                      MulOp,
                                      m_BinOp(MulOp, m_Deferred(A), m_SpecificInt(3)),
                                      m_Deferred(A)
                                  ),
                                  m_BinOp(MulOp, m_Deferred(B), m_Deferred(B))
                              )
                          ),
                          m_BinOp(
                              SubOp, 
                              m_CombineOr(
                                  m_BinOp(
                                      MulOp,
                                      m_BinOp(
                                          MulOp, 
                                          m_BinOp(SubOp, m_Value(B), m_Value(A)), 
                                          m_Deferred(A)
                                      ),
                                      m_SpecificInt(3)
                                  ),
                                  m_BinOp(
                                      MulOp,
                                      m_BinOp(MulOp, m_Value(A), m_SpecificInt(3)),
                                      m_BinOp(SubOp, m_Value(B), m_Deferred(A))
                                  )
                              ),
                              m_BinOp(MulOp, m_Deferred(B), m_Deferred(B))
                          )
                      )
                  ),
                  m_Deferred(B)
              ),
              m_BinOp(
                  MulOp,
                  m_BinOp(MulOp, m_Deferred(A), m_Deferred(A)),
                  m_Deferred(A)
              )
          )
  );
}

// Fold variations of a^3 - 3*a^2*b + 3*a*b^2 - b^3 -> (a - b)^3
static Instruction* foldCubeDif(BinaryOperator &I, BuilderTy &Builder) {
  Value *A, *B;
  if (matchesCubeDif(I, A, B)) {
    Value *AB = Builder.CreateSub(A, B);
    Value *AB2 = Builder.CreateMul(AB, AB);
    return BinaryOperator::CreateMul(AB2, AB);
  }
  return nullptr;
}

// match variation of (a+b) * (a^2 - a*b + b^2)
static bool matchesCubesSum(BinaryOperator &I, Value *&A, Value *&B) {
  constexpr unsigned MulOp = Instruction::Mul;
  constexpr unsigned AddOp = Instruction::Add;
  constexpr unsigned SubOp = Instruction::Sub;

  // (((a-b) * a) + b*b) * (b+a)
  return match(
      &I, m_BinOp(
              MulOp,
              m_BinOp(
                  AddOp, 
                  m_BinOp(MulOp, m_BinOp(SubOp, m_Value(A), m_Value(B)), m_Deferred(A)),
                  m_BinOp(MulOp, m_Deferred(B), m_Deferred(B))
              ),
              m_BinOp(
                  AddOp,
                  m_Deferred(B),
                  m_Deferred(A)
              )
          )
  );
}

// Fold variation of (a+b) * (a^2 - a*b + b^2) -> a^3 + b^3
static Instruction* foldCubesSum(BinaryOperator &I, BuilderTy &Builder) {
  Value *A, *B;
  if (matchesCubesSum(I, A, B)) {
    Value *AA = Builder.CreateMul(A, A);
    Value *AAA = Builder.CreateMul(AA, A);
    Value *BB = Builder.CreateMul(B, B);
    Value *BBB = Builder.CreateMul(BB, B);
    return BinaryOperator::CreateAdd(AAA, BBB);
  }
  return nullptr;
}

// match variation of a\b * b\a
static bool matchesFractionsMul(BinaryOperator &I, Value *&A, Value *&B) {
  constexpr unsigned SubOp = Instruction::Sub;
  constexpr unsigned DivOp = Instruction::UDiv;
  constexpr unsigned RemOp = Instruction::URem;

  // a\b * b\a
  return match(
      &I, m_BinOp(
              DivOp,
              m_BinOp(
                  SubOp, 
                  m_Value(A),
                  m_BinOp(RemOp, m_Deferred(A), m_Value(B))
              ),
              m_Deferred(A)
          )
  );
}

// Fold variation of a\b * b\a -> (a == b)? 1 : 0
static Instruction* foldFractionsMul(BinaryOperator &I, BuilderTy &Builder) {
  Value *A, *B;
  if (matchesFractionsMul(I, A, B)) {
    Value *ICmp = Builder.CreateICmpEQ(A, B);
    return new ZExtInst(ICmp, A->getType());
  }
  return nullptr;
}

static Instruction* visitBinOp(BinaryOperator &I, BuilderTy &Builder) {
  Instruction *Result = foldSquareDif(I, Builder);
  if (Result) {
    return Result;
  }
  Result = foldSquareSum3(I, Builder);
  if (Result) {
    return Result;
  }
  Result = foldCubeDif(I, Builder);
  if (Result) {
    return Result;
  }
  Result = foldCubesSum(I, Builder);
  if (Result) {
    return Result;
  }
  Result = foldFractionsMul(I, Builder);
  if (Result) {
    return Result;
  }
  return nullptr;
}

PreservedAnalyses ReduceMathPass::run(Function &F, FunctionAnalysisManager &AM) {
  auto &DL = F.getParent()->getDataLayout();
  IRBuilder<TargetFolder> Builder(F.getContext(), TargetFolder(DL));
  bool MadeIRChange = false;
  //errs() << F.getName() << "\n";

  for (inst_iterator Iter = inst_begin(F), E = inst_end(F); Iter != E; ++Iter) {
    Instruction *I = &(*Iter);
    if (!isa<BinaryOperator>(I)) {
      continue;
    }
    BinaryOperator *BO = cast<BinaryOperator>(I);

    BasicBlock::iterator InsertPos = I->getIterator();
    InsertPos++;
    Builder.SetInsertPoint(InsertPos);

    Instruction *Result = visitBinOp(*BO, Builder);
    if (Result) {
      //errs() << "Match\n";
      I->replaceAllUsesWith(Result);
      Builder.Insert(Result);
      MadeIRChange = true;
    }
  }

  if (MadeIRChange) {
    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
  } else {
    return PreservedAnalyses::all();  
  }
}
