//===--- Passes/HALO.h - Heap Object Group Instrumentation ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_BOLT_PASSES_HALO_H
#define LLVM_TOOLS_LLVM_BOLT_PASSES_HALO_H

#include "bolt/Core/BinaryContext.h"
#include "bolt/Core/BinaryFunction.h"
#include "bolt/Passes/BinaryPasses.h"
#include "bolt/Rewrite/RewriteInstance.h"

namespace llvm {
namespace bolt {

class HALO : public BinaryFunctionPass {
  uint64_t extendDataSegment(BinaryContext &BC);
  uint64_t createStateSection(BinaryContext &BC);
  void instrumentSite(BinaryContext &BC,
                      std::map<uint64_t, BinaryFunction> &BFs,
                      uint64_t Address,
                      unsigned Index,
                      uint64_t StateAddr);
  BinaryFunction *
  getBinaryFunctionContainingAddress(std::map<uint64_t, BinaryFunction> &BFs,
                                     uint64_t Address);

public:
  static constexpr unsigned GroupStateSize = sizeof(uint64_t);
  static constexpr unsigned NewSegmentSize = 64;

  explicit HALO(const cl::opt<bool> &PrintPass)
    : BinaryFunctionPass(PrintPass) { }

  const char *getName() const override {
    return "HALO instrumentation";
  }
  bool shouldPrint(const BinaryFunction &BF) const override {
    return BinaryFunctionPass::shouldPrint(BF);
  }
  Error runOnFunctions(BinaryContext &BC) override;
};

} // namespace bolt
} // namespace llvm

#endif

