#ifndef __INTERNAL__SEC_QC_LOGGER_H__
#define __INTERNAL__SEC_QC_LOGGER_H__

#include <linux/samsung/builder_pattern.h>

struct qc_logger;

struct qc_logger_drvdata {
	struct builder bd;
	const char *name;
	uint32_t unique_id;
	struct sec_dbg_region_client *client;
	struct qc_logger *logger;
};

struct qc_logger {
	struct builder bd;

	size_t (*get_data_size)(struct qc_logger *logger);
	int (*probe)(struct qc_logger *logger);
	void (*remove)(struct qc_logger *logger);

	uint32_t unique_id;
	struct qc_logger_drvdata *drvdata;
};

extern u64 notrace __qc_logger_cpu_clock(int cpu);
extern int __qc_logger_sub_module_probe(struct qc_logger *logger, const struct dev_builder *builder, ssize_t n);
extern void __qc_logger_sub_module_remove(struct qc_logger *logger, const struct dev_builder *builder, ssize_t n);

extern struct qc_logger __qc_logger_sched_log;
extern struct qc_logger __qc_logger_irq_log;
extern struct qc_logger __qc_logger_irq_exit_log;

#endif /* __INTERNAL__SEC_QC_LOGGER_H__ */
