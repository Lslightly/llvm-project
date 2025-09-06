//===- bolt/Passes/AllocCtxMgr.h - Pass for replace malloc with malloc_wrapper --*- C++ -*-===//

#ifndef BOLT_PASSES_ALLOC_CTX_MGR_H
#define BOLT_PASSES_ALLOC_CTX_MGR_H

#include "bolt/Core/BinaryBasicBlock.h"
#include "bolt/Core/BinaryContext.h"
#include "bolt/Passes/BinaryPasses.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCSymbol.h"
#include <cstddef>
#include <cstdint>

namespace llvm {
namespace bolt {

using Grp2AddrMapTy = DenseMap<size_t, SmallVector<uintptr_t, 8>>;
using InstIter = BinaryBasicBlock::iterator;

class AllocCtxMgr: public BinaryFunctionPass {
    BinaryContext* BC;
    Grp2AddrMapTy Grp2Addrs;
    DenseMap<StringRef, const MCSymbol*> Name2Symbol;
    const StringRef MallocWrapper = "malloc_wrapper";
    const StringRef AllocEnter = "alloc_enter";
    const StringRef AllocExit = "alloc_exit";

    void parseOpts();
    const MCSymbol* findPLTSymbol(StringRef FuncName);
    void init(BinaryContext& BC);
    InstIter findInst(uintptr_t Addr, BinaryBasicBlock*& BB);
    void replaceMalloc(size_t Grp, InstIter MallocII, BinaryBasicBlock* BB);
public:
    explicit AllocCtxMgr(): BinaryFunctionPass(false) {}
    const char* getName() const override { return "Alloc Context Manager"; }
    Error runOnFunctions(BinaryContext& BC) override;
};

} // namespace bolt
} // namespace llvm

#endif