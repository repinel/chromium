// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_net_log_parameters.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/disk_cache/simple/simple_entry_impl.h"

namespace {

base::Value* NetLogSimpleEntryCreationCallback(
    const disk_cache::SimpleEntryImpl* entry,
    net::NetLog::LogLevel log_level ALLOW_UNUSED) {
  base::DictionaryValue* dict = new base::DictionaryValue();
  dict->SetString("entry_hash",
                  base::StringPrintf("%#016" PRIx64, entry->entry_hash()));
  return dict;
}

}  // namespace

namespace disk_cache {

net::NetLog::ParametersCallback CreateNetLogSimpleEntryCreationCallback(
    const SimpleEntryImpl* entry) {
  DCHECK(entry);
  return base::Bind(&NetLogSimpleEntryCreationCallback, entry);
}

}  // namespace disk_cache
