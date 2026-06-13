#ifndef __INTERNAL__SEC_QC_SUMMARY_VAR_MON_H__
#define __INTERNAL__SEC_QC_SUMMARY_VAR_MON_H__

#if IS_ENABLED(CONFIG_ARM64)
extern void __summary_add_var_to_info_mon(struct sec_qc_summary_simple_var_mon *info_mon, const char *name, phys_addr_t pa, unsigned int size);
extern void __summary_add_var_to_var_mon(struct sec_qc_summary_simple_var_mon *var_mon, const char *name, phys_addr_t pa, unsigned int size);
extern void __summary_add_array_to_var_mon(struct sec_qc_summary_simple_var_mon *var_mon, const char *name, phys_addr_t pa, size_t index, unsigned int size);
#else
static inline void __summary_add_var_to_info_mon(struct sec_qc_summary_simple_var_mon *info_mon, const char *name, phys_addr_t pa, unsigned int size) {}
static inline void __summary_add_var_to_var_mon(struct sec_qc_summary_simple_var_mon *var_mon, const char *name, phys_addr_t pa, unsigned int size) {}
static inline void __summary_add_array_to_var_mon(struct sec_qc_summary_simple_var_mon *var_mon, const char *name, phys_addr_t pa, size_t index, unsigned int size) {}
#endif

#define __qc_summary_add_var_to_info_mon(info_mon, var) \
	__summary_add_var_to_info_mon(info_mon, #var, __pa(&var), sizeof(var))

#define __qc_summary_add_str_to_info_mon(info_mon, pstr) \
	__summary_add_var_to_info_mon(info_mon, #pstr, __pa(pstr), -1)

#define __qc_summary_add_info_mon(info_mon, pa, name, size) \
	__summary_add_var_to_info_mon(info_mon, pa, name, size)

#define __qc_summary_add_var_to_var_mon(var_mon, var) \
	__summary_add_var_to_var_mon(var_mon, #var, __pa(&var), sizeof(var))

#define __qc_summary_add_array_to_var_mon(name, arr, index) \
	__summary_add_array_to_var_mon(var_mon, name, __ap(&arr), index, sizeof(arr))

#define __qc_summary_add_str_array_to_var_mon(name, pstrarr, index) \
	__summary_add_array_to_var_mon(var_mon, name, __ap(&pstrarr), index, -1)

#endif /* __INTERNAL__SEC_QC_SUMMARY_VAR_MON_H__ */
