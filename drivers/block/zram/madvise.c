// SPDX-License-Identifier: GPL-2.0
/*
 * zram_madvise
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/blkdev.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/pagewalk.h>
#include <linux/page_size_compat.h>
#include <linux/pid.h>
#include <linux/ptrace.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/uio.h>
#include <trace/hooks/madvise.h>
#include "zram_ext.h"

#ifdef CONFIG_ZRAM_EXT
struct zram_private {
	struct zram *zram;
	struct list_head *list;
};

static int zram_madvise_pmd_entry(pmd_t *pmd, unsigned long addr,
		unsigned long end, struct mm_walk *walk)
{
	struct zram_private *private = walk->private;
	struct zram *zram = private->zram;
	struct list_head *list = private->list;
	struct vm_area_struct *vma = walk->vma;
	pte_t *ptep = NULL, pte;
	swp_entry_t entry;
	spinlock_t *ptl;

	if (list) {
		if (fatal_signal_pending(current))
			return -EINTR;
		if (rwsem_is_contended(&vma->vm_mm->mmap_lock))
			return -EBUSY;
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		if (pmd_trans_huge(*pmd))
			return 0;
#endif
	}

	for (; addr < end; addr += PAGE_SIZE) {
		if (!ptep++) {
			ptep = pte_offset_map_lock(vma->vm_mm, pmd, addr, &ptl);
			if (!ptep)
				break;
		}
		pte = __ptep_get(ptep);
		if (!is_swap_pte(pte))
			continue;
		entry = pte_to_swp_entry(pte);
		if (unlikely(non_swap_entry(entry)))
			continue;
		pte_unmap_unlock(ptep, ptl);
		ptep = NULL;
		if (list) {
			if (swp_swapcount(entry) > 1)
				continue;
			zram_request_writeback(zram, list, swp_offset(entry));
		} else {
			zram_request_prefetch(zram, swp_offset(entry));
		}
	}

	if (ptep)
		pte_unmap_unlock(ptep, ptl);
	cond_resched();
	return 0;
}

static const struct mm_walk_ops zram_madvise_ops = {
	.pmd_entry		= zram_madvise_pmd_entry,
	.walk_lock		= PGWALK_RDLOCK,
};

static inline bool can_madv_lru_vma(struct vm_area_struct *vma)
{
	return !(vma->vm_flags & (VM_LOCKED|VM_PFNMAP|VM_HUGETLB));
}

static long zram_madvise(struct zram *zram, struct vm_area_struct *vma,
		struct list_head *list, unsigned long start, unsigned long end)
{
	struct zram_private private = {
		.zram = zram,
		.list = list,
	};

	if (list) {
		if (!can_madv_lru_vma(vma))
			return -EINVAL;
		if (!vma_is_anonymous(vma))
			return 0;
	} else {
		if (vma->vm_file)
			return 0;
	}
	return walk_page_range(vma->vm_mm, start, end, &zram_madvise_ops, &private);
}

static struct vm_area_struct *
__find_vma_prev(struct mm_struct *mm, unsigned long addr,
			struct vm_area_struct **pprev)
{
	struct vm_area_struct *vma;
	MA_STATE(mas, &mm->mm_mt, addr, addr);

	vma = mas_walk(&mas);
	*pprev = mas_prev(&mas, 0);
	if (!vma)
		vma = mas_next(&mas, ULONG_MAX);
	return vma;
}

static int madvise_walk_vmas(struct zram *zram, struct mm_struct *mm,
		struct list_head *list, unsigned long start, unsigned long end)
{
	struct vm_area_struct *vma;
	struct vm_area_struct *prev;
	unsigned long tmp;
	int unmapped_error = 0;

	/*
	 * If the interval [start,end) covers some unmapped address
	 * ranges, just ignore them, but return -ENOMEM at the end.
	 * - different from the way of handling in mlock etc.
	 */
	vma = __find_vma_prev(mm, start, &prev);
	if (vma && start > vma->vm_start)
		prev = vma;

	for (;;) {
		int error;

		/* Still start < end. */
		if (!vma)
			return -ENOMEM;

		/* Here start < (end|vma->vm_end). */
		if (start < vma->vm_start) {
			unmapped_error = -ENOMEM;
			start = vma->vm_start;
			if (start >= end)
				break;
		}

		/* Here vma->vm_start <= start < (end|vma->vm_end) */
		tmp = vma->vm_end;
		if (end < tmp)
			tmp = end;

		/* Here vma->vm_start <= start < tmp <= (end|vma->vm_end). */
		error = zram_madvise(zram, vma, list, start, end);
		prev = vma;
		if (error)
			return error;
		start = tmp;
		if (prev && start < prev->vm_end)
			start = prev->vm_end;
		if (start >= end)
			break;
		if (prev)
			vma = find_vma(mm, prev->vm_end);
		else	/* madvise_remove dropped mmap_lock */
			vma = find_vma(mm, start);
	}

	return unmapped_error;
}

static int zram_do_madvise(struct zram *zram, struct mm_struct *mm,
		struct list_head *list, unsigned long start, size_t len_in)
{
	unsigned long end;
	int error;
	size_t len;
	struct blk_plug plug;

	if (!__PAGE_ALIGNED(start))
		return -EINVAL;
	len = __PAGE_ALIGN(len_in);

	/* Check to see whether len was rounded up from small -ve to zero */
	if (len_in && !len)
		return -EINVAL;

	end = start + len;
	if (end < start)
		return -EINVAL;

	if (end == start)
		return 0;

	mmap_read_lock(mm);
	start = untagged_addr_remote(mm, start);
	end = start + len;

	blk_start_plug(&plug);
	error = madvise_walk_vmas(zram, mm, list, start, end);
	blk_finish_plug(&plug);
	mmap_read_unlock(mm);

	return error;
}

static struct task_struct *__pidfd_get_task(int pidfd, unsigned int *flags)
{
	unsigned int f_flags;
	struct pid *pid;
	struct task_struct *task;

	pid = pidfd_get_pid(pidfd, &f_flags);
	if (IS_ERR(pid))
		return ERR_CAST(pid);

	task = get_pid_task(pid, PIDTYPE_TGID);
	put_pid(pid);
	if (!task)
		return ERR_PTR(-ESRCH);

	*flags = f_flags;
	return task;
}

static void zram_process_madvise(void *data, int pidfd,
		const struct iovec __user *vec, size_t vlen, int arg,
		unsigned int flags, ssize_t *ret, bool *bypass)
{
	struct zram *zram = data;
	struct iovec iovstack[UIO_FASTIOV];
	struct iovec *iov = iovstack;
	struct iov_iter iter;
	struct task_struct *task;
	struct mm_struct *mm;
	size_t total_len;
	ssize_t err;
	unsigned int f_flags;
	LIST_HEAD(list);

	if (arg != MADV_WRITEBACK && arg != MADV_PREFETCH)
		return;

	err = import_iovec(ITER_DEST, vec, vlen, ARRAY_SIZE(iovstack), &iov, &iter);
	if (err < 0)
		goto out;

	task = __pidfd_get_task(pidfd, &f_flags);
	if (IS_ERR(task)) {
		err = PTR_ERR(task);
		goto free_iov;
	}

	/* Require PTRACE_MODE_READ to avoid leaking ASLR metadata. */
	mm = mm_access(task, PTRACE_MODE_READ_FSCREDS);
	if (IS_ERR_OR_NULL(mm)) {
		err = IS_ERR(mm) ? PTR_ERR(mm) : -ESRCH;
		goto release_task;
	}

	/*
	 * Require CAP_SYS_NICE for influencing process performance. Note that
	 * only non-destructive hints are currently supported.
	 */
	if (!capable(CAP_SYS_NICE)) {
		err = -EPERM;
		goto release_mm;
	}

	total_len = iov_iter_count(&iter);

	while (iov_iter_count(&iter)) {
		err = zram_do_madvise(zram, mm,
				arg == MADV_WRITEBACK ? &list : NULL,
				(unsigned long)iter_iov_addr(&iter),
				iter_iov_len(&iter));
		if (err < 0)
			break;
		iov_iter_advance(&iter, iter_iov_len(&iter));
	}

release_mm:
	mmput(mm);
release_task:
	put_task_struct(task);
free_iov:
	kfree(iov);
out:
	if (arg == MADV_WRITEBACK)
		err = zram_writeback_list(zram, &list);
	*bypass = true;
	*ret = err < 0 ? err : (total_len - iov_iter_count(&iter));
}

static void zram_madvise_pageout_return_error(void *data,
		int ret, bool *return_error)
{
	if (ret == -EBUSY)
		*return_error = true;
}

static void zram_process_madvise_return_error(void *data,
		int behavior, int ret,  bool *return_error)
{
	if (behavior == MADV_PAGEOUT && ret < 0)
		*return_error = true;
}

static void zram_madvise_pageout_bypass(void *data,
		struct mm_struct *mm, bool pageout, int *ret)
{
	if (am_app_launch || (pageout && rwsem_is_contended(&mm->mmap_lock)))
		*ret = -EBUSY;
}

void init_zram_madvise(struct zram *zram)
{
	register_trace_android_vh_madvise_pageout_return_error(
			zram_madvise_pageout_return_error, NULL);
	register_trace_android_vh_process_madvise_return_error(
			zram_process_madvise_return_error, NULL);
	register_trace_android_vh_madvise_pageout_bypass(
			zram_madvise_pageout_bypass, NULL);
	register_trace_android_rvh_process_madvise_bypass(
			zram_process_madvise, zram);
}

void exit_zram_madvise(void)
{
	unregister_trace_android_vh_madvise_pageout_return_error(
			zram_madvise_pageout_return_error, NULL);
	unregister_trace_android_vh_process_madvise_return_error(
			zram_process_madvise_return_error, NULL);
	unregister_trace_android_vh_madvise_pageout_bypass(
			zram_madvise_pageout_bypass, NULL);
}
#endif
