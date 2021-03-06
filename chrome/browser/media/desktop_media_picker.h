// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_H_
#define CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_H_

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/media/desktop_media_picker_model.h"
#include "ui/gfx/native_widget_types.h"


// Abstract interface for desktop media picker UI. It's used by Desktop Media
// API to let user choose a desktop media source.
class DesktopMediaPicker {
 public:
  typedef base::Callback<void(DesktopMediaPickerModel::SourceId)> DoneCallback;

  // Creates default implementation of DesktopMediaPicker for the current
  // platform.
  static scoped_ptr<DesktopMediaPicker> Create();

  DesktopMediaPicker() {}
  virtual ~DesktopMediaPicker() {}

  // Shows dialog with list of desktop media sources (screens, windows, tabs)
  // provided by |model| and calls |done_callback| when user chooses one of the
  // sources or closes the dialog.
  virtual void Show(gfx::NativeWindow context,
                    gfx::NativeWindow parent,
                    const string16& app_name,
                    scoped_ptr<DesktopMediaPickerModel> model,
                    const DoneCallback& done_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPicker);
};

#endif  // CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_PICKER_H_
