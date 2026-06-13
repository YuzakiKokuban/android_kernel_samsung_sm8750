/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __SMCI_QSEECOMCOMPATAPPLOADER_H
#define __SMCI_QSEECOMCOMPATAPPLOADER_H

#include "smci_object.h"
#include "IQSEEComCompatAppLoader.h"

#define SMCI_QSEECOMCOMPATAPPLOADER_MAX_FILENAME_LEN UINT32_C(64)
#define SMCI_QSEECOMCOMPATAPPLOADER_ELFCLASS32 UINT32_C(1)
#define SMCI_QSEECOMCOMPATAPPLOADER_ELFCLASS64 UINT32_C(2)

#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_INVALID_BUFFER INT32_C(10)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_PIL_ROLLBACK_FAILURE INT32_C(11)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_ELF_SIGNATURE_ERROR INT32_C(12)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_METADATA_INVALID INT32_C(13)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_MAX_NUM_APPS INT32_C(14)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_NO_NAME_IN_METADATA INT32_C(15)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_ALREADY_LOADED INT32_C(16)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_EMBEDDED_IMAGE_NOT_FOUND INT32_C(17)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_TZ_HEAP_MALLOC_FAILURE INT32_C(18)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_TA_APP_REGION_MALLOC_FAILURE INT32_C(19)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_CLIENT_CRED_PARSING_FAILURE INT32_C(20)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_APP_UNTRUSTED_CLIENT INT32_C(21)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_APP_BLACKLISTED INT32_C(22)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_APP_NOT_LOADED INT32_C(23)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_NOT_QSEECOM_COMPAT_APP INT32_C(24)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_FILENAME_TOO_LONG INT32_C(25)
#define SMCI_QSEECOMCOMPATAPPLOADER_ERROR_APP_ARCH_NOT_SUPPORTED INT32_C(26)

#define SMCI_QSEECOMCOMPATAPPLOADER_OP_LOADFROMREGION 0
#define SMCI_QSEECOMCOMPATAPPLOADER_OP_LOADFROMBUFFER 1
#define SMCI_QSEECOMCOMPATAPPLOADER_OP_LOOKUPTA 2


static inline int32_t
smci_qseecomcompatapploader_release(struct smci_object self)
{
	return IQSEEComCompatAppLoader_release(self);
}

static inline int32_t
smci_qseecomcompatapploader_retain(struct smci_object self)
{
	return IQSEEComCompatAppLoader_retain(self);
}

static inline int32_t
smci_qseecomcompatapploader_loadfromregion(struct smci_object self,
			struct smci_object app_elf_val, const void *filename_ptr,
			size_t filename_len, struct smci_object *app_compat_ptr)
{
	return IQSEEComCompatAppLoader_loadFromRegion(self,
			app_elf_val, filename_ptr,
			filename_len, app_compat_ptr);
}

static inline int32_t
smci_qseecomcompatapploader_loadfrombuffer(struct smci_object self,
			const void *app_elf_ptr, size_t app_elf_len,
			const void *filename_ptr, size_t filename_len,
			void *dist_name_ptr, size_t dist_name_len,
			size_t *dist_name_lenout, struct smci_object *app_compat_ptr)
{
	return IQSEEComCompatAppLoader_loadFromBuffer(self,
			app_elf_ptr, app_elf_len,
			filename_ptr, filename_len,
			dist_name_ptr, dist_name_len,
			dist_name_lenout, app_compat_ptr);
}

static inline int32_t
smci_qseecomcompatapploader_lookupta(struct smci_object self, const void *app_name_ptr,
			size_t app_name_len, struct smci_object *app_compat_ptr)
{
	return IQSEEComCompatAppLoader_lookupTA(self, app_name_ptr,
			app_name_len, app_compat_ptr);
}

#endif /* __SMCI_QSEECOMCOMPATAPPLOADER_H */
