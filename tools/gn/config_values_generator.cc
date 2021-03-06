// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/config_values_generator.h"

#include "tools/gn/config_values.h"
#include "tools/gn/scope.h"
#include "tools/gn/value.h"
#include "tools/gn/value_extractors.h"

namespace {

void GetStringList(
    const Scope* scope,
    const char* var_name,
    ConfigValues* config_values,
    void (ConfigValues::* swapper_inner)(std::vector<std::string>*),
    Err* err) {
  const Value* value = scope->GetValue(var_name);
  if (!value)
    return;  // No value, empty input and succeed.

  std::vector<std::string> result;
  ExtractListOfStringValues(*value, &result, err);
  (config_values->*swapper_inner)(&result);
}

}  // namespace

ConfigValuesGenerator::ConfigValuesGenerator(ConfigValues* dest_values,
                                             const Scope* scope,
                                             const Token& function_token,
                                             const SourceDir& input_dir,
                                             Err* err)
    : config_values_(dest_values),
      scope_(scope),
      function_token_(function_token),
      input_dir_(input_dir),
      err_(err) {
}

ConfigValuesGenerator::~ConfigValuesGenerator() {
}

void ConfigValuesGenerator::Run() {
  FillDefines();
  FillIncludes();
  FillCflags();
  FillCflags_C();
  FillCflags_CC();
  FillLdflags();
}

void ConfigValuesGenerator::FillDefines() {
  GetStringList(scope_, "defines", config_values_,
                &ConfigValues::swap_in_defines, err_);
}

void ConfigValuesGenerator::FillIncludes() {
  const Value* value = scope_->GetValue("includes");
  if (!value)
    return;  // No value, empty input and succeed.

  std::vector<SourceDir> includes;
  if (!ExtractListOfRelativeDirs(*value, input_dir_, &includes, err_))
    return;
  config_values_->swap_in_includes(&includes);
}

void ConfigValuesGenerator::FillCflags() {
  GetStringList(scope_, "cflags", config_values_,
                &ConfigValues::swap_in_cflags, err_);
}

void ConfigValuesGenerator::FillCflags_C() {
  GetStringList(scope_, "cflags_c", config_values_,
                &ConfigValues::swap_in_cflags_c, err_);
}

void ConfigValuesGenerator::FillCflags_CC() {
  GetStringList(scope_, "cflags_cc", config_values_,
                &ConfigValues::swap_in_cflags_cc, err_);
}

void ConfigValuesGenerator::FillLdflags() {
  GetStringList(scope_, "ldflags", config_values_,
                &ConfigValues::swap_in_ldflags, err_);
}
