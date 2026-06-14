#ifndef __INTERNAL__SEC_QC_SUMMARY_COREINFO_H__
#define __INTERNAL__SEC_QC_SUMMARY_COREINFO_H__

#include <linux/linkage.h>
#include <linux/elfcore.h>
#include <linux/elf.h>

#define QC_SUMMARY_CORE_NOTE_NAME		"CORE"
#define QC_SUMMARY_CORE_NOTE_HEAD_BYTES		ALIGN(sizeof(struct elf_note), 4)

#define QC_SUMMARY_CORE_NOTE_NAME_BYTES		ALIGN(sizeof(QC_SUMMARY_CORE_NOTE_NAME), 4)
#define QC_SUMMARY_CORE_NOTE_DESC_BYTES		ALIGN(sizeof(struct elf_prstatus), 4)

#define QC_SUMMARY_CORE_NOTE_BYTES		((QC_SUMMARY_CORE_NOTE_HEAD_BYTES * 2) + QC_SUMMARY_CORE_NOTE_NAME_BYTES + QC_SUMMARY_CORE_NOTE_DESC_BYTES)

#define QC_SUMMARY_COREINFO_BYTES		(PAGE_SIZE)
#define QC_SUMMARY_COREINFO_NOTE_NAME		"QC_SUMMARY_COREINFO"
#define QC_SUMMARY_COREINFO_NOTE_NAME_BYTES	ALIGN(sizeof(QC_SUMMARY_COREINFO_NOTE_NAME), 4)
#define QC_SUMMARY_COREINFO_NOTE_SIZE		((QC_SUMMARY_CORE_NOTE_HEAD_BYTES * 2) + QC_SUMMARY_COREINFO_NOTE_NAME_BYTES + QC_SUMMARY_COREINFO_BYTES)

#define QC_SUMMARY_COREINFO_SYMBOL(name)	sec_qc_summary_coreinfo_append_str("SYMBOL(%s)=0x%llx\n", #name, (unsigned long long)&name)
#define QC_SUMMARY_COREINFO_SYMBOL_ARRAY(name)	sec_qc_summary_coreinfo_append_str("SYMBOL(%s)=0x%llx\n", #name, (unsigned long long)name)
#define QC_SUMMARY_COREINFO_SIZE(name)		sec_qc_summary_coreinfo_append_str("SIZE(%s)=%zu\n", #name, sizeof(name))
#define QC_SUMMARY_COREINFO_STRUCT_SIZE(name)	sec_qc_summary_coreinfo_append_str("SIZE(%s)=%zu\n", #name, sizeof(struct name))
#define QC_SUMMARY_COREINFO_OFFSET(name, field)	sec_qc_summary_coreinfo_append_str("OFFSET(%s.%s)=%zu\n", #name, #field, (size_t)offsetof(struct name, field))
#define QC_SUMMARY_COREINFO_LENGTH(name, value)	sec_qc_summary_coreinfo_append_str("LENGTH(%s)=%llu\n", #name, (unsigned long long)value)
#define QC_SUMMARY_COREINFO_NUMBER(name)	sec_qc_summary_coreinfo_append_str("NUMBER(%s)=%lld\n", #name, (long long)name)

void sec_qc_summary_coreinfo_append_str(const char *fmt, ...);

#endif /* __INTERNAL__SEC_QC_SUMMARY_COREINFO_H__ */
