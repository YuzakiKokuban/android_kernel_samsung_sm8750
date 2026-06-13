/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 Config file parser

 GENERAL DESCRIPTION
 This component parses any specified config file and allows to be queried later

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <new>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include <base_util/log.h>
#include <base_util/list.h>
#include <base_util/config_file.h>

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

namespace qc_loc_fw
{
static const char * const TAG = "ConfigFile";

ConfigFile::~ConfigFile()
{
  // dummy
}

struct ConfigPair
{
  const char * name;
  const char * value;
};

class ConfigFileImpl: public ConfigFile
{
public:
  ConfigFileImpl(const char * const filename, const size_t max_line_length, const bool verbose);
  ~ConfigFileImpl();

  bool loaded() const
  {
    return m_loaded;
  }
  ;

  int getString(const char * const name, const char ** pStr);
  int getStringDup(const char * const name, const char ** pStr, const char * const strDefault);
  int getInt32(const char * const name, int & value);
  int getInt32Default(const char * const name, int & value, const int & Default);
  int get_PZ_Int32Default(const char * const name, int & value, const int & Default);
  int get_PNZ_Int32Default(const char * const name, int & value, const int & Default);
  int getDouble(const char * const name, double & value);
  int getDoubleDefault(const char * const name, double & value, const double & Default);
private:
  bool m_loaded;

  typedef List<ConfigPair> PAIRS;
  PAIRS m_config;

  void trim(size_t & cursor_begin, size_t & cursor_end, const char * const line);
  void substring(char * dest, const size_t capacity, const char * const src, const size_t begin, const size_t end);
  int find_name(size_t & cursor_begin, size_t & cursor_end, const char * const line, size_t & name_begin,
      size_t & name_end);
  int find_equal(size_t & cursor_begin, size_t & cursor_end, const char * const line);
  int find_value(size_t & cursor_begin, size_t & cursor_end, const char * const line, size_t & value_begin,
      size_t & value_end);

  const char * subStringDup(const char * const src, const size_t begin, const size_t end);
};

void ConfigFileImpl::trim(size_t & cursor_begin, size_t & cursor_end, const char * const line)
{
  // check arguments
  if(cursor_end <= cursor_begin)
  {
    // empty line already
    return;
  }

  // left trim
  for (; cursor_begin < cursor_end; ++cursor_begin)
  {
    if(!isspace(line[cursor_begin]))
    {
      break;
    }
  }

  // right trim
  for (; cursor_begin < cursor_end; --cursor_end)
  {
    if(!isspace(line[cursor_end - 1]))
    {
      break;
    }
  }
}

void ConfigFileImpl::substring(char * dest, const size_t capacity, const char * const src, const size_t begin,
    const size_t end)
{
  memset(dest, 0, capacity);
  for (size_t i = begin, d = 0; ((i < end) && (d < capacity)); ++i, ++d)
  {
    dest[d] = src[i];
  }
}

const char * ConfigFileImpl::subStringDup(const char * const src, const size_t begin, const size_t end)
{
  char * dest = 0;
  int result = 1;
  do
  {
    if(begin >= end)
    {
      result = 2;
    }
    BREAK_IF_ZERO(3, src);

    size_t length = end - begin;

    dest = new (std::nothrow) char[length + 1];
    BREAK_IF_ZERO(4, dest);
    memcpy(dest, src + begin, length);
    dest[length] = '\0';

    result = 0;
  } while (0);

  if(0 != result)
  {
    delete[] dest;
    log_error(TAG, "subStringDup failed %d", result);
    return 0;
  }
  else
  {
    return dest;
  }
}

int ConfigFileImpl::find_name(size_t & cursor_begin, size_t & cursor_end, const char * const line, size_t & name_begin,
    size_t & name_end)
{
  if(0 == line)
  {
    return 1;
  }
  if(cursor_begin >= cursor_end)
  {
    return 2;
  }

  bool found = false;
  for (size_t i = cursor_begin; i < cursor_end; ++i)
  {
    if(!found)
    {
      if(isalpha(line[i]))
      {
        found = true;
        name_begin = i;
        name_end = i + 1;
      }
      else
      {
        break;
      }
    }
    else
    {
      if(isalnum(line[i]) || ('-' == line[i]) || ('_' == line[i]))
      {
        name_end = i + 1;
        continue;
      }
      else
      {
        break;
      }
    }
  }

  if(found)
  {
    cursor_begin = name_end;
    trim(cursor_begin, cursor_end, line);
    return 0;
  }
  else
  {
    return 3;
  }
}

int ConfigFileImpl::find_equal(size_t & cursor_begin, size_t & cursor_end, const char * const line)
{

  if(0 == line)
  {
    return 1;
  }
  if(cursor_begin >= cursor_end)
  {
    return 2;
  }
  if('=' == line[cursor_begin])
  {
    ++cursor_begin;
    trim(cursor_begin, cursor_end, line);
    return 0;
  }
  return 3;
}

int ConfigFileImpl::find_value(size_t & cursor_begin, size_t & cursor_end, const char * const line,
    size_t & value_begin, size_t & value_end)
{
  if(0 == line)
  {
    return 1;
  }
  if(cursor_begin >= cursor_end)
  {
    return 2;
  }

  value_begin = cursor_begin;
  value_end = cursor_end;

  cursor_begin = cursor_end;

  if('"' == line[value_begin])
  {
    if(value_end - value_begin >= 2)
    {
      if('"' == line[value_end - 1])
      {
        // remove double quote
        ++value_begin;
        --value_end;
      }
      else
      {
        // start with " but doesn't end with one
        return 3;
      }
    }
    else
    {
      // start with " but doesn't end with one
      return 4;
    }
  }
  return 0;
}

ConfigFileImpl::ConfigFileImpl(const char * const filename, const size_t max_line_length, const bool verbose) :
    m_loaded(false)
{
  ConfigPair pair;
  pair.name = 0;
  pair.value = 0;

  char * line_debug = 0;
  char * line = 0;
  FILE * file = 0;
  int result = 1;
  int line_index = 0;
  do
  {
    BREAK_IF_ZERO(2, filename);
    file = fopen(filename, "r");
    BREAK_IF_ZERO(3, file);

    line = new (std::nothrow) char[max_line_length + 1];
    BREAK_IF_ZERO(4, line);

    if(verbose)
    {
      line_debug = new (std::nothrow) char[max_line_length + 1];
      BREAK_IF_ZERO(5, line_debug);
      line_debug[0] = '\0';
    }

    line[0] = '\0';
    result = 100;
    while (0 != fgets(line, max_line_length + 1, file))
    {
      ++line_index;
      //log_verbose(TAG, "line [%s]", line);

      size_t length = strlen(line);
      if(length >= max_line_length)
      {
        // line too long
        result = 101;
        break;
      }

      // token type 1: # comment till the end of line
      // token type 2: name, can only contain alphanumeric,'_', and '-'
      // token type 3: '='
      // token type 4: value: anything starting from the first non-space char after '=' to the end of this (trimmed) line

      size_t cursor_begin = 0;
      size_t cursor_end = length;
      trim(cursor_begin, cursor_end, line);

      if(verbose)
      {
        substring(line_debug, max_line_length + 1, line, cursor_begin, cursor_end);
        log_debug(TAG, "line trimed [%s]", line_debug);
      }

      if(cursor_begin >= cursor_end)
      {
        // empty line already
        if(verbose)
        {
          log_verbose(TAG, "Line[%d], skip empty line", line_index);
        }
      }
      else if('#' == line[cursor_begin])
      {
        // this whole line is comment
        cursor_begin = cursor_end;
        if(verbose)
        {
          log_verbose(TAG, "Line[%d], skip comment line", line_index);
        }
      }
      else
      {
        // we shall see name = value pattern
        size_t name_begin = cursor_end;
        size_t name_end = cursor_end;
        size_t value_begin = cursor_end;
        size_t value_end = cursor_end;
        BREAK_IF_NON_ZERO(110, find_name(cursor_begin, cursor_end, line, name_begin, name_end));
        BREAK_IF_NON_ZERO(111, find_equal(cursor_begin, cursor_end, line));
        BREAK_IF_NON_ZERO(112, find_value(cursor_begin, cursor_end, line, value_begin, value_end));
        if(cursor_begin < cursor_end)
        {
          // still anything after 'value' token!?
          // error
          result = 113;
          break;
        }

        if(verbose)
        {
          substring(line_debug, max_line_length + 1, line, name_begin, name_end);
          log_verbose(TAG, "name [%s]", line_debug);

          substring(line_debug, max_line_length + 1, line, value_begin, value_end);
          log_verbose(TAG, "value [%s]", line_debug);
        }

        pair.name = subStringDup(line, name_begin, name_end);
        pair.value = subStringDup(line, value_begin, value_end);

        BREAK_IF_ZERO(114, pair.name);
        BREAK_IF_ZERO(115, pair.value);
        BREAK_IF_NON_ZERO(116, m_config.add(pair));

        // we don't own them now
        pair.name = 0;
        pair.value = 0;
      }

      // reset the string
      line[0] = '\0';

      if(line_debug)
      {
        line_debug[0] = '\0';
      }
    }
    if(100 != result)
    {
      break;
    }
    BREAK_IF_NON_ZERO(10, ferror(file));

    m_loaded = true;
    result = 0;
  } while (0);

  delete[] line;
  line = 0;

  delete[] pair.name;
  pair.name = 0;
  delete[] pair.value;
  pair.value = 0;

  delete[] line_debug;
  line_debug = 0;

  if(0 != file)
  {
    (void) fclose(file);
    file = 0;
  }

  if(0 != result)
  {
    log_error(TAG, "ConfigFileImpl: cannot load config file result [%d] [%s], error [%d][%s], line[%d]", result,
        filename, errno, strerror(errno), line_index);
  }
}

ConfigFileImpl::~ConfigFileImpl()
{
  for (PAIRS::Iterator it = m_config.begin(); it != m_config.end(); ++it)
  {
    ConfigPair & pair = (*it);
    delete[] pair.name;
    delete[] pair.value;

    pair.name = 0;
    pair.value = 0;
  }
  m_config.flush();
}

int ConfigFileImpl::getString(const char * const name, const char ** pStr)
{
  int result = 1;
  do
  {
    if(!m_loaded)
    {
      result = 2;
      break;
    }
    BREAK_IF_ZERO(3, name);
    BREAK_IF_ZERO(4, pStr);
    *pStr = 0;

    result = NOT_FOUND;
    for (PAIRS::Iterator it = m_config.begin(); it != m_config.end(); ++it)
    {
      ConfigPair & pair = (*it);
      if(0 == strcmp(name, pair.name))
      {
        *pStr = pair.value;
        result = NO_ERROR_RESULT_SET;
        break;
      }
    }
  } while (0);

  if((NO_ERROR_RESULT_SET != result) && (NOT_FOUND != result))
  {
    if(name)
    {
      log_error(TAG, "getString failed [%s] %d", name, result);
    }
    else
    {
      log_error(TAG, "getString failed %d", result);
    }
  }
  return result;
}

int ConfigFileImpl::getStringDup(const char * const name, const char ** pStr, const char * const strDefault)
{
  int result = 1;
  do
  {
    if(!m_loaded)
    {
      // if not loaded, set return to NOT_FOUND, so that default value (strDefault) can be returned.
      result = NOT_FOUND;
    }
    else
    {
      BREAK_IF_ZERO(3, name);
      BREAK_IF_ZERO(4, pStr);
      *pStr = 0;

      result = NOT_FOUND;
      for (PAIRS::Iterator it = m_config.begin(); it != m_config.end(); ++it)
      {
        ConfigPair & pair = (*it);
        if(0 == strcmp(name, pair.name))
        {
          size_t length = strlen(pair.value) + 1;
          *pStr = new (std::nothrow) char[length];
          BREAK_IF_ZERO(101, *pStr);
          memcpy((void *) *pStr, (const void *) pair.value, length);
          result = NO_ERROR_RESULT_SET;
          break;
        }
      }
    }

    if((NOT_FOUND == result) && (0 != strDefault))
    {
      size_t length = strlen(strDefault) + 1;
      *pStr = new (std::nothrow) char[length];
      BREAK_IF_ZERO(111, *pStr);
      memcpy((void *) *pStr, (const void *) strDefault, length);
      result = NO_ERROR_RESULT_SET;
    }
  } while (0);

  if((NO_ERROR_RESULT_SET != result) && (NOT_FOUND != result))
  {
    if(name)
    {
      log_error(TAG, "getStringDup failed [%s] %d", name, result);
    }
    else
    {
      log_error(TAG, "getStringDup failed %d", result);
    }
  }
  return result;
}

int ConfigFileImpl::get_PZ_Int32Default(const char * const name, int & value, const int & Default)
{
  int result = getInt32(name, value);
  if(NOT_FOUND == result)
  {
    value = Default;
    return 0;
  }
  else if(value < 0)
  {
    value = Default;
    return 1;
  }
  return result;
}

int ConfigFileImpl::get_PNZ_Int32Default(const char * const name, int & value, const int & Default)
{
  int result = getInt32(name, value);
  if(NOT_FOUND == result)
  {
    value = Default;
    return 0;
  }
  else if(value <= 0)
  {
    value = Default;
    return 1;
  }
  return result;
}

int ConfigFileImpl::getInt32Default(const char * const name, int & value, const int & Default)
{
  int result = getInt32(name, value);
  if(NOT_FOUND == result)
  {
    value = Default;
    return 0;
  }
  return result;
}

int ConfigFileImpl::getInt32(const char * const name, int & value)
{
  int result = 1;
  do
  {
    if(!m_loaded)
    {
      // if not loaded, return NOT_FOUND, so that other util functions invoking
      // this function (eg:getInt32Default) can return back default value.
      log_debug(TAG, "getInt32 m_loaded is false");
      result = NOT_FOUND;
      break;
    }
    BREAK_IF_ZERO(3, name);

    result = NOT_FOUND;
    for (PAIRS::Iterator it = m_config.begin(); it != m_config.end(); ++it)
    {
      ConfigPair & pair = (*it);
      if(0 == strcmp(name, pair.name))
      {
        if(strlen(pair.value) < 1)
        {
          result = 101;
          break;
        }

        errno = 0;
        char * end_ptr = 0;
        long temp = strtol(pair.value, &end_ptr, 0);
        if((LONG_MAX == temp) || (LONG_MIN == temp))
        {
          result = 102;
        }
        else if(0 != errno)
        {
          // conversion error
          log_error(TAG, "conversion error [%d] [%s]", errno, strerror(errno));
          result = 103;
        }
        else if('\0' != *end_ptr)
        {
          // string not fully consumed
          result = 104;
        }
        else if((temp > INT_MAX) || (temp < INT_MIN))
        {
          // beyond range for int32
          result = 105;
        }
        else
        {
          // conversion okay
          value = (int) temp;
          result = 0;
        }
        break;
      }
    }
  } while (0);

  if((NO_ERROR_RESULT_SET != result) && (NOT_FOUND != result))
  {
    if(name)
    {
      log_error(TAG, "getInt32 failed [%s] %d", name, result);
    }
    else
    {
      log_error(TAG, "getInt32 failed %d", result);
    }
  }
  return result;
}

int ConfigFileImpl::getDoubleDefault(const char * const name, double & value, const double & Default)
{
  int result = getDouble(name, value);
  if(NOT_FOUND == result)
  {
    value = Default;
    return 0;
  }
  return result;
}

int ConfigFileImpl::getDouble(const char * const name, double & value)
{
  int result = 1;
  do
  {
    if(!m_loaded)
    {
      // if not loaded, return NOT_FOUND, so that other util functions invoking
      // this function (eg:getDoubleDefault) can return back default value.
      log_debug(TAG, "getDouble m_loaded is false");
      result = NOT_FOUND;
      break;
    }
    BREAK_IF_ZERO(3, name);

    result = NOT_FOUND;
    for (PAIRS::Iterator it = m_config.begin(); it != m_config.end(); ++it)
    {
      ConfigPair & pair = (*it);
      if(0 == strcmp(name, pair.name))
      {
        if(strlen(pair.value) < 1)
        {
          result = 101;
          break;
        }

        errno = 0;
        char * end_ptr = 0;
        double temp = strtod(pair.value, &end_ptr);
        if(0 != errno)
        {
          // conversion error
          log_error(TAG, "conversion error [%d] [%s]", errno, strerror(errno));
          result = 102;
        }
        else if('\0' != *end_ptr)
        {
          // string not fully consumed
          result = 103;
        }
        else
        {
          // conversion okay
          value = temp;
          result = 0;
        }
        break;
      }
    }
  } while (0);

  if((NO_ERROR_RESULT_SET != result) && (NOT_FOUND != result))
  {
    if(name)
    {
      log_error(TAG, "getDouble failed [%s] %d", name, result);
    }
    else
    {
      log_error(TAG, "getDouble failed %d", result);
    }
  }
  return result;
}

ConfigFile * ConfigFile::createInstance(const char * const filename, const size_t max_line_length, const bool verbose)
{
  return new (std::nothrow) ConfigFileImpl(filename, max_line_length, verbose);
}

} // namespace qc_loc_fw

