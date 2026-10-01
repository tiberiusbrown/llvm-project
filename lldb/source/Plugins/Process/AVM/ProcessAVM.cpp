//===-- ProcessAVM.cpp - Embedded AVM interpreter process ----------------===//

#include "AvmEmulator.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Interpreter/CommandObject.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#include "lldb/Breakpoint/BreakpointSite.h"
#include "lldb/Breakpoint/Breakpoint.h"
#include "lldb/Breakpoint/BreakpointList.h"
#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Breakpoint/BreakpointOptions.h"
#include "lldb/Breakpoint/Watchpoint.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/ThreadList.h"
#include "lldb/Target/Unwind.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Stream.h"
#include "lldb/Utility/Args.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/TargetParser/Triple.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace lldb;
using namespace lldb_private;
namespace fs = std::filesystem;

namespace {
class ProcessAVM;

class RegisterContextAVM final : public RegisterContext {
public:
  explicit RegisterContextAVM(Thread &thread) : RegisterContext(thread, 0) {}
  void InvalidateAllRegisters() override {}
  size_t GetRegisterCount() override { return 15; }
  const RegisterInfo *GetRegisterInfoAtIndex(size_t index) override;
  size_t GetRegisterSetCount() override { return 1; }
  const RegisterSet *GetRegisterSet(size_t index) override;
  bool ReadRegister(const RegisterInfo *info, RegisterValue &value) override;
  bool WriteRegister(const RegisterInfo *, const RegisterValue &) override;
};

class ThreadAVM final : public Thread {
public:
  explicit ThreadAVM(Process &process) : Thread(process, 1) {}
  ~ThreadAVM() override { DestroyThread(); }
  void RefreshStateAfterStop() override {
    if (m_context)
      m_context->InvalidateIfNeeded(true);
  }
  RegisterContextSP GetRegisterContext() override {
    if (!m_context)
      m_context = std::make_shared<RegisterContextAVM>(*this);
    return m_context;
  }
  RegisterContextSP CreateRegisterContextForFrame(StackFrame *frame) override {
    if (frame && frame->GetConcreteFrameIndex())
      return GetUnwinder().CreateRegisterContextForFrame(frame);
    return GetRegisterContext();
  }
  bool CalculateStopInfo() override;
private:
  RegisterContextSP m_context;
};

class ProcessAVM final : public Process {
public:
  ProcessAVM(TargetSP target, ListenerSP listener)
      : Process(std::move(target), std::move(listener)) {}
  ~ProcessAVM() override {
    m_emulator.interrupt();
    if (m_worker.joinable())
      m_worker.join();
    RemoveTemporaryImage();
    Finalize(true);
  }

  static void Initialize() {
    static llvm::once_flag once;
    llvm::call_once(once, [] {
      PluginManager::RegisterPlugin("avm", "Embedded AVM interpreter",
                                    CreateInstance, DebuggerInitialize);
    });
  }
  static void Terminate() { PluginManager::UnregisterPlugin(CreateInstance); }
  static void DebuggerInitialize(Debugger &debugger);
  static ProcessSP CreateInstance(TargetSP target, ListenerSP listener,
                                  const FileSpec *crash_file, bool) {
    if (crash_file)
      return {};
    return std::make_shared<ProcessAVM>(std::move(target), std::move(listener));
  }
  static llvm::StringRef GetPluginNameStatic() { return "avm"; }
  llvm::StringRef GetPluginName() override { return GetPluginNameStatic(); }
  bool CanDebug(TargetSP target, bool explicitly_selected) override {
    if (explicitly_selected)
      return true;
    Module *module = target->GetExecutableModulePointer();
    return module && module->GetObjectFile() &&
           module->GetObjectFile()->GetArchitecture().GetMachine() ==
               llvm::Triple::avm;
  }

  Status DoLaunch(Module *module, ProcessLaunchInfo &) override;
  void DidLaunch() override {
    // Section load addresses are established by DoLaunch. Rebind locations
    // once LLDB has consumed the initial stop, including numeric-address
    // breakpoints that had no section before the process was launched.
    for (bool internal : {false, true})
      for (auto const &breakpoint : GetTarget().GetBreakpointList(internal).Breakpoints())
        if (breakpoint->IsEnabled())
          for (size_t i = 0; i < breakpoint->GetNumLocations(); ++i)
            if (BreakpointLocationSP location = breakpoint->GetLocationAtIndex(i);
                location && location->IsEnabled())
              llvm::consumeError(location->ResolveBreakpointSite());
  }
  Status DoResume(RunDirection direction) override;
  Status DoHalt(bool &caused_stop) override {
    m_emulator.interrupt();
    caused_stop = true;
    return {};
  }
  Status DoDestroy() override {
    m_emulator.interrupt();
    if (m_worker.joinable())
      m_worker.join();
    RemoveTemporaryImage();
    SetPrivateState(eStateExited);
    return {};
  }
  bool DestroyRequiresHalt() override { return false; }
  void RefreshStateAfterStop() override {
    if (m_thread)
      m_thread->RefreshStateAfterStop();
  }
  bool DoUpdateThreadList(ThreadList &, ThreadList &updated) override {
    if (!m_thread)
      m_thread = std::make_shared<ThreadAVM>(*this);
    updated.AddThread(m_thread);
    return true;
  }
  size_t DoReadMemory(addr_t address, void *out, size_t size,
                      Status &error) override;
  size_t DoWriteMemory(addr_t address, const void *bytes, size_t size,
                       Status &error) override;
  Status EnableBreakpointSite(BreakpointSite *site) override {
    try {
      auto guard = LockEmulator();
      if (site->HardwareRequired())
        return Status::FromErrorString("AVM hardware breakpoints are unsupported");
      m_emulator.add_breakpoint(site->GetLoadAddress());
      return {};
    } catch (const std::exception &error) {
      return Status::FromErrorString(error.what());
    }
  }
  Status DisableBreakpointSite(BreakpointSite *site) override {
    try {
      auto guard = LockEmulator();
      m_emulator.remove_breakpoint(site->GetLoadAddress());
      return {};
    } catch (const std::exception &error) {
      return Status::FromErrorString(error.what());
    }
  }
  std::optional<uint32_t> GetWatchpointSlotCount() override { return 1024; }
  Status EnableWatchpoint(WatchpointSP watchpoint, bool notify = true) override {
    try {
      auto guard = LockEmulator();
      m_emulator.add_watchpoint(watchpoint->GetID(),
                                uint16_t(watchpoint->GetLoadAddress()),
                                watchpoint->GetByteSize(),
                                watchpoint->WatchpointRead(),
                                watchpoint->WatchpointWrite() ||
                                    watchpoint->WatchpointModify());
      watchpoint->SetEnabled(true, notify);
      return {};
    } catch (const std::exception &error) {
      return Status::FromErrorString(error.what());
    }
  }
  Status DisableWatchpoint(WatchpointSP watchpoint, bool notify = true) override {
    try {
      auto guard = LockEmulator();
      m_emulator.remove_watchpoint(watchpoint->GetID());
      watchpoint->SetEnabled(false, notify);
      return {};
    } catch (const std::exception &error) {
      return Status::FromErrorString(error.what());
    }
  }
  std::unique_lock<std::recursive_mutex> LockEmulator() {
    std::unique_lock<std::recursive_mutex> guard(m_emulator_mutex,
                                                  std::try_to_lock);
    if (!guard.owns_lock() || GetState() == eStateRunning)
      throw std::runtime_error("AVM emulator is running; interrupt it first");
    return guard;
  }
  avm_debug::Snapshot Snapshot() {
    auto guard = LockEmulator();
    return m_emulator.snapshot();
  }
  void WriteRegister(unsigned number, uint64_t value) {
    auto guard = LockEmulator();
    m_emulator.write_register(number, value);
    m_mod_id.BumpMemoryID();
    m_memory_cache.Clear();
    if (m_thread)
      m_thread->ClearStackFrames();
  }
  size_t ProgramSize() const { return m_emulator.program_size(); }
  bool HasUnsupportedBreakpointConditions() {
    for (auto const &breakpoint : GetTarget().GetBreakpointList().Breakpoints())
      if (breakpoint->IsEnabled())
        for (size_t i = 0; i < breakpoint->GetNumLocations(); ++i)
          if (BreakpointLocationSP location = breakpoint->GetLocationAtIndex(i);
              location && location->IsEnabled() &&
              location->GetOptionsSpecifyingKind(
                  BreakpointOptions::eCondition).GetCondition())
            return true;
    return false;
  }
  avm_debug::Stop RunFor(uint64_t cycles) {
    if (HasUnsupportedBreakpointConditions())
      throw std::invalid_argument(
          "AVM breakpoint conditions require unsupported expression execution");
    // AVM commands execute synchronously while LLDB's command interpreter is
    // stopped. Publishing a transient running event here races the next
    // command with LLDB's private-state event thread.
    avm_debug::Stop stopped = m_emulator.run_for(cycles);
    {
      std::lock_guard<std::mutex> lock(m_stop_mutex);
      m_stop = stopped;
    }
    m_mod_id.BumpStopID();
    m_mod_id.BumpMemoryID();
    m_memory_cache.Clear();
    if (m_thread) {
      m_thread->Flush();
      m_thread->RefreshStateAfterStop();
      m_thread->CalculateStopInfo();
    }
    return stopped;
  }
  void SetButtons(uint8_t pressed) { m_emulator.set_buttons(pressed); }
  uint8_t Buttons() const { return m_emulator.buttons(); }
  void SetRealtime(bool enabled) { m_emulator.set_realtime(enabled); }
  bool Realtime() const { return m_emulator.realtime(); }
  uint64_t EntryCycle() const { return m_emulator.entry_cycle(); }
  avm_debug::ReplayIdentity const &ReplayIdentity() const {
    return m_emulator.replay_identity();
  }
  std::vector<avm_debug::TimedButtons> const &ButtonHistory() const {
    return m_emulator.button_history();
  }
  size_t PendingEvents() const { return m_emulator.pending_events(); }
  void ScheduleButtons(std::vector<avm_debug::TimedButtons> events) {
    m_emulator.schedule_buttons(std::move(events));
  }
  void ClearEvents() { m_emulator.clear_scheduled_buttons(); }
  void SetReplayFinalCycle(uint64_t cycle) { m_replay_final_cycle = cycle; }
  uint64_t ReplayFinalCycle() const { return m_replay_final_cycle; }
  void SetReplayPaused(bool paused) { m_replay_paused = paused; }
  bool ReplayPaused() const { return m_replay_paused; }
  std::vector<uint8_t> Capture(llvm::StringRef mode) const {
    if (mode == "visible") return m_emulator.visible_pixels();
    std::vector<uint8_t> pages = mode == "logical"
        ? m_emulator.logical_framebuffer()
        : m_emulator.controller_ram();
    std::vector<uint8_t> pixels(128 * 64);
    for (unsigned y = 0; y < 64; ++y)
      for (unsigned x = 0; x < 128; ++x)
        pixels[y * 128 + x] =
            (pages[(y / 8) * 128 + x] & (1u << (y % 8))) ? 255 : 0;
    return pixels;
  }
  avm_debug::Stop LastStop() const {
    std::lock_guard<std::mutex> lock(m_stop_mutex);
    return m_stop;
  }
private:
  void RemoveTemporaryImage() {
    if (!m_image.empty()) {
      std::error_code ignored;
      fs::remove(m_image, ignored);
      m_image.clear();
    }
  }
  avm_debug::Emulator m_emulator;
  std::shared_ptr<ThreadAVM> m_thread;
  std::thread m_worker;
  fs::path m_image;
  std::recursive_mutex m_emulator_mutex;
  mutable std::mutex m_stop_mutex;
  avm_debug::Stop m_stop;
  uint64_t m_replay_final_cycle = 0;
  bool m_replay_paused = false;
};

static uint8_t buttonMask(llvm::StringRef name) {
  std::string upper = name.upper();
  if (upper == "UP") return avm_debug::Up;
  if (upper == "RIGHT") return avm_debug::Right;
  if (upper == "LEFT") return avm_debug::Left;
  if (upper == "DOWN") return avm_debug::Down;
  if (upper == "A") return avm_debug::A;
  if (upper == "B") return avm_debug::B;
  throw std::invalid_argument("unknown AVM button: " + name.str());
}

static uint64_t durationCycles(llvm::StringRef text) {
  uint64_t scale = 1;
  if (text.consume_back("ms")) scale = 16000;
  else if (text.consume_back("cycles")) scale = 1;
  else if (text.consume_back("s")) scale = 16000000;
  if (text.empty()) throw std::invalid_argument("empty AVM duration");
  uint64_t amount = 0;
  if (text.getAsInteger(10, amount) ||
      amount > UINT64_MAX / scale)
    throw std::invalid_argument("AVM duration must be an integer number of cycles, milliseconds, or seconds");
  return amount * scale;
}

static uint64_t unsignedArgument(llvm::StringRef text, const char *name) {
  uint64_t value = 0;
  if (text.empty() || text.getAsInteger(0, value))
    throw std::invalid_argument(std::string("invalid ") + name);
  return value;
}

static const char *stopName(avm_debug::StopReason reason) {
  switch (reason) {
  case avm_debug::StopReason::Entry: return "entry";
  case avm_debug::StopReason::Breakpoint: return "breakpoint";
  case avm_debug::StopReason::Watchpoint: return "watchpoint";
  case avm_debug::StopReason::DebugBreak: return "debug_break";
  case avm_debug::StopReason::Step: return "step";
  case avm_debug::StopReason::Deadline: return "deadline";
  case avm_debug::StopReason::Fault: return "fault";
  case avm_debug::StopReason::Interrupt: return "interrupt";
  }
  return "unknown";
}

static void emitStopJSON(Stream &out, avm_debug::Stop const &stopped,
                         size_t pending_events,
                         std::optional<uint64_t> requested_cycles = std::nullopt) {
  out.Printf("{\"reason\":\"%s\",\"pc\":%u,\"cycles\":%" PRIu64,
             stopName(stopped.reason), stopped.state.pc, stopped.state.cycles);
  if (requested_cycles)
    out.Printf(",\"requested_cycles\":%" PRIu64, *requested_cycles);
  out.Printf(",\"requested_stop_cycle\":%" PRIu64
             ",\"pending_events\":%zu",
             stopped.requested_cycle, pending_events);
  if (stopped.access) {
    auto const &access = *stopped.access;
    out.Printf(",\"access\":{\"watchpoint\":%" PRIu64
               ",\"instruction_pc\":%u,\"address\":%u,\"value\":%u,"
               "\"cycle\":%" PRIu64 ",\"kind\":\"%s\"}",
               access.id, access.instruction_pc, access.address,
               access.value, access.cycle, access.write ? "write" : "read");
  }
  if (stopped.fault_cycle)
    out.Printf(",\"fault_cycle\":%" PRIu64, *stopped.fault_cycle);
  out.PutCString("}\n");
}

class AVMCommand final : public CommandObjectRaw {
public:
  explicit AVMCommand(CommandInterpreter &interpreter)
      : CommandObjectRaw(
            interpreter, "avm", "AVM emulator controls and JSON queries.",
            "avm time | stop | run-for <duration> | realtime on|off|status | "
            "watch read|write|readwrite <data-address> [size] | "
            "button press|release|set|status ... | replay load|run|status|abort|export ... | "
            "display save|capture <file.pgm> [--mode visible|logical|controller]") {}

protected:
  void DoExecute(llvm::StringRef command, CommandReturnObject &result) override {
    Args args(command);
    ProcessSP process = GetTarget().GetProcessSP();
    if (!process || process->GetPluginName() != "avm") {
      result.GetErrorStream().PutCString(
          "{\"ok\":false,\"error\":\"avm commands require a launched AVM process\"}\n");
      result.AppendError("avm commands require a launched AVM process");
      return;
    }
    auto &avm = static_cast<ProcessAVM &>(*process);
    auto arg = [&args](size_t index) -> llvm::StringRef {
      const char *value = args.GetArgumentAtIndex(index);
      return value ? llvm::StringRef(value) : llvm::StringRef();
    };
    try {
      auto guard = avm.LockEmulator();
      llvm::StringRef action = arg(0);
      auto &out = result.GetOutputStream();
      if (action == "time") {
        auto state = avm.Snapshot();
        out.Printf("{\"cycles\":%" PRIu64 ",\"seconds_since_reset\":%.9f,"
                   "\"cycles_since_entry\":%" PRIu64 ","
                   "\"seconds_since_entry\":%.9f,\"pc\":%u}\n",
                   state.cycles, double(state.cycles) / 16000000.0,
                   state.cycles - avm.EntryCycle(),
                   double(state.cycles - avm.EntryCycle()) / 16000000.0,
                   state.pc);
      } else if (action == "stop") {
        emitStopJSON(out, avm.LastStop(), avm.PendingEvents());
      } else if (action == "run-for") {
        if (arg(1).empty())
          throw std::invalid_argument("usage: avm run-for <integer duration>s|ms|cycles");
        uint64_t requested = durationCycles(arg(1));
        auto stopped = avm.RunFor(requested);
        emitStopJSON(out, stopped, avm.PendingEvents(), requested);
      } else if (action == "realtime") {
        llvm::StringRef mode = arg(1);
        if (mode == "on") avm.SetRealtime(true);
        else if (mode == "off") avm.SetRealtime(false);
        else if (mode != "status")
          throw std::invalid_argument("usage: avm realtime on|off|status");
        out.Printf("{\"realtime\":%s}\n", avm.Realtime() ? "true" : "false");
      } else if (action == "watch") {
        llvm::StringRef verb = arg(1);
        if (verb == "delete") {
          uint64_t id = unsignedArgument(arg(2), "watchpoint id");
          if (id > UINT32_MAX || !GetTarget().RemoveWatchpointByID(id))
            throw std::invalid_argument("unknown AVM watchpoint id");
          out.Printf("{\"deleted_watchpoint\":%" PRIu64 "}\n", id);
        } else {
          uint32_t kind = verb == "read" ? LLDB_WATCH_TYPE_READ
              : verb == "write" ? LLDB_WATCH_TYPE_WRITE
              : verb == "readwrite" ? LLDB_WATCH_TYPE_READ |
                                       LLDB_WATCH_TYPE_WRITE : 0;
          if (!kind)
            throw std::invalid_argument("usage: avm watch read|write|readwrite <data-address> [size]");
          uint64_t address = unsignedArgument(arg(2), "data address");
          uint64_t size = arg(3).empty() ? 1 : unsignedArgument(arg(3), "watch size");
          if (address < 0x100 || address >= 0xa00 || !size ||
              size > 0xa00 - address)
            throw std::invalid_argument("AVM watchpoint must be within data RAM 0x100-0x9ff");
          Status error;
          WatchpointSP watchpoint = GetTarget().CreateWatchpoint(
              0x01000000 + address, size, nullptr, kind, error);
          if (!watchpoint)
            throw std::runtime_error(error.AsCString("cannot create AVM watchpoint"));
          out.Printf("{\"watchpoint\":%u,\"address\":%" PRIu64
                     ",\"size\":%" PRIu64 ",\"access\":\"%s\"}\n",
                     watchpoint->GetID(), address, size, verb.str().c_str());
        }
      } else if (action == "button") {
        llvm::StringRef verb = arg(1);
        uint8_t mask = avm.Buttons();
        if (verb == "status") {
          out.Printf("{\"pressed_mask\":%u}\n", unsigned(mask));
        } else if (verb == "press" || verb == "release" || verb == "set") {
          if (verb == "set") mask = 0;
          if (args.GetArgumentCount() < 3 && verb != "set")
            throw std::invalid_argument("button name required");
          for (size_t i = 2; i < args.GetArgumentCount(); ++i) {
            uint8_t bit = buttonMask(arg(i));
            if (verb == "release") mask &= ~bit;
            else mask |= bit;
          }
          avm.SetButtons(mask);
          out.Printf("{\"pressed_mask\":%u,\"cycle\":%" PRIu64 "}\n",
                     unsigned(mask), avm.Snapshot().cycles);
        } else {
          throw std::invalid_argument("usage: avm button press|release|set|status [UP RIGHT LEFT DOWN A B]");
        }
      } else if (action == "replay") {
        llvm::StringRef verb = arg(1);
        if (verb == "load") {
          if (arg(2).empty()) throw std::invalid_argument("replay file required");
          std::ifstream input(arg(2).str(), std::ios::binary);
          if (!input) throw std::runtime_error("cannot open replay file");
          std::string json{std::istreambuf_iterator<char>(input), {}};
          auto parsed = llvm::json::parse(json);
          if (!parsed)
            throw std::invalid_argument("invalid replay JSON: " +
                                        llvm::toString(parsed.takeError()));
          auto *root = parsed->getAsObject();
          if (!root || root->getInteger("version") != 1)
            throw std::invalid_argument("unsupported replay schema version");
          if (auto *identity = root->getObject("identity")) {
            auto const &actual = avm.ReplayIdentity();
            auto require_hash = [&](llvm::StringRef key,
                                    llvm::StringRef expected) {
              if (identity->getString(key) != expected)
                throw std::invalid_argument("replay identity mismatch: " +
                                            key.str());
            };
            require_hash("elf_sha256", actual.elf_sha256);
            require_hash("image_sha256", actual.image_sha256);
            require_hash("interpreter_sha256", actual.interpreter_sha256);
            require_hash("eeprom_sha256", actual.eeprom_sha256);
            require_hash("fxsave_sha256", actual.fxsave_sha256);
            if (identity->getInteger("adc_seed") != actual.adc_seed ||
                identity->getBoolean("adc_nondeterminism") !=
                    actual.adc_nondeterminism ||
                identity->getInteger("usb_bus_state") != actual.usb_bus_state)
              throw std::invalid_argument("replay peripheral identity mismatch");
          }
          auto *entries = root->getArray("events");
          if (!entries) throw std::invalid_argument("replay events must be an array");
          uint64_t start = avm.Snapshot().cycles;
          uint64_t previous = 0;
          std::vector<avm_debug::TimedButtons> events;
          for (auto const &entry : *entries) {
            auto *item = entry.getAsObject();
            if (!item) throw std::invalid_argument("replay event must be an object");
            auto when = item->getInteger("cycle");
            auto *buttons = item->getArray("pressed");
            if (!when || *when < 0 || !buttons ||
                uint64_t(*when) < previous ||
                uint64_t(*when) > UINT64_MAX - start)
              throw std::invalid_argument("invalid replay cycle or pressed array");
            uint8_t mask = 0;
            for (auto const &button : *buttons) {
              auto name = button.getAsString();
              if (!name) throw std::invalid_argument("replay button must be a name");
              mask |= buttonMask(*name);
            }
            previous = uint64_t(*when);
            events.push_back({start + previous, mask});
          }
          avm.ScheduleButtons(std::move(events));
          avm.SetReplayFinalCycle(start + previous);
          avm.SetReplayPaused(false);
          out.Printf("{\"loaded\":true,\"start_cycle\":%" PRIu64 ","
                     "\"end_cycle\":%" PRIu64 ",\"pending_events\":%zu}\n",
                     start, avm.ReplayFinalCycle(), avm.PendingEvents());
        } else if (verb == "abort") {
          avm.ClearEvents();
          avm.SetReplayFinalCycle(0);
          avm.SetReplayPaused(false);
          out.PutCString("{\"aborted\":true}\n");
        } else if (verb == "export") {
          if (arg(2).empty())
            throw std::invalid_argument("usage: avm replay export <file.json>");
          auto const &history = avm.ButtonHistory();
          if (history.empty())
            throw std::invalid_argument("no button events to export");
          fs::path path(arg(2).str());
          if (fs::exists(path))
            throw std::invalid_argument("replay export path already exists");
          llvm::json::Object identity;
          auto const &source = avm.ReplayIdentity();
          identity["elf_sha256"] = source.elf_sha256;
          identity["image_sha256"] = source.image_sha256;
          identity["interpreter_sha256"] = source.interpreter_sha256;
          identity["eeprom_sha256"] = source.eeprom_sha256;
          identity["fxsave_sha256"] = source.fxsave_sha256;
          identity["adc_seed"] = source.adc_seed;
          identity["adc_nondeterminism"] = source.adc_nondeterminism;
          identity["usb_bus_state"] = source.usb_bus_state;
          llvm::json::Array events;
          constexpr std::array<std::pair<uint8_t, const char *>, 6> names = {{
              {avm_debug::Up, "UP"}, {avm_debug::Right, "RIGHT"},
              {avm_debug::Left, "LEFT"}, {avm_debug::Down, "DOWN"},
              {avm_debug::A, "A"}, {avm_debug::B, "B"}}};
          uint64_t origin = history.front().cycle;
          for (auto const &event : history) {
            if (event.cycle - origin > INT64_MAX)
              throw std::overflow_error("replay export exceeds JSON cycle range");
            llvm::json::Array pressed;
            for (auto const &[bit, name] : names)
              if (event.pressed & bit)
                pressed.push_back(name);
            llvm::json::Object entry;
            entry["cycle"] = int64_t(event.cycle - origin);
            entry["pressed"] = std::move(pressed);
            events.push_back(std::move(entry));
          }
          llvm::json::Object document;
          document["version"] = 1;
          document["identity"] = std::move(identity);
          document["events"] = std::move(events);
          std::ofstream output(path, std::ios::binary);
          if (!output)
            throw std::runtime_error("cannot create replay export");
          output << llvm::formatv("{0:2}\n",
                                  llvm::json::Value(std::move(document))).str();
          output.close();
          if (!output)
            throw std::runtime_error("replay export write failed");
          out.Printf("{\"exported\":true,\"events\":%zu,\"path\":\"%s\"}\n",
                     history.size(), fs::absolute(path).generic_string().c_str());
        } else if (verb == "status") {
          out.Printf("{\"pending_events\":%zu,\"end_cycle\":%" PRIu64
                     ",\"paused\":%s}\n",
                     avm.PendingEvents(), avm.ReplayFinalCycle(),
                     avm.ReplayPaused() ? "true" : "false");
        } else if (verb == "run" || verb == "resume") {
          uint64_t now = avm.Snapshot().cycles;
          uint64_t end = avm.ReplayFinalCycle();
          if (!end) throw std::invalid_argument("no replay is loaded");
          if (verb == "run" && avm.ReplayPaused())
            throw std::invalid_argument("replay is paused; use avm replay resume");
          avm.SetReplayPaused(false);
          auto stopped = avm.RunFor(end > now ? end - now : 1);
          emitStopJSON(out, stopped, avm.PendingEvents());
        } else if (verb == "pause") {
          avm.SetReplayPaused(true);
          out.PutCString("{\"paused\":true}\n");
        } else {
          throw std::invalid_argument("usage: avm replay load|run|resume|pause|status|abort|export");
        }
      } else if (action == "display") {
        if ((arg(1) != "save" && arg(1) != "capture") || arg(2).empty())
          throw std::invalid_argument("usage: avm display save|capture <file.pgm> [--mode visible|logical|controller]");
        llvm::StringRef mode = "visible";
        if (arg(3) == "--mode") mode = arg(4);
        if (mode != "visible" && mode != "logical" && mode != "controller")
          throw std::invalid_argument("unknown AVM display mode");
        std::vector<uint8_t> pixels = avm.Capture(mode);
        fs::path path(arg(2).str());
        if (fs::exists(path)) throw std::invalid_argument("capture path already exists");
        std::ofstream image(path, std::ios::binary);
        if (!image) throw std::runtime_error("cannot create capture file");
        image << "P5\n128 64\n255\n";
        image.write(reinterpret_cast<char const *>(pixels.data()), pixels.size());
        image.close();
        if (!image) throw std::runtime_error("capture write failed");
        auto hash = llvm::SHA256::hash(llvm::ArrayRef<uint8_t>(pixels));
        out.Printf("{\"mode\":\"%s\",\"width\":128,\"height\":64,"
                   "\"pixel_format\":\"gray8\",\"cycle\":%" PRIu64 ","
                   "\"sha256\":\"%s\",\"path\":\"%s\"}\n",
                   mode.str().c_str(), avm.Snapshot().cycles,
                   llvm::toHex(llvm::ArrayRef<uint8_t>(hash), true).c_str(),
                   fs::absolute(path).generic_string().c_str());
      } else {
        throw std::invalid_argument("usage: avm time|stop|run-for|realtime|watch|button|replay|display");
      }
      result.SetStatus(eReturnStatusSuccessFinishResult);
    } catch (const std::exception &error) {
      llvm::json::Object failure;
      failure["ok"] = false;
      failure["error"] = error.what();
      result.GetErrorStream().PutCString(
          (llvm::formatv("{0}\n", llvm::json::Value(std::move(failure))).str())
              .c_str());
      result.AppendError(error.what());
    }
  }
};

void ProcessAVM::DebuggerInitialize(Debugger &debugger) {
  auto &interpreter = debugger.GetCommandInterpreter();
  interpreter.AddCommand("avm", std::make_shared<AVMCommand>(interpreter), true);
}

// The LLDB register number and DWARF number are deliberately the same.
// The four q registers alias adjacent 16-bit registers.
static constexpr const char *names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "sp", "pc", "cc", "q0", "q1", "q2", "q3"};
static RegisterInfo register_info[15];
static uint32_t register_indices[15];
static RegisterSet register_set;
static llvm::once_flag register_once;

void initializeRegisterInfo() {
  llvm::call_once(register_once, [] {
    uint32_t offset = 0;
    for (uint32_t i = 0; i < 15; ++i) {
      RegisterInfo &info = register_info[i];
      info.name = names[i];
      info.byte_size = i == 9 ? 3 : (i == 10 ? 1 : (i >= 11 ? 4 : 2));
      info.byte_offset = offset;
      info.encoding = eEncodingUint;
      info.format = eFormatHex;
      for (auto &kind : info.kinds)
        kind = LLDB_INVALID_REGNUM;
      info.kinds[eRegisterKindEHFrame] = i;
      info.kinds[eRegisterKindDWARF] = i;
      info.kinds[eRegisterKindLLDB] = i;
      if (i == 8)
        info.kinds[eRegisterKindGeneric] = LLDB_REGNUM_GENERIC_SP;
      else if (i == 9)
        info.kinds[eRegisterKindGeneric] = LLDB_REGNUM_GENERIC_PC;
      else if (i == 10)
        info.kinds[eRegisterKindGeneric] = LLDB_REGNUM_GENERIC_FLAGS;
      offset += info.byte_size;
      register_indices[i] = i;
    }
    register_set.name = "AVM Registers";
    register_set.short_name = "avm";
    register_set.num_registers = 15;
    register_set.registers = register_indices;
  });
}

const RegisterInfo *RegisterContextAVM::GetRegisterInfoAtIndex(size_t index) {
  initializeRegisterInfo();
  return index < 15 ? &register_info[index] : nullptr;
}
const RegisterSet *RegisterContextAVM::GetRegisterSet(size_t index) {
  initializeRegisterInfo();
  return index == 0 ? &register_set : nullptr;
}
bool RegisterContextAVM::ReadRegister(const RegisterInfo *info,
                                      RegisterValue &value) {
  try {
    uint32_t number = info->kinds[eRegisterKindLLDB];
    if (number >= 15)
      return false;
    auto &process = static_cast<ProcessAVM &>(*m_thread.GetProcess());
    avm_debug::Snapshot state = process.Snapshot();
    uint64_t raw = 0;
    if (number < 8)
      raw = state.registers[number];
    else if (number == 8)
      raw = state.sp;
    else if (number == 9)
      raw = state.pc;
    else if (number == 10)
      raw = state.cc;
    else {
      unsigned low = (number - 11) * 2;
      raw = state.registers[low] | (uint32_t(state.registers[low + 1]) << 16);
    }
    return value.SetUInt(raw, info->byte_size);
  } catch (const std::exception &) {
    return false;
  }
}

bool RegisterContextAVM::WriteRegister(const RegisterInfo *info,
                                       const RegisterValue &value) {
  try {
    auto &process = static_cast<ProcessAVM &>(*m_thread.GetProcess());
    process.WriteRegister(info->kinds[eRegisterKindLLDB], value.GetAsUInt64());
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool ThreadAVM::CalculateStopInfo() {
  auto &process = static_cast<ProcessAVM &>(*GetProcess());
  avm_debug::Stop stopped = process.LastStop();
  StopInfoSP info;
  switch (stopped.reason) {
  case avm_debug::StopReason::Breakpoint: {
    BreakpointSiteSP site =
        process.GetBreakpointSiteList().FindByAddress(stopped.state.pc);
    if (site)
      info = StopInfo::CreateStopReasonWithBreakpointSiteID(*this, site->GetID());
    break;
  }
  case avm_debug::StopReason::Step:
    info = StopInfo::CreateStopReasonToTrace(*this);
    break;
  case avm_debug::StopReason::Watchpoint:
    if (stopped.access)
      info = StopInfo::CreateStopReasonWithWatchpointID(
          *this, stopped.access->id);
    break;
  case avm_debug::StopReason::Fault:
    info = StopInfo::CreateStopReasonWithException(*this, "AVM interpreter fault");
    break;
  case avm_debug::StopReason::DebugBreak:
    info = StopInfo::CreateStopReasonWithException(*this, "AVM SYS debug_break");
    break;
  case avm_debug::StopReason::Interrupt:
    info = StopInfo::CreateStopReasonWithInterrupt(*this, 2, "AVM interrupted");
    break;
  default:
    info = StopInfo::CreateStopReasonWithSignal(*this, 5, "AVM stopped");
    break;
  }
  if (!info)
    info = StopInfo::CreateStopReasonToTrace(*this);
  SetStopInfo(info);
  return true;
}

Status ProcessAVM::DoLaunch(Module *module, ProcessLaunchInfo &) {
  if (!module)
    return Status::FromErrorString("AVM launch requires an executable ELF");
  if (HasUnsupportedBreakpointConditions())
    return Status::FromErrorString(
        "AVM breakpoint conditions require unsupported expression execution");
  try {
    auto guard = LockEmulator();
    RemoveTemporaryImage();
    m_replay_final_cycle = 0;
    m_replay_paused = false;
    fs::path elf(module->GetFileSpec().GetPath());
    fs::path bin_dir = fs::path(llvm::sys::fs::getMainExecutable(
                                   "avm-lldb", nullptr)).parent_path();
    auto overridePath = [](const char *name, fs::path fallback) {
      const char *value = std::getenv(name);
      return value && *value ? fs::path(value) : fallback;
    };
    fs::path firmware = overridePath("AVM_LLDB_INTERP",
                                     bin_dir / "avm" / "interp.hex");
    fs::path boundary = overridePath("AVM_LLDB_BOUNDARY",
                                     bin_dir / "avm" / "interp-boundary.json");
    fs::path packer = overridePath("AVM_LLDB_IMAGE_TOOL",
#ifdef _WIN32
                                    bin_dir / "avm-image.exe");
#else
                                    bin_dir / "avm-image");
#endif
    m_image = fs::temp_directory_path() /
        ("avm-lldb-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".bin");
    std::string tool_arg = packer.string();
    std::string image_arg = m_image.string();
    std::string elf_arg = elf.string();
    std::vector<llvm::StringRef> args =
        {tool_arg, "--development", "-o", image_arg, elf_arg};
    std::string execution_error;
    int result = llvm::sys::ExecuteAndWait(tool_arg, args, std::nullopt,
                                            {}, 0, 0, &execution_error);
    if (result != 0)
      return Status::FromErrorStringWithFormat(
          "AVM image packer failed (%d): %s", result, execution_error.c_str());
    avm_debug::Stop stopped = m_emulator.load(elf, m_image, firmware, boundary);
    if (!module->GetObjectFile()->SetLoadAddress(GetTarget(), 0, true))
      return Status::FromErrorString("cannot map AVM ELF sections into LLDB");
    {
      std::lock_guard<std::mutex> lock(m_stop_mutex);
      m_stop = stopped;
    }
    SetID(1);
    SetPrivateState(eStateStopped);
    return {};
  } catch (const std::exception &error) {
    return Status::FromErrorString(error.what());
  }
}

Status ProcessAVM::DoResume(RunDirection direction) {
  if (direction != RunDirection::eRunForward)
    return Status::FromErrorString("AVM reverse execution is unsupported");
  if (HasUnsupportedBreakpointConditions())
    return Status::FromErrorString(
        "AVM breakpoint conditions require unsupported expression execution");
  std::unique_lock<std::recursive_mutex> guard;
  try {
    guard = LockEmulator();
  } catch (const std::exception &error) {
    return Status::FromErrorString(error.what());
  }
  if (m_worker.joinable())
    m_worker.join();
  bool stepping = m_thread && m_thread->GetTemporaryResumeState() == eStateStepping;
  SetPrivateState(eStateRunning);
  m_worker = std::thread([this, stepping] {
    try {
      avm_debug::Stop stopped;
      {
        std::lock_guard<std::recursive_mutex> guard(m_emulator_mutex);
        stopped = stepping ? m_emulator.step()
                           : m_emulator.continue_execution();
      }
      {
        std::lock_guard<std::mutex> lock(m_stop_mutex);
        m_stop = stopped;
      }
      SetPrivateState(eStateStopped);
    } catch (...) {
      {
        std::lock_guard<std::mutex> lock(m_stop_mutex);
        m_stop.reason = avm_debug::StopReason::Fault;
      }
      SetPrivateState(eStateCrashed);
    }
  });
  return {};
}

size_t ProcessAVM::DoReadMemory(addr_t address, void *out, size_t size,
                                Status &error) {
  try {
    auto guard = LockEmulator();
    bool data_space = address >= 0x01000000 && address < 0x01010000;
    bool typed_program_pointer =
        address >= 0x02000000 && address < 0x03000000;
    if (typed_program_pointer)
      address -= 0x02000000;
    if (data_space) {
      uint32_t raw = address - 0x01000000;
      if (raw >= 0xa00) {
        error = Status::FromErrorString("AVM data read is outside guest RAM");
        return 0;
      }
      size = std::min<size_t>(size, 0xa00 - raw);
    } else {
      if (address >= ProgramSize()) {
        error = Status::FromErrorString("AVM program read is outside FX image");
        return 0;
      }
      size = std::min<size_t>(size, ProgramSize() - address);
    }
    std::vector<uint8_t> bytes = data_space
        ? m_emulator.read_data(uint16_t(address), size)
        : m_emulator.read_program(uint32_t(address), size);
    std::memcpy(out, bytes.data(), bytes.size());
    return bytes.size();
  } catch (const std::exception &exception) {
    error = Status::FromErrorString(exception.what());
    return 0;
  }
}

size_t ProcessAVM::DoWriteMemory(addr_t address, const void *bytes,
                                 size_t size, Status &error) {
  try {
    auto guard = LockEmulator();
    if (address < 0x01000100 || address >= 0x01000a00) {
      error = Status::FromErrorString("AVM writes require mapped guest RAM");
      return 0;
    }
    size = std::min<size_t>(size, 0x01000a00 - address);
    auto *first = static_cast<const uint8_t *>(bytes);
    m_emulator.write_data(uint16_t(address),
                          std::vector<uint8_t>(first, first + size));
    return size;
  } catch (const std::exception &exception) {
    error = Status::FromErrorString(exception.what());
    return 0;
  }
}
} // namespace

LLDB_PLUGIN_DEFINE(ProcessAVM)
