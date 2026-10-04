//===- AVMStack.cpp - Post-codegen AVM stack analysis
//----------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "AVMStack.h"
#include "InputFiles.h"
#include "InputSection.h"
#include "Relocations.h"
#include "SymbolTable.h"
#include "Symbols.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/BinaryFormat/AVM.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/LEB128.h"
#include <limits>

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace lld;
using namespace lld::elf;

namespace {
using Address = std::pair<InputSectionBase *, uint64_t>;
constexpr unsigned None = std::numeric_limits<unsigned>::max();
struct Edge {
  unsigned target;
  uint64_t args;
  bool tail;
  bool cut = false;
  bool indirect = false;
};
struct Node {
  Address address;
  StringRef name;
  uint64_t frame = 0;
  bool frameKnown = false;
  bool callsKnown = false;
  bool incompleteMarker = false;
  bool complete = true;
  SmallVector<Edge, 0> edges;
  uint64_t peak = 0;
  unsigned best = None;
  unsigned index = None, low = None, component = None;
  bool active = false;
  bool visiting = false;
};

class Analysis {
  Ctx &ctx;
  DenseMap<Address, unsigned> ids;
  DenseMap<Address, StringRef> names;
  SmallVector<Node, 0> nodes;
  SmallVector<std::pair<unsigned, uint64_t>, 0> roots;

  Address address(Symbol &s, int64_t addend = 0) {
    auto *d = dyn_cast<Defined>(&s);
    auto *sec = d ? dyn_cast_or_null<InputSectionBase>(d->section) : nullptr;
    if (!sec || !sec->isLive() || !(sec->flags & SHF_EXECINSTR) ||
        sec == &InputSection::discarded)
      return {};
    return {sec, d->value + addend};
  }

  unsigned node(Address a) {
    auto [it, inserted] = ids.try_emplace(a, nodes.size());
    if (inserted) {
      nodes.emplace_back();
      nodes.back().address = a;
      nodes.back().name = names.lookup(a);
    }
    return it->second;
  }

  template <class RelTy>
  void readAddresses(InputSectionBase &sec, DenseMap<uint64_t, Address> &result,
                     Relocs<RelTy> rels) {
    for (const auto &r : rels) {
      if (r.getType(false) != R_AVM_DEBUG24 && r.getType(false) != R_AVM_PROG24)
        continue;
      int64_t addend = getAddend<ELF32LE>(r);
      if constexpr (std::is_same_v<RelTy, ELF32LE::Rel>) {
        auto bytes = sec.content();
        if (r.r_offset > bytes.size() || bytes.size() - r.r_offset < 3) {
          Err(ctx) << &sec << ": truncated AVM stack relocation";
          continue;
        }
        addend = bytes[r.r_offset] | bytes[r.r_offset + 1] << 8 |
                 bytes[r.r_offset + 2] << 16;
      }
      result[r.r_offset] =
          address(sec.getFile<ELF32LE>()->getRelocTargetSym(r), addend);
    }
  }

  DenseMap<uint64_t, Address> addresses(InputSectionBase &sec) {
    DenseMap<uint64_t, Address> result;
    using ELFT = ELF32LE;
    invokeOnRelocs(sec, readAddresses, sec, result);
    return result;
  }

  void readFrames(InputSectionBase &sec) {
    auto reloc = addresses(sec);
    auto bytes = sec.content();
    uint64_t pos = 0;
    while (pos < bytes.size()) {
      uint64_t offset = pos;
      if (bytes.size() - pos < 4) {
        Err(ctx) << &sec << ": truncated AVM .stack_sizes record";
        return;
      }
      pos += 3;
      unsigned count;
      const char *error = nullptr;
      uint64_t frame = decodeULEB128(bytes.data() + pos, &count,
                                     bytes.data() + bytes.size(), &error);
      if (error) {
        Err(ctx) << &sec << ": invalid AVM stack size: " << error;
        return;
      }
      pos += count;
      Address a = reloc.lookup(offset);
      if (!a.first)
        continue; // Discarded COMDAT target or unavailable legacy metadata.
      Node &n = nodes[node(a)];
      if (n.frameKnown && n.frame != frame)
        Err(ctx) << &sec << ": conflicting AVM frame sizes";
      n.frame = frame;
      n.frameKnown = true;
    }
  }

  void readCalls(InputSectionBase &sec) {
    auto reloc = addresses(sec);
    auto bytes = sec.content();
    if (bytes.size() % AVM::StackCallRecordSize) {
      Err(ctx) << &sec << ": truncated .avm.stackcalls record";
      return;
    }
    for (uint64_t pos = 0; pos < bytes.size();
         pos += AVM::StackCallRecordSize) {
      unsigned flags = bytes[pos + 8];
      if ((flags & ~(AVM::StackCallIndirect | AVM::StackCallTail |
                     AVM::StackCallIncomplete | AVM::StackCallFunction)) ||
          ((flags & (AVM::StackCallIncomplete | AVM::StackCallFunction)) &&
           (flags & (AVM::StackCallIndirect | AVM::StackCallTail)))) {
        Err(ctx) << &sec << ": unknown AVM stack call flags";
        continue;
      }
      Address caller = reloc.lookup(pos);
      if (!reloc.count(pos)) {
        Err(ctx) << &sec << ": missing AVM stack caller relocation";
        continue;
      }
      if (!caller.first)
        continue;
      unsigned from = node(caller);
      if (flags & AVM::StackCallFunction) {
        nodes[from].callsKnown = true;
        if (flags & AVM::StackCallIncomplete) {
          nodes[from].incompleteMarker = true;
          nodes[from].complete = false;
        }
        continue;
      }
      if (flags & AVM::StackCallIncomplete) {
        nodes[from].incompleteMarker = true;
        nodes[from].complete = false;
        continue;
      }
      Address callee = reloc.lookup(pos + 3);
      unsigned to = callee.first && !(flags & AVM::StackCallIndirect)
                        ? node(callee)
                        : None;
      nodes[from].edges.push_back(
          {to, support::endian::read16le(bytes.data() + pos + 6),
           bool(flags & AVM::StackCallTail), false,
           bool(flags & AVM::StackCallIndirect)});
    }
  }

  // Iterative Tarjan also supplies a DFS finish order. Cut only DFS back
  // edges, retaining the other concrete paths within recursive components.
  // A cut edge still proves the cost of entering its target's fixed frame;
  // it does not recursively reuse a previously calculated peak. This avoids
  // inventing a recursion depth while preserving finite overflow witnesses.
  SmallVector<unsigned, 0> components() {
    struct Visit {
      unsigned id, next;
    };
    SmallVector<Visit, 0> work;
    SmallVector<unsigned, 0> active, order;
    unsigned index = 0, component = 0;
    auto enter = [&](unsigned id) {
      nodes[id].index = nodes[id].low = index++;
      nodes[id].active = true;
      nodes[id].visiting = true;
      active.push_back(id);
      work.push_back({id, 0});
    };
    for (auto [root, cost] : roots) {
      if (nodes[root].index != None)
        continue;
      enter(root);
      while (!work.empty()) {
        auto &visit = work.back();
        Node &n = nodes[visit.id];
        if (visit.next < n.edges.size()) {
          Edge &e = n.edges[visit.next++];
          if (e.target == None)
            continue;
          Node &target = nodes[e.target];
          if (target.index == None) {
            enter(e.target);
            continue;
          }
          if (target.active)
            n.low = std::min(n.low, target.index);
          // Tarjan's SCC stack includes finished siblings. Only the DFS
          // ancestor stack defines edges that must be cut for longest paths.
          if (target.visiting)
            e.cut = true;
          continue;
        }
        unsigned id = visit.id;
        if (n.low == n.index) {
          unsigned member;
          unsigned size = 0;
          do {
            member = active.pop_back_val();
            nodes[member].active = false;
            nodes[member].component = component;
            ++size;
          } while (member != id);
          ++component;
          if (size > 1)
            n.complete = false;
        }
        work.pop_back();
        n.visiting = false;
        order.push_back(id);
        if (!work.empty())
          nodes[work.back().id].low =
              std::min(nodes[work.back().id].low, n.low);
      }
    }
    return order;
  }

  static uint64_t add(uint64_t a, uint64_t b) {
    return a > UINT64_MAX - b ? UINT64_MAX : a + b;
  }

  std::string functionName(const Node &n) {
    StringRef name = n.name.empty() ? n.address.first->name : n.name;
    return ctx.arg.demangle ? demangle(name.str()) : name.str();
  }

  std::string path(unsigned id, uint64_t entryCost) {
    std::string result;
    raw_string_ostream os(result);
    if (entryCost)
      os << "  callback entry: return address " << entryCost << "\n";
    for (;;) {
      Node &n = nodes[id];
      os << "  " << functionName(n) << ": frame " << n.frame;
      if (!n.frameKnown)
        os << " (unknown; lower bound)";
      os << "\n";
      if (n.best == None)
        break;
      Edge &e = n.edges[n.best];
      os << "    " << (e.tail ? "tail transfer" : "call") << ": return address "
         << (e.tail ? 0 : AVM::ReturnAddressSize) << ", outgoing arguments "
         << e.args;
      if (e.tail)
        os << "; caller frame released";
      os << "\n";
      if (e.target == None) {
        os << "  <unknown target>\n";
        break;
      }
      if (e.cut) {
        Node &target = nodes[e.target];
        os << "  " << functionName(target) << ": frame " << target.frame
           << " (recursive continuation unknown)\n";
        break;
      }
      id = e.target;
    }
    return result;
  }

  // One shortest route per reachable function, rather than exponentially many
  // caller paths. Iterative traversal also handles recursive graphs safely.
  void printGaps() {
    struct Route {
      bool seen = false;
      unsigned parent = None;
      unsigned edge = None;
      uint64_t entryCost = 0;
    };
    SmallVector<Route, 0> routes(nodes.size());
    SmallVector<unsigned, 0> queue;
    auto sortedRoots = roots;
    // The ELF entry precedes callbacks. Sort callback roots for repeatable
    // reports even when their relocation maps have a different iteration order.
    auto rootLess = [&](auto a, auto b) {
      if (a.second != b.second)
        return a.second < b.second;
      const Node &x = nodes[a.first], &y = nodes[b.first];
      StringRef xn = x.name.empty() ? x.address.first->name : x.name;
      StringRef yn = y.name.empty() ? y.address.first->name : y.name;
      if (xn != yn)
        return xn < yn;
      if (x.address.first->name != y.address.first->name)
        return x.address.first->name < y.address.first->name;
      return x.address.second < y.address.second;
    };
    llvm::sort(sortedRoots, rootLess);
    for (auto [id, cost] : sortedRoots) {
      if (routes[id].seen)
        continue;
      routes[id].seen = true;
      routes[id].entryCost = cost;
      queue.push_back(id);
    }
    for (size_t i = 0; i < queue.size(); ++i) {
      unsigned from = queue[i];
      for (unsigned j = 0; j < nodes[from].edges.size(); ++j) {
        unsigned to = nodes[from].edges[j].target;
        if (to == None || routes[to].seen)
          continue;
        routes[to] = {true, from, j, 0};
        queue.push_back(to);
      }
    }

    uint64_t total = 0, shown = 0;
    auto report = [&](unsigned id, StringRef reason, unsigned edge = None) {
      ++total;
      if (shown >= ctx.arg.avmStackGapLimit)
        return;
      ++shown;
      std::string result;
      raw_string_ostream os(result);
      os << "AVM stack gap " << shown << ": " << functionName(nodes[id]) << ": "
         << reason << "\nwitness path (known costs only):\n";
      SmallVector<unsigned, 0> chain;
      for (unsigned at = id; at != None; at = routes[at].parent)
        chain.push_back(at);
      uint64_t cost = routes[chain.back()].entryCost;
      if (cost)
        os << "  callback entry: return address " << cost << "\n";
      auto printNode = [&](unsigned at) {
        const Node &n = nodes[at];
        os << "  " << functionName(nodes[at]) << ": frame " << n.frame;
        if (!n.frameKnown)
          os << " (unknown; lower bound)";
        os << "\n";
      };
      auto printEdge = [&](unsigned from, const Edge &e) {
        os << "    " << (e.tail ? "tail transfer" : "call")
           << ": return address " << (e.tail ? 0 : AVM::ReturnAddressSize)
           << ", outgoing arguments " << e.args;
        if (e.tail) {
          os << "; caller frame released";
          cost -= nodes[from].frame;
        }
        os << "\n";
        cost = add(cost, add(e.args, e.tail ? 0 : AVM::ReturnAddressSize));
      };
      unsigned previous = None;
      for (unsigned at : llvm::reverse(chain)) {
        if (previous != None)
          printEdge(previous, nodes[previous].edges[routes[at].edge]);
        printNode(at);
        cost = add(cost, nodes[at].frame);
        previous = at;
      }
      if (edge != None) {
        const Edge &e = nodes[id].edges[edge];
        printEdge(id, e);
        if (e.target == None)
          os << "  <unknown target>\n";
        else {
          printNode(e.target);
          cost = add(cost, nodes[e.target].frame);
          os << "    <recursive continuation unknown>\n";
        }
      }
      os << "  known stack at gap: " << cost << " bytes (lower bound)\n";
      Msg(ctx) << result;
    };
    for (unsigned id : queue) {
      const Node &n = nodes[id];
      if (!n.frameKnown || !n.callsKnown) {
        StringRef reason = !n.frameKnown && !n.callsKnown
                               ? "missing frame and call metadata"
                           : !n.frameKnown ? "missing frame metadata"
                                           : "missing call metadata";
        report(id, reason);
      }
      if (n.incompleteMarker)
        report(id, "compiler-marked incomplete (dynamic stack allocation or "
                   "opaque inline assembly)");
      for (unsigned i = 0; i < n.edges.size(); ++i) {
        const Edge &e = n.edges[i];
        if (e.target == None)
          report(id,
                 e.indirect ? "unresolved indirect call target"
                            : "unavailable direct call target",
                 i);
        else if (e.cut)
          report(id, "recursive call continuation", i);
      }
    }
    Msg(ctx) << "AVM stack analysis gaps: " << total
             << "; paths shown: " << shown << "; omitted: " << total - shown;
  }

public:
  explicit Analysis(Ctx &ctx) : ctx(ctx) {}

  void run() {
    // Index symbols once: section-symbol + addend relocations and aliases
    // identify the same function, including functions sharing a text section.
    for (ELFFileBase *file : ctx.objectFiles)
      for (Symbol *s : file->getSymbols()) {
        Address a = address(*s);
        if (a.first && !s->isSection() && !s->getName().empty() &&
            (s->isFunc() || !names.count(a)))
          names[a] = s->getName();
      }

    for (InputSectionBase *sec : ctx.inputSections)
      if (sec->isLive() && sec->name == ".stack_sizes")
        readFrames(*sec);
    for (InputSectionBase *sec : ctx.inputSections)
      if (sec->isLive() && sec->name == ".avm.stackcalls")
        readCalls(*sec);

    // The loader jumps to the ELF entry with no return record. Address-taken
    // callbacks in live data (including init/fini arrays) are additional
    // entry lower bounds with a return record. Their invoking indirect branch
    // remains incomplete; no fabricated caller stack is added.
    if (Symbol *entry = ctx.symtab->find(ctx.arg.entry)) {
      Address a = address(*entry);
      if (a.first)
        roots.emplace_back(node(a), 0);
    }
    for (InputSectionBase *sec : ctx.inputSections)
      if (sec->isLive() && (sec->flags & SHF_ALLOC) &&
          !(sec->flags & SHF_EXECINSTR))
        for (auto [offset, a] : addresses(*sec))
          if (a.first && ids.count(a))
            roots.emplace_back(node(a), AVM::ReturnAddressSize);

    auto order = components();
    for (unsigned id : order) {
      Node &n = nodes[id];
      n.peak = n.frame;
      n.complete &= n.frameKnown && n.callsKnown;
      for (unsigned i = 0; i < n.edges.size(); ++i) {
        Edge &e = n.edges[i];
        uint64_t below = 0;
        if (e.target == None) {
          n.complete = false;
        } else if (e.cut) {
          below = nodes[e.target].frame;
          n.complete = false;
        } else {
          below = nodes[e.target].peak;
          n.complete &= nodes[e.target].complete;
        }
        uint64_t peak =
            e.tail
                ? add(e.args, below)
                : add(n.frame, add(e.args, add(AVM::ReturnAddressSize, below)));
        if (peak > n.peak) {
          n.peak = peak;
          n.best = i;
        }
      }
    }
    uint64_t peak = 0, cost = 0;
    unsigned best = None;
    bool complete = !roots.empty();
    for (auto [id, entryCost] : roots) {
      complete &= nodes[id].complete;
      uint64_t candidate = add(entryCost, nodes[id].peak);
      if (best == None || candidate > peak) {
        best = id;
        peak = candidate;
        cost = entryCost;
      }
    }
    if (peak > AVM::StackLimit)
      Err(ctx) << "AVM maximum provable stack usage is " << peak
               << " bytes; limit is " << AVM::StackLimit
               << " bytes\nmaximum stack path:\n"
               << path(best, cost);
    else if (ctx.arg.avmPrintStackUsage)
      Msg(ctx) << "AVM maximum provable stack usage is " << peak
               << " bytes; complete bound: " << (complete ? "yes" : "no")
               << (best == None ? "\n"
                                : "\nmaximum stack path:\n" + path(best, cost));
    if (ctx.arg.avmPrintStackGaps)
      printGaps();

    // Preserve records in relocatable links, but consume them in final links.
    for (InputSectionBase *sec : ctx.inputSections)
      if (sec->name == ".stack_sizes" || sec->name == ".avm.stackcalls") {
        sec->markDead();
        for (InputSection *dep : sec->dependentSections)
          dep->markDead();
      }
  }
};
} // namespace

void elf::analyzeAVMStack(Ctx &ctx) {
  if (ctx.arg.emachine == EM_AVM && !ctx.arg.relocatable)
    Analysis(ctx).run();
}
