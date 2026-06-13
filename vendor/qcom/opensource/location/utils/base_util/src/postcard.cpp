/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 IPC message holder

 GENERAL DESCRIPTION
 This component implements two IPC message holders, one for input and one
 for output

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <new>
#include <string.h>

#include <base_util/log.h>
#include <base_util/memorystream.h>
#include <base_util/postcard.h>

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

namespace qc_loc_fw
{

static const char * const IN_TAG = "InPostcard";
static const char * const OUT_TAG = "OutPostcard";

PostcardBase::~PostcardBase()
{
  // dummy destructor
}

OutPostcard::~OutPostcard()
{
  // dummy destructor
}

InPostcard::~InPostcard()
{
  // dummy destructor
}

enum eFieldType
{
  FtUnknown = 0,
  FtCard = 1,
  FtInt64 = 10,
  FtUInt64 = 11,
  FtInt32 = 12,
  FtUInt32 = 13,
  FtInt16 = 14,
  FtUInt16 = 15,
  FtInt8 = 16,
  FtUInt8 = 17,
  FtBool = 18,
  FtString = 19,
  FtBlob = 20,
  FtDouble = 21,
  FtFloat = 22,

  FtArrayInt64 = 30,
  FtArrayUInt64 = 31,
  FtArrayInt32 = 32,
  FtArrayUInt32 = 33,
  FtArrayInt16 = 34,
  FtArrayUInt16 = 35,
  FtArrayInt8 = 36,
  FtArrayUInt8 = 37,
  FtArrayBool = 38,
  FtArrayDouble = 39,
  FtArrayFloat = 40,
};

typedef PostcardBase::UINT32 LengthType;
typedef PostcardBase::UINT8 EndType;
typedef PostcardBase::UINT16 MetaFieldType;
typedef PostcardBase::UINT16 NameLengthType;

template<MetaFieldType FT, typename VT>
class SimpleNode
{
public:
  static const MetaFieldType field_type;
  typedef VT ValueType;
  ValueType & value;
  SimpleNode(ValueType & v) :
      value(v)
  {
  }
};

// we cannot declare this inside the class itself,
// for we want to take its address later
// defining it inside the class would be similar to #define it
template<MetaFieldType FT, typename VT>
const MetaFieldType SimpleNode<FT, VT>::field_type = FT;

typedef SimpleNode<FtDouble, PostcardBase::DOUBLE> MetaDouble;
typedef SimpleNode<FtFloat, PostcardBase::FLOAT> MetaFloat;
typedef SimpleNode<FtInt64, PostcardBase::INT64> MetaInt64;
typedef SimpleNode<FtUInt64, PostcardBase::UINT64> MetaUInt64;
typedef SimpleNode<FtInt32, PostcardBase::INT32> MetaInt32;
typedef SimpleNode<FtUInt32, PostcardBase::UINT32> MetaUInt32;
typedef SimpleNode<FtInt16, PostcardBase::INT16> MetaInt16;
typedef SimpleNode<FtUInt16, PostcardBase::UINT16> MetaUInt16;
typedef SimpleNode<FtInt8, PostcardBase::INT8> MetaInt8;
typedef SimpleNode<FtUInt8, PostcardBase::UINT8> MetaUInt8;
typedef SimpleNode<FtBool, PostcardBase::UINT8> MetaBool;

typedef SimpleNode<FtDouble, const PostcardBase::DOUBLE> MetaDoubleC;
typedef SimpleNode<FtFloat, const PostcardBase::FLOAT> MetaFloatC;
typedef SimpleNode<FtInt64, const PostcardBase::INT64> MetaInt64C;
typedef SimpleNode<FtUInt64, const PostcardBase::UINT64> MetaUInt64C;
typedef SimpleNode<FtInt32, const PostcardBase::INT32> MetaInt32C;
typedef SimpleNode<FtUInt32, const PostcardBase::UINT32> MetaUInt32C;
typedef SimpleNode<FtInt16, const PostcardBase::INT16> MetaInt16C;
typedef SimpleNode<FtUInt16, const PostcardBase::UINT16> MetaUInt16C;
typedef SimpleNode<FtInt8, const PostcardBase::INT8> MetaInt8C;
typedef SimpleNode<FtUInt8, const PostcardBase::UINT8> MetaUInt8C;
typedef SimpleNode<FtBool, const PostcardBase::UINT8> MetaBoolC;

template<MetaFieldType FT, typename ET>
class ArrayNode
{
public:
  static const MetaFieldType field_type;
  typedef ET * ElemType;
  ElemType & head;
  const int num_element;
  ArrayNode(ElemType & h, const int num) :
    head(h), num_element(num)
  {
  }
};

class OutPostcardImpl: public OutPostcard
{
public:
  OutPostcardImpl();
  ~OutPostcardImpl();

  int init();
  int finalize();
  const MemoryStreamBase * getEncodedBuffer() const
  {
    return m_pOut;
  }

  OutMemoryStream * getInternalBuffer()
  {
    return m_pOut;
  }

  int addDouble(const char * const name, const DOUBLE & value)
  {
    return add(name, MetaDoubleC(value));
  }
  int addFloat(const char * const name, const FLOAT & value)
  {
    return add(name, MetaFloatC(value));
  }
  int addInt64(const char * const name, const INT64 & value)
  {
    return add(name, MetaInt64C(value));
  }
  int addUInt64(const char * const name, const UINT64 & value)
  {
    return add(name, MetaUInt64C(value));
  }
  int addInt32(const char * const name, const INT32 & value)
  {
    return add(name, MetaInt32C(value));
  }
  int addUInt32(const char * const name, const UINT32 & value)
  {
    return add(name, MetaUInt32C(value));
  }
  int addInt16(const char * const name, const INT16 & value)
  {
    return add(name, MetaInt16C(value));
  }
  int addUInt16(const char * const name, const UINT16 & value)
  {
    return add(name, MetaUInt16C(value));
  }
  int addInt8(const char * const name, const INT8 & value)
  {
    return add(name, MetaInt8C(value));
  }
  int addUInt8(const char * const name, const UINT8 & value)
  {
    return add(name, MetaUInt8C(value));
  }
  int addBool(const char * const name, const BOOL & value)
  {
    return add(name, MetaBoolC((true == value) ? 1 : 0));
  }
  int addPtr(const char * const name, const PTR & value)
  {
#ifdef ANDROID_32_BIT_PTR_SUPPORT
      return add(name, MetaInt32C((INT32)value));
#else // 64-bit ptr
      return add(name, MetaInt64C((INT64)value));
#endif
  }
  int addString(const char * const name, const char * const str);
  int addBlob(const char * const name, const void * const blob, const size_t length);
  int addCard(const char * const name, const OutPostcard * const pCard);

  int addArrayDouble(const char * const name, const int num_element, const DOUBLE array[]);
  int addArrayFloat(const char * const name, const int num_element, const FLOAT array[]);
  int addArrayInt64(const char * const name, const int num_element, const INT64 array[]);
  int addArrayUInt64(const char * const name, const int num_element, const UINT64 array[]);
  int addArrayInt32(const char * const name, const int num_element, const INT32 array[]);
  int addArrayUInt32(const char * const name, const int num_element, const UINT32 array[]);
  int addArrayInt16(const char * const name, const int num_element, const INT16 array[]);
  int addArrayUInt16(const char * const name, const int num_element, const UINT16 array[]);
  int addArrayInt8(const char * const name, const int num_element, const INT8 array[]);
  int addArrayUInt8(const char * const name, const int num_element, const UINT8 array[]);
  int addArrayBool(const char * const name, const int num_element, const BOOL array[]);
  int addArrayPtr(const char * const name, const int num_element, const PTR array[]);

private:
  enum eCardState
  {
    CS_PRE_INIT, CS_ADD, CS_FINALIZED
  };
  eCardState m_state;
  OutMemoryStream * m_pOut;

  template<typename META_V>
  int add(const char * const name, const META_V & mv);

  template<typename ELEM_TYPE>
  int addArray(const char * const name, const eFieldType fieldType, const int num_element, const ELEM_TYPE array[]);

  int addBlobWithType(const char * const name, const eFieldType fieldType, const void * const blob, const size_t length,
      const bool pushLength);
};

OutPostcardImpl::OutPostcardImpl() :
    m_state(CS_PRE_INIT), m_pOut(0)
{
}

OutPostcardImpl::~OutPostcardImpl()
{
  delete m_pOut;
  m_pOut = 0;
}

int OutPostcardImpl::init()
{
  int result = 1;
  do
  {
    if(CS_PRE_INIT != m_state)
    {
      result = 2;
      break;
    }

    m_pOut = OutMemoryStream::createInstance();
    BREAK_IF_ZERO(3, m_pOut);

    const LengthType card_length = 0x12345678;
    m_pOut->append(&card_length, sizeof(card_length));

    m_state = CS_ADD;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "init failed %d", result);
  }
  return result;
}

int OutPostcardImpl::finalize()
{
  int result = 1;
  do
  {
    if(CS_ADD != m_state)
    {
      result = 2;
      break;
    }

    // we should have inserted the length at init!
    if(m_pOut->getSize() < sizeof(LengthType))
    {
      result = 3;
      break;
    }

    //log_verbose(OUT_TAG, "A data length to send %u", (unsigned int)m_pOut->getSize());

    // add trailing zero of size 1
    const LengthType card_length = LengthType(m_pOut->getSize()) + 1 - sizeof(card_length);
    memcpy(m_pOut->getBufferNonConst(), &card_length, sizeof(card_length));

    const EndType trailing_zero = 0;
    BREAK_IF_NON_ZERO(3, m_pOut->append(&trailing_zero, sizeof(trailing_zero)));

    //log_verbose(OUT_TAG, "B data length to send %u, card length %u", (unsigned int)m_pOut->getSize(), (unsigned int)card_length);

    m_state = CS_FINALIZED;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "init failed %d", result);
  }
  return result;
}

template<typename META_V>
int OutPostcardImpl::add(const char * const name, const META_V & mv)
{
  int result = 1;
  do
  {
    if(CS_ADD != m_state)
    {
      result = 2;
      break;
    }

    BREAK_IF_ZERO(3, name);

    size_t name_length = strlen(name);
    BREAK_IF_ZERO(4, name_length);

    if(name_length >= MAX_ENCODE_NAME_LENGTH)
    {
      result = 5;
      break;
    }
    NameLengthType name_length_short = (NameLengthType) (name_length + 1);

    BREAK_IF_NON_ZERO(6, m_pOut->append(&META_V::field_type, sizeof(META_V::field_type)));
    BREAK_IF_NON_ZERO(7, m_pOut->append(&name_length_short, sizeof(name_length_short)));
    BREAK_IF_NON_ZERO(8, m_pOut->append(name, name_length_short));
    BREAK_IF_NON_ZERO(9, m_pOut->append(&mv.value, sizeof(mv.value)));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "add simple failed %d", result);
  }
  return result;
}

int OutPostcardImpl::addBlob(const char * const name, const void * const blob, const size_t length)
{
  return addBlobWithType(name, FtBlob, blob, length, true);
}

int OutPostcardImpl::addBlobWithType(const char * const name, const eFieldType fieldType, const void * const blob,
    const size_t length, const bool pushLength)
{
  int result = 1;
  do
  {
    if(CS_ADD != m_state)
    {
      result = 2;
      break;
    }

    BREAK_IF_ZERO(3, name);

    size_t name_length = strlen(name);
    BREAK_IF_ZERO(4, name_length);

    if(name_length >= MAX_ENCODE_NAME_LENGTH)
    {
      result = 5;
      break;
    }
    NameLengthType name_length_short = (NameLengthType) (name_length + 1);

    BREAK_IF_ZERO(6, blob);
    LengthType blob_length = length;

    const MetaFieldType type = fieldType;
    BREAK_IF_NON_ZERO(8, m_pOut->append(&type, sizeof(type)));

    BREAK_IF_NON_ZERO(9, m_pOut->append(&name_length_short, sizeof(name_length_short)));
    BREAK_IF_NON_ZERO(10, m_pOut->append(name, name_length_short));
    if(pushLength)
    {
      BREAK_IF_NON_ZERO(11, m_pOut->append(&blob_length, sizeof(blob_length)));
    }
    BREAK_IF_NON_ZERO(12, m_pOut->append(blob, blob_length));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "addBlobWithType failed %d", result);
  }
  return result;
}

template<typename ELEM_TYPE>
int OutPostcardImpl::addArray(const char * const name, const eFieldType fieldType, const int num_element, const ELEM_TYPE array[])
{
  int result = 1;
  do
  {
    if(CS_ADD != m_state)
    {
      result = 2;
      break;
    }

    BREAK_IF_ZERO(3, name);

    const size_t name_length = strlen(name);
    BREAK_IF_ZERO(4, name_length);

    if(name_length >= MAX_ENCODE_NAME_LENGTH)
    {
      result = 5;
      break;
    }
    const NameLengthType name_length_short = (NameLengthType) (name_length + 1);

    BREAK_IF_ZERO(6, array);
    if(num_element <= 0)
    {
      result = 7;
      break;
    }
    const LengthType data_block_length = num_element * sizeof(ELEM_TYPE);

    const MetaFieldType type = (MetaFieldType)fieldType;
    BREAK_IF_NON_ZERO(8, m_pOut->append(&type, sizeof(type)));

    BREAK_IF_NON_ZERO(9, m_pOut->append(&name_length_short, sizeof(name_length_short)));
    BREAK_IF_NON_ZERO(10, m_pOut->append(name, name_length_short));
    BREAK_IF_NON_ZERO(11, m_pOut->append(&data_block_length, sizeof(data_block_length)));
    BREAK_IF_NON_ZERO(12, m_pOut->append(array, data_block_length));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "add array failed %d", result);
  }
  return result;
}

int OutPostcardImpl::addArrayBool(const char * const name, const int num_element, const BOOL array[])
{
  return addArray(name, FtArrayBool, num_element, array);
}

int OutPostcardImpl::addArrayPtr(const char * const name, const int num_element, const PTR array[])
{
  // 8-bytes ptr
  if (sizeof (PTR) == 8)
  {
    return addArray(name, FtArrayInt64, num_element, array);
  }
  // 4-byte ptr
  else
  {
    return addArray(name, FtArrayInt32, num_element, array);
  }
}

int OutPostcardImpl::addArrayInt8(const char * const name, const int num_element, const INT8 array[])
{
  return addArray(name, FtArrayInt8, num_element, array);
}

int OutPostcardImpl::addArrayUInt8(const char * const name, const int num_element, const UINT8 array[])
{
  return addArray(name, FtArrayUInt8, num_element, array);
}

int OutPostcardImpl::addArrayInt16(const char * const name, const int num_element, const INT16 array[])
{
  return addArray(name, FtArrayInt16, num_element, array);
}

int OutPostcardImpl::addArrayUInt16(const char * const name, const int num_element, const UINT16 array[])
{
  return addArray(name, FtArrayUInt16, num_element, array);
}

int OutPostcardImpl::addArrayInt32(const char * const name, const int num_element, const INT32 array[])
{
  return addArray(name, FtArrayInt32, num_element, array);
}

int OutPostcardImpl::addArrayUInt32(const char * const name, const int num_element, const UINT32 array[])
{
  return addArray(name, FtArrayUInt32, num_element, array);
}

int OutPostcardImpl::addArrayInt64(const char * const name, const int num_element, const INT64 array[])
{
  return addArray(name, FtArrayInt64, num_element, array);
}

int OutPostcardImpl::addArrayUInt64(const char * const name, const int num_element, const UINT64 array[])
{
  return addArray(name, FtArrayUInt64, num_element, array);
}

int OutPostcardImpl::addArrayFloat(const char * const name, const int num_element, const FLOAT array[])
{
  return addArray(name, FtArrayFloat, num_element, array);
}

int OutPostcardImpl::addArrayDouble(const char * const name, const int num_element, const DOUBLE array[])
{
  return addArray(name, FtArrayDouble, num_element, array);
}

int OutPostcardImpl::addString(const char * const name, const char * const str)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, str);
    size_t str_length = strlen(str) + 1;

    BREAK_IF_NON_ZERO(4, addBlobWithType(name, FtString, str, str_length, true));
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "add string failed %d", result);
  }
  return result;
}

int OutPostcardImpl::addCard(const char * const name, const OutPostcard * const pCard)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, pCard);

    const OutPostcardImpl * const pCardImpl = reinterpret_cast<const OutPostcardImpl *>(pCard);
    if(CS_FINALIZED != pCardImpl->m_state)
    {
      result = 4;
      break;
    }

    BREAK_IF_NON_ZERO(5,
        addBlobWithType(name, FtCard, pCardImpl->m_pOut->getBuffer(), pCardImpl->m_pOut->getSize(), false));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(OUT_TAG, "add card failed %d", result);
  }
  return result;
}

class InPostcardImpl: public InPostcard
{
public:
  InPostcardImpl();
  InPostcardImpl(InMemoryStream * pInMem);
  InPostcardImpl(OutPostcard * const pCard);

  ~InPostcardImpl();

  int init(const void * const pIn, const size_t length, bool assume_ownership = false);

  const MemoryStreamBase * getBuffer() const;

  // it appears we need a temporary object v, instead of a nameless object like MetaInt32(value),
  // so the reference used in getSimple can be instantiated
  int getDouble(const char * const name, DOUBLE & value)
  {
    MetaDouble v(value);
    return getSimple(name, v);
  }
  int getFloat(const char * const name, FLOAT & value)
  {
    MetaFloat v(value);
    return getSimple(name, v);
  }
  int getInt64(const char * const name, INT64 & value)
  {
    MetaInt64 v(value);
    return getSimple(name, v);
  }
  int getUInt64(const char * const name, UINT64 & value)
  {
    MetaUInt64 v(value);
    return getSimple(name, v);
  }
  int getInt32(const char * const name, INT32 & value)
  {
    MetaInt32 v(value);
    return getSimple(name, v);
  }
  int getUInt32(const char * const name, UINT32 & value)
  {
    MetaUInt32 v(value);
    return getSimple(name, v);
  }
  int getInt16(const char * const name, INT16 & value)
  {
    MetaInt16 v(value);
    return getSimple(name, v);
  }
  int getUInt16(const char * const name, UINT16 & value)
  {
    MetaUInt16 v(value);
    return getSimple(name, v);
  }
  int getInt8(const char * const name, INT8 & value)
  {
    MetaInt8 v(value);
    return getSimple(name, v);
  }
  int getUInt8(const char * const name, UINT8 & value)
  {
    MetaUInt8 v(value);
    return getSimple(name, v);
  }
  int getBool(const char * const name, BOOL & value);
  int getPtr(const char * const name, PTR & value);
  int getString(const char * const name, const char ** const pStr);
  int getStringDup(const char * const name, const char ** const pStr);

  int getDoubleDefault(const char * const name, DOUBLE & value)
  {
    int rc = getDouble(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getFloatDefault(const char * const name, FLOAT & value)
  {
    int rc = getFloat(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getInt64Default(const char * const name, INT64 & value)
  {
    int rc = getInt64(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getUInt64Default(const char * const name, UINT64 & value)
  {
    int rc = getUInt64(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getInt32Default(const char * const name, INT32 & value)
  {
    int rc = getInt32(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getUInt32Default(const char * const name, UINT32 & value)
  {
    int rc = getUInt32(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getInt16Default(const char * const name, INT16 & value)
  {
    int rc = getInt16(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getUInt16Default(const char * const name, UINT16 & value)
  {
    int rc = getUInt16(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getInt8Default(const char * const name, INT8 & value)
  {
    int rc = getInt8(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getUInt8Default(const char * const name, UINT8 & value)
  {
    int rc = getUInt8(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getBoolDefault(const char * const name, BOOL & value)
  {
    int rc = getBool(name, value);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }
  int getStringOptional(const char * const name, const char ** const pStr)
  {
    int rc = getString(name, pStr);
    return ((0 == rc) || (FIELD_NOT_FOUND == rc)) ? 0 : rc;
  }

  int getBlob(const char * const name, const void ** const pBlob, size_t * const pLength);

  int getCard(const char * const name, InPostcard ** const ppCard, const int index);

  int getArrayDouble(const char * const name, int * const pNumElem, DOUBLE * const array);
  int getArrayFloat(const char * const name, int * const pNumElem, FLOAT * const array);
  int getArrayInt64(const char * const name, int * const pNumElem, INT64 * const array);
  int getArrayUInt64(const char * const name, int * const pNumElem, UINT64 * const array);
  int getArrayInt32(const char * const name, int * const pNumElem, INT32 * const array);
  int getArrayUInt32(const char * const name, int * const pNumElem, UINT32 * const array);
  int getArrayInt16(const char * const name, int * const pNumElem, INT16 * const array);
  int getArrayUInt16(const char * const name, int * const pNumElem, UINT16 * const array);
  int getArrayInt8(const char * const name, int * const pNumElem, INT8 * const array);
  int getArrayUInt8(const char * const name, int * const pNumElem, UINT8 * const array);
  int getArrayBool(const char * const name, int * const pNumElem, BOOL * const array);
  int getArrayPtr(const char * const name, int * const pNumElem, PTR * const array);

private:
  enum eCardState
  {
    CS_PRE_INIT, CS_GET
  };
  eCardState m_state;
  InMemoryStream * m_pIn;

  int findField(const MetaFieldType type, const char * const name, const int index = 0);
  int skipValueByType(const MetaFieldType type);

  template<typename META_V>
  int getSimple(const char * const name, META_V & mv);

  int getBlobByType(const char * const name, const eFieldType fieldType, const void ** const pBlob,
      size_t * const pLength, const bool pointerPassRecordLength, const int index = 0);

  template<typename ELEM_TYPE>
  int getArray(const char * const name, const eFieldType fieldType, int * const pNumElem, ELEM_TYPE * const array);
};

InPostcardImpl::InPostcardImpl() :
    m_state(CS_PRE_INIT), m_pIn(0)
{
}

InPostcardImpl::InPostcardImpl(InMemoryStream * pInMem) :
    m_state(CS_PRE_INIT), m_pIn(pInMem)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, pInMem);

    m_state = CS_GET;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(IN_TAG, "InPostcardImpl(pInMem) failed %d", result);
  }
}

InPostcardImpl::InPostcardImpl(OutPostcard * const pCard) :
    m_state(CS_PRE_INIT), m_pIn(0)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, pCard);

    m_pIn = InMemoryStream::createInstance(pCard->getInternalBuffer());
    BREAK_IF_ZERO(3, m_pIn);

    m_state = CS_GET;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(IN_TAG, "InPostcardImpl(pInMem) failed %d", result);
  }
}

InPostcardImpl::~InPostcardImpl()
{
  delete m_pIn;
  m_pIn = 0;
}

int InPostcardImpl::init(const void * const pIn, const size_t length, bool assume_ownership)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, pIn);
    BREAK_IF_ZERO(3, length);

    m_pIn = InMemoryStream::createInstance();
    BREAK_IF_ZERO(4, m_pIn);

    if (assume_ownership) {
        BREAK_IF_NON_ZERO(5, m_pIn->setBufferOwnership((const void **)&pIn,length));
    } else {
        BREAK_IF_NON_ZERO(6, m_pIn->setBufferNoDup(pIn,length));
    }
    m_state = CS_GET;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(IN_TAG, "init failed %d", result);
  }
  return result;
}

const MemoryStreamBase * InPostcardImpl::getBuffer() const
{
  return m_pIn;
}

int InPostcardImpl::findField(const MetaFieldType type, const char * const name, const int index)
{
  int result = 1;
  int field_index = 0;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_NON_ZERO(3, m_pIn->setGetCursor(0));

    // extract postcard length
    LengthType card_length = 0;
    BREAK_IF_NON_ZERO(4, m_pIn->extract(&card_length, sizeof(card_length)));

    const size_t given_name_length = strlen(name);
    BREAK_IF_ZERO(5, given_name_length);

    int match_count = 0;
    result = 100;
    while (true)
    {
      if(m_pIn->getGetCursor() < (card_length + sizeof(card_length)))
      {
        // there should be exactly ONE byte of 0 at the end of the postcard stream
        const size_t space_left = card_length + sizeof(card_length) - m_pIn->getGetCursor();
        if(1 == space_left)
        {
          result = FIELD_NOT_FOUND;
          break;
        }
      }
      else
      {
        // format error, something is wrong
        result = 101;
        break;
      }

      bool isTypeMatching = false;
      bool isNameMatching = false;
      //bool isNameLengthMatching = false;
      MetaFieldType current_type = FtUnknown;

      BREAK_IF_NON_ZERO(102, m_pIn->extract(&current_type, sizeof(MetaFieldType)));
      if(type == current_type)
      {
        isTypeMatching = true;
      }

      NameLengthType recorded_name_length = 0;
      BREAK_IF_NON_ZERO(103, m_pIn->extract(&recorded_name_length, sizeof(recorded_name_length)));
      if(isTypeMatching)
      {
        // check name length
        if(recorded_name_length == (given_name_length + 1))
        {
          //isNameLengthMatching = true;

          // check name
          const char * const current_name = reinterpret_cast<const char *>(m_pIn->getBuffer() + m_pIn->getGetCursor());

          if(0 == strncmp(name, current_name, given_name_length))
          {
            // found a match!
            isNameMatching = true;
          }
        }
      }

      // skip name section, with the trailing zero
      BREAK_IF_NON_ZERO(104, m_pIn->extract(0, recorded_name_length));

      if(isNameMatching)
      {
        if(index == match_count)
        {
          result = 0;
          break;
        }
        else
        {
          // wait for next blob of the same type and name
          ++match_count;
        }
      }

      // skip value section
      BREAK_IF_NON_ZERO(105, skipValueByType(current_type));

      // jump to next field
      ++field_index;
    }

    // keep the result value and leave
  } while (0);

  if(0 == result)
  {
    // found match, and current cursor is parked at the beginning of the value section
    // the caller already knows what type it is, so we'd leave type-specific unpacking
    // to the caller
  }
  else if(FIELD_NOT_FOUND == result)
  {
    // not found!
    // do nothing, leave this to higher level functions
  }
  else if(0 != result)
  {
    log_error(IN_TAG, "findName failed %d, field index %d", result, field_index);
  }
  return result;
}

int InPostcardImpl::skipValueByType(const MetaFieldType type)
{
  int result = 1;
  do
  {
    // it has to be LengthType for we are using it to extract the length of a Postcard
    LengthType value_size = 0;
    switch (type)
    {
    case FtInt64:
      value_size = sizeof(MetaInt64::ValueType);
      result = 0;
      break;
    case FtUInt64:
      value_size = sizeof(MetaUInt64::ValueType);
      result = 0;
      break;
    case FtInt32:
      value_size = sizeof(MetaInt32::ValueType);
      result = 0;
      break;
    case FtUInt32:
      value_size = sizeof(MetaUInt32::ValueType);
      result = 0;
      break;
    case FtInt16:
      value_size = sizeof(MetaInt16::ValueType);
      result = 0;
      break;
    case FtUInt16:
      value_size = sizeof(MetaUInt16::ValueType);
      result = 0;
      break;
    case FtInt8:
      value_size = sizeof(MetaInt8::ValueType);
      result = 0;
      break;
    case FtUInt8:
      value_size = sizeof(MetaUInt8::ValueType);
      result = 0;
      break;
    case FtBool:
      value_size = sizeof(MetaBool::ValueType);
      result = 0;
      break;
    case FtDouble:
      value_size = sizeof(MetaDouble::ValueType);
      result = 0;
      break;
    case FtFloat:
      value_size = sizeof(MetaFloat::ValueType);
      result = 0;
      break;

    case FtString:
      // fall through to FtCard
    case FtCard:
      if(0 != m_pIn->extract(&value_size, sizeof(value_size)))
      {
        result = 102;
      }
      else
      {
        if(value_size >= 1)
        {
          // the card/string length should contain at least ONE trailing zero
          // even if the card/string is empty
          result = 0;
        }
        else
        {
          result = 103;
        }
      }
      break;

    case FtArrayInt64:    // fall through to blob
    case FtArrayUInt64:   // fall through to blob
    case FtArrayInt32:    // fall through to blob
    case FtArrayUInt32:   // fall through to blob
    case FtArrayInt16:    // fall through to blob
    case FtArrayUInt16:   // fall through to blob
    case FtArrayInt8:     // fall through to blob
    case FtArrayUInt8:    // fall through to blob
    case FtArrayBool:     // fall through to blob
    case FtArrayDouble:   // fall through to blob
    case FtArrayFloat:    // fall through to blob
    case FtBlob:
      if(0 != m_pIn->extract(&value_size, sizeof(value_size)))
      {
        result = 104;
      }
      else
      {
        result = 0;
      }
      break;

    case FtUnknown:
      result = 100;
      break;

    default:
      result = 101;
      break;
    }

    if(0 != result)
    {
      // keep the result value and break
      break;
    }

    BREAK_IF_ZERO(2, value_size);
    BREAK_IF_NON_ZERO(3, m_pIn->extract(0, value_size));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(IN_TAG, "skipValueByType failed %d, type %u", result, (unsigned int) type);
  }
  return result;
}

template<typename META_V>
int InPostcardImpl::getSimple(const char * const name, META_V & mv)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);

    int find_result = findField(mv.field_type, name);
    if(0 == find_result)
    {
      // found one!
    }
    else if(FIELD_NOT_FOUND == find_result)
    {
      result = FIELD_NOT_FOUND;
      break;
    }
    else
    {
      result = 3;
      break;
    }

    // we found a match, and current get cursor is parked at the beginning of the value
    BREAK_IF_NON_ZERO(4, m_pIn->extract(&mv.value, sizeof(mv.value)));

    result = 0;
  } while (0);

  if(FIELD_NOT_FOUND == result)
  {
    //log_verbose(IN_TAG, "get simple [%s] not found", name);
  }
  else if(0 != result)
  {
    log_error(IN_TAG, "get simple failed %d", result);
  }
  return result;
}
int InPostcardImpl::getBool(const char * const name, BOOL & value)
{
  UINT8 u = 0;
  MetaBool v(u);
  int result = getSimple(name, v);
  if(0 == result)
  {
    value = (1 == u) ? true : false;
  }
  return result;
}

int InPostcardImpl::getPtr(const char * const name, PTR & value)
{
#ifdef ANDROID_32_BIT_PTR_SUPPORT
  INT32 ivalue;
  MetaInt32 v(ivalue);
  int result = getSimple(name, v);
  if (result == 0)
  {
    value = (PTR) ivalue;
  }
  return result;
#else // 64-bit
  INT64 ivalue;
  MetaInt64 v(ivalue);
  int result = getSimple(name, v);
  if (result == 0)
  {
    value = (PTR) ivalue;
  }
  return result;
#endif
}

int InPostcardImpl::getBlob(const char * const name, const void ** const pBlob, size_t * const pLength)
{
  return getBlobByType(name, FtBlob, pBlob, pLength, true);
}

int InPostcardImpl::getBlobByType(const char * const name, const eFieldType fieldType, const void ** const pBlob,
    size_t * const pLength, const bool pointerPassRecordLength, const int index)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, pBlob);
    *pBlob = 0;
    BREAK_IF_ZERO(3, pLength);
    *pLength = 0;

    int find_result = findField(fieldType, name, index);
    if(0 == find_result)
    {
      // found one!
    }
    else if(FIELD_NOT_FOUND == find_result)
    {
      result = FIELD_NOT_FOUND;
      break;
    }
    else
    {
      result = 4;
      break;
    }

    const size_t cursor_before_length = m_pIn->getGetCursor();

    LengthType recorded_str_length = 0;
    BREAK_IF_NON_ZERO(5, m_pIn->extract(&recorded_str_length, sizeof(recorded_str_length)));

    // make sure we have that much data to extract!
    BREAK_IF_NON_ZERO(7, m_pIn->extract(0, recorded_str_length));
    *pLength = recorded_str_length;

    size_t cursor = cursor_before_length;
    if(pointerPassRecordLength)
    {
      cursor += sizeof(recorded_str_length);
    }

    *pBlob = reinterpret_cast<const char *>(m_pIn->getBuffer() + cursor);

    result = 0;
  } while (0);

  if(FIELD_NOT_FOUND == result)
  {
    //log_verbose(IN_TAG, "get blob [%s] not found", name);
  }
  else if(0 != result)
  {
    log_error(IN_TAG, "get blob failed %d", result);
  }
  return result;
}

template<typename ELEM_TYPE>
int InPostcardImpl::getArray(const char * const name, const eFieldType fieldType, int * const pGivenNumElem, ELEM_TYPE * const array)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, pGivenNumElem);
    int num_stored_element = 0;

    size_t data_block_length = 0;
    const void * internal_array = 0;
    BREAK_IF_NON_ZERO(4, getBlobByType(name, fieldType, &internal_array, &data_block_length, true));
    BREAK_IF_NON_ZERO(5, data_block_length % sizeof(ELEM_TYPE));
    num_stored_element = data_block_length / sizeof(ELEM_TYPE);
    BREAK_IF_ZERO(6, num_stored_element);

    if(0 != array)
    {
      if(num_stored_element > *pGivenNumElem)
      {
        result = 7;
        break;
      }
      // copy data from internal to external buffer
      memcpy(array, internal_array, data_block_length);
    }
    else
    {
      // skip
    }
    *pGivenNumElem = num_stored_element;

    result = 0;
  } while(0);

  if(0 != result)
  {
    log_error(IN_TAG, "getArray failed %d", result);
  }
  return result;
}

int InPostcardImpl::getArrayDouble(const char * const name, int * const pNumElem, DOUBLE * const array)
{
  return getArray(name, FtArrayDouble, pNumElem, array);
}

int InPostcardImpl::getArrayFloat(const char * const name, int * const pNumElem, FLOAT * const array)
{
  return getArray(name, FtArrayFloat, pNumElem, array);
}

int InPostcardImpl::getArrayInt64(const char * const name, int * const pNumElem, INT64 * const array)
{
  return getArray(name, FtArrayInt64, pNumElem, array);
}

int InPostcardImpl::getArrayUInt64(const char * const name, int * const pNumElem, UINT64 * const array)
{
  return getArray(name, FtArrayUInt64, pNumElem, array);
}

int InPostcardImpl::getArrayInt32(const char * const name, int * const pNumElem, INT32 * const array)
{
  return getArray(name, FtArrayInt32, pNumElem, array);
}

int InPostcardImpl::getArrayUInt32(const char * const name, int * const pNumElem, UINT32 * const array)
{
  return getArray(name, FtArrayUInt32, pNumElem, array);
}

int InPostcardImpl::getArrayInt16(const char * const name, int * const pNumElem, INT16 * const array)
{
  return getArray(name, FtArrayInt16, pNumElem, array);
}

int InPostcardImpl::getArrayUInt16(const char * const name, int * const pNumElem, UINT16 * const array)
{
  return getArray(name, FtArrayUInt16, pNumElem, array);
}

int InPostcardImpl::getArrayInt8(const char * const name, int * const pNumElem, INT8 * const array)
{
  return getArray(name, FtArrayInt8, pNumElem, array);
}

int InPostcardImpl::getArrayUInt8(const char * const name, int * const pNumElem, UINT8 * const array)
{
  return getArray(name, FtArrayUInt8, pNumElem, array);
}

int InPostcardImpl::getArrayBool(const char * const name, int * const pNumElem, BOOL * const array)
{
  return getArray(name, FtArrayBool, pNumElem, array);
}

int InPostcardImpl::getArrayPtr(const char * const name, int * const pNumElem, PTR * const array)
{
  // 8 bytes pointer
  if (sizeof (PTR) == 8)
  {
    return getArray(name, FtArrayInt64, pNumElem, array);
  }
  else
  {
    return getArray(name, FtArrayInt32, pNumElem, array);
  }
}

int InPostcardImpl::getString(const char * const name, const char ** const pStr)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, pStr);
    *pStr = 0;

    const void * blob = 0;
    size_t length = 0;
    int find_result = getBlobByType(name, FtString, &blob, &length, true);
    if(0 == find_result)
    {
    }
    else if(FIELD_NOT_FOUND == find_result)
    {
      result = FIELD_NOT_FOUND;
      break;
    }
    else
    {
      result = 4;
      break;
    }

    BREAK_IF_ZERO(5, blob);
    // length should be at least 1 for the trailing zero
    BREAK_IF_ZERO(6, length);
    BREAK_IF_NON_ZERO(7, ((const char*)blob)[length-1]);

    *pStr = reinterpret_cast<const char *>(blob);

    result = 0;
  } while (0);

  if(FIELD_NOT_FOUND == result)
  {
    // ignore, not reporting anything
  }
  else if(0 != result)
  {
    log_error(IN_TAG, "get string failed %d", result);
  }
  return result;
}

int InPostcardImpl::getStringDup(const char * const name, const char ** const pStr)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, pStr);

    const char * str = 0;
    int rc = getString(name, &str);
    if(0 == rc)
    {
      // found! do nothing
    }
    else if(FIELD_NOT_FOUND == rc)
    {
      result = FIELD_NOT_FOUND;
      break;
    }
    else
    {
      result = 3;
      break;
    }
    size_t length = strlen(str) + 1;
    *pStr = new (std::nothrow) char[length];

    BREAK_IF_ZERO(4, *pStr);

    memcpy((void*) *pStr, (void*) str, length);

    result = 0;
  } while (0);

  if((0 != result) && (FIELD_NOT_FOUND != result))
  {
    log_error(IN_TAG, "get string duplicate failed %d", result);
  }
  return result;
}

int InPostcardImpl::getCard(const char * const name, InPostcard ** const ppCard, const int index)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name);
    BREAK_IF_ZERO(3, ppCard);
    *ppCard = 0;

    const void * blob = 0;
    size_t card_length = 0;
    int find_result = getBlobByType(name, FtCard, &blob, &card_length, false, index);
    if(0 == find_result)
    {
    }
    else if(FIELD_NOT_FOUND == find_result)
    {
      result = FIELD_NOT_FOUND;
      break;
    }
    else
    {
      result = 4;
      break;
    }

    InPostcardImpl * const p_new_card = new (std::nothrow) InPostcardImpl();
    BREAK_IF_ZERO(8, p_new_card);

    if(0 != p_new_card->init(blob, card_length + sizeof(LengthType)))
    {
      delete p_new_card;
      result = 9;
      break;
    }

    *ppCard = p_new_card;

    result = 0;
  } while (0);

  if(FIELD_NOT_FOUND == result)
  {
    //log_verbose(IN_TAG, "get card [%s] not found", name);
  }
  else if(0 != result)
  {
    log_error(IN_TAG, "get card failed %d", result);
  }
  return result;
}

OutPostcard * OutPostcard::createInstance()
{
  return new (std::nothrow) OutPostcardImpl();
}

InPostcard * InPostcard::createInstance()
{
  return new (std::nothrow) InPostcardImpl();
}

InPostcard * InPostcard::createInstance(InMemoryStream * const pInMem)
{
  return new (std::nothrow) InPostcardImpl(pInMem);
}

InPostcard * InPostcard::createInstance(OutPostcard * const pCard)
{
  return new (std::nothrow) InPostcardImpl(pCard);
}

} // namespace qc_loc_fw

