/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 Memory stream

 GENERAL DESCRIPTION
 This component implements one input memory stream and one auto-expandable
 output memory stream.

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <new>
#include <string.h>

#include <base_util/log.h>
#include <base_util/memorystream.h>

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

namespace qc_loc_fw
{

static const char * const TAG = "MemoryStream";

MemoryStreamBase::~MemoryStreamBase()
{
  // dummy destructor
}

OutMemoryStream::~OutMemoryStream()
{
  // dummy destructor
}

InMemoryStream::~InMemoryStream()
{
  // dummy destructor
}

class OutMemoryStreamImpl: public OutMemoryStream
{
public:
  OutMemoryStreamImpl();
  ~OutMemoryStreamImpl();
  size_t getSize() const
  {
    return getPutCursor();
  }

  BYTE * getBufferNonConst()
  {
    return m_pData;
  }

  const BYTE * getBuffer() const
  {
    return m_pData;
  }

  size_t getPutCursor() const
  {
    return m_put_cursor;
  }

  int append(const void * const pData, const size_t length);

  void releaseBufferOwnership()
  {
    m_pData = 0;
    m_put_cursor = 0;
    m_capacity = 0;
  }

private:

  BYTE * m_pData;
  size_t m_put_cursor;
  size_t m_capacity;

  static const size_t min_length = 64;
  int expand(const size_t length);
};

OutMemoryStreamImpl::OutMemoryStreamImpl() :
    m_pData(0), m_put_cursor(0), m_capacity(0)
{
}

OutMemoryStreamImpl::~OutMemoryStreamImpl()
{
  if(0 != m_pData)
  {
    delete[] m_pData;
    m_pData = 0;
  }
  m_put_cursor = 0;
  m_capacity = 0;
}

int OutMemoryStreamImpl::expand(const size_t length)
{
  int result = 1;
  do
  {
    if(0 >= length)
    {
      result = 2;
      break;
    }

    const size_t space_left = m_capacity - m_put_cursor;
    if(length < space_left)
    {
      result = 0;
      break;
    }

    size_t new_length = m_put_cursor + length;
    if(new_length < min_length)
    {
      new_length = min_length;
    }
    else if(new_length < (m_capacity * 2))
    {
      new_length = m_capacity * 2;
    }

    BYTE * new_buffer = new (std::nothrow) BYTE[new_length];

    if(0 == new_buffer)
    {
      result = 3;
      break;
    }

    if(0 != m_pData)
    {
      memcpy(new_buffer, m_pData, m_put_cursor);
      delete[] m_pData;
    }
    m_pData = new_buffer;
    m_capacity = new_length;
    // m_put_cursor is not changed

    //log_verbose(TAG, "stream expanded to %u bytes, with length %u", (unsigned int)m_capacity, (unsigned int)m_put_cursor);

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "expand error %d", result);
  }
  return result;
}
int OutMemoryStreamImpl::append(const void * const pData, const size_t length)
{
  int result = 1;
  do
  {
    if(0 != expand(length))
    {
      result = 2;
      break;
    }

    memcpy(m_pData + m_put_cursor, pData, length);
    m_put_cursor += length;

    //log_verbose(TAG, "current stream length %u", (unsigned int)m_put_cursor);

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "append error %d", result);
  }
  return result;
}

class InMemoryStreamImpl: public InMemoryStream
{
public:
  InMemoryStreamImpl(OutMemoryStream * const os = 0);
  ~InMemoryStreamImpl();
  size_t getSize() const
  {
    return getCapacity();
  }

  const BYTE * getBuffer() const
  {
    return m_pData;
  }

  int setBufferOwnership(const void ** const ppIn, const size_t length)
  {
    int result = 1;
    do
    {
      BREAK_IF_ZERO(2, ppIn);

      BREAK_IF_NON_ZERO(3, setBufferNoDup(*ppIn, length));
      *ppIn = 0;
      m_ownership = true;

      result = 0;
    } while (0);

    if(0 != result)
    {
      log_error(TAG, "setBufferOwnership failed %d", result);
    }
    return result;
  }

  int setBufferNoDup(const void * const pIn, const size_t length)
  {
    if(m_ownership)
    {
      delete[] m_pData;
      m_ownership = false;
    }
    m_pData = reinterpret_cast<const BYTE*>(pIn);
    m_capacity = length;
    m_get_cursor = 0;
    return 0;
  }

  int extract(void * const pData, const size_t length);

  size_t getGetCursor() const
  {
    return m_get_cursor;
  }

  size_t getCapacity() const
  {
    return m_capacity;
  }

  int setGetCursor(const size_t cursor)
  {
    if(cursor <= m_capacity)
    {
      m_get_cursor = cursor;
      return 0;
    }
    else
    {
      log_error(TAG, "setGetCursor range error");
      return 1;
    }
  }
  virtual InMemoryStream * clone() const;

private:
  bool m_ownership;
  const BYTE * m_pData;
  size_t m_get_cursor;
  size_t m_capacity;
};

InMemoryStreamImpl::InMemoryStreamImpl(OutMemoryStream * const os) :
    m_ownership(false), m_pData(0), m_get_cursor(0), m_capacity(0)
{
  if(os != 0)
  {
    m_ownership = true;
    m_pData = os->getBuffer();
    m_capacity = os->getSize();
    static_cast<OutMemoryStreamImpl *>(os)->releaseBufferOwnership();
  }
}

InMemoryStreamImpl::~InMemoryStreamImpl()
{
  if(m_ownership)
  {
    delete[] m_pData;
  }
  m_pData = 0;
  m_get_cursor = 0;
  m_capacity = 0;
}

int InMemoryStreamImpl::extract(void * const pData, const size_t length)
{
  int result = 1;
  do
  {
    if(0 == length)
    {
      result = 3;
      break;
    }
    if(0 == m_pData)
    {
      result = 4;
      break;
    }

    const size_t data_left = m_capacity - m_get_cursor;
    if(data_left < length)
    {
      result = 5;
      break;
    }

    if(0 != pData)
    {
      memcpy(pData, m_pData + m_get_cursor, length);
    }
    m_get_cursor += length;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "extract error %d", result);
  }
  return result;
}

OutMemoryStream * OutMemoryStream::createInstance()
{
  return new (std::nothrow) OutMemoryStreamImpl();
}

InMemoryStream * InMemoryStream::createInstance()
{
  return new (std::nothrow) InMemoryStreamImpl();
}

InMemoryStream * InMemoryStream::createInstance(OutMemoryStream * const os)
{
  return new (std::nothrow) InMemoryStreamImpl(os);
}

InMemoryStream * InMemoryStreamImpl::clone() const
{
  InMemoryStreamImpl *pNewIs = (InMemoryStreamImpl *)InMemoryStream::createInstance();

  if (NULL != pNewIs) {
    // do shallow copy first
    *pNewIs = *this;
    if (m_pData) {
      pNewIs->m_pData = new BYTE[m_capacity];
      if (pNewIs->m_pData) {
        memcpy((void *)pNewIs->m_pData, (const void *)m_pData, m_capacity);
        pNewIs->m_ownership = true;
      } else {
        delete pNewIs;
        pNewIs = NULL;
      }
    }
  }

  return pNewIs;
}

} // namespace qc_loc_fw
