// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/user_chooser_ui.h"

#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/signin/user_chooser_screen_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/webui/web_ui_util.h"

namespace {
  // JS file names.
  const char kStringsJSPath[] = "strings.js";
  const char kUserChooserJSPath[] = "user_chooser.js";
  const char kHeaderBarJSPath[] = "header_bar.js";
  const char kAccountPickerJSPath[] = "screen_account_picker.js";
}

UserChooserUI::UserChooserUI(content::WebUI* web_ui)
  : WebUIController(web_ui) {
  // The web_ui object takes ownership of the handler, and will
  // destroy it when it (the WebUI) is destroyed.
  user_chooser_screen_handler_ = new UserChooserScreenHandler();
  web_ui->AddMessageHandler(user_chooser_screen_handler_);

  base::DictionaryValue localized_strings;
  GetLocalizedStrings(&localized_strings);

  Profile* profile = Profile::FromWebUI(web_ui);
  // Set up the chrome://user-chooser/ source.
  content::WebUIDataSource::Add(profile, CreateUIDataSource(localized_strings));

#if defined(ENABLE_THEMES)
  // Set up the chrome://theme/ source
  ThemeSource* theme = new ThemeSource(profile);
  content::URLDataSource::Add(profile, theme);
#endif
}

UserChooserUI::~UserChooserUI() {
}

content::WebUIDataSource* UserChooserUI::CreateUIDataSource(
    const base::DictionaryValue& localized_strings) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIUserChooserHost);
  source->SetUseJsonJSFormatV2();
  source->AddLocalizedStrings(localized_strings);
  source->SetJsonPath(kStringsJSPath);

  source->SetDefaultResource(IDR_USER_CHOOSER_HTML);
  source->AddResourcePath(kUserChooserJSPath, IDR_USER_CHOOSER_JS);

  return source;
}

void UserChooserUI::GetLocalizedStrings(
    base::DictionaryValue* localized_strings) {
  user_chooser_screen_handler_->GetLocalizedValues(localized_strings);
  webui::SetFontAndTextDirection(localized_strings);

#if defined(GOOGLE_CHROME_BUILD)
  localized_strings->SetString("buildType", "chrome");
#else
  localized_strings->SetString("buildType", "chromium");
#endif
}

