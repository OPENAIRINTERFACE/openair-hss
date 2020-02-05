/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file pid_file.c
   \brief
   \author  Lionel GAUTHIER
   \date 2016
   \email: lionel.gauthier@eurecom.fr
*/

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bstrlib.h"

#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "pid_file.h"

int g_fd_pid_file = -1;
__pid_t g_pid = -1;
//------------------------------------------------------------------------------
char *get_exe_absolute_path(char const *basepath, unsigned int instance) {
#define MAX_FILE_PATH_LENGTH 255
  char pid_file_name[MAX_FILE_PATH_LENGTH + 1] = {0};
  char *exe_basename = NULL;
  int rv = 0;
  int num_chars = 0;

  // get executable name
  rv = readlink("/proc/self/exe", pid_file_name, 256);
  if (-1 == rv) {
    return NULL;
  }
  pid_file_name[rv] = 0;
  exe_basename = basename(pid_file_name);

  // Add 6 for the other 5 characters in the path + null terminator + 2 chars
  // for instance.
  num_chars = strlen(basepath) + strlen(exe_basename) + 6 + 2;
  snprintf(pid_file_name, min(num_chars, MAX_FILE_PATH_LENGTH), "%s/%s%02u.pid",
           basepath, exe_basename, instance);
  return strdup(pid_file_name);
}

//------------------------------------------------------------------------------
int lockfile(int fd, int lock_type) {
  // lock on fd only, not on file on disk (do not prevent another process from
  // modifying the file)
  return lockf(fd, F_TLOCK, 0);
}

//------------------------------------------------------------------------------
bool is_pid_file_lock_success(char const *pid_file_name) {
  char pid_dec[64] = {0};

  g_fd_pid_file =
      open(pid_file_name, O_RDWR | O_CREAT,
           S_IRUSR | S_IWUSR | S_IRGRP |
               S_IROTH); /* Read/write by owner, read by grp, others */
  if (0 > g_fd_pid_file) {
    printf("filename %s failed %d:%s\n", pid_file_name, errno, strerror(errno));
    return false;
  }

  if (0 > lockfile(g_fd_pid_file, F_TLOCK)) {
    if (EACCES == errno || EAGAIN == errno) {
      printf("filename %s failed %d:%s\n", pid_file_name, errno,
             strerror(errno));
      close(g_fd_pid_file);
    }
    printf("filename %s failed %d:%s\n", pid_file_name, errno, strerror(errno));
    return false;
  }
  // fruncate file content
  ftruncate(g_fd_pid_file, 0);
  // write PID in file
  g_pid = getpid();
  snprintf(pid_dec, sizeof(pid_dec), "%ld", (long)g_pid);
  write(g_fd_pid_file, pid_dec, strlen(pid_dec));
  return true;
}

//------------------------------------------------------------------------------
void pid_file_unlock(void) {
  lockfile(g_fd_pid_file, F_ULOCK);
  close(g_fd_pid_file);
  g_fd_pid_file = -1;
}
