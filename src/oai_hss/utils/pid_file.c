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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>

#include "pid_file.h"
#include "log.h"

int    g_fd_pid_file = -1;

//------------------------------------------------------------------------------
char* get_exe_basename(void)
{

  char   pid_file_name[256] = {0};
  char   *exe_basename      = NULL;
  int    rv                 = 0;

  // get executable name
  rv = readlink("/proc/self/exe",pid_file_name, 256);
  if ( -1 == rv) {
    return NULL;
  }
  pid_file_name[rv] = 0;
  exe_basename = basename(pid_file_name);
  snprintf(pid_file_name, 128, "/var/run/%s.pid", exe_basename);
  return strdup(pid_file_name);
}

//------------------------------------------------------------------------------
int lockfile(int fd, int lock_type)
{
  // lock on fd only, not on file on disk (do not prevent another process from modifying the file)
  return lockf(fd, F_TLOCK, 0);
}

//------------------------------------------------------------------------------
bool is_pid_file_lock_success(char const *pid_file_name)
{
  char       pid_dec[32] = {0};

  g_fd_pid_file = open(pid_file_name,
                       O_RDWR | O_CREAT,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); /* Read/write by owner, read by grp, others */
  if ( 0 > g_fd_pid_file) {
    FPRINTF_DEBUG("filename %s failed %d:%s\n", pid_file_name, errno, strerror(errno));
    return false;
  }

  if ( 0 > lockfile(g_fd_pid_file, F_TLOCK)) {
    if ( EACCES == errno || EAGAIN == errno ) {
      close(g_fd_pid_file);
    }
    FPRINTF_DEBUG("filename %s failed %d:%s\n", pid_file_name, errno, strerror(errno));
    return false;
  }
  // fruncate file content
  if (! ftruncate(g_fd_pid_file, 0)) {
    // write PID in file
    sprintf(pid_dec, "%ld", (long)getpid());
    if (0 < write(g_fd_pid_file, pid_dec, strlen(pid_dec))) {
      return true;
    }
  }
  pid_file_unlock();
  close(g_fd_pid_file);
  return false;
}

//------------------------------------------------------------------------------
void pid_file_unlock(void)
{
  lockfile(g_fd_pid_file, F_ULOCK);
  close(g_fd_pid_file);
  g_fd_pid_file = -1;
}
