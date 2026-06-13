// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <trace/hooks/cpufreq.h>
#include <trace/hooks/topology.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "walt.h"

unsigned int cpuinfo_max_freq_cached;

char sched_lib_name[LIB_PATH_LENGTH];
char sched_lib_task[LIB_PATH_LENGTH];
unsigned int sched_lib_mask_force;

static bool is_sched_lib_based_app(pid_t pid)
{
	const char *name = NULL;
	char *libname, *lib_list;
	struct vm_area_struct *vma;
	char path_buf[LIB_PATH_LENGTH];
	char *tmp_lib_name;
	bool found = false;
	struct task_struct *p;
	struct mm_struct *mm;

	if (strnlen(sched_lib_name, LIB_PATH_LENGTH) == 0)
		return false;

	tmp_lib_name = kmalloc(LIB_PATH_LENGTH, GFP_KERNEL);
	if (!tmp_lib_name)
		return false;

	rcu_read_lock();
	p = pid ? get_pid_task(find_vpid(pid), PIDTYPE_PID) : get_task_struct(current);
	rcu_read_unlock();
	if (!p) {
		kfree(tmp_lib_name);
		return false;
	}

	mm = get_task_mm(p);
	if (mm) {
		MA_STATE(mas, &mm->mm_mt, 0, 0);
		down_read(&mm->mmap_lock);

		mas_for_each(&mas, vma, ULONG_MAX) {
			if (vma->vm_file && vma->vm_flags & VM_EXEC) {
				name = d_path(&vma->vm_file->f_path,
						path_buf, LIB_PATH_LENGTH);
				if (IS_ERR(name))
					goto release_sem;

				strscpy(tmp_lib_name, sched_lib_name, LIB_PATH_LENGTH);
				lib_list = tmp_lib_name;
				while ((libname = strsep(&lib_list, ","))) {
					libname = skip_spaces(libname);
					if (strnstr(name, libname,
						strnlen(name, LIB_PATH_LENGTH))) {
						found = true;
						goto release_sem;
					}
				}
			}
		}

release_sem:
		up_read(&mm->mmap_lock);
		mmput(mm);

	}
	put_task_struct(p);
	kfree(tmp_lib_name);
	return found;
}

bool is_sched_lib_task(void)
{
	if (strnlen(sched_lib_task, LIB_PATH_LENGTH) == 0)
		return false;

	if (strnstr(current->comm, sched_lib_task, strnlen(current->comm, LIB_PATH_LENGTH)))
		return true;

	return false;
}

static char cpu_cap_fixup_target[TASK_COMM_LEN];

static int proc_cpu_capacity_fixup_target_show(struct seq_file *m, void *data)
{
	seq_printf(m, "%s\n", cpu_cap_fixup_target);
	return 0;
}

static int proc_cpu_capacity_fixup_target_open(struct inode *inode,
		struct file *file)
{
	return single_open(file, proc_cpu_capacity_fixup_target_show, NULL);
}

static ssize_t proc_cpu_capacity_fixup_target_write(struct file *file,
		const char __user *buf, size_t count, loff_t *offs)
{
	char temp[TASK_COMM_LEN] = {0, };
	int len = 0;

	len = (count > TASK_COMM_LEN) ? TASK_COMM_LEN : count;
	if (copy_from_user(temp, buf, len))
		return -EFAULT;

	if (temp[len - 1] == '\n')
		temp[len - 1] = '\0';

	strlcpy(cpu_cap_fixup_target, temp, TASK_COMM_LEN);

	return count;
}

static const struct proc_ops proc_cpu_capacity_fixup_target_op = {
	.proc_open = proc_cpu_capacity_fixup_target_open,
	.proc_write = proc_cpu_capacity_fixup_target_write,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static void android_rvh_show_max_freq(void *unused, struct cpufreq_policy *policy,
				     unsigned int *max_freq)
{
	int curr_len = 0;

	if (!cpuinfo_max_freq_cached)
		return;

	curr_len = strnlen(current->comm, TASK_COMM_LEN);
	if (strnlen(cpu_cap_fixup_target, TASK_COMM_LEN) == curr_len) {
		if (!strncmp(current->comm, cpu_cap_fixup_target, curr_len)) {
			*max_freq = cpuinfo_max_freq_cached;
			return;
		}
	}

	if (!(BIT(policy->cpu) & sched_lib_mask_force))
		return;

	if (is_sched_lib_based_app(current->pid) || is_sched_lib_task())
		*max_freq = cpuinfo_max_freq_cached << 1;
}

static void android_rvh_cpu_capacity_show(void *unused,
		unsigned long *capacity, int cpu)
{
	int curr_len = 0;

	curr_len = strnlen(current->comm, TASK_COMM_LEN);
	if (strnlen(cpu_cap_fixup_target, TASK_COMM_LEN) == curr_len) {
		if (!strncmp(current->comm, cpu_cap_fixup_target, curr_len)) {
			*capacity = SCHED_CAPACITY_SCALE;
			return;
		}
	}

	if (!soc_sched_lib_name_capacity)
		return;

	if ((is_sched_lib_based_app(current->pid) || is_sched_lib_task()) &&
			cpu < soc_sched_lib_name_capacity)
		*capacity = 100;
}

void walt_fixup_init(void)
{
	if (!proc_create("cpu_capacity_fixup_target",
			0660, NULL, &proc_cpu_capacity_fixup_target_op))
		pr_err("Failed to register 'cpu_capacity_fixup_target'\n");

	register_trace_android_rvh_show_max_freq(android_rvh_show_max_freq, NULL);
	register_trace_android_rvh_cpu_capacity_show(android_rvh_cpu_capacity_show, NULL);
}
