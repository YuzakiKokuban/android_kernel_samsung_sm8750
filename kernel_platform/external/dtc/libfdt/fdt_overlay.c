// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2016 Free Electrons
 * Copyright (C) 2016 NextThing Co.
 */
#include "libfdt_env.h"

#include <fdt.h>
#include <libfdt.h>
#include <stdio.h>

#include "libfdt_internal.h"

/**
 * overlay_get_target_phandle - retrieves the target phandle of a fragment
 * @fdto: pointer to the device tree overlay blob
 * @fragment: node offset of the fragment in the overlay
 *
 * overlay_get_target_phandle() retrieves the target phandle of an
 * overlay fragment when that fragment uses a phandle (target
 * property) instead of a path (target-path property).
 *
 * returns:
 *      the phandle pointed by the target property
 *      0, if the phandle was not found
 *	-1, if the phandle was malformed
 */
static uint32_t overlay_get_target_phandle(const void *fdto, int fragment)
{
	const fdt32_t *val;
	int len;

	val = fdt_getprop(fdto, fragment, "target", &len);
	if (!val)
		return 0;

	if ((len != sizeof(*val)) || (fdt32_to_cpu(*val) == (uint32_t)-1))
		return (uint32_t)-1;

	return fdt32_to_cpu(*val);
}

/**
 * overlay_get_target - retrieves the offset of a fragment's target
 * @fdt: Base device tree blob
 * @fdto: Device tree overlay blob
 * @fragment: node offset of the fragment in the overlay
 * @pathp: pointer which receives the path of the target (or NULL)
 *
 * overlay_get_target() retrieves the target offset in the base
 * device tree of a fragment, no matter how the actual targeting is
 * done (through a phandle or a path)
 *
 * returns:
 *      the targeted node offset in the base device tree
 *      Negative error code on error
 */
static int overlay_get_target(const void *fdt, const void *fdto,
			      int fragment, char const **pathp)
{
	uint32_t phandle;
	const char *path = NULL;
	int path_len = 0, ret;

	/* Try first to do a phandle based lookup */
	phandle = overlay_get_target_phandle(fdto, fragment);
	if (phandle == (uint32_t)-1)
		return -FDT_ERR_BADPHANDLE;

	/* no phandle, try path */
	if (!phandle) {
		/* And then a path based lookup */
		path = fdt_getprop(fdto, fragment, "target-path", &path_len);
		if (path)
			ret = fdt_path_offset(fdt, path);
		else
			ret = path_len;
	} else
		ret = fdt_node_offset_by_phandle(fdt, phandle);

	/*
	* If we haven't found either a target or a
	* target-path property in a node that contains a
	* __overlay__ subnode (we wouldn't be called
	* otherwise), consider it a improperly written
	* overlay
	*/
	if (ret < 0 && path_len == -FDT_ERR_NOTFOUND)
		ret = -FDT_ERR_BADOVERLAY;

	/* return on error */
	if (ret < 0)
		return ret;

	/* return pointer to path (if available) */
	if (pathp)
		*pathp = path ? path : NULL;

	return ret;
}

/**
 * overlay_phandle_add_offset - Increases a phandle by an offset
 * @fdt: Base device tree blob
 * @node: Device tree overlay blob
 * @name: Name of the property to modify (phandle or linux,phandle)
 * @delta: offset to apply
 *
 * overlay_phandle_add_offset() increments a node phandle by a given
 * offset.
 *
 * returns:
 *      0 on success.
 *      Negative error code on error
 */
static int overlay_phandle_add_offset(void *fdt, int node,
				      const char *name, uint32_t delta)
{
	const fdt32_t *val;
	uint32_t adj_val;
	int len;

	val = fdt_getprop(fdt, node, name, &len);
	if (!val)
		return len;

	if (len != sizeof(*val))
		return -FDT_ERR_BADPHANDLE;

	adj_val = fdt32_to_cpu(*val);
	if ((adj_val + delta) < adj_val)
		return -FDT_ERR_NOPHANDLES;

	adj_val += delta;
	if (adj_val == (uint32_t)-1)
		return -FDT_ERR_NOPHANDLES;

	return fdt_setprop_inplace_u32(fdt, node, name, adj_val);
}

/**
 * overlay_adjust_node_phandles - Offsets the phandles of a node
 * @fdto: Device tree overlay blob
 * @node: Offset of the node we want to adjust
 * @delta: Offset to shift the phandles of
 *
 * overlay_adjust_node_phandles() adds a constant to all the phandles
 * of a given node. This is mainly use as part of the overlay
 * application process, when we want to update all the overlay
 * phandles to not conflict with the overlays of the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_adjust_node_phandles(void *fdto, int node,
					uint32_t delta)
{
	int child;
	int ret;

	ret = overlay_phandle_add_offset(fdto, node, "phandle", delta);
	if (ret && ret != -FDT_ERR_NOTFOUND)
		return ret;

	ret = overlay_phandle_add_offset(fdto, node, "linux,phandle", delta);
	if (ret && ret != -FDT_ERR_NOTFOUND)
		return ret;

	fdt_for_each_subnode(child, fdto, node) {
		ret = overlay_adjust_node_phandles(fdto, child, delta);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_adjust_local_phandles - Adjust the phandles of a whole overlay
 * @fdto: Device tree overlay blob
 * @delta: Offset to shift the phandles of
 *
 * overlay_adjust_local_phandles() adds a constant to all the
 * phandles of an overlay. This is mainly use as part of the overlay
 * application process, when we want to update all the overlay
 * phandles to not conflict with the overlays of the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_adjust_local_phandles(void *fdto, uint32_t delta)
{
	/*
	 * Start adjusting the phandles from the overlay root
	 */
	return overlay_adjust_node_phandles(fdto, 0, delta);
}

/**
 * overlay_update_local_node_references - Adjust the overlay references
 * @fdto: Device tree overlay blob
 * @tree_node: Node offset of the node to operate on
 * @fixup_node: Node offset of the matching local fixups node
 * @delta: Offset to shift the phandles of
 *
 * overlay_update_local_nodes_references() update the phandles
 * pointing to a node within the device tree overlay by adding a
 * constant delta.
 *
 * This is mainly used as part of a device tree application process,
 * where you want the device tree overlays phandles to not conflict
 * with the ones from the base device tree before merging them.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_update_local_node_references(void *fdto,
						int tree_node,
						int fixup_node,
						uint32_t delta)
{
	int fixup_prop;
	int fixup_child;
	int ret;

	fdt_for_each_property_offset(fixup_prop, fdto, fixup_node) {
		const fdt32_t *fixup_val;
		const char *tree_val;
		const char *name;
		int fixup_len;
		int tree_len;
		int i;

		fixup_val = fdt_getprop_by_offset(fdto, fixup_prop,
						  &name, &fixup_len);
		if (!fixup_val)
			return fixup_len;

		if (fixup_len % sizeof(uint32_t))
			return -FDT_ERR_BADOVERLAY;

		tree_val = fdt_getprop(fdto, tree_node, name, &tree_len);
		if (!tree_val) {
			if (tree_len == -FDT_ERR_NOTFOUND)
				return -FDT_ERR_BADOVERLAY;

			return tree_len;
		}

		for (i = 0; i < (fixup_len / sizeof(uint32_t)); i++) {
			fdt32_t adj_val;
			uint32_t poffset;

			poffset = fdt32_to_cpu(fixup_val[i]);

			/*
			 * phandles to fixup can be unaligned.
			 *
			 * Use a memcpy for the architectures that do
			 * not support unaligned accesses.
			 */
			memcpy(&adj_val, tree_val + poffset, sizeof(adj_val));

			adj_val = cpu_to_fdt32(fdt32_to_cpu(adj_val) + delta);

			ret = fdt_setprop_inplace_namelen_partial(fdto,
								  tree_node,
								  name,
								  strlen(name),
								  poffset,
								  &adj_val,
								  sizeof(adj_val));
			if (ret == -FDT_ERR_NOSPACE)
				return -FDT_ERR_BADOVERLAY;

			if (ret)
				return ret;
		}
	}

	fdt_for_each_subnode(fixup_child, fdto, fixup_node) {
		const char *fixup_child_name = fdt_get_name(fdto, fixup_child,
							    NULL);
		int tree_child;

		tree_child = fdt_subnode_offset(fdto, tree_node,
						fixup_child_name);
		if (tree_child == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_BADOVERLAY;
		if (tree_child < 0)
			return tree_child;

		ret = overlay_update_local_node_references(fdto,
							   tree_child,
							   fixup_child,
							   delta);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_update_local_references - Adjust the overlay references
 * @fdto: Device tree overlay blob
 * @delta: Offset to shift the phandles of
 *
 * overlay_update_local_references() update all the phandles pointing
 * to a node within the device tree overlay by adding a constant
 * delta to not conflict with the base overlay.
 *
 * This is mainly used as part of a device tree application process,
 * where you want the device tree overlays phandles to not conflict
 * with the ones from the base device tree before merging them.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_update_local_references(void *fdto, uint32_t delta)
{
	int fixups;

	fixups = fdt_path_offset(fdto, "/__local_fixups__");
	if (fixups < 0) {
		/* There's no local phandles to adjust, bail out */
		if (fixups == -FDT_ERR_NOTFOUND)
			return 0;

		return fixups;
	}

	/*
	 * Update our local references from the root of the tree
	 */
	return overlay_update_local_node_references(fdto, 0, fixups,
						    delta);
}

/**
 * overlay_fixup_one_phandle - Set an overlay phandle to the base one
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @symbols_off: Node offset of the symbols node in the base device tree
 * @path: Path to a node holding a phandle in the overlay
 * @path_len: number of path characters to consider
 * @name: Name of the property holding the phandle reference in the overlay
 * @name_len: number of name characters to consider
 * @poffset: Offset within the overlay property where the phandle is stored
 * @label: Label of the node referenced by the phandle
 *
 * overlay_fixup_one_phandle() resolves an overlay phandle pointing to
 * a node in the base device tree.
 *
 * This is part of the device tree overlay application process, when
 * you want all the phandles in the overlay to point to the actual
 * base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_one_phandle(void *fdt, void *fdto,
				     int symbols_off,
				     const char *path, uint32_t path_len,
				     const char *name, uint32_t name_len,
				     int poffset, const char *label)
{
	const char *symbol_path;
	uint32_t phandle;
	fdt32_t phandle_prop;
	int symbol_off, fixup_off;
	int prop_len;

	if (symbols_off < 0)
		return symbols_off;

	symbol_path = fdt_getprop(fdt, symbols_off, label,
				  &prop_len);
	if (!symbol_path)
		return prop_len;

	symbol_off = fdt_path_offset(fdt, symbol_path);
	if (symbol_off < 0)
		return symbol_off;

	phandle = fdt_get_phandle(fdt, symbol_off);
	if (!phandle)
		return -FDT_ERR_NOTFOUND;

	fixup_off = fdt_path_offset_namelen(fdto, path, path_len);
	if (fixup_off == -FDT_ERR_NOTFOUND)
		return -FDT_ERR_BADOVERLAY;
	if (fixup_off < 0)
		return fixup_off;

	phandle_prop = cpu_to_fdt32(phandle);
	return fdt_setprop_inplace_namelen_partial(fdto, fixup_off,
						   name, name_len, poffset,
						   &phandle_prop,
						   sizeof(phandle_prop));
};

static int overlay_add_to_local_fixups(void *fdt, const char *value, int len)
{
	const char *path, *fixup_end, *prop, *fixup_str;
	uint32_t clen;
	uint32_t fixup_len;
	char *sep, *endptr;
	const char *c;
	int poffset, nodeoffset, ret, localfixup_off;
	int pathlen, proplen;
	char propname[PATH_MAX];

	localfixup_off = fdt_path_offset(fdt, "/__local_fixups__");
	if (localfixup_off < 0 && localfixup_off == -FDT_ERR_NOTFOUND)
		localfixup_off = fdt_add_subnode(fdt, 0, "__local_fixups__");

	if (localfixup_off < 0)
		return localfixup_off;

	while (len > 0) {
		fixup_str = value;

		/* Assumes null-terminated properties! */
		fixup_end = memchr(value, '\0', len);
		if (!fixup_end)
			return -FDT_ERR_BADOVERLAY;

		fixup_len = fixup_end - fixup_str;

		len -= (fixup_len + 1);
		value += fixup_len + 1;

		c = path = fixup_str;
		sep = memchr(c, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;
		pathlen = sep - path;
		if (pathlen == (fixup_len - 1))
			return -FDT_ERR_BADOVERLAY;

		fixup_len -= (pathlen + 1);
		c = path + pathlen + 1;

		sep = memchr(c, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;

		prop = c;
		proplen = sep - c;

		if (proplen >= PATH_MAX)
			return -FDT_ERR_BADOVERLAY;

		/*
		 * Skip fixups that involves the special 'target' property found
		 * in overlay fragments such as
		 *	/fragment@0:target:0
		 *
		 * The check for one node in path below is to ensure that we
		 * handle 'target' properties present otherwise in any other
		 * node, for ex:
		 *	/fragment@0/__overlay__/xyz:target:0
		 */

		/* Does path have exactly one node? */
		c = path;
		clen = pathlen;
		if (*c == '/') {
			c++;
			clen -= 1;
		}

		sep = memchr(c, '/', clen);
		if (!sep && proplen >= 6 && !strncmp(prop, "target", 6))
			continue;

		memcpy(propname, prop, proplen);
		propname[proplen] = 0;

		fixup_len -= (proplen + 1);
		c = prop + proplen + 1;
		poffset = strtoul(c, &endptr, 10);

		nodeoffset = localfixup_off;

		c = path;
		clen = pathlen;

		if (*c == '/') {
			c++;
			clen -= 1;
		}

		while (clen > 0) {
			char nodename[PATH_MAX];
			int nodelen, childnode;

			sep = memchr(c, '/', clen);
			if (!sep)
				nodelen = clen;
			else
				nodelen = sep - c;

			if (nodelen + 1 >= PATH_MAX)
				return -FDT_ERR_BADSTRUCTURE;
			memcpy(nodename, c, nodelen);
			nodename[nodelen] = 0;

			childnode = fdt_add_subnode(fdt, nodeoffset, nodename);
			if (childnode == -FDT_ERR_EXISTS)
				childnode = fdt_subnode_offset(fdt,
							nodeoffset, nodename);
			nodeoffset = childnode;
			if (nodeoffset < 0)
				return nodeoffset;

			c += nodelen;
			clen -= nodelen;

			if (*c == '/') {
				c++;
				clen -= 1;
			}
		}

		ret = fdt_appendprop_u32(fdt, nodeoffset, propname, poffset);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/**
 * overlay_fixup_phandle - Set an overlay phandle to the base one
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @symbols_off: Node offset of the symbols node in the base device tree
 * @property: Property offset in the overlay holding the list of fixups
 * @fixups_off: Offset of __fixups__ node in @fdto
 * @merge: Both input blobs are overlay blobs that are being merged
 *
 * overlay_fixup_phandle() resolves all the overlay phandles pointed
 * to in a __fixups__ property, and updates them to match the phandles
 * in use in the base device tree.
 *
 * This is part of the device tree overlay application process, when
 * you want all the phandles in the overlay to point to the actual
 * base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_phandle(void *fdt, void *fdto, int symbols_off,
				int property, int fixups_off, int merge)
{
	const char *value, *total_value;
	const char *label;
	int len, total_len, ret = 0;

	total_value = value = fdt_getprop_by_offset(fdto, property,
				      &label, &len);
	if (!value) {
		if (len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_INTERNAL;

		return len;
	}

	total_len = len;

	do {
		const char *path, *name, *fixup_end;
		const char *fixup_str = value;
		uint32_t path_len, name_len;
		uint32_t fixup_len;
		char *sep, *endptr;
		int poffset;

		fixup_end = memchr(value, '\0', len);
		if (!fixup_end)
			return -FDT_ERR_BADOVERLAY;
		fixup_len = fixup_end - fixup_str;

		len -= fixup_len + 1;
		value += fixup_len + 1;

		path = fixup_str;
		sep = memchr(fixup_str, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;

		path_len = sep - path;
		if (path_len == (fixup_len - 1))
			return -FDT_ERR_BADOVERLAY;

		fixup_len -= path_len + 1;
		name = sep + 1;
		sep = memchr(name, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;

		name_len = sep - name;
		if (!name_len)
			return -FDT_ERR_BADOVERLAY;

		poffset = strtoul(sep + 1, &endptr, 10);
		if ((*endptr != '\0') || (endptr <= (sep + 1)))
			return -FDT_ERR_BADOVERLAY;

		ret = overlay_fixup_one_phandle(fdt, fdto, symbols_off,
						path, path_len, name, name_len,
						poffset, label);
		if (ret)
			return ret;
	} while (len > 0);

	/*
	 * Properties found in __fixups__ node are typically one of
	 * these types:
	 *
	 * 	abc = "/fragment@2:target:0"		(first type)
	 *	abc = "/fragment@2/__overlay__:xyz:0"	(second type)
	 *
	 * Both types could also be present in some properties as well such as:
	 *
	 *	abc = "/fragment@2:target:0", "/fragment@2/__overlay__:xyz:0"
	 *
	 * While merging two overlay blobs, a successfull overlay phandle fixup
	 * of second type needs to be recorded in __local_fixups__ node of the
	 * combined blob, so that the phandle value can be further updated via
	 * overlay_update_local_references() when the combined overlay blob gets
	 * overlaid on a different base blob.
	 *
	 * Further, since in the case of merging two overlay blobs, we will also
	 * be merging contents of nodes such as __fixups__ from both overlay
	 * blobs, delete this property in __fixup__  node, as it no longer
	 * represents a external phandle reference that needs to be resolved
	 * during a subsequent overlay of combined blob on a base blob.
	 */
	if (merge) {
		ret = overlay_add_to_local_fixups(fdt, total_value, total_len);
		if (!ret)
			ret = fdt_delprop(fdto, fixups_off, label);
	}

	return ret;
}

/**
 * overlay_fixup_phandles - Resolve the overlay phandles to the base
 *                          device tree
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @merge: Both input blobs are overlay blobs that are being merged
 *
 * overlay_fixup_phandles() resolves all the overlay phandles pointing
 * to nodes in the base device tree.
 *
 * This is one of the steps of the device tree overlay application
 * process, when you want all the phandles in the overlay to point to
 * the actual base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_phandles(void *fdt, void *fdto, int merge)
{
	int fixups_off, symbols_off;
	int property, ret = 0, next_property;

	/* We can have overlays without any fixups */
	fixups_off = fdt_path_offset(fdto, "/__fixups__");
	if (fixups_off == -FDT_ERR_NOTFOUND)
		return 0; /* nothing to do */
	if (fixups_off < 0)
		return fixups_off;

	/* And base DTs without symbols */
	symbols_off = fdt_path_offset(fdt, "/__symbols__");
	if ((symbols_off < 0 && (symbols_off != -FDT_ERR_NOTFOUND)))
		return symbols_off;

	/* Safeguard against property being deleted in below loop */
	property = fdt_first_property_offset(fdto, fixups_off);
	while (property >= 0) {
		next_property = fdt_next_property_offset(fdto, property);
		ret = overlay_fixup_phandle(fdt, fdto, symbols_off,
						property, fixups_off, merge);
		if (ret && (!merge || ret != -FDT_ERR_NOTFOUND))
			return ret;

		if (merge && !ret) {
			/* Bail if this was the last property */
			if (next_property < 0)
				break;

			/*
			 * Property is deleted in this case. Next property is
			 * available at the same offset, so loop back with
			 * 'property' offset unmodified. Also since @fdt would
			 * have been modified in this case, refresh the offset
			 * of /__symbols__ node
			 */
			symbols_off = fdt_path_offset(fdt, "/__symbols__");
			if (symbols_off < 0)
				return symbols_off;

			continue;
		}

		property = next_property;
	}

	if (merge && ret == -FDT_ERR_NOTFOUND)
		ret = 0;

	return ret;
}

/**
 * overlay_apply_node - Merges a node into the base device tree
 * @fdt: Base Device Tree blob
 * @target: Node offset in the base device tree to apply the fragment to
 * @fdto: Device tree overlay blob
 * @node: Node offset in the overlay holding the changes to merge
 *
 * overlay_apply_node() merges a node into a target base device tree
 * node pointed.
 *
 * This is part of the final step in the device tree overlay
 * application process, when all the phandles have been adjusted and
 * resolved and you just have to merge overlay into the base device
 * tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_apply_node(void *fdt, int target,
			      void *fdto, int node)
{
	int property;
	int subnode;

	fdt_for_each_property_offset(property, fdto, node) {
		const char *name;
		const void *prop;
		int prop_len;
		int ret;

		prop = fdt_getprop_by_offset(fdto, property, &name,
					     &prop_len);
		if (prop_len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_INTERNAL;
		if (prop_len < 0)
			return prop_len;

		ret = fdt_setprop(fdt, target, name, prop, prop_len);
		if (ret)
			return ret;
	}

	fdt_for_each_subnode(subnode, fdto, node) {
		const char *name = fdt_get_name(fdto, subnode, NULL);
		int nnode;
		int ret;

		nnode = fdt_add_subnode(fdt, target, name);
		if (nnode == -FDT_ERR_EXISTS) {
			nnode = fdt_subnode_offset(fdt, target, name);
			if (nnode == -FDT_ERR_NOTFOUND)
				return -FDT_ERR_INTERNAL;
		}

		if (nnode < 0)
			return nnode;

		ret = overlay_apply_node(fdt, nnode, fdto, subnode);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * copy_node - copy a node hierarchically
 * @fdt - pointer to base device tree
 * @fdto - pointer to overlay device tree
 * @fdt_child - offset of node in overlay device tree which needs to be copied
 * @fdt_parent - offset of parent node in base tree under which @fdto_child
 *		need to be copied
 *
 * This function copies a node in overlay tree along with its child-nodes and
 * their properties, under a given parent node in base tree.
 */
static int copy_node(void *fdt, void *fdto, int fdt_parent, int fdto_child)
{
	const char *name, *value;
	int offset, len, ret, prop, child;
	void *p;

	name = fdt_get_name(fdto, fdto_child, &len);
	if (!name)
		return len;

	offset = fdt_subnode_offset(fdt, fdt_parent, name);
	if (offset < 0) {
		offset = fdt_add_subnode(fdt, fdt_parent, name);
		if (offset < 0)
			return offset;
	}

	fdt_for_each_subnode(child, fdto, fdto_child) {
		ret = copy_node(fdt, fdto, offset, child);
		if (ret < 0)
			return ret;
	}

	fdt_for_each_property_offset(prop, fdto, fdto_child) {
		int fdt_len = 0;

		value = fdt_getprop_by_offset(fdto, prop,
						  &name, &len);

		if (fdt_getprop(fdt, offset, name, &fdt_len))
			len += fdt_len;

		ret = fdt_setprop_placeholder(fdt, offset, name,
								len, &p);
		if (ret < 0)
			return ret;

		if (fdt_len > 0) {
			p = (char *)p + fdt_len;
			len -= fdt_len;
		}

		memcpy(p, value, len);
	}

	return 0;
}

/**
 * overlay_merge - Merge an overlay into its base device tree
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @merge: Both input blobs are overlay blobs that are being merged
 *
 * overlay_merge() merges an overlay into its base device tree.
 *
 * This is the next to last step in the device tree overlay application
 * process, when all the phandles have been adjusted and resolved and
 * you just have to merge overlay into the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_merge(void *fdt, void *fdto, int merge)
{
	int fragment;

	fdt_for_each_subnode(fragment, fdto, 0) {
		int overlay;
		int target;
		int ret;

		/*
		 * Each fragments will have an __overlay__ node. If
		 * they don't, it's not supposed to be merged
		 */
		overlay = fdt_subnode_offset(fdto, fragment, "__overlay__");
		if (overlay == -FDT_ERR_NOTFOUND)
			continue;

		if (overlay < 0)
			return overlay;

		target = overlay_get_target(fdt, fdto, fragment, NULL);
		if (target < 0) {
			/*
			 * No target found which is acceptable situation in case
			 * of merging two overlay blobs. Copy this fragment to
			 * base/combined blob, so that it can be considered for
			 * overlay during a subsequent overlay operation of
			 * combined blob on another base blob.
			 */
			if (target == -FDT_ERR_BADPHANDLE && merge) {
				target = copy_node(fdt, fdto, 0, fragment);
				if (!target)
					continue;
			}
			return target;
		}

		ret = overlay_apply_node(fdt, target, fdto, overlay);
		if (ret)
			return ret;
	}

	return 0;
}

static int get_path_len(const void *fdt, int nodeoffset)
{
	int len = 0, namelen;
	const char *name;

	FDT_RO_PROBE(fdt);

	for (;;) {
		name = fdt_get_name(fdt, nodeoffset, &namelen);
		if (!name)
			return namelen;

		/* root? we're done */
		if (namelen == 0)
			break;

		nodeoffset = fdt_parent_offset(fdt, nodeoffset);
		if (nodeoffset < 0)
			return nodeoffset;
		len += namelen + 1;
	}

	/* in case of root pretend it's "/" */
	if (len == 0)
		len++;
	return len;
}

/**
 * overlay_symbol_update - Update the symbols of base tree after a merge
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @merge: Both input blobs are overlay blobs that are being merged
 *
 * overlay_symbol_update() updates the symbols of the base tree with the
 * symbols of the applied overlay
 *
 * This is the last step in the device tree overlay application
 * process, allowing the reference of overlay symbols by subsequent
 * overlay operations.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_symbol_update(void *fdt, void *fdto, int merge)
{
	int root_sym, ov_sym, prop, next_prop, path_len, fragment, target;
	int len, frag_name_len, ret, rel_path_len;
	const char *s, *e;
	const char *path;
	const char *name;
	const char *frag_name;
	const char *rel_path;
	const char *target_path;
	char *buf;
	void *p;

	ov_sym = fdt_subnode_offset(fdto, 0, "__symbols__");

	/* if no overlay symbols exist no problem */
	if (ov_sym < 0)
		return 0;

	root_sym = fdt_subnode_offset(fdt, 0, "__symbols__");

	/* it no root symbols exist we should create them */
	if (root_sym == -FDT_ERR_NOTFOUND)
		root_sym = fdt_add_subnode(fdt, 0, "__symbols__");

	/* any error is fatal now */
	if (root_sym < 0)
		return root_sym;

	/* iterate over each overlay symbol */

	/* Safeguard against property being possibly deleted in this loop */
	prop = fdt_first_property_offset(fdto, ov_sym);
	while (prop >= 0) {
		next_prop = fdt_next_property_offset(fdto, prop);

		path = fdt_getprop_by_offset(fdto, prop, &name, &path_len);
		if (!path)
			return path_len;

		/* verify it's a string property (terminated by a single \0) */
		if (path_len < 1 || memchr(path, '\0', path_len) != &path[path_len - 1])
			return -FDT_ERR_BADVALUE;

		/* keep end marker to avoid strlen() */
		e = path + path_len;

		if (*path != '/')
			return -FDT_ERR_BADVALUE;

		/* get fragment name first */
		s = strchr(path + 1, '/');
		if (!s) {
			/* Symbol refers to something that won't end
			 * up in the target tree */
			continue;
		}

		frag_name = path + 1;
		frag_name_len = s - path - 1;

		/* verify format; safe since "s" lies in \0 terminated prop */
		len = sizeof("/__overlay__/") - 1;
		if ((e - s) > len && (memcmp(s, "/__overlay__/", len) == 0)) {
			/* /<fragment-name>/__overlay__/<relative-subnode-path> */
			rel_path = s + len;
			rel_path_len = e - rel_path - 1;
		} else if ((e - s) == len
			   && (memcmp(s, "/__overlay__", len - 1) == 0)) {
			/* /<fragment-name>/__overlay__ */
			rel_path = "";
			rel_path_len = 0;
		} else {
			/* Symbol refers to something that won't end
			 * up in the target tree */
			continue;
		}

		/* find the fragment index in which the symbol lies */
		ret = fdt_subnode_offset_namelen(fdto, 0, frag_name,
					       frag_name_len);
		/* not found? */
		if (ret < 0)
			return -FDT_ERR_BADOVERLAY;
		fragment = ret;

		/* an __overlay__ subnode must exist */
		ret = fdt_subnode_offset(fdto, fragment, "__overlay__");
		if (ret < 0)
			return -FDT_ERR_BADOVERLAY;

		/* get the target of the fragment */
		ret = overlay_get_target(fdt, fdto, fragment, &target_path);
		if (ret < 0) {
			if (ret == -FDT_ERR_BADPHANDLE && merge) {
				prop = next_prop;
				continue;
			}

			return ret;
		}
		target = ret;

		/* if we have a target path use */
		if (!target_path) {
			ret = get_path_len(fdt, target);
			if (ret < 0)
				return ret;
			len = ret;
		} else {
			len = strlen(target_path);
		}

		ret = fdt_setprop_placeholder(fdt, root_sym, name,
				len + (len > 1) + rel_path_len + 1, &p);
		if (ret < 0)
			return ret;

		if (!target_path) {
			/* again in case setprop_placeholder changed it */
			ret = overlay_get_target(fdt, fdto, fragment, &target_path);
			if (ret < 0)
				return ret;
			target = ret;
		}

		buf = p;
		if (len > 1) { /* target is not root */
			if (!target_path) {
				ret = fdt_get_path(fdt, target, buf, len + 1);
				if (ret < 0)
					return ret;
			} else
				memcpy(buf, target_path, len + 1);

		} else
			len--;

		buf[len] = '/';
		memcpy(buf + len + 1, rel_path, rel_path_len);
		buf[len + 1 + rel_path_len] = '\0';

		/*
		 * In case of merging two overlay blobs, we will be merging
		 * contents of nodes such as __symbols__ from both overlay
		 * blobs. Delete this property in __symbols__ node of second
		 * overlay blob, as it has already been reflected in
		 * first/combined blob's __symbols__ node.
		 */
		if (merge) {
			ret = fdt_delprop(fdto, ov_sym, name);
			if (ret < 0)
				return ret;

			/* Bail if this was the last property */
			if (next_prop < 0)
				break;

			/*
			 * Continue with same 'prop' offset, as the next
			 * property is now available at the same offset
			 */
			continue;
		}

		prop = next_prop;
	}

	return 0;
}

int fdt_overlay_apply(void *fdt, void *fdto)
{
	uint32_t delta;
	int ret;

	FDT_RO_PROBE(fdt);
	FDT_RO_PROBE(fdto);

	ret = fdt_find_max_phandle(fdt, &delta);
	if (ret)
		goto err;

	ret = overlay_adjust_local_phandles(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_update_local_references(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_fixup_phandles(fdt, fdto, 0);
	if (ret)
		goto err;

	ret = overlay_merge(fdt, fdto, 0);
	if (ret)
		goto err;

	ret = overlay_symbol_update(fdt, fdto, 0);
	if (ret)
		goto err;

	/*
	 * The overlay has been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	return 0;

err:
	/*
	 * The overlay might have been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	/*
	 * The base device tree might have been damaged, erase its
	 * magic.
	 */
	fdt_set_magic(fdt, ~0);

	return ret;
}

/*
 * Property value could be in this format
 *	fragment@M ...fragment@N....fragment@O..
 *
 * This needs to be converted to
 *	fragment@M+delta...fragment@N+delta....fragment@O+delta
 */
static int rename_fragments_in_property(void *fdto, int offset,
	int property, int delta)
{
	char *start, *sep, *end, *stop, *value;
	int needed = 0, ret, len, found = 0, available, diff;
	unsigned long index, new_index;
	void *p = NULL;
	const char *label;

	value = (char *)(uintptr_t)fdt_getprop_by_offset(fdto, property,
				      &label, &len);
	if (!value)
		return len;

	start = value;
	end = value + len;

	/* Find the required additional space */
	while (start < end) {
		sep = memchr(start, '@', (end - start));
		if (!sep) {
			needed += end - start;
			break;
		}

		/* Check if string "fragment" exists */
		sep -= 8;

		if (sep < start || strncmp(sep, "fragment", 8)) {
			/* Start scan again after '@' */
			sep = sep + 9;
			needed += (sep - start);
			start = sep;
			continue;
		}

		found = 1;
		sep += 9;
		needed += (sep - start);
		index = strtoul(sep, &stop, 10);
		if (ULONG_MAX - index < delta)
			return -FDT_ERR_BADVALUE;

		new_index = index + delta;
		needed += snprintf(NULL, 0, "%lu", new_index);
		start = stop;
	}

	if (!found)
		return 0;

	p = value;
	if (needed > len) {
		ret = fdt_setprop_placeholder(fdto, offset, label, needed, &p);
		if (ret < 0)
			return ret;
	}

	start = p;
	end = start + len;
	ret = 0;

	while (start < end) {
		sep = memchr(start, '@', (end - start));
		if (!sep)
			break;

		/* Check if string "fragment" exists */
		sep -= 8;
		if (sep < start || strncmp(sep, "fragment", 8)) {
			/* Start scan again after '@' */
			start = sep + 9;
			continue;
		}

		sep += 9;
		index = strtoul(sep, &stop, 10);
		new_index = index + delta;

		needed = snprintf(NULL, 0, "%lu", new_index);
		available = stop - sep;

		if (available < needed) {
			diff = needed - available;
			memmove(stop + diff, stop, (end - stop));
			end += diff;
		}

		{
			/* +1 for NULL char */
			char buf[needed + 1];

			snprintf(buf, needed + 1, "%lu", new_index);
			memcpy(sep, buf, needed);
		}

		start = sep + needed;
	}

	return 0;
}

/**
 * rename_fragments_in_node - Rename fragment@xyz instances in a node's
 * properties
 *
 * @fdto    - pointer to a device-tree blob
 * @nodename - Node in whose properties fragments need to be renamed
 * @delta   - Increment to be applied to fragment index
 */
static int rename_fragments_in_node(void *fdto, const char *nodename,
				unsigned long delta)
{
	int offset, property;
	int ret;

	offset = fdt_path_offset(fdto, nodename);
	if (offset < 0)
		return offset;

	fdt_for_each_property_offset(property, fdto, offset) {
		ret = rename_fragments_in_property(fdto, offset,
						property, delta);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/**
 * rename_nodes - Rename all fragement@xyz nodes
 *
 * @fdto - pointer to device-tree blob
 * @parent_node - node offset of parent whose child fragment nodes need to be
 *		renamed
 * @delta - increment to be added to fragment number
 */
static int rename_nodes(void *fdto, int parent_node, unsigned long delta)
{
	int offset = -1, ret, len, strsize;
	int child_len, child_offset;
	const char *name, *child_name, *idx;
	char *stop = NULL;
	unsigned long index, new_index;

	offset = fdt_first_subnode(fdto, parent_node);
	while (offset >= 0) {
		name = fdt_get_name(fdto, offset, &len);
		if (!name)
			return len;

		if (len < 9 || strncmp(name, "fragment@", 9))
			goto next_node;

		child_offset = fdt_first_subnode(fdto, offset);
		if (child_offset < 0)
			return child_offset;

		child_name = fdt_get_name(fdto, child_offset, &child_len);
		if (!child_name)
			return child_len;

		/* Extra FDT_TAGSIZE bytes for expanded node name */
		strsize = FDT_TAGALIGN(len+1+FDT_TAGSIZE);

		if (child_len >= 11 &&
				!strncmp(child_name, "__overlay__", 11))
		{
			char new_name[strsize];

			idx = name + 9;
			stop = NULL;
			index = strtoul(idx, &stop, 10);
			if (ULONG_MAX - delta < index)
				return -FDT_ERR_BADVALUE;

			new_index = index + delta;
			ret = snprintf(new_name, sizeof(new_name),
						"fragment@%lu", new_index);
			if (ret >= sizeof(new_name))
				return -FDT_ERR_BADVALUE;

			ret = fdt_set_name(fdto, offset, new_name);
			if (ret < 0)
				return ret;
		}

next_node:
		offset = fdt_next_subnode(fdto, offset);
	}

	return 0;
}

/* Return maximum count of overlay fragments */
static int count_fragments(void *fdt, unsigned long *max_base_fragments)
{
	int offset = -1, child_offset, child_len, len, found = 0;
	const char *name, *child_name, *idx;
	char *stop;
	unsigned long index, max = 0;

	offset = fdt_first_subnode(fdt, 0);
	while (offset >= 0) {
		name = fdt_get_name(fdt, offset, &len);
		if (!name)
			return len;

		if (len < 9 || strncmp(name, "fragment@", 9))
			goto next_node;

		child_offset = fdt_first_subnode(fdt, offset);
		if (child_offset < 0)
			return child_offset;

		child_name = fdt_get_name(fdt, child_offset, &child_len);
		if (!child_name)
			return child_len;

		if (child_len < 11 || strncmp(child_name, "__overlay__", 11))
			goto next_node;

		found = 1;
		idx = name + 9;
		index = strtoul(idx, &stop, 10);
		if (index > max)
			max = index;
next_node:
		offset = fdt_next_subnode(fdt, offset);
	}

	if (!found)
		return -FDT_ERR_NOTFOUND;

	*max_base_fragments = max;

	return 0;
}

/*
 * Merging two overlay blobs involves copying some of the overlay fragment nodes
 * (named as fragment@xyz) from second overlay blob into first, which can lead
 * to naming conflicts (ex: two nodes of same name /fragment@0). To prevent such
 * naming conflicts, rename all occurences of fragment@xyz in second overlay
 * blob as fragment@xyz+delta, where delta is the maximum overlay fragments seen
 * in first overlay blob
 */
static int overlay_rename_fragments(void *fdt, void *fdto)
{
	int ret, local_offset;
	unsigned long max_base_fragments = 0;

	ret = count_fragments(fdt, &max_base_fragments);
	/* no fragments in base dtb? then nothing to rename */
	if (ret == -FDT_ERR_NOTFOUND)
		return 0;
	else if (ret < 0)
		return ret;

	max_base_fragments += 1;
	ret = rename_nodes(fdto, 0, max_base_fragments);
	if (ret < 0)
		return ret;

	ret = rename_fragments_in_node(fdto, "/__fixups__", max_base_fragments);
	if (ret < 0)
		return ret;

	ret = rename_fragments_in_node(fdto, "/__symbols__",
						max_base_fragments);
	if (ret < 0 && ret != -FDT_ERR_NOTFOUND)
		return ret;

	/*
	 * renaming fragments in __local_fixups__ node's properties should be
	 * covered by rename_nodes()
	 */
	local_offset = fdt_path_offset(fdto, "/__local_fixups__");
	if (local_offset >= 0)
		ret = rename_nodes(fdto, local_offset, max_base_fragments);


	if (ret == -FDT_ERR_NOTFOUND)
		ret = 0;

	return ret;
}

/* merge a node's properties from fdto to fdt */
static int overlay_merge_node_properties(void *fdt,
					void *fdto, const char *nodename)
{
	int fdto_offset, ret;

	fdto_offset = fdt_path_offset(fdto, nodename);
	if (fdto_offset < 0)
		return fdto_offset;

	ret = copy_node(fdt, fdto, 0, fdto_offset);

	return ret;
}

static int overlay_merge_local_fixups(void *fdt, void *fdto)
{
	int fdto_local_fixups, ret;

	fdto_local_fixups = fdt_path_offset(fdto, "/__local_fixups__");
	if (fdto_local_fixups < 0)
		return fdto_local_fixups;

	ret = copy_node(fdt, fdto, 0, fdto_local_fixups);

	return ret;
}

int fdt_overlay_merge(void *fdt, void *fdto, int *fdto_nospace)
{
	uint32_t delta = fdt_get_max_phandle(fdt);
	int ret;

	fdt_check_header(fdt);
	fdt_check_header(fdto);

	*fdto_nospace = 0;

	ret = overlay_rename_fragments(fdt, fdto);
	if (ret) {
		if (ret == -FDT_ERR_NOSPACE)
			*fdto_nospace = 1;
		goto err;
	}

	ret = overlay_adjust_local_phandles(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_update_local_references(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_fixup_phandles(fdt, fdto, 1);
	if (ret)
		goto err;

	ret = overlay_merge(fdt, fdto, 1);
	if (ret)
		goto err;

	ret = overlay_symbol_update(fdt, fdto, 1);
	if (ret)
		goto err;

	/* Can't have an overlay without __fixups__ ? */
	ret = overlay_merge_node_properties(fdt, fdto, "/__fixups__");
	if (ret)
		goto err;

	/* __symbols__ node need not be present */
	ret = overlay_merge_node_properties(fdt, fdto, "/__symbols__");
	if (ret && ret != -FDT_ERR_NOTFOUND)
		goto err;

	/* __local_fixups__ node need not be present */
	ret = overlay_merge_local_fixups(fdt, fdto);
	if (ret < 0 && ret != -FDT_ERR_NOTFOUND)
		goto err;

	/*
	 * The overlay has been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	return 0;

err:
	/*
	 * The overlay might have been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	/*
	 * The base device tree might have been damaged, erase its
	 * magic.
	 */
	if (!*fdto_nospace)
		fdt_set_magic(fdt, ~0);

	return ret;
}
