/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include "fbpcf/io/api/IWriterCloser.h"

namespace fbpcf::io {

/*
This class is the API for writing a file to any
storage, local or cloud. It can be in any supported
cloud provider or a file on disk. Internally, this
class will create a LocalFileWriter or CloudFileWriter
depending on what file path is provided.
*/
class FileWriter : public IWriterCloser {
 public:
  explicit FileWriter(std::string filePath);

  int close() override;
  int write(char buf[], size_t nBytes) override;
  ~FileWriter() override;

 private:
  std::unique_ptr<IWriterCloser> childWriter_;
};

} // namespace fbpcf::io
