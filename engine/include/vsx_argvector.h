/**
* Project: VSXu Engine: Realtime modular visual programming engine.
*
* This file is part of Vovoid VSXu Engine.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Lesser General Public License (LGPL)
*
* VSXu Engine is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef VSX_ARGVECTOR_H
#define VSX_ARGVECTOR_H


#include <vsx_platform.h>
#include <vsxfst.h>
#include <vsx_engine_dllimport.h>

#if PLATFORM_FAMILY == PLATFORM_FAMILY_UNIX
  #include "vsx_math.h"
  #include <sys/types.h>
  #include <unistd.h>
  #include <stdlib.h>
#endif

#include <container/vsx_avector.h>
#include <vsx_string.h>

class vsx_argvector
{
  vsx_avector<vsx_string> data;
public:
  vsx_string& operator[](size_t index)
  {
    return data[index];
  }

  size_t size()
  {
    return data.size();
  }

  void push_back(const vsx_string& val)
  {
    data.push_back(val);
  }

  bool has_param(const vsx_string& param)
  {
    size_t data_num_elements = size();

    if (data_num_elements == 0)
      return false;

    for (size_t i = 0; i < data_num_elements; i++)
    {
      if (data[i] == "-"+param)
      {
        return true;
      }
    }
    return false;
  }

  bool has_param_with_value(const vsx_string& param)
  {
    size_t data_num_elements = size();

    if (data_num_elements == 0)
      return false;

    for (size_t i = 0; i < data_num_elements; i++)
    {
      if (data[i] == "-"+param)
      {
        if (i+1 < data_num_elements)
        {
          if (data[i+1][0] != '-')
          {
            return true;
          }
        }
      }
    }
    return false;
  }

  vsx_string get_param_value(const vsx_string& param)
  {
    size_t data_num_elements = size();
    if (data_num_elements == 0) return "";
    for (size_t i = 0; i < data_num_elements; i++)
    {
      if (data[i] == "-"+param)
      {
        if (i+1 < data_num_elements)
        {
          if (data[i+1][0] != '-')
          {
            return data[i+1];
          }
        }
      }
    }
    return "";
  }

  void init_from_argc_argv(int argc, char *argv[])
  {
    for (int i = 0; i < argc; i++)
    {
      push_back(argv[i]);
    }
  }

  void init_from_string(vsx_string new_string)
  {
    vsx_string res;
    new_string.trim_lf();

    for (size_t i = 0; i < new_string.size(); i++)
    {
      if (new_string[i] == ' ' && res.size())
      {
        data.push_back( res );
        res.clear();
      } else
      {
        res.push_back( new_string[i] );
      }
    }
    if (res.size())
    {
      data.push_back(res);
    }
  }

  vsx_string serialize()
  {
    vsx_string res;
    for (size_t i = 0; i < data.size(); i++)
    {
      if (i) res.push_back(' ');
      res += data[i];
    }
    return res;
  }

  static vsx_string get_executable_directory()
  {
    char pBuf[512];
    const size_t len = 512;

    #if PLATFORM_FAMILY == PLATFORM_FAMILY_WINDOWS
      int bytes = GetModuleFileName(NULL, pBuf, len);
      if(bytes == 0)
        return -1;
      else
        return bytes;
    #else

      // if packed with UPX
      if (getenv("   "))
        return get_path_from_filename(vsx_string(getenv("   ")));

      char szTmp[32];
      sprintf(szTmp, "/proc/self/exe");
      int bytes = MIN(readlink(szTmp, pBuf, len), len - 1);

      if(bytes >= 0)
        pBuf[bytes] = '\0';

    #endif
    return get_path_from_filename(vsx_string(pBuf));
  }

private:
  ENGINE_DLLIMPORT static vsx_argvector instance;
public:
  static vsx_argvector* get_instance()
  {
    return &instance;
  }

  vsx_argvector() {}
};

#endif // VSX_ARGVECTOR_H
