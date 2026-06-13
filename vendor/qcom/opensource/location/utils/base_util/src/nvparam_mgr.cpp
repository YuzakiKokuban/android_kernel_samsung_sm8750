/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 XTRA-T WiFi storage manager for crowd sourcing parameter that need to
 be persistent across bootup.

 GENERAL DESCRIPTION
 This component implements the storage module for crowd sourcing
 module parameters coming from server that need to be persistent
 across bootup.

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <new>
#include <sqlite3.h>

#include <base_util/log.h>
#include <base_util/sync.h>
#include <base_util/string_routines.h>
#include <base_util/nvparam_mgr.h>

using namespace qc_loc_fw;

namespace qc_loc_fw
{
#ifndef IZAT_OFFLINE
#define NVPARAM_NAME "/data/vendor/location/nvparam.sqlite"
#else
#ifndef ROOT_DIR
#define ROOT_DIR ""
#endif
#define NVPARAM_NAME ROOT_DIR "nvparam.sqlite"
#endif // #ifndef IZAT_OFFLINE

static const char * const TAG = "NvParamMgr";

#define _SQLITE_CHECK(stmt, rt) \
  if (rt != stmt) \
  {\
    log_error(TAG, "%s:%d: reason:%s", __FILE__, __LINE__, sqlite3_errmsg((sqlite3 *)m_db)); \
    return false; \
  }

// return from function if check does not match
#define _SQLITE_CHECK_DONE(stmt) _SQLITE_CHECK(stmt, SQLITE_DONE)
#define _SQLITE_CHECK_OK(stmt) _SQLITE_CHECK(stmt, SQLITE_OK)

#define _SQLITE_CHECK_NO_RETURN(stmt, rt) \
  if (rt != stmt) \
  {\
    log_error(TAG, "%s:%d: reason:%s", __FILE__, __LINE__, sqlite3_errmsg((sqlite3 *)m_db)); \
  }

#define _SQLITE_CHECK_DONE_NO_RETURN(stmt) _SQLITE_CHECK_NO_RETURN(stmt, SQLITE_DONE)
#define _SQLITE_CHECK_OK_NO_RETURN(stmt) _SQLITE_CHECK_NO_RETURN(stmt, SQLITE_OK)

#define _BREAK_SQLITE_CHECK(stmt, rt, err)\
  sqlRes = stmt;\
  if ((SQLITE_BUSY == sqlRes) || (SQLITE_LOCKED == sqlRes))\
  {\
    log_error(TAG, "%s:%d: error code:%d,reason:%s", __FILE__, __LINE__, sqlRes, sqlite3_errmsg((sqlite3 *)m_db));\
    result = NV_PARAM_ERR_SQLITE_BUSY;\
    break;\
  }\
  else if (rt != sqlRes)\
  {\
    result = err;\
    log_error(TAG, "%s:%d: error code:%d,reason:%s", __FILE__, __LINE__, sqlRes, sqlite3_errmsg((sqlite3 *)m_db)); \
    break;\
  }\

// break from loop if check does not match
#define _BREAK_SQLITE_CHECK_DONE(stmt, err) _BREAK_SQLITE_CHECK(stmt, SQLITE_DONE, err)
#define _BREAK_SQLITE_CHECK_OK(stmt, err) _BREAK_SQLITE_CHECK(stmt, SQLITE_OK, err)
#define _BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}

// device to obtain pseudo client id
#define RANDOM_DEVICE "/dev/random"

class NvParamMgrImpl: public NvParamMgr
{
public:
  NvParamMgrImpl (const char* const filename);

  virtual ~NvParamMgrImpl () {};

  nv_param_err_code init();

  nv_param_err_code deinit();

  // save the given param value into the permanent storage file
  nv_param_err_code saveInt32Param (const char* param_name,
                                    const int param_value);

  nv_param_err_code saveBoolParam (const char* param_name,
                                   const bool param_value);

  nv_param_err_code saveFloatParam (const char* param_name,
                                    const float param_value);

  nv_param_err_code saveUInt32Param (const char* param_name,
                                     const unsigned int param_value);

  nv_param_err_code saveInt64Param (const char* param_name,
                                    const long long param_value);

  nv_param_err_code saveUInt64Param (const char* param_name,
                                     const unsigned long long param_value);

  nv_param_err_code saveStringParam (const char* param_name,
                                     const char* param_value);

  nv_param_err_code saveBlobParam (const char* param_name,
                                   const unsigned char* param_value,
                                   unsigned int param_size_in_bytes);

  // retriev the given param value of int type
  // If the value can not be retrieved, then
  // param_value will not be modified
  nv_param_err_code getInt32Param (const char* param_name,
                                   int & param_value);

  nv_param_err_code getBoolParam (const char* param_name,
                                  bool & param_value);

  nv_param_err_code getFloatParam (const char* param_name,
                                   float & param_value);

  nv_param_err_code getUInt32Param (const char* param_name,
                                    unsigned int & param_value);

  nv_param_err_code getStringParam (const char* param_name,
                                    char* &param_value,
                                    unsigned int & param_size_in_bytes);

  nv_param_err_code getInt64Param (const char* param_name,
                                   long long & param_value);

  nv_param_err_code getUInt64Param (const char* param_name,
                                    unsigned long long & param_value);

  nv_param_err_code getBlobParam (const char* param_name,
                                  unsigned char* &param_value,
                                  unsigned int &param_size_in_bytes);

private:
  nv_param_err_code openDB ();

  nv_param_err_code saveNvDataBlob (const char* param_name,
                                    const void* data,
                                    uint32_t data_len);

  nv_param_err_code getNvDataBlob (const char* param_name,
                                   void* data,
                                   int &data_len);

  // internal functions
  int generatePseudoClientIdIfNeeded ();

  // internal functions
  int wrapper_mprintf (char* &output_buf, unsigned int &output_size,
                       const char *sentence,... );

  // Variables
  // sqlite file and etc
  const char* m_filename;
  sqlite3*    m_db;

  char*        m_sqlite_sentence;
  unsigned int m_sqlite_sentence_size;

  bool               m_pseudo_client_id_exist;
  unsigned long long m_pseudo_client_id_64;
  unsigned int       m_pseudo_client_id_32;
};

// When schema below changes, please increase the NVPARAM_DB_USER_VERSION
static const int NVPARAM_DB_USER_VERSION = 1;

// When schema below changes, please increase the NVPARAM_DB_USER_VERSION
static const char* SQL_CREATE_NVPARAM_TABLE =
  "CREATE TABLE IF NOT EXISTS 'NV_PARAM_Table' ("
  "'param_name' TEXT NOT NULL ,"
  "'param_data' blob ,"
  "CONSTRAINT c_u_id UNIQUE (param_name));";

static Mutex*          m_mutex = Mutex::createInstance();
static NvParamMgrImpl* m_nv_mgr = NULL;
static int             m_ref_cnt = 0;

// If any of the parameters fail to load, then none of the
// parameters will be retrieved from the persistent parameter file,
// and we will use the default values from configuration file.
NvParamMgrImpl::NvParamMgrImpl(const char* const filename) :
  m_filename (filename),
  m_db (NULL),
  m_sqlite_sentence (NULL),
  m_sqlite_sentence_size (1000),
  // pseudo client id will be created if it does not yet
  // exist in database when attempting to retrieve it.
  // After first successful retrieval, it will be set to true
  m_pseudo_client_id_exist (false),
  m_pseudo_client_id_64 (0),
  m_pseudo_client_id_32 (0)
{}

nv_param_err_code NvParamMgrImpl::init()
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  do
  {
    int rc = sqlite3_threadsafe();
    log_debug (TAG, "init, sqite_threadsafe returned %d", rc);
    if (rc == 0)
    {
      log_error (TAG, "init, failed as sqlite is not compiled in threadsafe mode");
      result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
      break;
    }

    result = openDB ();
  }while (0);

  if (result != NV_PARAM_ERR_NO_ERR)
  {
    log_error (TAG, "init failed, result = %d", result);
  }

  return result;
}

nv_param_err_code NvParamMgrImpl::openDB()
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
  bool newFile = false;
  int sqlRes = SQLITE_OK;
  int ret_val = 0;

  do
  {
    if (m_db != NULL)
    {
      result = NV_PARAM_ERR_NO_ERR;
      break;
    }

    struct stat buf = {};
    ret_val = stat(m_filename, &buf);
    if (0 != ret_val)
    {
      int en = errno;
      if (ENOENT == en) {
        newFile = true;
        umask(0007);
      }

      log_debug (TAG, "openDB, stat for %s failed with error: %s, new file: %d\n",
                 m_filename, strerror(en), newFile);
    }

    sqlite3* t_db = 0;

    int sql_res = sqlite3_open_v2(m_filename, &t_db,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (sql_res != SQLITE_OK)
    {
      // need to free the resource regardless t_db is 0 or not
      sqlite3_close (t_db);
      if ((SQLITE_BUSY == sqlRes) || (SQLITE_LOCKED == sqlRes))
      {
        result = NV_PARAM_ERR_SQLITE_BUSY;
      }
      else
      {
        result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
      }
      break;
    }

    if (newFile)
    {
      stat(m_filename, &buf);
      if (((buf.st_mode & S_IRUSR) != S_IRUSR) ||
          ((buf.st_mode & S_IWUSR) != S_IWUSR) ||
          ((buf.st_mode & S_IRGRP) != S_IRGRP) ||
          ((buf.st_mode & S_IWGRP) != S_IWGRP))
      {
        ret_val = chmod(m_filename, 0660);
        if (ret_val)
        {
          log_error(TAG, "openDB, chmod err %s\n", strerror(errno));
        }
      }

      struct group * gps_group = getgrnam("gps");
      if (gps_group != NULL)
      {
        gid_t gps_group_gid = gps_group->gr_gid;

        struct group * system_group = getgrnam("system");
        if (system_group != NULL)
        {
          gid_t system_group_gid = system_group->gr_gid;
          // set gid to system or gps whichever is not this process'es uid
          gid_t newGid = gps_group_gid == getuid() ?
                       system_group_gid : gps_group_gid;
          ret_val = chown (m_filename, -1, newGid);
          if (ret_val)
          {
            log_error(TAG, "openDB, chown err %s\n", strerror(errno));
          }
        }
      }
    }

    m_db = t_db;
    _BREAK_SQLITE_CHECK_OK(sqlite3_errcode((sqlite3*)m_db),
                           NV_PARAM_ERR_SQLITE_OTHER_ERR)
    _BREAK_SQLITE_CHECK_OK(sqlite3_exec((sqlite3*)m_db, SQL_CREATE_NVPARAM_TABLE,
                                        NULL, NULL, NULL),
                           NV_PARAM_ERR_SQLITE_OTHER_ERR)

    _BREAK_IF_NON_ZERO(NV_PARAM_ERR_SQLITE_OTHER_ERR,
                       wrapper_mprintf(m_sqlite_sentence, m_sqlite_sentence_size,
                                      "PRAGMA user_version = %d",
                                      NVPARAM_DB_USER_VERSION))
    _BREAK_SQLITE_CHECK_OK(sqlite3_exec((sqlite3*)m_db,
                           m_sqlite_sentence, NULL, NULL, NULL),
                           NV_PARAM_ERR_SQLITE_OTHER_ERR)

    log_verbose (TAG, "openDB: successful, %s 0x%x",
                 m_filename, m_db);
    result = NV_PARAM_ERR_NO_ERR;
  } while(0);

  if (result != NV_PARAM_ERR_NO_ERR)
  {
    log_error (TAG, "openDB: failed to open %s, result = %d\n",
               m_filename, result);
  }

  return result;
}

nv_param_err_code NvParamMgrImpl::deinit()
{
  log_debug(TAG, "deinit");

  if (m_sqlite_sentence != NULL)
  {
    free (m_sqlite_sentence);
    m_sqlite_sentence = NULL;
  }

  if (m_db != NULL)
  {
    sqlite3_close ((sqlite3*) m_db);
    m_db = NULL;
  }

  return NV_PARAM_ERR_NO_ERR;
}

nv_param_err_code NvParamMgrImpl::saveInt32Param (const char* param_name,
                                    const int param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as bool type
nv_param_err_code NvParamMgrImpl::saveBoolParam (const char* param_name,
                                   const bool param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as bool type
nv_param_err_code NvParamMgrImpl::saveFloatParam (const char* param_name,
                                    const float param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as uint32_t type
nv_param_err_code NvParamMgrImpl::saveUInt32Param (const char* param_name,
                                     const unsigned int param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as unsigned long long type
nv_param_err_code NvParamMgrImpl::saveInt64Param (const char* param_name,
                                    const long long param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as unsigned long long type
nv_param_err_code NvParamMgrImpl::saveUInt64Param (const char* param_name,
                                     const unsigned long long param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) (&param_value),
                         sizeof(param_value));
}

// Save the param as string type
nv_param_err_code NvParamMgrImpl::saveStringParam (const char* param_name,
                                     const char* param_value)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name, param_value, strlen(param_value));
}

nv_param_err_code NvParamMgrImpl::saveBlobParam (const char* param_name,
                                   const unsigned char* param_value,
                                   unsigned int param_size_in_bytes)
{
  AutoLock autolock(m_mutex);
  return saveNvDataBlob (param_name,
                         (const char*) param_value,
                         param_size_in_bytes);
}

nv_param_err_code NvParamMgrImpl::saveNvDataBlob (const char* param_name,
                                    const void* data,
                                    uint32_t data_len)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
  sqlite3_stmt *pStmt = NULL;

  do
  {
    if (param_name == NULL || data == NULL)
    {
      log_error (TAG, "saveNvDataBlob failed: nv param name and data can not be null");
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
      break;
    }

    log_verbose(TAG, "saveNvDataBlob (name: %s, size: %d)", param_name, data_len);
    _BREAK_IF_NON_ZERO(NV_PARAM_ERR_SQLITE_OTHER_ERR,
                       wrapper_mprintf(m_sqlite_sentence, m_sqlite_sentence_size,
                                       "INSERT OR REPLACE INTO NV_PARAM_Table"
                                       "(param_name, param_data) "
                                       "VALUES(?, ?)"))

    int sqlRes;
    _BREAK_SQLITE_CHECK_OK(sqlite3_prepare_v2((sqlite3 *)m_db, m_sqlite_sentence,
                                              -1, &pStmt, NULL), NV_PARAM_ERR_SQLITE_OTHER_ERR)
    _BREAK_SQLITE_CHECK_OK(sqlite3_bind_text (pStmt, 1, param_name,
                                              strlen (param_name), SQLITE_STATIC),
                                              NV_PARAM_ERR_SQLITE_OTHER_ERR)
    _BREAK_SQLITE_CHECK_OK(sqlite3_bind_blob (pStmt, 2, data,
                                              data_len, SQLITE_STATIC),
                                              NV_PARAM_ERR_SQLITE_OTHER_ERR)
    _BREAK_SQLITE_CHECK_DONE(sqlite3_step(pStmt), NV_PARAM_ERR_SQLITE_OTHER_ERR)

    result = NV_PARAM_ERR_NO_ERR;

  } while (0);

  // regardless, finalize pStmt
  _SQLITE_CHECK_OK_NO_RETURN(sqlite3_finalize(pStmt));

  if (result != NV_PARAM_ERR_NO_ERR)
  {
    log_error (TAG, "saveNvDataBlob (name: %s, size: %d) failed, result = %d",
               param_name, data_len, result);
  }

  return result;
}

// Retrieve the param.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getInt32Param (const char* param_name,
                                   int & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  do
  {
    // Check to see whether we need to generate pseudo client ID
    // pseudo client ID will be generated upon first request to
    // retrieve the pseudo client ID. It will only be generated once
    // per device life time
    if (strncmp (param_name, PARAM_NAME_PSEUDO_CLIENT_ID,
                 sizeof (PARAM_NAME_PSEUDO_CLIENT_ID)) == 0)
    {
      int rc = generatePseudoClientIdIfNeeded ();
      if (rc == 0)
      {
        param_value = (int) m_pseudo_client_id_32;
        result = NV_PARAM_ERR_NO_ERR;
      }
      break;
    }

    int data_len = sizeof (int);
    result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
    if (result != NV_PARAM_ERR_NO_ERR)
    {
      break;
    }

    if (data_len != sizeof (int))
    {
      log_error (TAG, "getInt32Param: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes\n",
                 param_name, sizeof (int), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID ;
      break;
    }

    result = NV_PARAM_ERR_NO_ERR;
  } while (0);

  return result;
}

// Retrieve the param as bool type.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getBoolParam (const char* param_name,
                                  bool & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  int data_len = sizeof (bool);
  result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
  if (result == NV_PARAM_ERR_NO_ERR)
  {
    if (data_len != sizeof (bool))
    {
      log_error (TAG, "getInt32Param: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes\n",
                 param_name, sizeof (bool), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
    }
  }

  return result;
}

// Retrieve the param as float type.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getFloatParam (const char* param_name,
                                   float & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  int data_len = sizeof (float);
  result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
  if (result == NV_PARAM_ERR_NO_ERR)
  {
    if (data_len != sizeof (float))
    {
      log_error (TAG, "getFloatParam: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes\n",
                 param_name, sizeof (float), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
    }
  }

  return result;
}

// Retrieve the param as uint32_t type.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getUInt32Param (const char* param_name,
                                    unsigned int & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  int data_len = sizeof (unsigned int);
  result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
  if (result == NV_PARAM_ERR_NO_ERR)
  {
    if (data_len != sizeof (unsigned int))
    {
      log_error (TAG, "getUInt32Param: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes",
                 param_name, sizeof (unsigned int), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
    }
  }

  return result;
}

nv_param_err_code NvParamMgrImpl::getInt64Param (const char* param_name,
                                   long long & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  do
  {
    // Check to see whether we need to generate 64 bit pseudo client ID
    // 64bit pseudo client ID will be generated upon first request to
    // retrieve the pseudo client ID. It will only be generated once
    // per device life time
    if (strncmp (param_name, PARAM_NAME_PSEUDO_CLIENT_ID_64BIT,
                 sizeof (PARAM_NAME_PSEUDO_CLIENT_ID_64BIT)) == 0)
    {
      int rc = generatePseudoClientIdIfNeeded ();
      if (rc == 0)
      {
        param_value = (long long) m_pseudo_client_id_64;
        result = NV_PARAM_ERR_NO_ERR;
      }
      break;
    }

    int data_len = sizeof (long long);
    result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
    if (result != NV_PARAM_ERR_NO_ERR)
    {
      break;
    }
    if (data_len != sizeof (long long))
    {
      log_error (TAG, "getInt64Param: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes\n",
                 param_name, sizeof (long long), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
      break;
    }

    result = NV_PARAM_ERR_NO_ERR;
  }while (0);

  return result;
}

nv_param_err_code NvParamMgrImpl::getUInt64Param (const char* param_name,
                                    unsigned long long & param_value)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  do
  {
    // Check to see whether we need to generate 64 bit pseudo client ID
    // 64bit pseudo client ID will be generated upon first request to
    // retrieve the pseudo client ID. It will only be generated once
    // per device life time
    if (strncmp (param_name, PARAM_NAME_PSEUDO_CLIENT_ID_64BIT,
                 sizeof (PARAM_NAME_PSEUDO_CLIENT_ID_64BIT)) == 0)
    {
      int rc = generatePseudoClientIdIfNeeded ();
      if (rc == 0)
      {
        param_value = m_pseudo_client_id_64;
        result = NV_PARAM_ERR_NO_ERR;
      }
      break;
    }

    int data_len = sizeof (unsigned long long);
    result = getNvDataBlob(param_name, ((char*) (&param_value)), data_len);
    if (result != NV_PARAM_ERR_NO_ERR)
    {
      break;
    }
    if (data_len != sizeof (unsigned long long))
    {
      log_error (TAG, "getUInt64Param: failed for param %s, "
                      "expected %d bytes, retrieved %d bytes\n",
                 param_name, sizeof (unsigned long long), data_len);
      result = NV_PARAM_ERR_PRRAM_NOT_VALID;
      break;
    }

    result = NV_PARAM_ERR_NO_ERR;
  }while (0);

  return result;
}

// Retrieve the param as string type.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getStringParam (const char* param_name,
                                    char* &param_value,
                                    unsigned int & param_size_in_bytes)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  do
  {
    int data_len = 0;
    char* data = NULL;
    result = getNvDataBlob(param_name, &data, data_len);

    if (result != NV_PARAM_ERR_NO_ERR)
    {
      break;
    }

    param_value = data;
    param_size_in_bytes = data_len;
  }while (0);

  return result;
}

// Retrieve the param as string type.
// If param can not be retrieved, the param_value will not be changed.
nv_param_err_code NvParamMgrImpl::getBlobParam (const char* param_name,
                                  unsigned char* & param_value,
                                  unsigned int & param_size_in_bytes)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;

  AutoLock autolock(m_mutex);
  do
  {
    int data_len = 0;
    unsigned char* data = NULL;
    result = getNvDataBlob(param_name, &data, data_len);

    if (result != NV_PARAM_ERR_NO_ERR)
    {
      break;
    }

    param_value = data;
    param_size_in_bytes = data_len;
  }while (0);

  return result;
}

nv_param_err_code NvParamMgrImpl::getNvDataBlob (const char* param_name,
                                   void* data,
                                   int &data_len)
{
  nv_param_err_code result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
  int sqlRes = SQLITE_OK;
  sqlite3_stmt *pStmt = NULL;

  do
  {
    _BREAK_IF_NON_ZERO(NV_PARAM_ERR_SQLITE_OTHER_ERR,
                       wrapper_mprintf (m_sqlite_sentence, m_sqlite_sentence_size,
                          "SELECT param_data FROM NV_PARAM_Table "
                          "WHERE  param_name == \"%s\"", param_name));

    _BREAK_SQLITE_CHECK_OK(sqlite3_prepare_v2((sqlite3 *)m_db, m_sqlite_sentence,
                                              strlen(m_sqlite_sentence)+1,
                                              &pStmt, NULL), NV_PARAM_ERR_SQLITE_OTHER_ERR);

    // get the first result out
    int sqlRes = sqlite3_step(pStmt);
    if (sqlRes == SQLITE_ROW)
    {
      int blob_len = sqlite3_column_bytes(pStmt, 0);
      const char* blob = (const char*)sqlite3_column_blob(pStmt, 0);
      if (blob == NULL)
      {
        result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
        break;
      }
      char* blob_copy = NULL;
      if (data_len == 0)
      {
        blob_copy = new (std::nothrow) char [blob_len];
        if (blob_copy == NULL)
        {
          result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
          break;
        }
        memcpy ((void*)blob_copy, blob, blob_len);
        *((char**) data) = blob_copy;
        data_len = blob_len;
        result = NV_PARAM_ERR_NO_ERR;
      }
      else if (data_len >= blob_len)
      {
        memcpy (data, blob, blob_len);
        data_len = blob_len;
        result = NV_PARAM_ERR_NO_ERR;
      }
      else
      {
        log_warning (TAG, "getNvDataBlob, nv item %s size not expected, "
                          "expected <= %d bytes, received %d bytes",
                     param_name, data_len, blob_len);
        result = NV_PARAM_ERR_PRRAM_NOT_VALID;
      }
    }
    else if (sqlRes == SQLITE_DONE)
    {
      log_error (TAG, "getNvDataBlob, nv item %s not found", param_name);
      result = NV_PARAM_ERR_PARAM_NOT_FOUND;
    }
    else if (sqlRes == SQLITE_BUSY || sqlRes == SQLITE_LOCKED)
    {
      log_error (TAG, "getNvDataBlob, SQLITE is busy, retry later");
      result = NV_PARAM_ERR_SQLITE_BUSY;
    }
    else
    {
      log_error (TAG, "getNvDataBlob: failed %s\n",
                 sqlite3_errmsg((sqlite3 *)m_db));
      result = NV_PARAM_ERR_SQLITE_OTHER_ERR;
    }
  } while (0);

  // regardless, finalize pStmt
  _SQLITE_CHECK_OK_NO_RETURN(sqlite3_finalize(pStmt));

  if (result != NV_PARAM_ERR_NO_ERR)
  {
    log_debug (TAG, "getNvDataBlob: result = %d", result);
  }

  return result;
}

// 0: pseudo client already exists or is generated successfully
// non-0: pseudo client does not exist and can not be generated successfully
// As this is internval function, return value is not mapped to NV_PARAM_ERR_XXX.
int NvParamMgrImpl::generatePseudoClientIdIfNeeded ()
{
  // This is internal function, no need to hold the lock
  int      ret_val = -1;
  unsigned long long random_uint64 = 0;
  bool     is_valid = false;

  do
  {
    // No-op if pseudo client ID has been generated
    if (m_pseudo_client_id_exist == true)
    {
      ret_val = 0;
      break;
    }

    // try to retrieve it from sqlite, if it is found
    // set m_pseudo_client_id_exist to true
    unsigned long long param_value;

    int data_len = sizeof (unsigned long long);
    ret_val = getNvDataBlob(PARAM_NAME_PSEUDO_CLIENT_ID_64BIT,
                            ((char*) (&param_value)), data_len);
    if ((ret_val == 0) && (data_len == sizeof (unsigned long long)))
    {
      m_pseudo_client_id_64 = param_value;
      m_pseudo_client_id_32 = (int32_t) (param_value & 0x00ffffffff);
      m_pseudo_client_id_exist = true;
      ret_val = 0;
      break;
    }

#ifndef IZAT_OFFLINE
    // On Android, we first try to use hardware random number generator
    // If for any reason that fails, we fall back to UTC time
    int      random_fd = -1;
    struct   pollfd fds[1]; // polling file descriptor

    random_fd = open(RANDOM_DEVICE, O_RDONLY);
    if (random_fd == -1)
    {
      log_error (TAG, "generatePseudoClientIdIfNeeded: "
                      "open random number device failed: %s\n",
                 strerror(errno));
      break;
    }

    // setup poll() data
    memset(fds, 0 , sizeof(fds));
    fds[0].fd = random_fd;
    fds[0].events = POLLIN;

    int retry_count;
    for (retry_count = 0; retry_count < 5; retry_count++)
    {
      // Wait 2000ms on the data to become available
      int rc = poll(fds, 1, 2000);
      if (rc == 0)
      {
        log_error (TAG, "generatePseudoClientIdIfNeeded: "
                        "poll random number device failed: %s\n",
                   strerror (errno));
        continue;
      }

      int read_size = read(random_fd, &random_uint64, sizeof (random_uint64));
      // any read failure or not able to read out 4-byte is bad
      if (read_size != sizeof (random_uint64))
      {
         log_error (TAG, "generatePseudoClientIdIfNeeded: "
                         "read from random number device failed, "
                         "read %d bytes, expecting %d bytes, error %s\n",
                 read_size, sizeof (random_uint64), strerror (errno));
        continue;
      }

      // ensure pseudo client id both 32 bit and 64 bit is not 0
      uint32_t random_uint32 = (int32_t) (random_uint64 & 0x00ffffffff);
      if (random_uint32 == 0)
      {
        continue;
      }
      is_valid = true;
      break;
    }

    // close the random number generator
    if (random_fd >= 0)
    {
      close (random_fd);
      random_fd = -1;
    }

    // hardware pseudo number generator failed or random_int generated is 0,
    // use RTC time as seed and software based pseudo number generator
    if (is_valid == false)
    {
      int time_sec = time (NULL);
      // Make sure device is getting proper time
      // Will need to be larger than 43 years interval (1970 - 2014)
      if (time_sec > (43 * 365 * 24 * 60 * 60 ))
      {
        srand48 (time_sec);
        // ensure the lower 32 bit of psuedo client id is not 0
        for (int i = 0; i < 5; i++)
        {
          random_uint64 = (uint32_t) mrand48();
          if (random_uint64 != 0)
          {
            break;
          }
        }
        random_uint64 <<= 32;
        random_uint64 += (uint32_t) mrand48();
        is_valid = true;
      }
      else
      {
        log_debug (TAG, "generatePseudoClientIdIfNeeded: time is too small for seed");
      }
    }

    // For real target, we prevent 0 to be used as pseudo client id
    // so we will keep trying to generate a none zero pseudo client id
    // when pseudo client id is needed
#else
    int time_sec = time (NULL);
    // Make sure device is getting proper time
    // Will need to be larger than 43 years interval (1970 - 2014)
    if (time_sec > (43 * 365 * 24 * 60 * 60 ))
    {
      srand48 (time_sec);
      random_uint64 = (uint32_t) mrand48();
      random_uint64 <<= 32;
      random_uint64 += (uint32_t) mrand48();
      is_valid = true;
    }
    else
    {
      log_debug (TAG, "generatePseudoClientIdIfNeeded: time is too small for seed");
    }

    is_valid = true;
#endif

    if (is_valid == true)
    {
      m_pseudo_client_id_64 = random_uint64;
      m_pseudo_client_id_32 = (int32_t) (random_uint64 & 0x00ffffffff);
      int rc1 = saveNvDataBlob (PARAM_NAME_PSEUDO_CLIENT_ID,
                                &m_pseudo_client_id_32, sizeof (m_pseudo_client_id_32));
      int rc2 = saveNvDataBlob (PARAM_NAME_PSEUDO_CLIENT_ID_64BIT,
                                &m_pseudo_client_id_64, sizeof (m_pseudo_client_id_64));
      if (rc1 == 0 || rc2 == 0)
      {
        m_pseudo_client_id_exist = true;
        ret_val = 0;
      }
      else
      {
        ret_val = -10;
      }
    }
  } while (0);

  if (ret_val != 0)
  {
    log_error (TAG, "generatePseudoClientIdIfNeeded: failed, ret_val = %d", ret_val);
  }

  return ret_val;
}

// Output is the buffer that where output string should be written,
// it can be NULL if output need to be allocated.
//
// Size is the minimum output buffer to be allocated for subsequent reuse
// if size is set to 0, program will determine the size for the output
//
// As this is an internal function, the returned result does not need
// to map to NV_PARAM error code
int NvParamMgrImpl::wrapper_mprintf(char* &output_buf, unsigned int &output_size,
                                    const char *sentence,... )
{
  int result = -1;
  int nchars = 0;
  char temp_buf[1];

  do
  {
    va_list args;
    va_start (args, sentence);
    if (output_buf != NULL)
    {
      nchars = vsnprintf(output_buf, output_size, sentence, args);
    }
    else
    {
      nchars = vsnprintf(temp_buf, sizeof (temp_buf), sentence, args);
    }

    va_end (args);

    if (nchars <= 0)
    {
      log_error(TAG, "vsnprintf failed, return %d bytes writen\n", nchars);
      result = -2;
      break;
    }

    if ((output_buf == NULL) || (((unsigned int)nchars + 1) > output_size))
    {
      // if the needed buffer to store the output is less than
      // the input output_size, we go ahead allocate more memory
      // as specified by output_size
      if (((unsigned int)nchars + 1) > output_size)
      {
        output_size = (unsigned int) nchars + 1;
      }

      if (output_buf == NULL)
      {
        output_buf = (char*) malloc (output_size);
      }
      else
      {
        output_buf = (char*) realloc (output_buf, output_size);
      }

      if (output_buf == NULL)
      {
        log_error(TAG, "failed to realloc %d bytes\n", output_size);
        result = -3;
        break;
      }

      va_list args_2;
      va_start (args_2, sentence);
      nchars = vsnprintf (output_buf, output_size ,sentence,args_2);
      va_end (args_2);

      if(nchars < 0)
      {
        log_error(TAG, "vsnprintf failed, error code %d", nchars);
        result = -4;
        break;
      }
      else if ((unsigned int) nchars >= output_size)
      {
        log_error(TAG, "vsnprintf failed, size written %d bytes exceed buf size %d bytes, "
                       "failed sentence\n", nchars, output_size, sentence);
        result = -5;
        break;
      }
    }

    result = 0;
  }while(0);

  if (result != NV_PARAM_ERR_NO_ERR)
  {
    log_error(TAG, "wrapper_mprintf failed result = %d\n", result);
  }
  else
  {
    log_verbose (TAG, "wrapper_mprintf successful, returned string:"
                      " %s, size = %d",
                 output_buf, output_size);
  }

  return result;
}

NvParamMgr * NvParamMgr::getInstance()
{
  AutoLock autolock(m_mutex);

  // if m_nv_mgr is NULL, try to create one and initialize it
  if (m_nv_mgr == NULL)
  {
    m_nv_mgr = new (std::nothrow) NvParamMgrImpl (NVPARAM_NAME);
    if ((m_nv_mgr != NULL) && (m_nv_mgr->init () != NV_PARAM_ERR_NO_ERR))
    {
      delete m_nv_mgr;
      m_nv_mgr = NULL;
    }
  }

  if (m_nv_mgr != NULL)
  {
    m_ref_cnt++;
  }

  return m_nv_mgr;
}

void NvParamMgr::releaseInstance()
{
  AutoLock autolock(m_mutex);

  if (m_nv_mgr != NULL)
  {
    if (--m_ref_cnt <= 0)
    {
      m_nv_mgr->deinit ();
      delete m_nv_mgr;
      m_nv_mgr = NULL;
      m_ref_cnt = 0;
    }
  }
}

} // namespace qc_loc_fw
