// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/pairing_registry_delegate.h"

#include "base/task_runner.h"

namespace remoting {

using protocol::PairingRegistry;

scoped_refptr<PairingRegistry> CreatePairingRegistry(
    scoped_refptr<base::TaskRunner> task_runner) {
  scoped_refptr<PairingRegistry> pairing_registry;
  scoped_ptr<PairingRegistry::Delegate> delegate(
      CreatePairingRegistryDelegate(task_runner));
  if (delegate) {
    pairing_registry = new PairingRegistry(delegate.Pass());
  }
  return pairing_registry;
}

}  // namespace remoting
