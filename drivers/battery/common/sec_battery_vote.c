
#include "sec_battery_vote.h"
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>

static struct dentry *debug_root;
static struct dentry *status_all;
static LIST_HEAD(vote_list);
static DEFINE_MUTEX(vote_lock);
struct sec_voter {
		int enable;
		int value;
		int pri;
};
struct sec_vote {
	const char * name;
	int type;
	int num;
	struct sec_voter *voter;
	const char ** voter_name;
	int id;
	int res;
	int init_val;
	struct mutex lock;
	void * data;
	int(*cb)(void *data, int value);
	struct list_head list;

	struct dentry * root;
	struct dentry * status_ent;
	int force_set;
	int force_val;
	struct dentry * force_set_ent;
};

const char * none_str = "None";
const char * force_str = "Force";

static int select_min(struct sec_voter *voter, int max, int *id, int *res)
{
	int i;
	int pri = INT_MIN;
	*res = INT_MAX;
	*id = -EINVAL;
	for (i = 0; i < max; i++) {
		if (voter[i].enable) {
			if (pri < voter[i].pri) {
				*res = voter[i].value;
				pri = voter[i].pri;
				*id = i;
			} else if (pri > voter[i].pri) {
				continue;
			} else if (*res > voter[i].value) {
				*res = voter[i].value;
				*id = i;
			}
		}
	}
	return 0;
}
static int select_max(struct sec_voter *voter, int max, int *id, int *res)
{
	int i;
	int pri = INT_MIN;
	*res = INT_MIN;
	*id = -EINVAL;
	for (i = 0; i < max; i++) {
		if (voter[i].enable) {
			if (pri < voter[i].pri) {
				*res = voter[i].value;
				pri = voter[i].pri;
				*id = i;
			} else if (pri > voter[i].pri) {
				continue;
			} else if (*res < voter[i].value) {
				*res = voter[i].value;
				*id = i;
			}
		}
	}
	return 0;
}
static int select_enable(struct sec_voter * voter, int max, int * id, int * res)
{
	int i;
	*res = 0;
	*id = -EINVAL;
	for (i = 0; i < max; i++) {
		if (voter[i].enable) {
			*res = voter[i].enable;
			*id = i;
			break;
		}
	}
	return 0;
}

static int select_vote_value(struct sec_vote * vote, int * id, int * res)
{
	int ret = 0;

	switch (vote->type) {
		case SEC_VOTE_MIN:
			select_min(vote->voter, vote->num, id, res);
			if (*res == INT_MAX)
				*res = vote->init_val;
			break;
		case SEC_VOTE_MAX:
			select_max(vote->voter, vote->num, id, res);
			if (*res == INT_MIN)
				*res = vote->init_val;
			break;
		case SEC_VOTE_EN:
			select_enable(vote->voter, vote->num, id, res);
			break;
		default:
			pr_err("%s type invalid\n", __func__);
			ret = -EINVAL;
	}

	return ret;
}

int get_sec_vote(struct sec_vote * vote, const char ** name, int * value)
{
	mutex_lock(&vote->lock);
	if (vote->id >= 0) {
		*name = vote->voter_name[vote->id];
	}
	else {
		*name = none_str;
	}
	*value = vote->res;
	mutex_unlock(&vote->lock);
	return 0;
}
EXPORT_SYMBOL(get_sec_vote);
int get_sec_vote_result(struct sec_vote *vote)
{
	int v;
	mutex_lock(&vote->lock);
	if (vote->force_set)
		v = vote->force_val;
	else
		v = vote->res;
	mutex_unlock(&vote->lock);
	return v;
}
EXPORT_SYMBOL(get_sec_vote_result);

const char* get_sec_keyvoter_name(struct sec_vote *vote)
{
	const char * str;
	mutex_lock(&vote->lock);
	if (vote->force_set)
		str = force_str;
	else
		str = (vote->id >= 0)?vote->voter_name[vote->id]: none_str;
	mutex_unlock(&vote->lock);
	return str;
}
EXPORT_SYMBOL(get_sec_keyvoter_name);

int get_sec_voter_status(struct sec_vote *vote, int id, int * v)
{
	if (id >= vote->num || id < 0)
		return -EINVAL;

	mutex_lock(&vote->lock);
	if (vote->type == SEC_VOTE_EN)
		*v = vote->voter[id].enable;
	else if (vote->voter[id].enable)
		*v = vote->voter[id].value;
	else
		*v = INT_MIN;
	mutex_unlock(&vote->lock);
	return (*v == INT_MIN) ? -EINVAL : 0;
}
EXPORT_SYMBOL(get_sec_voter_status);

int show_sec_vote_status(char *buf, unsigned int p_size)
{
	struct sec_vote *vote;
	int i, j = 0;
	char *type_str = "Unkonwn";

	if (list_empty(&vote_list)) {
		j += scnprintf(buf + j, p_size - j, "No vote\n");
		return j;
	}

	mutex_lock(&vote_lock);
	list_for_each_entry(vote, &vote_list, list) {
		mutex_lock(&vote->lock);
		for (i = 0; i < vote->num; i++) {
			if (vote->voter[i].enable) {
				j += scnprintf(buf + j, p_size - j, "%s: %s:\t\t\ten=%d v=%d p=%d\n",
						vote->name,
						vote->voter_name[i],
						vote->voter[i].enable,
						vote->voter[i].value,
						vote->voter[i].pri);
			}
		}
		switch (vote->type) {
		case SEC_VOTE_MIN:
			type_str = "Min";
			break;
		case SEC_VOTE_MAX:
			type_str = "Max";
			break;
		case SEC_VOTE_EN:
			type_str = "Set_any";
			break;
		default:
			type_str = "Invalid";
		}
		j += scnprintf(buf + j, p_size - j, "%s: INIT: v=%d\n",
				vote->name, vote->init_val);
		if (vote->force_set) {
			j += scnprintf(buf + j, p_size - j, "%s: voter=%s type=%s v=%d\n",
					vote->name, force_str, type_str, vote->force_val);
		} else {
			j += scnprintf(buf + j, p_size - j, "%s: voter=%s type=%s v=%d\n",
					vote->name,
					(vote->id >= 0) ? vote->voter_name[vote->id] : none_str,
					type_str, vote->res);
		}
		mutex_unlock(&vote->lock);
	}
	mutex_unlock(&vote_lock);

	return j;
}
EXPORT_SYMBOL(show_sec_vote_status);

static int show_vote_clients(struct seq_file *m, void *data)
{
	struct sec_vote *vote = m->private;
	int i;
	char *type_str = "Unkonwn";

	mutex_lock(&vote->lock);
	for (i = 0; i < vote->num; i++) {
		if (vote->voter[i].enable) {
			seq_printf(m, "%s: %s:\t\t\ten=%d v=%d p=%d\n",
					vote->name,
					vote->voter_name[i],
					vote->voter[i].enable,
					vote->voter[i].value,
					vote->voter[i].pri);
		}
	}
	switch (vote->type) {
		case SEC_VOTE_MIN:
			type_str = "Min";
			break;
		case SEC_VOTE_MAX:
			type_str = "Max";
			break;
		case SEC_VOTE_EN:
			type_str = "Set_any";
			break;
		default:
			type_str = "Invalid";
	}
	seq_printf(m, "%s: INIT: v=%d\n",
			vote->name, vote->init_val);
	if (vote->force_set) {
		seq_printf(m, "%s: voter=%s type=%s v=%d\n",
				vote->name, force_str, type_str, vote->force_val);
	} else {
		seq_printf(m, "%s: voter=%s type=%s v=%d\n",
				vote->name,
				(vote->id >= 0)?vote->voter_name[vote->id]: none_str,
				type_str, vote->res);
	}
	mutex_unlock(&vote->lock);

	return 0;
}

static int vote_status_open(struct inode *inode, struct file *file)
{
	struct sec_vote *vote = inode->i_private;

	return single_open(file, show_vote_clients, vote);
}

static const struct file_operations vote_status_ops = {
	.owner		= THIS_MODULE,
	.open		= vote_status_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int show_all_clients(struct seq_file *m, void *data)
{
	struct sec_vote *vote;
	int i;
	char *type_str = "Unkonwn";

	if (list_empty(&vote_list)) {
		seq_printf(m, "No vote\n");
		return 0;
	}

	mutex_lock(&vote_lock);
	list_for_each_entry(vote, &vote_list, list) {
		mutex_lock(&vote->lock);
		for (i = 0; i < vote->num; i++) {
			if (vote->voter[i].enable) {
				seq_printf(m, "%s: %s:\t\t\ten=%d v=%d p=%d\n",
						vote->name,
						vote->voter_name[i],
						vote->voter[i].enable,
						vote->voter[i].value,
						vote->voter[i].pri);
			}
		}
		switch (vote->type) {
			case SEC_VOTE_MIN:
				type_str = "Min";
				break;
			case SEC_VOTE_MAX:
				type_str = "Max";
				break;
			case SEC_VOTE_EN:
				type_str = "Set_any";
				break;
			default:
				type_str = "Invalid";
		}
		seq_printf(m, "%s: INIT: v=%d\n",
				vote->name, vote->init_val);
		if (vote->force_set) {
			seq_printf(m, "%s: voter=%s type=%s v=%d\n",
					vote->name, force_str, type_str, vote->force_val);
		} else {
			seq_printf(m, "%s: voter=%s type=%s v=%d\n",
					vote->name,
					(vote->id >= 0)?vote->voter_name[vote->id]: none_str,
					type_str, vote->res);
		}
		mutex_unlock(&vote->lock);
	}
	mutex_unlock(&vote_lock);

	return 0;
}

static int vote_status_all_open(struct inode *inode, struct file *file)
{

	return single_open(file, show_all_clients, NULL);
}

static const struct file_operations vote_status_all_ops = {
	.owner		= THIS_MODULE,
	.open		= vote_status_all_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int force_get(void *data, u64 *val)
{
	struct sec_vote *vote = data;

	*val = vote->force_set;

	return 0;
}

static int force_set(void *data, u64 val)
{
	struct sec_vote *vote = data;

	mutex_lock(&vote->lock);
	vote->force_set = val;

	if (!vote->cb)
		goto out;

	if (vote->force_set) {
		vote->res = vote->cb(vote->data, vote->force_val);
	} else {
		vote->res = vote->cb(vote->data, vote->res);
	}
out:
	mutex_unlock(&vote->lock);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(vote_force_ops, force_get, force_set, "%lld\n");
struct sec_vote *find_vote(const char *name)
{
	struct sec_vote *vote;

	list_for_each_entry(vote, &vote_list, list) {
		if (strcmp(vote->name, name) == 0) {
			return vote;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(find_vote);

struct sec_vote *sec_vote_init(const char *name, int type, int num, int init_val,
		const char **voter_name, int(*cb)(void *data, int value), void *data)
{
	struct sec_vote * vote = NULL;
	struct sec_voter * voter = NULL;

	mutex_lock(&vote_lock);
	vote = find_vote(name);
	if (vote) {
		pr_info("%s: %s exist \n", __func__, name);
		goto err;
	}
	if (voter_name == NULL) {
		pr_info("%s: Please add voter name list \n", __func__);
		goto err;
	}

	vote = kzalloc(sizeof(struct sec_vote), GFP_KERNEL);
	if (!vote) {
		pr_info("%s: mem aloocate fail \n", __func__);
		goto err;
	}
	vote->name = name;
	vote->type = type;
	voter = kzalloc(sizeof(struct sec_voter) * num, GFP_KERNEL);
	if (!voter) {
		pr_info("%s: mem aloocate fail \n", __func__);
		kfree(vote);
		goto err;
	}
	vote->voter = voter;
	vote->num = num;
	vote->voter_name = voter_name;
	vote->init_val = init_val;
	vote->cb = cb;
	vote->id = -EINVAL;
	vote->res = -EINVAL;
	vote->data = data;
	mutex_init(&vote->lock);

	if (debug_root == NULL) {
		debug_root = debugfs_create_dir("sec-vote", NULL);
		if (!debug_root) {
			pr_err("Couldn't create debug dir\n");
		} else {
			status_all = debugfs_create_file("status_all",
					S_IFREG | 0444,
					debug_root, NULL,
					&vote_status_all_ops);
			if (!status_all) {
				pr_err("Couldn't create status_all dbg file \n");
			}
		}
	}
	if (debug_root)
		vote->root = debugfs_create_dir(name, debug_root);
	if (!vote->root) {
		pr_err("Couldn't create debug dir %s\n", name);
	} else {
		vote->status_ent = debugfs_create_file("status", S_IFREG | 0444,
				vote->root, vote,
				&vote_status_ops);
		if (!vote->status_ent) {
			pr_err("Couldn't create status dbg file for %s\n", name);
		}

		debugfs_create_u32("force_val", S_IFREG | 0644,
						vote->root, &(vote->force_val));		

		vote->force_set_ent = debugfs_create_file("force_set",
				S_IFREG | 0444,
				vote->root, vote,
				&vote_force_ops);
		if (!vote->force_set_ent) {
			pr_err("Couldn't create force_set dbg file for %s\n", name);
		}
	}
	pr_info("%s: %s \n", __func__, name);
	list_add(&vote->list, &vote_list);
	mutex_unlock(&vote_lock);
	return vote;
err:
	mutex_unlock(&vote_lock);
	return NULL;
}
EXPORT_SYMBOL(sec_vote_init);

void sec_vote_destroy(struct sec_vote *vote)
{
	pr_info("%s: %s\n", __func__, vote->name);
	list_del(&vote->list);
	kfree(vote->voter);
	debugfs_remove_recursive(vote->root);
	mutex_destroy(&vote->lock);
	kfree(vote);
}
EXPORT_SYMBOL(sec_vote_destroy);

void change_sec_voter_pri(struct sec_vote *vote, int event, int pri)
{
	if (event >= vote->num) {
		pr_info("%s id Error(%d)\n", __func__, event);
		return;
	}
	mutex_lock(&vote->lock);
	vote->voter[event].pri = pri;
	mutex_unlock(&vote->lock);
}
EXPORT_SYMBOL(change_sec_voter_pri);

void _sec_vote(struct sec_vote *vote, int event, int en, int value, const char *fname, int line)
{
	int id, res, ret;

	if (event >= vote->num) {
		pr_info("%s id Error(%d)\n", __func__, event);
		return;
	}
	mutex_lock(&vote->lock);
	pr_debug("%s, %s en: %d->%d, v: %d->%d\n", vote->name,vote->voter_name[event],
		vote->voter[event].enable, en, vote->voter[event].value, value);

	if ((vote->voter[event].enable == en) &&
		(((vote->voter[event].value == value) || !en)))
		goto out;

	vote->voter[event].enable = en;
	vote->voter[event].value = value;

	ret = select_vote_value(vote, &id, &res);
	if (ret < 0)
			goto out;

	pr_info("%s(%s:%d): %s (%s, %d) -> (%s, %d)\n", __func__, fname, line, vote->name,
		(vote->id >= 0) ? vote->voter_name[vote->id] : none_str, vote->res,
		(id >= 0) ? vote->voter_name[id] : none_str, res);

	if (res != vote->res) {
		vote->id = id;
		vote->res = res;
		if (vote->force_set)
			pr_err("%s skip by force_set\n", __func__);
		else
			vote->res = vote->cb(vote->data, res);
	} else if (!en && (vote->id == event)) {
		vote->id = id;
	}

out:
	mutex_unlock(&vote->lock);
}
EXPORT_SYMBOL(_sec_vote);

void sec_vote_refresh(struct sec_vote *vote)
{
	mutex_lock(&vote->lock);
	if (vote->res == -EINVAL && vote->id == -EINVAL) {
		pr_info("%s: skip. not used before\n", __func__);
	} else {
		if (vote->force_set) {
			pr_info("%s: refresh (%s, %d)\n", vote->name, force_str, vote->force_val);
			vote->res = vote->cb(vote->data, vote->force_val);
		} else {
			int id, res, ret;

			ret = select_vote_value(vote, &id, &res);
			pr_info("%s: refresh (%s, %d, %d)\n", vote->name,
				(id >= 0) ? vote->voter_name[id] : none_str, res, ret);
			if (ret < 0)
				goto out;
			vote->res = vote->cb(vote->data, res);
		}
	}
out:
	mutex_unlock(&vote->lock);
}
EXPORT_SYMBOL(sec_vote_refresh);

const char *get_sec_vote_name(struct sec_vote *vote)
{
	return vote->name;
}
EXPORT_SYMBOL(get_sec_vote_name);

