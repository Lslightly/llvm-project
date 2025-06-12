//===--- Passes/HALO.cpp - Heap Object Group Instrumentation --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "bolt/Passes/HALO.h"
#include "bolt/Core/BinarySection.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "bolt-halo"

using namespace llvm;

namespace opts {
extern cl::OptionCategory BoltOptCategory;
cl::list<std::string>
HALO("halo",
  cl::CommaSeparated,
  cl::desc("turn on HALO instrumentation for a set of call sites"),
  cl::value_desc("index1:site1,index2:site2,index3:site3,..."),
  cl::ZeroOrMore,
  cl::cat(BoltOptCategory));
} // namespace opts

namespace llvm {
namespace bolt {

BinaryFunction *
HALO::getBinaryFunctionContainingAddress(std::map<uint64_t,
                                                  BinaryFunction> &BFs,
                                         uint64_t Address) {
  auto FI = BFs.upper_bound(Address);
  if (FI == BFs.begin())
    return nullptr;
  --FI;

  const auto UsedSize = FI->second.getMaxSize();
  if (Address >= FI->first + UsedSize)
    return nullptr;
  return &FI->second;
}

uint64_t HALO::extendDataSegment(BinaryContext &BC) {
  // Find the data section
  auto DataSection = BC.getUniqueSectionByName(".data");
  if (!DataSection) {
    errs() << "BOLT-ERROR: HALO: unable to find data section\n";
    exit(1);
  }

  outs() << "BOLT: .data OutputFileOffset: " << DataSection->getOutputFileOffset() << " InputFileOffset: " << DataSection->getInputFileOffset() << '\n';
  outs() << " BOLT: .data type: " << DataSection->getELFType() << '\n'
        << *DataSection << '\n';

  // Find the segment to which the data section belongs
  auto Address = DataSection->getAddress();
  auto NextSegmentInfoI = BC.SegmentMapInfo.upper_bound(Address);
  if (NextSegmentInfoI == BC.SegmentMapInfo.begin()) {
    errs() << "BOLT-ERROR: HALO: unable to find data segment\n";
    exit(1);
  }
  auto &SegmentInfo = std::prev(NextSegmentInfoI)->second;
  if (Address < SegmentInfo.Address ||
      Address >= SegmentInfo.Address + SegmentInfo.FileSize) {
    errs() << "BOLT-ERROR: HALO: unable to find data segment\n";
    exit(1);
  }

  // Make sure there's enough space to fit the group state
  auto NewSize = SegmentInfo.Size + NewSegmentSize;
  auto NewFileSize = SegmentInfo.FileSize + NewSegmentSize;
  auto OldEndAddress = SegmentInfo.Address + SegmentInfo.Size;
  auto NewEndAddress = SegmentInfo.Address + NewSize;
  auto NewEndOffset = SegmentInfo.FileOffset + NewFileSize;
  if (NextSegmentInfoI != BC.SegmentMapInfo.end() &&
      NewEndAddress > NextSegmentInfoI->second.Address &&
      NewEndOffset > NextSegmentInfoI->second.FileOffset) {
    errs() << "BOLT-ERROR: HALO: insufficient space in data segment\n";
    exit(1);
  }

  // Expand the segment
  SegmentInfo.Size = NewSize;
  SegmentInfo.FileSize = NewFileSize;

  return OldEndAddress;
}

uint64_t HALO::createStateSection(BinaryContext &BC) {
  // Create a new section to hold group state
  std::string InitialData;
  const char *Name = ".data.halo_state";
  uint64_t Address = extendDataSegment(BC);
  raw_string_ostream OS(InitialData);
  for (unsigned i = 0; i < NewSegmentSize; ++i)
    OS << '\0';
  OS.str();

  // Register and return the address of the new section
  // NOTE: We could make this ELF::SHT_NOBITS, but for now it's staying as
  // ELF::SHT_PROGBITS for increased flexibility.
  auto flag = BinarySection::getFlags(false, false, true);
  auto &Section = BC.registerOrUpdateSection(Name, ELF::SHT_PROGBITS,
                                             flag,
                                             copyByteArray(InitialData),
                                             InitialData.size(),
                                             InitialData.size(),
                                             false, Address);
  Section.setOutputAddress(Address);
  outs() << " BOLT: .data.halo_state type: " << Section.getELFType() << '\n'
        << Section << '\n';
  outs() << "BOLT-INFO: HALO: state variable located at 0x"
         << Twine::utohexstr(Section.getAddress()) << "\n";
  return Section.getAddress();
}

void HALO::instrumentSite(BinaryContext &BC,
                          std::map<uint64_t, BinaryFunction> &BFs,
                          uint64_t Target,
                          unsigned Index,
                          uint64_t StateAddr) {
  // Find the target function
  auto* Function = getBinaryFunctionContainingAddress(BFs, Target);
  if (Function == nullptr) {
    errs() << "BOLT-ERROR: HALO: unable to find function at 0x"
           << Twine::utohexstr(Target) << "\n";
    exit(1);
  }

  // Find the target BB
  // NOTE: Functions with AVX-512 instructions, as well as those with other
  // quirks, won't be processed properly by BOLT and thus will fail BB lookup.
  // In general, this can always fail (as can writing the instrumentation if
  // there's not enough free space), so ideally we would recalculate
  // the instrumentation points to work around this when such failures occur.
  auto Offset = Target - Function->getAddress();
  auto* BB = Function->getBasicBlockContainingOffset(Offset);
  if (BB == nullptr) {
    errs() << "BOLT-ERROR: HALO: unable to find basic block at 0x"
           << Twine::utohexstr(Target) << " (0x" << Twine::utohexstr(Offset)
           << " from 0x" << Twine::utohexstr(Function->getAddress()) << ")\n";
    exit(1);
  }

  // Generate instructions and check for currently problematic cases
  // TODO: Currently we only support calls as we simply add instructions before
  // and after the target. In future, we should support direct branches by
  // setting the flag before the site and unsetting it at the end of the target.
  auto II = std::prev(BB->end());
  unsigned SiteBitFlag = 1 << Index;
  MCInst SetSiteBit, UnsetSiteBit;
  auto* State = MCConstantExpr::create(StateAddr, *BC.Ctx.get());
  BC.MIB->createOr(SetSiteBit, State, SiteBitFlag, BC.Ctx.get());
  BC.MIB->createAnd(UnsetSiteBit, State, ~SiteBitFlag, BC.Ctx.get());
  while (II != BB->begin() && !BC.MIB->isCall(*II))
    II = std::prev(II); // TODO: This shouldn't be necessary in theory, but
                        // sometimes instructions sneak below calls somehow...
  if (!BC.MIB->isCall(*II)) {
    errs() << "BOLT-ERROR: HALO: cannot instrument non-call instruction at 0x"
           << Twine::utohexstr(Target) << "\n";
    exit(1);
  }

  // Set group bit before call, unset group bit after call
  // NOTE: Currently requires compiling with '-no-pie'. For this to work
  // properly with PIC, we'll need to mess around with the GOT or calculate the
  // distance between each instruction and the group state in the final layout.
  // NOTE: With upstream BOLT, these modifications can fail silently if there's
  // not enough space in the target function.
  Function->IsMissionCritical = true;
  II = BB->insertInstruction(II, std::move(SetSiteBit));
  for (auto* Succ = BB->succ_begin(); Succ != BB->succ_end(); ++Succ) {
    MCInst UnsetSiteBitInst = UnsetSiteBit;
    (*Succ)->insertInstruction((*Succ)->begin(), std::move(UnsetSiteBitInst));
  }
}

Error HALO::runOnFunctions(BinaryContext &BC) {
  if (opts::HALO.size() > GroupStateSize * CHAR_BIT) {
    errs() << "BOLT-ERROR: HALO: too many sites\n";
    exit(1);
  }

  auto &BFs = BC.getBinaryFunctions();

  // Instrument each grouped call site
  // TODO: Right now, we don't update the '_end' symbol to the new end of the
  // data segment (e.g. thru OLT and by updating BinaryDataMap). This is
  // probably a bad idea in general, but it also means we can't use the existing
  // symbol infrastructure (getOrCreateGlobalSymbol).
  uint64_t StateAddr = createStateSection(BC);
  for (auto Input : opts::HALO) {
    if (!Input.length())
      continue;

    auto split = Input.find(":");
    if (split == std::string::npos || split == Input.length() - 1) {
      errs() << "BOLT-ERROR: HALO: invalid input '" << Input << "'\n";
      exit(1);
    }
    auto Label = Input.substr(0, split);
    auto Target = Input.substr(split + 1, std::string::npos);
    unsigned Index = unsigned(std::strtol(Label.c_str(), NULL, 0));
    uint64_t Address = uint64_t(std::strtol(Target.c_str(), NULL, 0));
    instrumentSite(BC, BFs, Address, Index, StateAddr);
  }
  return Error::success();
}
} // namespace bolt
} // namespace llvm
