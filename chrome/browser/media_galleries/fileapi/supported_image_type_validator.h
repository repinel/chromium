// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_SUPPORTED_IMAGE_TYPE_VALIDATOR_H_
#define CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_SUPPORTED_IMAGE_TYPE_VALIDATOR_H_

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "webkit/browser/fileapi/copy_or_move_file_validator.h"

class ImageDecoder;

namespace chrome {

class MediaFileValidatorFactory;

// Use ImageDecoder to determine if the file decodes without error. Handles
// image files supported by Chrome.
class SupportedImageTypeValidator : public fileapi::CopyOrMoveFileValidator {
 public:
  virtual ~SupportedImageTypeValidator();

  static bool SupportsFileType(const base::FilePath& path);

  virtual void StartPreWriteValidation(
      const ResultCallback& result_callback) OVERRIDE;

  virtual void StartPostWriteValidation(
      const base::FilePath& dest_platform_path,
      const ResultCallback& result_callback) OVERRIDE;

 private:
  friend class MediaFileValidatorFactory;

  explicit SupportedImageTypeValidator(const base::FilePath& file);

  void OnFileOpen(scoped_ptr<std::string> data);

  base::FilePath path_;
  scoped_refptr<ImageDecoder> decoder_;
  fileapi::CopyOrMoveFileValidator::ResultCallback callback_;
  fileapi::CopyOrMoveFileValidator::ResultCallback post_write_callback_;
  base::WeakPtrFactory<SupportedImageTypeValidator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupportedImageTypeValidator);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_SUPPORTED_IMAGE_TYPE_VALIDATOR_H_
