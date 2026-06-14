#include "sec_input.h"

int sec_input_handler_start(struct device *dev)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	unsigned int irq = gpio_to_irq(pdata->irq_gpio);
	struct irq_desc *desc = irq_to_desc(irq);

	if (desc && desc->action) {
		if ((desc->action->flags & IRQF_TRIGGER_LOW) && (gpio_get_value(pdata->irq_gpio) == 1))
			return SEC_ERROR;
	}

	if (sec_input_cmp_ic_status(dev, CHECK_LPMODE)) {
		__pm_wakeup_event(pdata->sec_ws, SEC_TS_WAKE_LOCK_TIME);

		if (!pdata->resume_done.done) {
			if (!IS_ERR_OR_NULL(pdata->irq_workqueue)) {
				input_info(true, dev, "%s: disable irq and queue waiting work\n", __func__);
				disable_irq_nosync(gpio_to_irq(pdata->irq_gpio));
				atomic_set(&pdata->irq_enabled, SEC_INPUT_IRQ_DISABLE_NOSYNC);
				queue_work(pdata->irq_workqueue, &pdata->irq_work);
			} else {
				input_err(true, dev, "%s: irq_workqueue not exist\n", __func__);
			}
			return SEC_ERROR;
		}
	}

	return SEC_SUCCESS;
}
EXPORT_SYMBOL(sec_input_handler_start);

void sec_input_handler_wait_resume_work(struct work_struct *work)
{
	struct sec_ts_plat_data *pdata = container_of(work, struct sec_ts_plat_data, irq_work);
	unsigned int irq = gpio_to_irq(pdata->irq_gpio);
	struct irq_desc *desc = irq_to_desc(irq);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&pdata->resume_done,
			msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
	if (ret <= 0) {
		input_err(true, pdata->dev, "%s: LPM: pm resume is not handled: %d\n", __func__, ret);
		goto out;
	}

	if (desc && desc->action && desc->action->thread_fn) {
		input_info(true, pdata->dev, "%s: run irq thread\n", __func__);
		desc->action->thread_fn(irq, desc->action->dev_id);
	}
out:
	sec_input_irq_enable(pdata);
}
EXPORT_SYMBOL(sec_input_handler_wait_resume_work);

void sec_input_irq_enable(struct sec_ts_plat_data *pdata)
{
	struct irq_desc *desc;

	if (!pdata->irq)
		return;

	desc = irq_to_desc(pdata->irq);
	if (!desc) {
		input_err(true, pdata->dev, "%s: invalid irq number: %d\n", __func__, pdata->irq);
		return;
	}

	if (!desc->action) {
		input_dbg(true, pdata->dev, "%s: irq is not requested yet.\n", __func__);
		return;
	}

	mutex_lock(&pdata->irq_lock);

	while (desc->depth > 0) {
		enable_irq(pdata->irq);
		input_info(true, pdata->dev, "%s: depth: %d\n", __func__, desc->depth);
	}
	atomic_set(&pdata->irq_enabled, SEC_INPUT_IRQ_ENABLE);
	mutex_unlock(&pdata->irq_lock);
}
EXPORT_SYMBOL(sec_input_irq_enable);

void sec_input_irq_disable(struct sec_ts_plat_data *pdata)
{
	struct irq_desc *desc;

	if (!pdata->irq)
		return;

	desc = irq_to_desc(pdata->irq);
	if (!desc) {
		input_err(true, pdata->dev, "%s: invalid irq number: %d\n", __func__, pdata->irq);
		return;
	}

	if (!desc->action) {
		input_dbg(true, pdata->dev, "%s: irq is not requested yet.\n", __func__);
		return;
	}

	mutex_lock(&pdata->irq_lock);
	if (atomic_read(&pdata->irq_enabled) == SEC_INPUT_IRQ_ENABLE)
		disable_irq(pdata->irq);
	input_info(true, pdata->dev, "%s: depth: %d\n", __func__, desc->depth);

	atomic_set(&pdata->irq_enabled, SEC_INPUT_IRQ_DISABLE);
	mutex_unlock(&pdata->irq_lock);
}
EXPORT_SYMBOL(sec_input_irq_disable);

void sec_input_irq_disable_nosync(struct sec_ts_plat_data *pdata)
{
	struct irq_desc *desc;

	if (!pdata->irq)
		return;

	desc = irq_to_desc(pdata->irq);
	if (!desc) {
		input_err(true, pdata->dev, "%s: invalid irq number: %d\n", __func__, pdata->irq);
		return;
	}

	if (!desc->action) {
		input_dbg(true, pdata->dev, "%s: irq is not requested yet.\n", __func__);
		return;
	}

	mutex_lock(&pdata->irq_lock);

	if (atomic_read(&pdata->irq_enabled) == SEC_INPUT_IRQ_ENABLE)
		disable_irq_nosync(pdata->irq);
	input_info(true, pdata->dev, "%s: depth: %d\n", __func__, desc->depth);

	atomic_set(&pdata->irq_enabled, SEC_INPUT_IRQ_DISABLE_NOSYNC);
	mutex_unlock(&pdata->irq_lock);
}
EXPORT_SYMBOL(sec_input_irq_disable_nosync);

MODULE_DESCRIPTION("Samsung input common irq");
MODULE_LICENSE("GPL");