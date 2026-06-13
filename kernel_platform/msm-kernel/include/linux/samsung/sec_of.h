// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#ifndef __SEC_OF_H__
#define __SEC_OF_H__

#include <linux/kernel.h>
#include <linux/of.h>

#ifdef CONFIG_OF
static inline int sec_of_parse_reg_prop(struct device_node *np,
		phys_addr_t *base, phys_addr_t *size)
{
	const __be32 *cell;
	int address_cells;
	int size_cells;

	cell = of_get_property(np, "reg", NULL);
	if (IS_ERR_OR_NULL(cell))
		return -EINVAL;

	/* NOTE: 'of_n_addr_cells' and 'of_n_size_cells' always
	 * search from the parent node except 'root'.
	 * To use '#address-cells' and '#size-cells' of the current node,
	 * these 'if' statements are rquired.
	 */
	if (of_property_read_u32(np, "#address-cells", &address_cells))
		address_cells = of_n_addr_cells(np);

	if (of_property_read_u32(np, "#size-cells", &size_cells))
		size_cells = of_n_size_cells(np);

	*base = (phys_addr_t)of_read_number(cell, address_cells);
	*size = (phys_addr_t)of_read_number(cell + address_cells, size_cells);

	return 0;
}

static inline int sec_of_test_debug_level(const struct device_node *np,
		const char *node_name, unsigned int sec_dbg_level)
{
	int nr_dbg_level;
	int i;

	nr_dbg_level = (int)of_property_count_u32_elems(np, node_name);
	if (nr_dbg_level <= 0)
		return -ENOENT;

	for (i = 0; i < nr_dbg_level; i++) {
		u32 dbg_level;

		of_property_read_u32_index(np, node_name, i, &dbg_level);
		if (sec_dbg_level == (unsigned int)dbg_level)
			return 0;
	}

	return -EINVAL;
}
#else
static inline int sec_of_parse_reg_prop(struct device_node *np, phys_addr_t *base, phys_addr_t *size) { return -ENODEV; }
static inline int sec_of_test_debug_level(const struct device_node *np, const char *node_name, unsigned int sec_dbg_level) { return -ENODEV; };
#endif

#endif /* __SEC_OF_H__ */
