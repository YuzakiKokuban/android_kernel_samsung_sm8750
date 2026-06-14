#include "sec_input.h"

struct device *ptsp;
EXPORT_SYMBOL(ptsp);

struct sec_ts_secure_data *psecuretsp;
EXPORT_SYMBOL(psecuretsp);

void stui_tsp_init(int (*stui_tsp_enter)(void), int (*stui_tsp_exit)(void), int (*stui_tsp_type)(void))
{
	pr_info("%s %s: called\n", SECLOG, __func__);

	psecuretsp = kzalloc(sizeof(struct sec_ts_secure_data), GFP_KERNEL);

	psecuretsp->stui_tsp_enter = stui_tsp_enter;
	psecuretsp->stui_tsp_exit = stui_tsp_exit;
	psecuretsp->stui_tsp_type = stui_tsp_type;
}
EXPORT_SYMBOL(stui_tsp_init);


int stui_tsp_enter(void)
{
	struct sec_ts_plat_data *pdata = NULL;

	if (psecuretsp != NULL) {
		pr_info("%s %s: psecuretsp->stui_tsp_enter called!\n", SECLOG, __func__);
		return psecuretsp->stui_tsp_enter();
	}

	if (ptsp == NULL) {
		pr_info("%s: ptsp is null\n", __func__);
		return -EINVAL;
	}

	pdata = ptsp->platform_data;
	if (pdata == NULL) {
		pr_info("%s: pdata is null\n", __func__);
		return  -EINVAL;
	}

	pr_info("%s %s: pdata->stui_tsp_enter called!\n", SECLOG, __func__);
	return pdata->stui_tsp_enter();
}
EXPORT_SYMBOL(stui_tsp_enter);

int stui_tsp_exit(void)
{
	struct sec_ts_plat_data *pdata = NULL;

	if (psecuretsp != NULL) {
		pr_info("%s %s: psecuretsp->stui_tsp_exit called!\n", SECLOG, __func__);
		return psecuretsp->stui_tsp_exit();
	}

	if (ptsp == NULL)
		return -EINVAL;

	pdata = ptsp->platform_data;
	if (pdata == NULL)
		return  -EINVAL;

	pr_info("%s %s: pdata->stui_tsp_exit called!\n", SECLOG, __func__);
	return pdata->stui_tsp_exit();
}
EXPORT_SYMBOL(stui_tsp_exit);

int stui_tsp_type(void)
{
	struct sec_ts_plat_data *pdata = NULL;

	if (psecuretsp != NULL) {
		pr_info("%s %s: psecuretsp->stui_tsp_type called!\n", SECLOG, __func__);
		return psecuretsp->stui_tsp_type();
	}

	if (ptsp == NULL)
		return -EINVAL;

	pdata = ptsp->platform_data;
	if (pdata == NULL)
		return  -EINVAL;

	pr_info("%s %s: pdata->stui_tsp_type called!\n", SECLOG, __func__);
	return pdata->stui_tsp_type();
}
EXPORT_SYMBOL(stui_tsp_type);

MODULE_DESCRIPTION("Samsung input common secure");
MODULE_LICENSE("GPL");