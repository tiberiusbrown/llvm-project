#include "AVMLegalizerInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"

using namespace llvm;

AVMLegalizerInfo::AVMLegalizerInfo(const AVMSubtarget &) {
  using namespace TargetOpcode;
  const LLT s1 = LLT::scalar(1);
  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT p0 = LLT::pointer(0, 16);

  getActionDefinitionsBuilder({G_ADD, G_SUB, G_AND, G_OR, G_XOR})
      .legalFor({s16})
      .clampScalar(0, s16, s16);
  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({s16, p0})
      .clampScalar(0, s16, s16);
  getActionDefinitionsBuilder(G_GLOBAL_VALUE).legalFor({p0});
  getActionDefinitionsBuilder(G_ICMP)
      .legalForCartesianProduct({s16}, {s16, p0})
      .clampScalar(0, s16, s16)
      .clampScalar(1, s16, s16);
  getActionDefinitionsBuilder(G_BRCOND).legalFor({s16}).clampScalar(0, s16,
                                                                    s16);
  getActionDefinitionsBuilder(G_PHI).legalFor({s16, p0}).clampScalar(0, s16,
                                                                     s16);
  getActionDefinitionsBuilder({G_LOAD, G_STORE})
      .legalForTypesWithMemDesc(
          {{s16, p0, s16, 1},
           {s16, p0, s8, 1},
           {s16, p0, s1, 1},
           {p0, p0, s16, 1}})
      .minScalar(0, s16);
  getActionDefinitionsBuilder(G_PTR_ADD).legalFor({{p0, s16}});
  getActionDefinitionsBuilder(G_FRAME_INDEX).legalFor({p0});
  getActionDefinitionsBuilder(G_FREEZE).legalFor({s1, s8, s16, p0});
  getActionDefinitionsBuilder(G_SELECT).lower();
  getActionDefinitionsBuilder({G_ZEXT, G_SEXT, G_ANYEXT})
      .legalFor({{s16, s1}, {s16, s8}});
  getActionDefinitionsBuilder(G_TRUNC).legalFor({{s8, s16}, {s1, s16}});
  getLegacyLegalizerInfo().computeTables();
}
