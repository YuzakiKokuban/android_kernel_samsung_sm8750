// SPDX-License-Identifier: GPL-2.0

#include <asm-generic/sections.h>
#include <linux/mm.h>
#include "../../mm/slab.h"
#include <linux/slub_def.h>
#include <linux/binfmts.h>

#include <linux/kdp.h>
#include <linux/mount.h>
#include <linux/cred.h>
#include <linux/security.h>
#include <linux/init_task.h>
#include "../../fs/mount.h"

#define VERITY_PARAM_LENGTH 20
#define KDP_CRED_SYS_ID 1000

/* security/selinux/include/objsec.h */
struct task_security_struct {
	u32 osid;               /* SID prior to last execve */
	u32 sid;                /* current SID */
	u32 exec_sid;           /* exec SID */
	u32 create_sid;         /* fscreate SID */
	u32 keycreate_sid;      /* keycreate SID */
	u32 sockcreate_sid;     /* fscreate SID */
	void *bp_cred;
};
/* security/selinux/hooks.c */
struct task_security_struct init_sec __kdp_ro;

bool kdp_enable __kdp_ro;
static int __check_verifiedboot __kdp_ro;
static int __is_kdp_recovery __kdp_ro;

static char verifiedbootstate[VERITY_PARAM_LENGTH];

static struct kmem_cache *cred_jar_ro;
static struct kmem_cache *tsec_jar;
static struct kmem_cache *usecnt_jar;

void __init kdp_init(void)
{
	struct kdp_init cred;

	memset((void *)&cred, 0, sizeof(kdp_init));

	cred._srodata		= (u64)__start_rodata;
	cred._erodata		= (u64)__end_rodata;
	cred.init_mm_pgd	= (u64)swapper_pg_dir;
	cred.credSize		= sizeof(struct cred_kdp);
	cred.sp_size		= sizeof(struct task_security_struct);
	cred.pgd_mm		= offsetof(struct mm_struct, pgd);
	cred.uid_cred		= offsetof(struct cred, uid);
	cred.euid_cred		= offsetof(struct cred, euid);
	cred.gid_cred		= offsetof(struct cred, gid);
	cred.egid_cred		= offsetof(struct cred, egid);

	cred.bp_pgd_cred	= offsetof(struct cred_kdp, bp_pgd);
	cred.bp_task_cred	= offsetof(struct cred_kdp, bp_task);
	cred.type_cred		= offsetof(struct cred_kdp, type);
	cred.security_cred	= offsetof(struct cred, security);
	cred.usage_cred		= offsetof(struct cred_kdp, use_cnt);
	cred.cred_task		= offsetof(struct task_struct, cred);
	cred.mm_task		= offsetof(struct task_struct, mm);

	cred.pid_task		= offsetof(struct task_struct, pid);
	cred.rp_task		= offsetof(struct task_struct, real_parent);
	cred.comm_task		= offsetof(struct task_struct, comm);
	cred.bp_cred_secptr	= offsetof(struct task_security_struct, bp_cred);
	cred.verifiedbootstate	= (u64)verifiedbootstate;

	uh_call(UH_APP_KDP, KDP_INIT, (u64)&cred, 0, 0, 0);
}

static int __init verifiedboot_state_setup(char *str)
{
	strscpy(verifiedbootstate, str, sizeof(verifiedbootstate));

	if (!strncmp(verifiedbootstate, "orange", sizeof("orange")))
		__check_verifiedboot = 1;
	return 0;
}
__setup("androidboot.verifiedbootstate=", verifiedboot_state_setup);

static int __init boot_recovery(char *str)
{
	int temp = 0;

	if (get_option(&str, &temp)) {
		__is_kdp_recovery = temp;
		return 0;
	}
	return -EINVAL;
}
early_param("androidboot.boot_recovery", boot_recovery);

/*------------------------------------------------
 * CRED
 *------------------------------------------------
 */
struct cred_kdp_init {
	atomic_long_t use_cnt;
	struct ro_rcu_head ro_rcu_head_init;
};

struct cred_kdp_init init_cred_use_cnt = {
	.use_cnt = ATOMIC_LONG_INIT(4),
	.ro_rcu_head_init = {
		.non_rcu = 0,
		.bp_cred = NULL,
	},
};

struct cred_kdp init_cred_kdp __kdp_ro = {
//struct cred_kdp init_cred_kdp  = {
	.use_cnt = (atomic_long_t *)&init_cred_use_cnt,
	.bp_task = &init_task,
	.bp_pgd	= NULL,
	.type = 0,
};


/* Dummy constructor to make sure we have separate slabs caches. */
static void cred_ctor(void *data) {}
static void sec_ctor(void *data) {}
static void usecnt_ctor(void *data) {}

void __init kdp_cred_init(void)
{
	slab_flags_t flags = SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_ACCOUNT;

	if (!kdp_enable)
		return;
	cred_jar_ro = kmem_cache_create("cred_jar_ro",
					sizeof(struct cred_kdp),
					0, flags, cred_ctor);
	if (!cred_jar_ro)
		panic("Unable to create RO Cred cache\n");
	cred_jar_ro->flags |= SLAB_SKIP_KFENCE;

	tsec_jar = kmem_cache_create("tsec_jar",
				     sizeof(struct task_security_struct),
				     0, flags, sec_ctor);
	if (!tsec_jar)
		panic("Unable to create RO security cache\n");
	tsec_jar->flags |= SLAB_SKIP_KFENCE;

	usecnt_jar = kmem_cache_create("usecnt_jar",
				       sizeof(struct cred_kdp_init),
				       0, flags, usecnt_ctor);
	if (!usecnt_jar)
		panic("Unable to create use count jar\n");

	uh_call(UH_APP_KDP, JARRO_TSEC_SIZE, (u64)cred_jar_ro->size,
		(u64)tsec_jar->size, 0, 0);
}

unsigned int kdp_get_usecount(struct cred *cred)
{
	int ret = is_kdp_protect_addr((unsigned long)cred);

	if (ret == PROTECT_INIT)
		return (unsigned int)atomic_long_read(init_cred_kdp.use_cnt);
	else if (ret == PROTECT_KMEM)
		return (unsigned int)atomic_long_read(((struct cred_kdp *)cred)->use_cnt);
	else
		return atomic_long_read(&cred->usage);
}

void kdp_usecount_inc(struct cred *cred)
{
	int ret = is_kdp_protect_addr((unsigned long)cred);

	if (ret == PROTECT_INIT)
		atomic_long_inc(init_cred_kdp.use_cnt);
	else if (ret == PROTECT_KMEM)
		atomic_long_inc(((struct cred_kdp *)cred)->use_cnt);
	else
		atomic_long_inc(&cred->usage);
}
EXPORT_SYMBOL(kdp_usecount_inc);

unsigned int kdp_usecount_inc_not_zero(struct cred *cred)
{
	int ret = is_kdp_protect_addr((unsigned long)cred);

	if (ret == PROTECT_INIT)
		return (unsigned int)atomic_long_inc_not_zero(init_cred_kdp.use_cnt);
	else if (ret == PROTECT_KMEM)
		return (unsigned int)atomic_long_inc_not_zero(((struct cred_kdp *)cred)->use_cnt);
	else
		return atomic_long_inc_not_zero(&cred->usage);
}

unsigned int kdp_usecount_dec_and_test(struct cred *cred)
{
	int ret = is_kdp_protect_addr((unsigned long)cred);

	if (ret == PROTECT_INIT)
		return (unsigned int)atomic_long_dec_and_test(init_cred_kdp.use_cnt);
	else if (ret == PROTECT_KMEM)
		return (unsigned int)atomic_long_dec_and_test(((struct cred_kdp *)cred)->use_cnt);
	else
		return atomic_long_dec_and_test(&cred->usage);
}
EXPORT_SYMBOL(kdp_usecount_dec_and_test);

void kdp_set_cred_non_rcu(struct cred *cred, int val)
{
	if (is_kdp_protect_addr((unsigned long)cred))
		GET_ROCRED_RCU(cred)->non_rcu = val;
	else
		cred->non_rcu = val;
}
EXPORT_SYMBOL(kdp_set_cred_non_rcu);

/* match for kernel/cred.c function */
inline void set_kdp_cred_subscribers(struct cred *cred, int n)
{
#ifdef CONFIG_DEBUG_CREDENTIALS
	atomic_set(&cred->subscribers, n);
#endif
}

/* Check whether the address belong to Cred Area */
int is_kdp_protect_addr(unsigned long addr)
{
	struct kmem_cache *s;
	struct page *page;
	struct slab *p_slab;
	void *objp = (void *)addr;

	if (!objp)
		return 0;

	if (!kdp_enable)
		return 0;

	if ((addr == ((unsigned long)&init_cred)) ||
		(addr == ((unsigned long)&init_sec)))
		return PROTECT_INIT;

	page = virt_to_head_page(objp);
	p_slab = page_slab(page);
	s = p_slab->slab_cache;
	if (s && (s == cred_jar_ro || s == tsec_jar))
		return PROTECT_KMEM;

	return 0;
}

/* We use another function to free protected creds. */
extern void security_cred_free_hook(struct cred *cred);
void put_rocred_rcu(struct rcu_head *rcu)
{
	struct cred *cred = container_of(rcu, struct ro_rcu_head, rcu)->bp_cred;

	if (atomic_long_read(((struct cred_kdp *)cred)->use_cnt) != 0)
		panic("RO_CRED: put rocred rcu func sees %p with usage %ld\n",
				cred, atomic_long_read(((struct cred_kdp *)cred)->use_cnt));

	security_cred_free_hook(cred);
	kdp_free_security((unsigned long)cred->security);

	key_put(cred->session_keyring);
	key_put(cred->process_keyring);
	key_put(cred->thread_keyring);
	key_put(cred->request_key_auth);
	if (cred->group_info)
		put_group_info(cred->group_info);
	free_uid(cred->user);
	if (cred->ucounts)
		put_ucounts(cred->ucounts);
	put_user_ns(cred->user_ns);
	if (((struct cred_kdp *)cred)->use_cnt)
		kmem_cache_free(usecnt_jar, (void *)((struct cred_kdp *)cred)->use_cnt);
	kmem_cache_free(cred_jar_ro, cred);
}

void kdp_put_cred_rcu(struct cred *cred, void *put_cred_rcu)
{
	if (is_kdp_protect_addr((unsigned long)cred)) {
		if (GET_ROCRED_RCU(cred)->non_rcu)
			put_rocred_rcu(&(GET_ROCRED_RCU(cred)->rcu));
		else
			call_rcu(&(GET_ROCRED_RCU(cred)->rcu), put_rocred_rcu);
	} else {
		void (*f)(struct rcu_head *) = put_cred_rcu;

		if (cred->non_rcu)
			f(&cred->rcu);
		else
			call_rcu(&cred->rcu, f);
	}
}

/* prepare_ro_creds - Prepare a new set of credentials which is protected by KDP */
struct cred *prepare_ro_creds(struct cred *old, int kdp_cmd, u64 p)
{
	u64 pgd = (u64)(current->mm ? current->mm->pgd : swapper_pg_dir);
	struct cred_kdp temp_old __aligned(256);
	struct cred_kdp *new_ro = NULL;
	struct cred_param param_data __aligned(64);
	void *use_cnt_ptr = NULL;
	void *rcu_ptr = NULL;
	void *tsec = NULL;

	new_ro = kmem_cache_alloc(cred_jar_ro, GFP_KERNEL | __GFP_NOFAIL);
	if (!new_ro)
		panic("[%d] : kmem_cache_alloc() failed", kdp_cmd);

	use_cnt_ptr = kmem_cache_alloc(usecnt_jar, GFP_KERNEL | __GFP_NOFAIL);
	if (!use_cnt_ptr)
		panic("[%d] : Unable to allocate usage pointer\n", kdp_cmd);

	// get_usecnt_rcu
	rcu_ptr = (struct ro_rcu_head *)((atomic_long_t *)use_cnt_ptr + 1);
	((struct ro_rcu_head *)rcu_ptr)->bp_cred = (void *)new_ro;

	tsec = kmem_cache_alloc(tsec_jar, GFP_KERNEL | __GFP_NOFAIL);
	if (!tsec)
		panic("[%d] : Unable to allocate security pointer\n", kdp_cmd);

	// make cred_kdp 'temp_old'
	if ((u64)current->cred == (u64)&init_cred)
		memcpy(&temp_old, &init_cred_kdp, sizeof(struct cred_kdp));
	else
		memcpy(&temp_old, current->cred, sizeof(struct cred_kdp));

	memcpy(&temp_old, old, sizeof(struct cred));

	// init
	memset((void *)&param_data, 0, sizeof(struct cred_param));
	param_data.cred = &temp_old;
	param_data.cred_ro = new_ro;
	param_data.use_cnt_ptr = use_cnt_ptr;
	param_data.sec_ptr = tsec;
	param_data.type = kdp_cmd;
	param_data.use_cnt = (u64)p;

	uh_call(UH_APP_KDP, PREPARE_RO_CRED, (u64)&param_data,
				(u64)current, (u64)&init_cred, (u64)&init_cred_kdp);
	if (kdp_cmd == CMD_COPY_CREDS) {
		if ((new_ro->bp_task != (void *)p) ||
			new_ro->cred.security != tsec ||
			new_ro->use_cnt != use_cnt_ptr) {
			panic("[%d]: KDP Call failed task=0x%lx:0x%lx, sec=0x%lx:0x%lx, usecnt=0x%lx:0x%lx",
					kdp_cmd,
					(unsigned long) new_ro->bp_task,
					(unsigned long) p,
					(unsigned long) new_ro->cred.security,
					(unsigned long) tsec,
					(unsigned long) new_ro->use_cnt,
					(unsigned long) use_cnt_ptr);
		}
	} else {
		if ((new_ro->bp_task != current) ||
			(current->mm && new_ro->bp_pgd != (void *)pgd) ||
			(new_ro->cred.security != tsec) ||
			(new_ro->use_cnt != use_cnt_ptr)) {
			panic("[%d]: KDP Call failed task=0x%lx:0x%lx, sec=0x%lx:0x%lx, usecnt=0x%lx:0x%lx, pgd=0x%lx:0x%lx",
					kdp_cmd, (unsigned long) new_ro->bp_task,
					(unsigned long) current,
					(unsigned long) new_ro->cred.security,
					(unsigned long) tsec,
					(unsigned long) new_ro->use_cnt,
					(unsigned long) use_cnt_ptr,
					(unsigned long) new_ro->bp_pgd,
					(unsigned long) pgd);
		}
	}

	GET_ROCRED_RCU(new_ro)->non_rcu = old->non_rcu;
	GET_ROCRED_RCU(new_ro)->reflected_cred = 0;
	atomic_long_set(new_ro->use_cnt, 2);

	set_kdp_cred_subscribers((struct cred *)new_ro, 0);
	get_group_info(new_ro->cred.group_info);
	get_uid(new_ro->cred.user);
	get_user_ns(new_ro->cred.user_ns);

#ifdef CONFIG_KEYS
	key_get(new_ro->cred.session_keyring);
	key_get(new_ro->cred.process_keyring);
	key_get(new_ro->cred.thread_keyring);
	key_get(new_ro->cred.request_key_auth);
#endif
	if (!get_ucounts(new_ro->cred.ucounts))
		panic("[KDP] : ucount is NULL\n");

	return (struct cred *)new_ro;
}

/* security/selinux/hooks.c */
static bool is_kdp_tsec_jar(unsigned long addr)
{
	struct kmem_cache *s;
	struct page *page;
	struct slab *p_slab;
	void *objp = (void *)addr;

	if (!objp)
		return false;

	page = virt_to_head_page(objp);
	p_slab = page_slab(page);
	s = p_slab->slab_cache;
	if (s && s == tsec_jar)
		return true;
	return false;
}

static inline int chk_invalid_kern_ptr(u64 tsec)
{
	return (((u64)tsec >> 39) != (u64)0x1FFFFFF);
}

void kdp_free_security(unsigned long tsec)
{
	if (!tsec || chk_invalid_kern_ptr(tsec))
		return;

	if (is_kdp_tsec_jar(tsec))
		kmem_cache_free(tsec_jar, (void *)tsec);
	else
		kfree((void *)tsec);
}

void kdp_assign_pgd(struct task_struct *p)
{
	struct cred_kdp *p_cred = (struct cred_kdp *)p->cred;
	u64 pgd = (u64)(p->mm ? p->mm->pgd : swapper_pg_dir);

	if (p_cred->bp_pgd == (void *)pgd)
		return;

	uh_call(UH_APP_KDP, SET_CRED_PGD, (u64)p_cred, (u64)pgd, 0, 0);
}

void set_rocred_ucounts(struct cred *cred, struct ucounts *new_ucounts)
{
	if (is_kdp_protect_addr((u64)cred)) {
		if (cred == &init_cred)
			uh_call(UH_APP_KDP, SET_CRED_UCOUNTS, (u64)cred,
				(u64)&init_cred, (u64)&(init_cred.ucounts), (u64)new_ucounts);
		else
			uh_call(UH_APP_KDP, SET_CRED_UCOUNTS, (u64)cred, (u64)&init_cred,
				(u64)&(cred->ucounts), (u64)new_ucounts);
	} else {
		cred->ucounts = new_ucounts;
	}
}

struct task_security_struct init_sec __kdp_ro;
static inline unsigned int
	cmp_sec_integrity(const struct cred *cred, struct mm_struct *mm)
{
	if (cred == &init_cred) {
		if (init_cred_kdp.bp_task != current)
			pr_err("[KDP] init_cred_kdp.bp_task: 0x%lx, current: 0x%lx\n",
				(unsigned long) init_cred_kdp.bp_task, (unsigned long) current);

		if (mm && (init_cred_kdp.bp_pgd != swapper_pg_dir) &&
					(init_cred_kdp.bp_pgd != mm->pgd))
			pr_err("[KDP] mm: 0x%lx, init_cred_kdp.bp_pgd: 0x%lx, swapper_pg_dir: %p, mm->pgd: 0x%lx\n",
					(unsigned long) mm, (unsigned long) init_cred_kdp.bp_pgd,
					swapper_pg_dir, (unsigned long) mm->pgd);

		return ((init_cred_kdp.bp_task != current) ||
				(mm && (!(in_interrupt() || in_softirq())) &&
				(init_cred_kdp.bp_pgd != swapper_pg_dir) &&
				(init_cred_kdp.bp_pgd != mm->pgd)));
	} else {
		if (((struct cred_kdp *)cred)->bp_task != current)
			pr_err("[KDP] cred->bp_task: 0x%lx, current: %s:0x%lx\n",
					(unsigned long) ((struct cred_kdp *)cred)->bp_task,
					current->comm,
					(unsigned long)current);

		if (mm && (((struct cred_kdp *)cred)->bp_pgd != swapper_pg_dir) &&
			(((struct cred_kdp *)cred)->bp_pgd != mm->pgd))
			pr_err("[KDP] mm: 0x%lx, cred->bp_pgd: 0x%lx, swapper_pg_dir: %p, mm->pgd: 0x%lx\n",
						(unsigned long) mm,
						(unsigned long) ((struct cred_kdp *)cred)->bp_pgd,
						swapper_pg_dir,
						(unsigned long) mm->pgd);

		return ((((struct cred_kdp *)cred)->bp_task != current) ||
				(mm && (!(in_interrupt() || in_softirq())) &&
				(((struct cred_kdp *)cred)->bp_pgd != swapper_pg_dir) &&
				(((struct cred_kdp *)cred)->bp_pgd != mm->pgd)));
	}
	// Want to not reaching
	return 1;
}

static inline bool is_kdp_invalid_cred_sp(u64 cred, u64 sec_ptr)
{
	struct task_security_struct *tsec = (struct task_security_struct *)sec_ptr;
	u64 cred_size = sizeof(struct cred_kdp);
	u64 tsec_size = sizeof(struct task_security_struct);

	if (cred == (u64)&init_cred)
		cred_size = sizeof(struct cred);

	if ((cred == (u64)&init_cred) && (sec_ptr == (u64)&init_sec))
		return false;

	if (!is_kdp_protect_addr(cred) ||
		!is_kdp_protect_addr(cred + cred_size) ||
		!is_kdp_protect_addr(sec_ptr) ||
		!is_kdp_protect_addr(sec_ptr + tsec_size)) {
		pr_err("[KDP] cred: %d, cred + sizeof(cred): %d, sp: %d, sp + sizeof(tsec): %d",
				is_kdp_protect_addr(cred),
				is_kdp_protect_addr(cred + cred_size),
				is_kdp_protect_addr(sec_ptr),
				is_kdp_protect_addr(sec_ptr + tsec_size));
		return true;
	}

	if ((u64)tsec->bp_cred != cred) {
		pr_err("[KDP] %s: tesc->bp_cred: %lx, cred: %lx\n",
				__func__, (unsigned long)tsec->bp_cred, (unsigned long)cred);
		return true;
	}

	return false;
}

/* Main function to verify cred security context of a process */
int security_integrity_current(void)
{
	const struct cred *cur_cred = current_cred();

	rcu_read_lock();
	if (kdp_enable &&
			(is_kdp_invalid_cred_sp((u64)cur_cred, (u64)cur_cred->security)
			|| cmp_sec_integrity(cur_cred, current->mm)
			)) {
		rcu_read_unlock();
		panic("KDP CRED PROTECTION VIOLATION\n");
	}
	rcu_read_unlock();
	return 0;
}

inline int get_kdp_kmem_cache_type(const char *name)
{
	if (name) {
#ifdef CONFIG_KDP
		if (!strncmp(name, CRED_JAR_RO, strlen(CRED_JAR_RO)))
			return CRED_JAR_TYPE;
		if (!strncmp(name, TSEC_JAR, strlen(TSEC_JAR)))
			return TSEC_JAR_TYPE;
#endif
	}
	return UNKNOWN_JAR_TYPE;
}

inline bool is_kdp_kmem_cache_name(const char *name)
{
	if (name) {
#ifdef CONFIG_KDP
		if (!strncmp(name, CRED_JAR_RO, strlen(CRED_JAR_RO)) ||
		 !strncmp(name, TSEC_JAR, strlen(TSEC_JAR)))
			return true;
#endif
	}
	return false;
}

inline bool is_kdp_kmem_cache(struct kmem_cache *s)
{
	if (!s->name)
		return false;

#ifdef CONFIG_KDP
	if (s == cred_jar_ro || s == tsec_jar)
		return true;
#endif
	return false;
}
