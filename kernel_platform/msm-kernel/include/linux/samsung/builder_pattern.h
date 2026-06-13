// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2021 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#ifndef __BUILDER_PATTERN_H__
#define __BUILDER_PATTERN_H__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/of.h>

struct director_threaded;

/* An interface 'Builder' struct
 * TODO: This struct should be embedded if the drvdata use
 * this 'Builder Pattern'.
 */
struct builder {
	struct device *dev;
	struct director_threaded *drct;
};

#define DT_BUILDER(__parse_dt) \
	{ .parse_dt = __parse_dt, }

/* The prototype of 'Concrete Builde' parsing a device tree */
typedef int (*parse_dt_t)(struct builder *bd, struct device_node *np);

struct dt_builder {
	parse_dt_t parse_dt;
};

#define DEVICE_BUILDER(__construct_dev, __destruct_dev) \
	{ .construct_dev = __construct_dev, .destruct_dev = __destruct_dev, }

/* The prototype of 'Concrete Builde' constructing a device */
typedef int (*construct_dev_t)(struct builder *bd);

/* The prototype of 'Concrete Builde' destructing a device */
typedef void (*destruct_dev_t)(struct builder *bd);

struct dev_builder {
	construct_dev_t construct_dev;
	destruct_dev_t destruct_dev;
};

#ifdef _DEBUG_BUILDER_PATTERN
static inline int __bp_call_parse_dt(parse_dt_t parse_dt,
		struct builder *bd, struct device_node *np)
{
	ktime_t calltime, rettime;
	int err;

	calltime = ktime_get();
	err = parse_dt(bd, np);
	rettime = ktime_get();

	dev_info(bd->dev, "%ps returned %d after %llu\n",
			parse_dt, err,
			(unsigned long long)ktime_sub(rettime, calltime));

	return err;
}

static inline int __bp_call_construct_dev(construct_dev_t construct_dev,
		struct builder *bd)
{
	ktime_t calltime, rettime;
	int err;

	calltime = ktime_get();
	err = construct_dev(bd);
	rettime = ktime_get();

	dev_info(bd->dev, "%ps returned %d after %llu\n",
			construct_dev , err,
			(unsigned long long)ktime_sub(rettime, calltime));

	return err;
}

static inline void __bp_call_destruct_dev(destruct_dev_t destruct_dev,
		struct builder *bd)
{
	ktime_t calltime, rettime;

	calltime = ktime_get();
	destruct_dev(bd);
	rettime = ktime_get();

	dev_info(bd->dev, "%ps returned after %llu\n",
			destruct_dev,
			(unsigned long long)ktime_sub(rettime, calltime));
}
#else
static inline int __bp_call_parse_dt(parse_dt_t parse_dt,
		struct builder *bd, struct device_node *np)
{
	return parse_dt(bd, np);
}

static inline int __bp_call_construct_dev(construct_dev_t construct_dev,
		struct builder *bd)
{
	return construct_dev(bd);
}

static inline void __bp_call_destruct_dev(destruct_dev_t destruct_dev,
		struct builder *bd)
{
	destruct_dev(bd);
}
#endif

static inline int __bp_call_concrete_parse_dt(struct builder *bd,
		const struct dt_builder *builder, size_t i)
{
	struct device_node *np = dev_of_node(bd->dev);
	int err;
	parse_dt_t parse_dt = builder[i].parse_dt;

	err = __bp_call_parse_dt(parse_dt, bd, np);
	if (err)
		dev_err(bd->dev, "failed to parse a device tree - [%zu] %ps (%d)\n",
				i, parse_dt, err);

	return err;
}

static inline int __bp_call_concrete_construct_dev(struct builder *bd,
		const struct dev_builder *builder, size_t i)
{
	int err;
	construct_dev_t construct_dev = builder[i].construct_dev;

	if (!construct_dev)
		return 0;

	err = __bp_call_construct_dev(construct_dev, bd);
	if (err)
		dev_err(bd->dev, "failed to construct_dev a device - [%zu] %ps (%d)\n",
				i, construct_dev, err);

	return err;
}

static inline void __bp_call_concrete_destruct_dev(struct builder *bd,
		const struct dev_builder *builder, size_t i)
{
	destruct_dev_t destruct_dev = builder[i].destruct_dev;

	if (!destruct_dev)
		return;

	__bp_call_destruct_dev(destruct_dev, bd);
}

/* A common 'Director' parsing a device tree
 * @return - 0 on success. 'errno' of last failed on failure.
 */
static inline int sec_director_parse_dt(struct builder *bd,
		const struct dt_builder *builder, size_t n)
{
	int err;
	size_t i;

	for (i = 0; i < n; i++) {
		err = __bp_call_concrete_parse_dt(bd, builder, i);
		if (err)
			return err;
	}

	return 0;
}

/* A common 'Director' constructing a device
 * @last_failed - The number of called builders on success.
 *           The "NEGATIVE" index of last failed on failure.
 * @return - 0 on success.
 *           return value of the last concrete builder on failure.
 */
static inline int sec_director_construct_dev(struct builder *bd,
		const struct dev_builder *builder, ssize_t n,
		ssize_t *last_failed)
{
	int err;
	ssize_t i;

	for (i = 0; i < n; i++) {
		err = __bp_call_concrete_construct_dev(bd, builder, i);
		if (err) {
			*last_failed = -i;
			return err;
		}
	}

	*last_failed = n;

	return 0;
}

/* A common 'Director' destructing a device */
static inline void sec_director_destruct_dev(struct builder *bd,
		const struct dev_builder *builder, ssize_t n,
		ssize_t last_failed)
{
	ssize_t i;

	BUG_ON((last_failed > n) || (last_failed < 0));

	for (i = last_failed - 1; i >= 0; i--)
		__bp_call_concrete_destruct_dev(bd, builder, i);
}

/* A wrapper function for probe call-backs */
static inline int sec_director_probe_dev(struct builder *bd,
		const struct dev_builder *builder, ssize_t n)
{
	int err;
	ssize_t last_failed;

	err = sec_director_construct_dev(bd, builder, n, &last_failed);
	if (last_failed <= 0)
		goto err_dev_director;

	return 0;

err_dev_director:
	sec_director_destruct_dev(bd, builder, n, -last_failed);
	return err;
}

struct director_threaded {
	struct builder *bd;
	const struct dev_builder *builder;
	ssize_t n;
	int *construct_result;
};

/* A common 'Director' constructing a device - threaded */
static int sec_director_construct_dev_threaded(void *__drct)
{
	struct director_threaded *drct = __drct;
	struct builder *bd = drct->bd;
	const struct dev_builder *builder = drct->builder;
	ssize_t n = drct->n;
	int *construct_result = drct->construct_result;
	ssize_t i;

	for (i = 0; i < n; i++)
		construct_result[i] =
			__bp_call_concrete_construct_dev(bd, builder, i);

	return 0;
}

/* A common 'Director' destructing a device */
static inline void sec_director_destruct_dev_threaded(
		struct director_threaded *drct)
{
	struct builder *bd = drct->bd;
	const struct dev_builder *builder = drct->builder;
	ssize_t n = drct->n;
	int *construct_result = drct->construct_result;
	ssize_t i;

	for (i = n - 1; i >= 0; i--) {
		if (!construct_result[i])
			__bp_call_concrete_destruct_dev(bd, builder, i);
	}
}

/* A wrapper function for probe call-backs - threaded */
static inline int sec_director_probe_dev_threaded(struct builder *bd,
		const struct dev_builder *builder, ssize_t n, const char *name)
{
	struct device *dev = bd->dev;
	struct director_threaded *drct;
	int *construct_result;
	struct task_struct *thread;

	drct = devm_kzalloc(dev, sizeof(*drct), GFP_KERNEL);
	if (!drct)
		return -ENOMEM;

	construct_result = devm_kcalloc(dev, n, sizeof(*construct_result),
			GFP_KERNEL);
	if (!construct_result)
		return -ENOMEM;

	drct->bd = bd;
	drct->builder = builder;
	drct->n = n;
	drct->construct_result = construct_result;

	thread = kthread_run(sec_director_construct_dev_threaded,
			drct, "drct-%s", name);
	if (IS_ERR_OR_NULL(thread)) {
		dev_err(dev, "failed to created drct thread - (%ld)!\n",
				PTR_ERR(thread));
		return -ENOMEM;
	}

	bd->drct = drct;

	return 0;
}

#endif /* __BUILDER_PATTERN_H__ */

