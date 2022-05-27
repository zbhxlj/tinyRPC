#include "ServiceMethodLocator.h"

namespace tinyRPC::protobuf {

void ServiceMethodLocator::RegisterMethodProvider(
    LocatorProviderCallback init, LocatorProviderCallback fini) {
  providers_.emplace_back(std::move(init), std::move(fini));
}

void ServiceMethodLocator::AddService(
    const google::protobuf::ServiceDescriptor* service_desc) {
  std::unique_lock lk(services_lock_);
  if (services_[service_desc]++) {
    return;  // Was there.
  }
  lk.unlock();

  for (int i = 0; i != service_desc->method_count(); ++i) {
    auto method = service_desc->method(i);
    for (auto&& e : providers_) {
      e.first(method);
    }
  }
}

void ServiceMethodLocator::DeleteService(
    const google::protobuf::ServiceDescriptor* service_desc) {
  std::unique_lock lk(services_lock_);
  if (--services_[service_desc]) {
    return;  // Was there.
  }
  lk.unlock();

  for (int i = 0; i != service_desc->method_count(); ++i) {
    auto method = service_desc->method(i);
    for (auto&& e : providers_) {
      e.second(method);
    }
  }
}

std::vector<const google::protobuf::ServiceDescriptor*>
ServiceMethodLocator::GetAllServices() const {
  std::unique_lock lk(services_lock_);
  std::vector<const google::protobuf::ServiceDescriptor*> services;
  for (auto&& service : services_) {
    services.push_back(service.first);
  }
  return services;
}

ServiceMethodLocator::ServiceMethodLocator() = default;

}  // namespace tinyRPC::protobuf
