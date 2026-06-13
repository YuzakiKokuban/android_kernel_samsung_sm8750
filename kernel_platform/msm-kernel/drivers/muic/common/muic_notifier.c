
#define pr_fmt(fmt)	"[MUIC] " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/notifier.h>
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
#include <linux/switch.h>
#endif  /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */
#include <linux/muic/common/muic.h>
#include <linux/muic/common/muic_notifier.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif

#define SET_MUIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_MUIC_NOTIFIER_BLOCK(nb)			\
		SET_MUIC_NOTIFIER_BLOCK(nb, NULL, -1)

struct device *switch_device;
EXPORT_SYMBOL(switch_device);

static struct muic_notifier_struct muic_notifier;
#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
static struct muic_notifier_struct muic_pdic_notifier;
#endif

static int muic_uses_new_noti;
#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
static int muic_pdic_uses_new_noti;
#endif

static int muic_notifier_init_done;
static int muic_notifier_init(void);

#if IS_ENABLED(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
struct switch_dev switch_muic_dev = {
	.name = "attached_muic_cable",
};

static void send_muic_cable_intent(int type)
{
	pr_info("%s: MUIC attached_muic_cable type(%d)\n", __func__, type);
	switch_set_state(&switch_muic_dev, type);
}
#endif  /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */
#endif

void muic_notifier_set_new_noti(bool flag)
{
	muic_uses_new_noti = flag ? 1 : 0;
}
EXPORT_SYMBOL(muic_notifier_set_new_noti);

static void __set_noti_cxt_dest(int attach, int type, int dest)
{
	if (type < 0) {
		muic_notifier.cmd = attach;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		muic_notifier.cxt.attach = attach;
		muic_notifier.cxt.dest = dest;
#endif
		return;
	}

	/* Old Interface */
	muic_notifier.cmd = attach;
	muic_notifier.attached_dev = type;

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	/* New Interface */
#if defined(CONFIG_USE_DEDICATED_MUIC)
	muic_notifier.cxt.src = PDIC_NOTIFY_DEV_DEDICATED_MUIC;
#else
	muic_notifier.cxt.src = PDIC_NOTIFY_DEV_MUIC;
#endif
	muic_notifier.cxt.dest = dest;
	muic_notifier.cxt.id = PDIC_NOTIFY_ID_ATTACH;
	muic_notifier.cxt.attach = attach;
	muic_notifier.cxt.cable_type = type;
	muic_notifier.cxt.rprd = 0;
#endif
}

static void __set_noti_cxt(int attach, int type)
{
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	__set_noti_cxt_dest(attach, type, PDIC_NOTIFY_DEV_ALL);
#else
	__set_noti_cxt_dest(attach, type, 0);
#endif
}

#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
static void __set_pdic_noti_cxt(int attach, int type)
{
	if (type < 0) {
		muic_pdic_notifier.cmd = attach;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		muic_pdic_notifier.cxt.attach = attach;
#endif
		return;
	}

	/* Old Interface */
	muic_pdic_notifier.cmd = attach;
	muic_pdic_notifier.attached_dev = type;

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	/* New Interface */
#if defined(CONFIG_USE_DEDICATED_MUIC)
	muic_pdic_notifier.cxt.src = PDIC_NOTIFY_DEV_DEDICATED_MUIC;
#else
	muic_pdic_notifier.cxt.src = PDIC_NOTIFY_DEV_MUIC;
#endif
	muic_pdic_notifier.cxt.dest = PDIC_NOTIFY_DEV_ALL;
	muic_pdic_notifier.cxt.id = PDIC_NOTIFY_ID_ATTACH;
	muic_pdic_notifier.cxt.attach = attach;
	muic_pdic_notifier.cxt.cable_type = type;
	muic_pdic_notifier.cxt.rprd = 0;
#endif
}
#endif

#if IS_ENABLED(CONFIG_MUIC_POGO)
static void __set_pogo_noti_cxt(int attach, int type)
{
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (type < 0) {
		muic_notifier.pogo_cxt.attach = attach;
		return;
	}
	/* New Interface */
	muic_notifier.pogo_cxt.src = PDIC_NOTIFY_DEV_MUIC;
	muic_notifier.pogo_cxt.dest = PDIC_NOTIFY_DEV_ALL;
	muic_notifier.pogo_cxt.id = PDIC_NOTIFY_ID_POGO;
	muic_notifier.pogo_cxt.attach = attach;
	muic_notifier.pogo_cxt.cable_type = type;
	muic_notifier.pogo_cxt.rprd = 0;
#endif
}
#endif /* CONFIG_MUIC_POGO */

int muic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			muic_notifier_device_t listener)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	void *pcxt;
#endif

	pr_info("%s: listener=%d register\n", __func__, listener);
	if (!muic_notifier_init_done)
		muic_notifier_init();

	SET_MUIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(muic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	pcxt = muic_uses_new_noti ? &(muic_notifier.cxt) :
			(void *)&(muic_notifier.attached_dev);

	/* current muic's attached_device status notify */
	nb->notifier_call(nb, muic_notifier.cxt.attach, pcxt);
#if IS_ENABLED(CONFIG_MUIC_POGO)
	nb->notifier_call(nb, muic_notifier.pogo_cxt.attach, &(muic_notifier.pogo_cxt));
#endif /* CONFIG_MUIC_POGO */
#else
	nb->notifier_call(nb, muic_notifier.cmd,
			&(muic_notifier.attached_dev));
#endif

	return ret;
}
EXPORT_SYMBOL(muic_notifier_register);

int muic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(muic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_MUIC_NOTIFIER_BLOCK(nb);

	return ret;
}
EXPORT_SYMBOL(muic_notifier_unregister);

static int muic_notifier_notify(void)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	void *pcxt;

	pr_info("%s: CMD=%d, DATA=%d\n", __func__, muic_notifier.cxt.attach,
			muic_notifier.cxt.cable_type);

	pcxt = muic_uses_new_noti ? &(muic_notifier.cxt) :
			(void *)&(muic_notifier.attached_dev);

	ret = blocking_notifier_call_chain(&(muic_notifier.notifier_call_chain),
			muic_notifier.cxt.attach, pcxt);
#else
	pr_info("%s: CMD=%d, DATA=%d\n", __func__, muic_notifier.cmd,
			muic_notifier.attached_dev);
	ret = blocking_notifier_call_chain(&(muic_notifier.notifier_call_chain),
			muic_notifier.cmd, &(muic_notifier.attached_dev));
#endif

#if IS_ENABLED(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
#if IS_ENABLED(CONFIG_MUIC_SUPPORT_PDIC) && IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_notifier.cxt.attach != 0)
		send_muic_cable_intent(muic_notifier.cxt.cable_type);
	else
		send_muic_cable_intent(0);
#else
	if (muic_notifier.cmd != 0)
		send_muic_cable_intent(muic_notifier.attached_dev);
	else
		send_muic_cable_intent(0);
#if IS_ENABLED(CONFIG_MUIC_POGO)
	if (muic_notifier.pogo_cxt.cable_type)
		send_muic_cable_intent(muic_notifier.pogo_cxt.cable_type);
#endif /* CONFIG_MUIC_POGO */
#endif	/* CONFIG_MUIC_SUPPORT_PDIC */
#endif  /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */
#endif	/* CONFIG_SEC_FACTORY */

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
int muic_pdic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			muic_notifier_device_t listener)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	void *pcxt;
#endif

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_MUIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(muic_pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	pcxt = muic_pdic_uses_new_noti ? &(muic_pdic_notifier.cxt) : \
			(void *)&(muic_pdic_notifier.attached_dev);

	/* current muic's attached_device status notify */
	nb->notifier_call(nb, muic_pdic_notifier.cxt.attach, pcxt);
#else
	nb->notifier_call(nb, muic_pdic_notifier.cmd,
			&(muic_pdic_notifier.attached_dev));
#endif

	return ret;
}
EXPORT_SYMBOL(muic_pdic_notifier_register);

int muic_pdic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(muic_pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_MUIC_NOTIFIER_BLOCK(nb);

	return ret;
}
EXPORT_SYMBOL(muic_pdic_notifier_unregister);

static int muic_pdic_notifier_notify(void)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	void *pcxt;

	pr_info("%s: CMD=%d, DATA=%d\n", __func__, muic_pdic_notifier.cxt.attach,
			muic_pdic_notifier.cxt.cable_type);

	pcxt = muic_pdic_uses_new_noti ? &(muic_pdic_notifier.cxt) : \
			(void *)&(muic_pdic_notifier.attached_dev);

	ret = blocking_notifier_call_chain(&(muic_pdic_notifier.notifier_call_chain),
			muic_pdic_notifier.cxt.attach, pcxt);
#else
	pr_info("%s: CMD=%d, DATA=%d\n", __func__, muic_pdic_notifier.cmd,
			muic_pdic_notifier.attached_dev);
	ret = blocking_notifier_call_chain(&(muic_pdic_notifier.notifier_call_chain),
			muic_pdic_notifier.cmd, &(muic_pdic_notifier.attached_dev));
#endif

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}
#endif	/* CONFIG_PDIC_SLSI_NON_MCU */

#if IS_ENABLED(CONFIG_MUIC_POGO)
static int muic_pogo_notifier_notify(void)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	void *pcxt = &(muic_notifier.pogo_cxt);

	pr_info("%s: CMD=%d, DATA=%d\n", __func__, muic_notifier.pogo_cxt.attach,
			muic_notifier.pogo_cxt.cable_type);

	ret = blocking_notifier_call_chain(&(muic_notifier.notifier_call_chain),
			muic_notifier.pogo_cxt.attach, pcxt);
#endif

#if IS_ENABLED(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_MUIC_SUPPORT_PDIC) && IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_notifier.pogo_cxt.attach != 0)
		send_muic_cable_intent(muic_notifier.pogo_cxt.cable_type);
	else
		send_muic_cable_intent(0);
	if (muic_notifier.cxt.cable_type)
		send_muic_cable_intent(muic_notifier.cxt.cable_type);
#endif	/* CONFIG_MUIC_SUPPORT_CCIC */
#endif	/* CONFIG_SEC_FACTORY */

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}

void muic_pogo_notifier_attach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

	__set_pogo_noti_cxt(MUIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_pogo_notifier_notify();
}
EXPORT_SYMBOL(muic_pogo_notifier_attach_attached_dev);

void muic_pogo_notifier_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	pr_info("%s: (%d)\n", __func__, cur_dev);

	__set_pogo_noti_cxt(MUIC_NOTIFY_CMD_DETACH, -1);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_notifier.pogo_cxt.cable_type != cur_dev)
		pr_warn("%s: attached_dev of muic_notifier(%d) != muic_data(%d)\n",
			__func__, muic_notifier.pogo_cxt.cable_type, cur_dev);

	if (muic_notifier.pogo_cxt.cable_type != ATTACHED_DEV_NONE_MUIC) {
		/* muic's attached_device detach broadcast */
		muic_pogo_notifier_notify();
	}
#endif

	__set_pogo_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
}
EXPORT_SYMBOL(muic_pogo_notifier_detach_attached_dev);
#endif /* CONFIG_MUIC_POGO */

void muic_notifier_attach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

	__set_noti_cxt(MUIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify();
}
EXPORT_SYMBOL(muic_notifier_attach_attached_dev);

void muic_pdic_notifier_attach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
	__set_pdic_noti_cxt(MUIC_PDIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_pdic_notifier_notify();
#else
	__set_noti_cxt(MUIC_PDIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify();
#endif
}
EXPORT_SYMBOL(muic_pdic_notifier_attach_attached_dev);

void muic_pdic_notifier_detach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
	__set_pdic_noti_cxt(MUIC_PDIC_NOTIFY_CMD_DETACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_pdic_notifier_notify();
#else
	__set_noti_cxt(MUIC_PDIC_NOTIFY_CMD_DETACH, muic_notifier.attached_dev);
	/* muic's attached_device attach broadcast */
	muic_notifier_notify();
#endif
}
EXPORT_SYMBOL(muic_pdic_notifier_detach_attached_dev);

void muic_notifier_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	pr_info("%s: (%d)\n", __func__, cur_dev);

	__set_noti_cxt(MUIC_NOTIFY_CMD_DETACH, -1);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_notifier.cxt.cable_type != cur_dev)
		pr_warn("%s: attached_dev of muic_notifier(%d) != muic_data(%d)\n",
			__func__, muic_notifier.cxt.cable_type, cur_dev);

	if (muic_notifier.cxt.cable_type != ATTACHED_DEV_NONE_MUIC) {
		/* muic's attached_device detach broadcast */
		muic_notifier_notify();
	}
#else
	if (muic_notifier.attached_dev != cur_dev)
		pr_warn("%s: attached_dev of muic_notifier(%d) != muic_data(%d)\n",
			__func__, muic_notifier.attached_dev, cur_dev);

	if (muic_notifier.attached_dev != ATTACHED_DEV_NONE_MUIC) {
		/* muic's attached_device detach broadcast */
		muic_notifier_notify();
	}
#endif

	__set_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
}
EXPORT_SYMBOL(muic_notifier_detach_attached_dev);

void muic_notifier_logically_attach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

	__set_noti_cxt(MUIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify();
}
EXPORT_SYMBOL(muic_notifier_logically_attach_attached_dev);

void muic_notifier_logically_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	pr_info("%s: (%d)\n", __func__, cur_dev);

	__set_noti_cxt(MUIC_NOTIFY_CMD_DETACH, cur_dev);

	/* muic's attached_device detach broadcast */
	muic_notifier_notify();

	__set_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
}
EXPORT_SYMBOL(muic_notifier_logically_detach_attached_dev);

#if IS_ENABLED(CONFIG_VIRTUAL_MUIC)
void vt_muic_notifier_attach_attached_dev(muic_attached_dev_t new_dev)
{
	pr_info("%s: (%d)\n", __func__, new_dev);

	__set_noti_cxt_dest(MUIC_NOTIFY_CMD_ATTACH, new_dev, PDIC_NOTIFY_DEV_PDIC);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify();
}
EXPORT_SYMBOL(vt_muic_notifier_attach_attached_dev);

void vt_muic_notifier_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	pr_info("%s: (%d)\n", __func__, cur_dev);

	__set_noti_cxt_dest(MUIC_NOTIFY_CMD_DETACH, -1, PDIC_NOTIFY_DEV_PDIC);

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	if (muic_notifier.cxt.cable_type != cur_dev)
		pr_warn("%s: attached_dev of muic_notifier(%d) != muic_data(%d)\n",
			__func__, muic_notifier.cxt.cable_type, cur_dev);

	if (muic_notifier.cxt.cable_type != ATTACHED_DEV_NONE_MUIC) {
		/* muic's attached_device detach broadcast */
		muic_notifier_notify();
	}
#else
	if (muic_notifier.attached_dev != cur_dev)
		pr_warn("%s: attached_dev of muic_notifier(%d) != muic_data(%d)\n",
			__func__, muic_notifier.attached_dev, cur_dev);

	if (muic_notifier.attached_dev != ATTACHED_DEV_NONE_MUIC) {
		/* muic's attached_device detach broadcast */
		muic_notifier_notify();
	}
#endif

	__set_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
}
EXPORT_SYMBOL(vt_muic_notifier_detach_attached_dev);
#endif

static int muic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	if (muic_notifier_init_done) {
		pr_info("%s already registered\n", __func__);
		return ret;
	}
	muic_notifier_init_done = 1;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	switch_device = sec_device_create(NULL, "switch");
	if (IS_ERR(switch_device)) {
		pr_err("%s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
	}
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	muic_uses_new_noti = 1;
#endif
	BLOCKING_INIT_NOTIFIER_HEAD(&(muic_notifier.notifier_call_chain));
	__set_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
#if IS_ENABLED(CONFIG_PDIC_SLSI_NON_MCU)
	BLOCKING_INIT_NOTIFIER_HEAD(&(muic_pdic_notifier.notifier_call_chain));
	__set_pdic_noti_cxt(0, ATTACHED_DEV_UNKNOWN_MUIC);
	muic_pdic_uses_new_noti = 1;
#endif

#if IS_ENABLED(CONFIG_MUIC_POGO)
	__set_pogo_noti_cxt(0, ATTACHED_DEV_NONE_MUIC);
#endif /* CONFIG_MUIC_POGO */

#if IS_ENABLED(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_ANDROID_SWITCH) || IS_ENABLED(CONFIG_SWITCH)
	ret = switch_dev_register(&switch_muic_dev);
	if (ret < 0)
		pr_err("%s: Failed to register attached_muic_cable switch(%d)\n",
				__func__, ret);
#endif  /* CONFIG_ANDROID_SWITCH || CONFIG_SWITCH */
#endif
	return ret;
}

static void __exit muic_notifier_exit(void)
{
	pr_info("%s: exit\n", __func__);
}

device_initcall(muic_notifier_init);
module_exit(muic_notifier_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("Muic Notifier");
MODULE_LICENSE("GPL");
