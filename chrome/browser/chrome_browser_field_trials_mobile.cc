// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_field_trials_mobile.h"

#include <string>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/prefs/pref_service.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"

namespace chrome {

namespace {

// Governs the rollout of the compression proxy for Chrome on mobile platforms.
// Always enabled in DEV and BETA versions.
// Stable percentage will be controlled from server.
void DataCompressionProxyFieldTrial() {
  const char kDataCompressionProxyFieldTrialName[] =
      "DataCompressionProxyRollout";
  const base::FieldTrial::Probability kDataCompressionProxyDivisor = 1000;

  // 10/1000 = 1% for starters.
  const base::FieldTrial::Probability kDataCompressionProxyStable = 10;
  const char kEnabled[] = "Enabled";
  const char kDisabled[] = "Disabled";

  // Find out if this is a stable channel.
  const bool kIsStableChannel =
      chrome::VersionInfo::GetChannel() == chrome::VersionInfo::CHANNEL_STABLE;

  // Experiment enabled until Jan 1, 2015. By default, disabled.
  scoped_refptr<base::FieldTrial> trial(
      base::FieldTrialList::FactoryGetFieldTrial(
          kDataCompressionProxyFieldTrialName, kDataCompressionProxyDivisor,
          kDisabled, 2015, 1, 1, base::FieldTrial::ONE_TIME_RANDOMIZED, NULL));

  // Non-stable channels will run with probability 1.
  const int kEnabledGroup = trial->AppendGroup(
      kEnabled,
      kIsStableChannel ?
          kDataCompressionProxyStable : kDataCompressionProxyDivisor);

  const int v = trial->group();
  VLOG(1) << "DataCompression proxy enabled group id: " << kEnabledGroup
          << ". Selected group id: " << v;
}

void NewTabButtonInToolbarFieldTrial(const CommandLine& parsed_command_line) {
  // Do not enable this field trials for tablet devices.
  if (parsed_command_line.HasSwitch(switches::kTabletUI))
    return;

  const char kPhoneNewTabToolbarButtonFieldTrialName[] =
      "PhoneNewTabToolbarButton";
  const base::FieldTrial::Probability kPhoneNewTabToolbarButtonDivisor = 100;

  // 50/100 = 50% for Non-Stable users.
  //  0/100 = 0%  for Stable users.
  const base::FieldTrial::Probability kPhoneNewTabToolbarButtonNonStable = 50;
  const base::FieldTrial::Probability kPhoneNewTabToolbarButtonStable = 0;
  const char kEnabled[] = "Enabled";
  const char kDisabled[] = "Disabled";

  // Find out if this is a stable channel.
  const bool kIsStableChannel =
      chrome::VersionInfo::GetChannel() == chrome::VersionInfo::CHANNEL_STABLE;

  // Experiment enabled until Jan 1, 2015. By default, disabled.
  scoped_refptr<base::FieldTrial> trial(
      base::FieldTrialList::FactoryGetFieldTrial(
          kPhoneNewTabToolbarButtonFieldTrialName,
          kPhoneNewTabToolbarButtonDivisor,
          kDisabled, 2015, 1, 1, base::FieldTrial::ONE_TIME_RANDOMIZED, NULL));

  const int kEnabledGroup = trial->AppendGroup(
      kEnabled,
      kIsStableChannel ?
          kPhoneNewTabToolbarButtonStable : kPhoneNewTabToolbarButtonNonStable);

  const int v = trial->group();
  VLOG(1) << "Phone NewTab toolbar button enabled group id: " << kEnabledGroup
          << ". Selected group id: " << v;
}

}  // namespace

void SetupMobileFieldTrials(const CommandLine& parsed_command_line,
                            const base::Time& install_time,
                            PrefService* local_state) {
  DataCompressionProxyFieldTrial();

#if defined(OS_ANDROID)
  NewTabButtonInToolbarFieldTrial(parsed_command_line);
#endif
}

}  // namespace chrome
