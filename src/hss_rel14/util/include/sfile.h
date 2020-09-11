/*
 * Copyright (c) 2017 Sprint
 *
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SFILE_H
#define __SFILE_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <fstream>

class SFile {
 public:
  SFile();
  ~SFile();

  bool open(const char* filename);
  bool open(const std::string& filename) { return open(filename.c_str()); }

  void setStream(FILE* stream) { csv_file = stream; }

  void setfilename(std::string& filename) { m_filename = filename; }

  char* getStreamBuffer() { return stream_buffer; }

  void close();

  bool read();

  bool is_open() { return (csv_file != NULL); }

  const std::string& filename() { return m_filename; }
  const std::string& data() { return m_data; }
  int64_t dataoffset() { return m_dataofs; }
  uint32_t datarecnbr() { return m_datarn; }

  bool renamed();

  bool seek(uint32_t recnbr, std::ios::streampos offset);

 private:
  FILE* csv_file;
  char* stream_buffer;
  std::ifstream m_stream;
  std::string m_filename;
  std::string m_data;
  uint32_t m_datarn;
  std::ios::streampos m_dataofs;
  uint32_t m_nextrn;
  std::ios::streampos m_nextofs;
};

#endif  // #define __SFILE_H
