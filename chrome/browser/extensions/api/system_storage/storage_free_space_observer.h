// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_EXTENSIONS_API_SYSTEM_STORAGE_STORAGE_FREE_SPACE_OBSERVER_H_
#define CHROME_BROWSER_EXTENSIONS_API_SYSTEM_STORAGE_STORAGE_FREE_SPACE_OBSERVER_H_

#include <string>

namespace extensions {

// Observes the storage free space changes.
//
// StorageInfoProvider class maintains a StorageFreeSpaceObserver list for
// storage devices' free space change event.
class StorageFreeSpaceObserver {
 public:
  // Called when the storage free space changes.
  virtual void OnFreeSpaceChanged(const std::string& transient_id,
                                  double old_value, /* in bytes */
                                  double new_value  /* in bytes */) = 0;
 protected:
  virtual ~StorageFreeSpaceObserver() {}
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SYSTEM_STORAGE_STORAGE_FREE_SPACE_OBSERVER_H_

