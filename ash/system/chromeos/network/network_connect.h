// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_CONNECT_H
#define ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_CONNECT_H

#include <string>

#include "ash/ash_export.h"
#include "base/strings/string16.h"

namespace ash {
namespace network_connect {

// Request a network connection and handle any errors and notifications.
ASH_EXPORT void ConnectToNetwork(const std::string& service_path);

// Returns the localized string for shill error string |error|.
ASH_EXPORT base::string16 ErrorString(const std::string& error);

}  // network_connect
}  // ash

#endif  // ASH_SYSTEM_CHROMEOS_NETWORK_NETWORK_CONNECT_H
