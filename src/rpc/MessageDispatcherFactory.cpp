#include "MessageDispatcherFactory.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_map>

#include "load_balancer/LoadBalancer.h"
#include "message_dispatcher/Composited.h"
#include "name_resolver/NameResolver.h"

using namespace std::literals;

namespace tinyRPC {

namespace {

// [Priority, Factory].
using Factories = std::vector<std::pair<
    int, UniqueFunction<std::unique_ptr<MessageDispatcher>(std::string_view)>>>;

using MessageDispatcherFactory =
    UniqueFunction<std::unique_ptr<MessageDispatcher>(std::string_view)>;
using CatchAllMessageDispatcherFactory =
    UniqueFunction<std::unique_ptr<MessageDispatcher>(std::string_view,
                                                std::string_view)>;
using DefaultMessageDispatcherFactory =
    UniqueFunction<std::unique_ptr<MessageDispatcher>(
        std::string_view, std::string_view, std::string_view)>;

// I don't expect too many different factories for a given scheme. In this case
// linear-scan on a vector should perform better than (unordered_)map lookup.

std::unordered_map<std::string, std::vector<std::pair<std::string, Factories>>>*
GetFactoryRegistry() {
  static std::unordered_map<
      std::string, std::vector<std::pair<std::string, Factories>>>
      registry;
  return registry.Get();
}

// Usable only at startup. If the given subsystem or scheme is not recognized,
// it's added.
Factories* GetOrCreateFactoriesForWriteUnsafe(std::string_view subsys,
                                              std::string_view scheme) {
  auto&& per_subsys = (*GetFactoryRegistry())[subsys];

  for (auto&& [k, factories] : per_subsys) {
    if (k == scheme) {
      return &factories;
    }
  }
  auto&& added = per_subsys.emplace_back();
  added.first = scheme;
  return &added.second;
}

// If the given subsystem or scheme is not recognized, `nullptr` is returned.
const Factories* GetFactoriesForRead(std::string_view subsys,
                                     std::string_view scheme) {
  auto ptr = GetFactoryRegistry()->TryGet(subsys);
  if (!ptr) {
    return nullptr;
  }
  for (auto&& [k, factories] : *ptr) {
    if (k == scheme) {
      return &factories;
    }
  }
  return nullptr;
}

CatchAllMessageDispatcherFactory* GetCatchAllMessageDispatcherFactoryFor(
    std::string_view subsys) {
  static std::unordered_map<std::string, CatchAllMessageDispatcherFactory> registry;
  return &(*registry)[subsys];
}

DefaultMessageDispatcherFactory* GetDefaultMessageDispatcherFactory() {
  static DefaultMessageDispatcherFactory factory =
      [](auto&& subsys, auto&& scheme,
         auto&& address) -> std::unique_ptr<MessageDispatcher> {
    FLARE_LOG_ERROR_EVERY_SECOND(
        "No message dispatcher factory is provided for subsystem [{}], uri "
        "[{}://{}].",
        subsys, scheme, address);
    return nullptr;
  };
  return &factory;
}

}  // namespace

std::unique_ptr<MessageDispatcher> MakeMessageDispatcher(
    std::string_view subsys, std::string_view uri) {
  static constexpr auto kSep = "://"sv;

  auto pos = uri.find_first_of(kSep);
  FLARE_CHECK_NE(pos, std::string_view::npos, "No `scheme` found in URI [{}].",
                 uri);
  auto scheme = uri.substr(0, pos);
  auto address = uri.substr(pos + kSep.size());
  if (auto factories = GetFactoriesForRead(subsys, scheme)) {
    for (auto&& [_ /* priority */, e] : *factories) {
      if (auto ptr = e(address)) {
        return ptr;
      }
    }
  }
  if (auto&& catch_all = *GetCatchAllMessageDispatcherFactoryFor(subsys)) {
    if (auto ptr = catch_all(scheme, address)) {
      return ptr;
    }
  }
  return (*GetDefaultMessageDispatcherFactory())(subsys, scheme, address);
}

void RegisterMessageDispatcherFactoryFor(
    const std::string& subsys, const std::string& scheme, int priority,
    UniqueFunction<std::unique_ptr<MessageDispatcher>(std::string_view)> factory) {
  auto&& factories = GetOrCreateFactoriesForWriteUnsafe(subsys, scheme);
  factories->emplace_back(priority, std::move(factory));

  // If multiple factories with the same priority present, we'd like to make
  // sure the one added first is of higher priority.
  std::stable_sort(factories->begin(), factories->end(),
                   [](auto&& x, auto&& y) { return x.first < y.first; });
}

void SetCatchAllMessageDispatcherFor(
    const std::string& subsys,
    UniqueFunction<std::unique_ptr<MessageDispatcher>(std::string_view,
                                                std::string_view)>
        factory) {
  *GetCatchAllMessageDispatcherFactoryFor(subsys) = std::move(factory);
  // I'm not sure if we should return the old factory. Is it of any use?
}

void SetDefaultMessageDispatcherFactory(
    UniqueFunction<std::unique_ptr<MessageDispatcher>(
        std::string_view, std::string_view, std::string_view)>
        factory) {
  *GetDefaultMessageDispatcherFactory() = std::move(factory);
}

std::unique_ptr<MessageDispatcher> MakeCompositedMessageDispatcher(
    std::string_view resolver, std::string_view load_balancer) {
  auto r = name_resolver_registry.TryGet(resolver);
  auto lb = load_balancer_registry.TryNew(load_balancer);
  if (!r || !lb) {
    return nullptr;
  }
  return std::make_unique<message_dispatcher::Composited>(r, std::move(lb));
}

}  // namespace tinyRPC
