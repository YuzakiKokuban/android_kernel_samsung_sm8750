// SPDX-License-Identifier: GPL-2.0
//
// Samsung's performance logging
//
// Copyright (c) 2014 Samsung Electronics Co., Ltd
//              http://www.samsung.com

#ifndef PERFLOG_H_
#define PERFLOG_H_

#define PERFLOG_LOC __FILE__, __LINE__

#ifdef __cplusplus
extern "C" {
#endif

#include "olog.pb.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

// extern FILE* perflog_fout;
#define PERFLOG_PACKET_SIZE			256
#define PERFLOG_HEADER_SIZE 			24
#define PERFLOG_BUFF_STR_MAX_SIZE 		(PERFLOG_PACKET_SIZE - PERFLOG_HEADER_SIZE)
#define PERFLOG_BUFF_STR_MAX_SIZE_FOR_MULTILINE	4096
#define PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTI	PERFLOG_BUFF_STR_MAX_SIZE - PERFLOG_UINT16_SIZE
#define PERFLOG_UINT16_SIZE			2
#define PERFLOG_INT_SIZE			4

/* PerfLog Phase 2 :: header format modification 
   should be changed to protobuff type 
*/
typedef enum PerfLogAffectTag {
    AFFECT_K,
    AFFECT_F,
    AFFECT_A
}PerfLogAffect;

typedef enum EvtNamingTag {
    NAMING_LockC,
    NAMING_AppLaunch,    
}EvtNamingTag;

typedef enum PerfLevelTag{
    LOW,
    MID,
    HIGH,
    CRITICAL
}PerfLevelTag;

// ###############################################################################
#pragma pack(push, 1)

struct Payload {
	int param1;
	int param2;
	char logbuffer[PERFLOG_BUFF_STR_MAX_SIZE + 1];
};  

struct LogPacket {
#if defined(KPERFMON_KERNEL)
	struct timespec64 logtime;
#else
	struct timespec logtime;
#endif
        uint16 logtype;
        uint16 logid;
        uint16 pid;
        uint16 tid;
	struct Payload payload;
};

struct _Timestamp {
	uint8  month;
	uint8  day;
	uint8  hour;
	uint8  minute;
	uint8  second;
	uint16 msecond;
};

struct _PLogPacket {
	struct _Timestamp timestamp;
        uint16 pid;
        uint16 tid;

	uint8  type;
	uint8  id;

	char   pname[10];
	uint8  context_length;
	char   context_buffer[PERFLOG_BUFF_STR_MAX_SIZE + 1];
};

union _uPLogPacket {
	struct _PLogPacket itemes;
	char stream[PERFLOG_HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE];
};

#pragma pack(pop) 

// Start API
int perflog_sending_log_via_socket(uint16 type, uint16 logid, int param1, int param2, char const *str);

int perflog_write(char const * fmt, ...);

int perflog_write_log(uint16 type, uint16 logid, char const * fmt, ...);

int perflog_write_evt(uint16 maintype, uint16 logid, uint16 param1, char const * fmt, ...);

// int perflog_getlog(char **buff);
// End API

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif
