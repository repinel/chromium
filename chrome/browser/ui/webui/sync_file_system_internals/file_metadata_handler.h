// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SYNC_FILE_SYSTEM_INTERNALS_FILE_METADATA_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SYNC_FILE_SYSTEM_INTERNALS_FILE_METADATA_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/sync_file_system/remote_file_sync_service.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "webkit/browser/fileapi/syncable/sync_status_code.h"

class Profile;

namespace syncfs_internals {

// This class handles messages from WebUI page of chrome://syncfs-internals/
// for the File Metadata tab. It corresponds to browser/resources/
// sync_file_system_internals/file_metadata.html. All methods in this class
// should be called on UI thread.
class FileMetadataHandler : public content::WebUIMessageHandler {
 public:
  explicit FileMetadataHandler(Profile* profile);
  virtual ~FileMetadataHandler();

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages() OVERRIDE;

 private:
  void GetExtensions(const base::ListValue* args);

  void GetFileMetadata(const base::ListValue* args);
  void DidGetFileMetadata(const base::ListValue* files);

  Profile* profile_;
  base::WeakPtrFactory<FileMetadataHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileMetadataHandler);
};
}  // namespace syncfs_internals

#endif  // CHROME_BROWSER_UI_WEBUI_SYNC_FILE_SYSTEM_INTERNALS_FILE_METADATA_HANDLER_H_
