// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/shill_service_client_stub.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "chromeos/dbus/shill_property_changed_observer.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

void ErrorFunction(const std::string& error_name,
                   const std::string& error_message) {
  LOG(ERROR) << "Shill Error: " << error_name << " : " << error_message;
}

void PassStubListValue(const ShillServiceClient::ListValueCallback& callback,
                       base::ListValue* value) {
  callback.Run(*value);
}

void PassStubServiceProperties(
    const ShillServiceClient::DictionaryValueCallback& callback,
    DBusMethodCallStatus call_status,
    const base::DictionaryValue* properties) {
  callback.Run(call_status, *properties);
}

}  // namespace

ShillServiceClientStub::ShillServiceClientStub() : weak_ptr_factory_(this) {
  SetDefaultProperties();
}

ShillServiceClientStub::~ShillServiceClientStub() {
  STLDeleteContainerPairSecondPointers(
      observer_list_.begin(), observer_list_.end());
}

// ShillServiceClient overrides.

void ShillServiceClientStub::AddPropertyChangedObserver(
    const dbus::ObjectPath& service_path,
    ShillPropertyChangedObserver* observer) {
  GetObserverList(service_path).AddObserver(observer);
}

void ShillServiceClientStub::RemovePropertyChangedObserver(
    const dbus::ObjectPath& service_path,
    ShillPropertyChangedObserver* observer) {
  GetObserverList(service_path).RemoveObserver(observer);
}

void ShillServiceClientStub::GetProperties(
    const dbus::ObjectPath& service_path,
    const DictionaryValueCallback& callback) {
  base::DictionaryValue* nested_dict = NULL;
  scoped_ptr<base::DictionaryValue> result_properties;
  DBusMethodCallStatus call_status;
  stub_services_.GetDictionaryWithoutPathExpansion(service_path.value(),
                                                   &nested_dict);
  if (nested_dict) {
    result_properties.reset(nested_dict->DeepCopy());
    // Remove credentials that Shill wouldn't send.
    result_properties->RemoveWithoutPathExpansion(flimflam::kPassphraseProperty,
                                                  NULL);
    call_status = DBUS_METHOD_CALL_SUCCESS;
  } else {
    result_properties.reset(new base::DictionaryValue);
    call_status = DBUS_METHOD_CALL_FAILURE;
  }

  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&PassStubServiceProperties,
                 callback,
                 call_status,
                 base::Owned(result_properties.release())));
}

void ShillServiceClientStub::SetProperty(const dbus::ObjectPath& service_path,
                                         const std::string& name,
                                         const base::Value& value,
                                         const base::Closure& callback,
                                         const ErrorCallback& error_callback) {
  if (!SetServiceProperty(service_path.value(), name, value)) {
    LOG(ERROR) << "Service not found: " << service_path.value();
    error_callback.Run("Error.InvalidService", "Invalid Service");
    return;
  }
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillServiceClientStub::SetProperties(
    const dbus::ObjectPath& service_path,
    const base::DictionaryValue& properties,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  for (base::DictionaryValue::Iterator iter(properties);
       !iter.IsAtEnd(); iter.Advance()) {
    if (!SetServiceProperty(service_path.value(), iter.key(), iter.value())) {
      LOG(ERROR) << "Service not found: " << service_path.value();
      error_callback.Run("Error.InvalidService", "Invalid Service");
      return;
    }
  }
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillServiceClientStub::ClearProperty(
    const dbus::ObjectPath& service_path,
    const std::string& name,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::DictionaryValue* dict = NULL;
  if (!stub_services_.GetDictionaryWithoutPathExpansion(
      service_path.value(), &dict)) {
    error_callback.Run("Error.InvalidService", "Invalid Service");
    return;
  }
  dict->RemoveWithoutPathExpansion(name, NULL);
  // Note: Shill does not send notifications when properties are cleared.
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillServiceClientStub::ClearProperties(
    const dbus::ObjectPath& service_path,
    const std::vector<std::string>& names,
    const ListValueCallback& callback,
    const ErrorCallback& error_callback) {
  base::DictionaryValue* dict = NULL;
  if (!stub_services_.GetDictionaryWithoutPathExpansion(
      service_path.value(), &dict)) {
    error_callback.Run("Error.InvalidService", "Invalid Service");
    return;
  }
  scoped_ptr<base::ListValue> results(new base::ListValue);
  for (std::vector<std::string>::const_iterator iter = names.begin();
      iter != names.end(); ++iter) {
    dict->RemoveWithoutPathExpansion(*iter, NULL);
    // Note: Shill does not send notifications when properties are cleared.
    results->AppendBoolean(true);
  }
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&PassStubListValue,
                 callback, base::Owned(results.release())));
}

void ShillServiceClientStub::Connect(const dbus::ObjectPath& service_path,
                                     const base::Closure& callback,
                                     const ErrorCallback& error_callback) {
  VLOG(1) << "ShillServiceClientStub::Connect: " << service_path.value();
  base::Value* service;
  if (!stub_services_.Get(service_path.value(), &service)) {
    LOG(ERROR) << "Service not found: " << service_path.value();
    error_callback.Run("Error.InvalidService", "Invalid Service");
    return;
  }
  base::TimeDelta delay;
  // Set Associating
  base::StringValue associating_value(flimflam::kStateAssociation);
  SetServiceProperty(service_path.value(),
                     flimflam::kStateProperty,
                     associating_value);
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kEnableStubInteractive)) {
    const int kConnectDelaySeconds = 5;
    delay = base::TimeDelta::FromSeconds(kConnectDelaySeconds);
  }
  // Set Online after a delay
  base::StringValue online_value(flimflam::kStateOnline);
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ShillServiceClientStub::SetProperty,
                 weak_ptr_factory_.GetWeakPtr(),
                 service_path,
                 flimflam::kStateProperty,
                 online_value,
                 base::Bind(&base::DoNothing),
                 error_callback),
      delay);
  callback.Run();
}

void ShillServiceClientStub::Disconnect(const dbus::ObjectPath& service_path,
                                        const base::Closure& callback,
                                        const ErrorCallback& error_callback) {
  base::Value* service;
  if (!stub_services_.Get(service_path.value(), &service)) {
    error_callback.Run("Error.InvalidService", "Invalid Service");
    return;
  }
  base::TimeDelta delay;
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kEnableStubInteractive)) {
    const int kConnectDelaySeconds = 2;
    delay = base::TimeDelta::FromSeconds(kConnectDelaySeconds);
  }
  // Set Idle after a delay
  base::StringValue idle_value(flimflam::kStateIdle);
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ShillServiceClientStub::SetProperty,
                 weak_ptr_factory_.GetWeakPtr(),
                 service_path,
                 flimflam::kStateProperty,
                 idle_value,
                 base::Bind(&base::DoNothing),
                 error_callback),
      delay);
  callback.Run();
}

void ShillServiceClientStub::Remove(const dbus::ObjectPath& service_path,
                                    const base::Closure& callback,
                                    const ErrorCallback& error_callback) {
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillServiceClientStub::ActivateCellularModem(
    const dbus::ObjectPath& service_path,
    const std::string& carrier,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

void ShillServiceClientStub::CompleteCellularActivation(
    const dbus::ObjectPath& service_path,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::MessageLoop::current()->PostTask(FROM_HERE, callback);
}

bool ShillServiceClientStub::CallActivateCellularModemAndBlock(
    const dbus::ObjectPath& service_path,
    const std::string& carrier) {
  return true;
}

void ShillServiceClientStub::GetLoadableProfileEntries(
    const dbus::ObjectPath& service_path,
    const DictionaryValueCallback& callback) {
  // Provide a dictionary with a single { profile_path, service_path } entry
  // if the Profile property is set, or an empty dictionary.
  scoped_ptr<base::DictionaryValue> result_properties(
      new base::DictionaryValue);
  base::DictionaryValue* service_properties =
      GetModifiableServiceProperties(service_path.value());
  if (service_properties) {
    std::string profile_path;
    if (service_properties->GetStringWithoutPathExpansion(
            flimflam::kProfileProperty, &profile_path)) {
      result_properties->SetStringWithoutPathExpansion(
          profile_path, service_path.value());
    }
  } else {
    LOG(WARNING) << "Service not in profile: " << service_path.value();
  }

  DBusMethodCallStatus call_status = DBUS_METHOD_CALL_SUCCESS;
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&PassStubServiceProperties,
                 callback,
                 call_status,
                 base::Owned(result_properties.release())));
}

ShillServiceClient::TestInterface* ShillServiceClientStub::GetTestInterface() {
  return this;
}

// ShillServiceClient::TestInterface overrides.

void ShillServiceClientStub::AddService(const std::string& service_path,
                                        const std::string& name,
                                        const std::string& type,
                                        const std::string& state,
                                        bool add_to_visible_list,
                                        bool add_to_watch_list) {
  AddServiceWithIPConfig(service_path, name, type, state, "",
                         add_to_visible_list, add_to_watch_list);
}

void ShillServiceClientStub::AddServiceWithIPConfig(
    const std::string& service_path,
    const std::string& name,
    const std::string& type,
    const std::string& state,
    const std::string& ipconfig_path,
    bool add_to_visible_list,
    bool add_to_watch_list) {
  DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface()->
      AddManagerService(service_path, add_to_visible_list, add_to_watch_list);

  base::DictionaryValue* properties =
      GetModifiableServiceProperties(service_path);
  properties->SetWithoutPathExpansion(
      flimflam::kSSIDProperty,
      base::Value::CreateStringValue(service_path));
  properties->SetWithoutPathExpansion(
      flimflam::kNameProperty,
      base::Value::CreateStringValue(name));
  properties->SetWithoutPathExpansion(
      flimflam::kTypeProperty,
      base::Value::CreateStringValue(type));
  properties->SetWithoutPathExpansion(
      flimflam::kStateProperty,
      base::Value::CreateStringValue(state));
  if (!ipconfig_path.empty())
    properties->SetWithoutPathExpansion(
        shill::kIPConfigProperty,
        base::Value::CreateStringValue(ipconfig_path));
}

void ShillServiceClientStub::RemoveService(const std::string& service_path) {
  DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface()->
      RemoveManagerService(service_path);

  stub_services_.RemoveWithoutPathExpansion(service_path, NULL);
}

bool ShillServiceClientStub::SetServiceProperty(const std::string& service_path,
                                                const std::string& property,
                                                const base::Value& value) {
  base::DictionaryValue* dict = NULL;
  if (!stub_services_.GetDictionaryWithoutPathExpansion(service_path, &dict))
    return false;

  VLOG(1) << "Service.SetProperty: " << property << " = " << value
          << " For: " << service_path;
  if (property == flimflam::kStateProperty) {
    // If the service went into a connected state, then move it to the top of
    // the list in the manager client.
    // TODO(gauravsh): Generalize to sort services properly to allow for testing
    //  more complex scenarios.
    std::string state;
    if (value.GetAsString(&state) && (state == flimflam::kStateOnline ||
                                      state == flimflam::kStatePortal))  {
      DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface()->
          MoveServiceToIndex(service_path, 0, true);
    }
  }
  dict->SetWithoutPathExpansion(property, value.DeepCopy());
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&ShillServiceClientStub::NotifyObserversPropertyChanged,
                 weak_ptr_factory_.GetWeakPtr(),
                 dbus::ObjectPath(service_path), property));
  return true;
}

const base::DictionaryValue* ShillServiceClientStub::GetServiceProperties(
    const std::string& service_path) const {
  const base::DictionaryValue* properties = NULL;
  stub_services_.GetDictionaryWithoutPathExpansion(service_path, &properties);
  return properties;
}

void ShillServiceClientStub::ClearServices() {
  DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface()->
      ClearManagerServices();

  stub_services_.Clear();
}

void ShillServiceClientStub::SetDefaultProperties() {
  const bool add_to_visible = true;
  const bool add_to_watchlist = true;

  if (!CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kDisableStubEthernet)) {
    AddService("eth1", "eth1",
               flimflam::kTypeEthernet,
               flimflam::kStateOnline,
               add_to_visible, add_to_watchlist);
  }

  // Wifi

  AddService("wifi1", "wifi1",
             flimflam::kTypeWifi,
             flimflam::kStateOnline,
             add_to_visible, add_to_watchlist);
  SetServiceProperty("wifi1",
                     flimflam::kSecurityProperty,
                     base::StringValue(flimflam::kSecurityWep));

  AddService("wifi2", "wifi2_PSK",
             flimflam::kTypeWifi,
             flimflam::kStateIdle,
             add_to_visible, add_to_watchlist);
  SetServiceProperty("wifi2",
                     flimflam::kSecurityProperty,
                     base::StringValue(flimflam::kSecurityPsk));
  base::FundamentalValue strength_value(80);
  SetServiceProperty("wifi2",
                     flimflam::kSignalStrengthProperty,
                     strength_value);

  // Cellular

  AddService("cellular1", "cellular1",
             flimflam::kTypeCellular,
             flimflam::kStateIdle,
             add_to_visible, add_to_watchlist);
  base::StringValue technology_value(flimflam::kNetworkTechnologyGsm);
  SetServiceProperty("cellular1",
                     flimflam::kNetworkTechnologyProperty,
                     technology_value);
  SetServiceProperty("cellular1",
                     flimflam::kActivationStateProperty,
                     base::StringValue(flimflam::kActivationStateNotActivated));
  SetServiceProperty("cellular1",
                     flimflam::kRoamingStateProperty,
                     base::StringValue(flimflam::kRoamingStateHome));

  // VPN

  // Set the "Provider" dictionary properties. Note: when setting these in
  // Shill, "Provider.Type", etc keys are used, but when reading the values
  // "Provider" . "Type", etc keys are used. Here we are setting the values
  // that will be read (by the UI, tests, etc).
  base::DictionaryValue provider_properties;
  provider_properties.SetString(flimflam::kTypeProperty,
                                flimflam::kProviderOpenVpn);
  provider_properties.SetString(flimflam::kHostProperty, "vpn_host");

  AddService("vpn1", "vpn1",
             flimflam::kTypeVPN,
             flimflam::kStateOnline,
             add_to_visible, add_to_watchlist);
  SetServiceProperty("vpn1",
                     flimflam::kProviderProperty,
                     provider_properties);

  AddService("vpn2", "vpn2",
             flimflam::kTypeVPN,
             flimflam::kStateOffline,
             add_to_visible, add_to_watchlist);
  SetServiceProperty("vpn2",
                     flimflam::kProviderProperty,
                     provider_properties);
}

void ShillServiceClientStub::NotifyObserversPropertyChanged(
    const dbus::ObjectPath& service_path,
    const std::string& property) {
  base::DictionaryValue* dict = NULL;
  std::string path = service_path.value();
  if (!stub_services_.GetDictionaryWithoutPathExpansion(path, &dict)) {
    LOG(ERROR) << "Notify for unknown service: " << path;
    return;
  }
  base::Value* value = NULL;
  if (!dict->GetWithoutPathExpansion(property, &value)) {
    LOG(ERROR) << "Notify for unknown property: "
               << path << " : " << property;
    return;
  }
  FOR_EACH_OBSERVER(ShillPropertyChangedObserver,
                    GetObserverList(service_path),
                    OnPropertyChanged(property, *value));
}

base::DictionaryValue* ShillServiceClientStub::GetModifiableServiceProperties(
    const std::string& service_path) {
  base::DictionaryValue* properties = NULL;
  if (!stub_services_.GetDictionaryWithoutPathExpansion(
      service_path, &properties)) {
    properties = new base::DictionaryValue;
    stub_services_.Set(service_path, properties);
  }
  return properties;
}

ShillServiceClientStub::PropertyObserverList&
ShillServiceClientStub::GetObserverList(const dbus::ObjectPath& device_path) {
  std::map<dbus::ObjectPath, PropertyObserverList*>::iterator iter =
      observer_list_.find(device_path);
  if (iter != observer_list_.end())
    return *(iter->second);
  PropertyObserverList* observer_list = new PropertyObserverList();
  observer_list_[device_path] = observer_list;
  return *observer_list;
}

}  // namespace chromeos
