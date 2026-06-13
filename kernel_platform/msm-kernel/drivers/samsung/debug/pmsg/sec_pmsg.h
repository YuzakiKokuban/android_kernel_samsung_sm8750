#ifndef __INTERNAL__SEC_PMSG_H__
#define __INTERNAL__SEC_PMSG_H__

#include <linux/of_reserved_mem.h>
#include <linux/samsung/builder_pattern.h>

#define MAX_BUFFER_SIZE	1024

struct ss_pmsg_log_header_t {
	uint8_t magic;
	uint16_t len;
	uint16_t uid;
	uint16_t pid;
} __attribute__((__packed__));

struct ss_android_log_header_t {
	unsigned char id;
	uint16_t tid;
	int32_t tv_sec;
	int32_t tv_nsec;
} __attribute__((__packed__));

struct pmsg_logger {
	uint16_t len;
	uint16_t id;
	uint16_t pid;
	uint16_t tid;
	uint16_t uid;
	uint16_t level;
	int32_t tv_sec;
	int32_t tv_nsec;
	union {
		char msg[0];
		char __msg;	/* 1 byte reserved area for 'unsigned char' request from user */
	};
};

struct pmsg_buffer {
	char buffer[MAX_BUFFER_SIZE];
};

struct pmsg_drvdata {
	struct builder bd;
	struct reserved_mem *rmem;
	phys_addr_t paddr;
	size_t size;
	bool nomap;
	struct pstore_info *pstore;
	struct pmsg_logger *logger;
	struct pmsg_buffer __percpu *buf;
};

struct logger_level_header_ctx {
	int cpu;
	const char *comm;
	u64 tv_kernel;
	char *buffer;
	size_t count;
};

#endif /* __INTERNAL__SEC_PMSG_H__ */
