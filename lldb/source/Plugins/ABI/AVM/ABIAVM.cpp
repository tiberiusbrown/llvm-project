//===-- ABIAVM.cpp - AVM stack and pointer ABI for LLDB ------------------===//

#include "lldb/Core/PluginManager.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/ABI.h"
#include "lldb/Utility/ArchSpec.h"
#include "llvm/TargetParser/Triple.h"

using namespace lldb;
using namespace lldb_private;

namespace {
class ABIAVM final : public RegInfoBasedABI {
public:
  ABIAVM(ProcessSP process, std::unique_ptr<llvm::MCRegisterInfo> info)
      : RegInfoBasedABI(std::move(process), std::move(info)) {}

  static void Initialize() {
    PluginManager::RegisterPlugin("avm", "AVM debugger ABI", CreateInstance);
  }
  static void Terminate() { PluginManager::UnregisterPlugin(CreateInstance); }
  static ABISP CreateInstance(ProcessSP process, const ArchSpec &arch) {
    if (arch.GetMachine() != llvm::Triple::avm)
      return {};
    return std::make_shared<ABIAVM>(std::move(process), MakeMCRegisterInfo(arch));
  }
  llvm::StringRef GetPluginName() override { return "avm"; }

  // This stack backs LLDB's host-side IR interpreter, not the AVM guest.
  uint64_t GetStackFrameSize() override { return 16 * 1024; }

  size_t GetRedZoneSize() const override { return 0; }
  bool PrepareTrivialCall(Thread &, addr_t, addr_t, addr_t,
                          llvm::ArrayRef<addr_t>) const override {
    return false;
  }
  bool GetArgumentValues(Thread &, ValueList &) const override { return false; }
  Status SetReturnValueObject(StackFrameSP &, ValueObjectSP &) override {
    return Status::FromErrorString("AVM target calls are unsupported");
  }
  ValueObjectSP GetReturnValueObjectImpl(Thread &,
                                          CompilerType &) const override {
    return {};
  }
  UnwindPlanSP CreateFunctionEntryUnwindPlan() override {
    UnwindPlan::Row row;
    row.GetCFAValue().SetIsRegisterPlusOffset(8, 3);
    row.SetRegisterLocationToAtCFAPlusOffset(9, -3, true);
    row.SetRegisterLocationToIsCFAPlusOffset(8, 0, true);
    auto plan = std::make_shared<UnwindPlan>(eRegisterKindDWARF);
    plan->AppendRow(std::move(row));
    plan->SetSourceName("AVM function-entry CFA");
    plan->SetSourcedFromCompiler(eLazyBoolNo);
    return plan;
  }
  UnwindPlanSP CreateDefaultUnwindPlan() override {
    auto plan = CreateFunctionEntryUnwindPlan();
    plan->SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
    return plan;
  }
  bool RegisterIsVolatile(const RegisterInfo *info) override {
    unsigned number = info->kinds[eRegisterKindDWARF];
    return number >= 4;
  }
  bool CallFrameAddressIsValid(addr_t cfa) override {
    cfa = cfa >= 0x01000000 ? cfa - 0x01000000 : cfa;
    return cfa >= 0x100 && cfa <= 0xa03;
  }
  bool CodeAddressIsValid(addr_t pc) override { return pc <= 0xffffff; }
  addr_t FixDataAddress(addr_t address) override {
    return address <= 0xffff ? 0x01000000 + address : address;
  }
  addr_t FixCodeAddress(addr_t address) override { return address & 0xffffff; }
  const RegisterInfo *GetRegisterInfoArray(uint32_t &count) override {
    count = 0;
    return nullptr;
  }
};
} // namespace

LLDB_PLUGIN_DEFINE(ABIAVM)
