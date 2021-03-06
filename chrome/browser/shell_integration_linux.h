// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_
#define CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_

#include <string>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "chrome/browser/shell_integration.h"
#include "url/gurl.h"

namespace base {
class Environment;
}

namespace ShellIntegrationLinux {

// Returns filename of the desktop shortcut used to launch the browser.
std::string GetDesktopName(base::Environment* env);

// Returns name of the browser icon (without a path or file extension).
std::string GetIconName();

// Returns the set of locations in which shortcuts are installed for the
// extension with |extension_id| in |profile_path|.
// This searches the file system for .desktop files in appropriate locations. A
// shortcut with NoDisplay=true causes hidden to become true, instead of
// in_applications_menu.
ShellIntegration::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id);

// Version of GetExistingShortcutLocations which takes an explicit path
// to the user's desktop directory. Useful for testing.
// If |desktop_path| is empty, the desktop is not searched.
ShellIntegration::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id,
    const base::FilePath& desktop_path);

// Returns the contents of an existing .desktop file installed in the system.
// Searches the "applications" subdirectory of each XDG data directory for a
// file named |desktop_filename|. If the file is found, populates |output| with
// its contents and returns true. Else, returns false.
bool GetExistingShortcutContents(base::Environment* env,
                                 const base::FilePath& desktop_filename,
                                 std::string* output);

// Returns filename for .desktop file based on |url|, sanitized for security.
base::FilePath GetWebShortcutFilename(const GURL& url);

// Returns filename for .desktop file based on |profile_path| and
// |extension_id|, sanitized for security.
base::FilePath GetExtensionShortcutFilename(const base::FilePath& profile_path,
                                            const std::string& extension_id);

// Returns contents for .desktop file based on |url| and |title|. If
// |no_display| is true, the shortcut will not be visible to the user in menus.
std::string GetDesktopFileContents(const base::FilePath& chrome_exe_path,
                                   const std::string& app_name,
                                   const GURL& url,
                                   const std::string& extension_id,
                                   const base::FilePath& extension_path,
                                   const string16& title,
                                   const std::string& icon_name,
                                   const base::FilePath& profile_path,
                                   bool no_display);

// Returns contents for .directory file named |title| with icon |icon_name|. If
// |icon_name| is empty, will use the Chrome icon.
std::string GetDirectoryFileContents(const string16& title,
                                     const std::string& icon_name);

// Create shortcuts on the desktop or in the application menu (as specified by
// |shortcut_info|), for the web page or extension in |shortcut_info|.
// For extensions, duplicate shortcuts are avoided, so if a requested shortcut
// already exists it is deleted first.
bool CreateDesktopShortcut(
    const ShellIntegration::ShortcutInfo& shortcut_info,
    const ShellIntegration::ShortcutLocations& creation_locations);

// Delete any desktop shortcuts on desktop or in the application menu that have
// been added for the extension with |extension_id| in |profile_path|.
void DeleteDesktopShortcuts(const base::FilePath& profile_path,
                            const std::string& extension_id);

}  // namespace ShellIntegrationLinux

#endif  // CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_
