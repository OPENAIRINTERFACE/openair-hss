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

#ifndef __SUTILITY_H
#define __SUTILITY_H

#include <string>
#include <vector>

class SUtility {
 public:
  static int indexOf(const char* path, char search, int start = 0);
  static int indexOfAny(const char* path, const char* search);
  static int lastIndexOfAny(const char* path, const char* search);
  static std::vector<std::string> split(const std::string& s, const char delim);
  static std::string string_format(const char* format, ...);
  static void string_format(std::string& dest, const char* format, ...);

  static void copyfile(const std::string& dst, const std::string& src) {
    copyfile(dst.c_str(), src.c_str());
  }
  static void copyfile(const char* dst, const std::string& src) {
    copyfile(dst, src.c_str());
  }
  static void copyfile(const std::string& dst, const char* src) {
    copyfile(dst.c_str(), src);
  }
  static void copyfile(const char* dst, const char* src);

  static void deletefile(const std::string& fn) { deletefile(fn.c_str()); }
  static void deletefile(const char* fn);

  static std::string currentTime();

 private:
  static void _string_format(
      std::string& dest, const char* format, va_list& args);
};

#endif  // #define __SUTILITY_H
