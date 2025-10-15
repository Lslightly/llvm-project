#include "bolt/Passes/AllocCtxMgr.h"
#include "bolt/Core/BinaryBasicBlock.h"
#include "bolt/Core/BinaryContext.h"
#include "bolt/Core/MCPlus.h"
#include "bolt/Utils/CommandLineOpts.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <cstdlib>

#define DEBUG_TYPE "bolt-alloc-ctx"

using namespace llvm;

namespace opts {

extern cl::OptionCategory BoltOptCategory;
cl::list<std::string> AllocCtxs(
    "alloc-ctx",
    cl::CommaSeparated,
    cl::desc("turn on Alloc Context Manager to replace certain malloc callsites"),
    cl::value_desc("grp0:addr0,grp0:addr1,grp1:addr2,..."),
    cl::ZeroOrMore,
    cl::cat(BoltOptCategory)
);

} // namespace opts

namespace llvm {
namespace bolt {

void AllocCtxMgr::parseOpts() {
    for (auto CtxStr: opts::AllocCtxs) {
        StringRef Ctx(CtxStr);
        if (auto SplitPos = Ctx.find(":"); SplitPos == llvm::StringRef::npos || SplitPos == Ctx.size() - 1) {
            errs() << "BOLT-ERROR: AllocCtxMgr: invalid ctx '" << Ctx << "'\n";
            std::exit(1);
        }
        auto [GrpStr, AddrStr] = Ctx.split(":");
        uintptr_t Addr;
        size_t Grp;
        GrpStr.getAsInteger(0, Grp);
        AddrStr.getAsInteger(0, Addr);
        Grp2Addrs[Grp].push_back(Addr);
    }
}

const MCSymbol* AllocCtxMgr::findPLTSymbol(StringRef FuncName) {
    /*
        lld: FuncName@PLT
        gold: FuncName@PLT
        mold: FuncName$plt/1
    */
    SmallVector<std::string, 4> TryNames{FuncName.str()+"@PLT", FuncName.str()+"@plt", FuncName.str()+"$plt/1", FuncName.str()};
    for (auto Name: TryNames) {
        auto* Data = BC->getBinaryDataByName(Name);
        if (Data != nullptr) {
            return Data->getSymbol();
        }
    }
    return nullptr;
}

void AllocCtxMgr::init(BinaryContext& BC) {
    this->BC = &BC;
    for (auto Name: SmallVector<StringRef, 3>{MallocWrapper, AllocEnter, AllocExit}) {
        Name2Symbol[Name] = findPLTSymbol(Name);
    }
}

BinaryBasicBlock::iterator AllocCtxMgr::findInst(uintptr_t Addr, BinaryBasicBlock*& BB) {
    auto* Func = BC->getBinaryFunctionContainingAddress(Addr);
    auto Offset = Addr - Func->getAddress();
    BB = Func->getBasicBlockContainingOffset(Offset);
    auto InstOffset = BB->getOffset();
    InstOffset = alignTo(InstOffset, BB->getAlignment());
    auto II = BB->begin();
    for (; II != BB->end(); II++) {
        InstOffset += BC->computeCodeSize(II, II+1);
        if (InstOffset == Offset) {
            II++;
            break;
        }
    }
    return II;
}

AllocCtxMgr::CalleeType AllocCtxMgr::getCalleeType(InstIter II) {
    auto& Inst = *II;
    if (!BC->MIB->isCall(Inst)) {
        return Other;
    }
    auto* SymRefExpr = dyn_cast<MCSymbolRefExpr>(Inst.getOperand(0).getExpr());
    if (!SymRefExpr) {
        return Other;
    }
    auto CalleeName = SymRefExpr->getSymbol().getName();
    if (auto Ty = CalleeName2Type.find(CalleeName); Ty != CalleeName2Type.end()) {
        return Ty->second;
    }
    return Other;
}



void AllocCtxMgr::replaceMalloc(size_t Grp, InstIter MallocII, BinaryBasicBlock* BB) {
    /*
        mov size, %edi
        push %rsi
        push %rsi
        mov $Grp, %rsi
        call malloc_wrapper@PLT
        pop %rsi
        pop %rsi
    */
    InstructionListType WrapperInsts(6);
    size_t I = 0;
    auto RSI = BC->MIB->getIntArgRegister(1);
    BC->MIB->createPushRegister(WrapperInsts[I++], RSI, 8);
    BC->MIB->createPushRegister(WrapperInsts[I++], RSI, 8);
    BC->MIB->createMov32RIInst(WrapperInsts[I++], Grp, RSI);
    BC->MIB->createCall(WrapperInsts[I++], Name2Symbol[MallocWrapper], BC->Ctx.get());
    BC->MIB->createPopRegister(WrapperInsts[I++], RSI, 8);
    BC->MIB->createPopRegister(WrapperInsts[I++], RSI, 8);
    BB->replaceInstruction(MallocII, WrapperInsts);
}

void AllocCtxMgr::wrapContext(size_t Grp, InstIter II, BinaryBasicBlock* BB) {
    /*
        push %rdi
        push %rax
        mov $Grp, %rdi
        call alloc_enter@PLT
        pop %rax
        pop %rdi
        II
        push %rdi
        push %rax
        call alloc_exit
        pop %rax
        pop %rdi
    */
    InstructionListType WrapperInsts(12);
    size_t I = 0;
    auto RDI = BC->MIB->getIntArgRegister(0);
    auto RAX = BC->MIB->getRetRegister();
    BC->MIB->createPushRegister(WrapperInsts[I++], RDI, 8);
    BC->MIB->createPushRegister(WrapperInsts[I++], RAX, 8);
    BC->MIB->createMov32RIInst(WrapperInsts[I++], Grp, RDI);
    BC->MIB->createCall(WrapperInsts[I++], Name2Symbol["alloc_enter"], BC->Ctx.get());
    BC->MIB->createPopRegister(WrapperInsts[I++], RAX, 8);
    BC->MIB->createPopRegister(WrapperInsts[I++], RDI, 8);
    WrapperInsts[I++] = *II;
    BC->MIB->createPushRegister(WrapperInsts[I++], RDI, 8);
    BC->MIB->createPushRegister(WrapperInsts[I++], RAX, 8);
    BC->MIB->createCall(WrapperInsts[I++], Name2Symbol["alloc_exit"], BC->Ctx.get());
    BC->MIB->createPopRegister(WrapperInsts[I++], RAX, 8);
    BC->MIB->createPopRegister(WrapperInsts[I++], RDI, 8);
    BB->replaceInstruction(II, WrapperInsts);
}

Error AllocCtxMgr::runOnFunctions(BinaryContext& BC) {
    init(BC);
    parseOpts();
    size_t SuccCnt = 0;
    for (auto& [Grp, Addrs]: Grp2Addrs) {
        for (auto Addr: Addrs) {
            BinaryBasicBlock* BB = nullptr;
            auto TgtII = findInst(Addr, BB);
            BC.printInstruction(outs(), *TgtII);
            BC.printInstructions(outs(), BB->begin(), BB->end());
            auto CalleeType = getCalleeType(TgtII);
            SuccCnt += 1;
            switch (CalleeType) {
                case Malloc:
                case New:
                    replaceMalloc(Grp, TgtII, BB);
                    break;
                case Other:
                    wrapContext(Grp, TgtII, BB);
                    break;
                default:
                    SuccCnt -= 1;
                    break;
            }
            BC.printInstructions(outs(), BB->begin(), BB->end());
        }
    }
    outs() << "AllocCtxMgr succeed in replacing " << SuccCnt << " callsite\n";
    return Error::success();
}

} // namespace bolt
} // namespace llvm
