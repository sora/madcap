/*
 * madcap.c
 * Encapsulation Madness!!
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rwlock.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>

#include "../include/madcap.h"	/* XXX */


#define MADCAP_NAME	"madcap"
#define MADCAP_VERSION	"0.0.0"

#define MADCAP_GENL_NAME	MADCAP_NAME
#define MADCAP_GENL_VERSION	0x00


MODULE_VERSION (MADCAP_VERSION);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("upa@haeena.net");



/* XXX: Many features such as driver/device lock and resource
 * allocation like switchdev trans.ph_prepare phasing are needed. but
 * not implemented...
 */

int
madcap_acquire_dev (struct net_device *dev, struct net_device *vdev)
{
	struct madcap_ops *mc_ops;

	mc_ops = get_madcap_ops (dev);

	if (mc_ops->mco_acquire_dev)
		return mc_ops->mco_acquire_dev (dev, vdev);

	return -EOPNOTSUPP;
}
EXPORT_SYMBOL (madcap_acquire_dev);

int
madcap_release_dev (struct net_device *dev, struct net_device *vdev)
{
	struct madcap_ops *mc_ops;

	mc_ops = get_madcap_ops (dev);

	if (mc_ops->mco_release_dev)
		return mc_ops->mco_release_dev (dev, vdev);

	return -EOPNOTSUPP;
}
EXPORT_SYMBOL (madcap_release_dev);

/* XXX: */
#define __MADCAP_OBJ_DEFUN(funcname)                                    \
	int madcap_##funcname (struct net_device *dev,			\
			       struct madcap_obj *obj)			\
	{                                                               \
		struct madcap_ops *mc_ops;                              \
		mc_ops = get_madcap_ops (dev);				\
		if (mc_ops->mco_##funcname)				\
			return mc_ops->mco_##funcname (dev, obj);	\
									\
		return -EOPNOTSUPP;					\
	}								\
	EXPORT_SYMBOL (madcap_##funcname);				\

__MADCAP_OBJ_DEFUN(llt_offset_cfg);
__MADCAP_OBJ_DEFUN(llt_length_cfg);
__MADCAP_OBJ_DEFUN(llt_entry_add);
__MADCAP_OBJ_DEFUN(llt_entry_del);


/* Generic Netlink MadCap family */

static struct genl_family madcap_nl_family = {
	.id		= GENL_ID_GENERATE,
	.name		= MADCAP_GENL_NAME,
	.version	= MADCAP_GENL_VERSION,
	.maxattr	= MADCAP_ATTR_MAX,
};

static struct nla_policy madcap_nl_policy[MADCAP_ATTR_MAX + 1] = {
	[MADCAP_ATTR_NONE]	 = { .type = NLA_UNSPEC, },
	[MADCAP_ATTR_IFINDEX]	 = { .type = NLA_U32, },
	[MADCAP_ATTR_OBJ_OFFSET] = { .type = NLA_BINARY,
				     .len = sizeof (struct madcap_obj_offset)},
	[MADCAP_ATTR_OBJ_LENGTH] = { .type = NLA_BINARY,
				     .len = sizeof (struct madcap_obj_length)},
	[MADCAP_ATTR_OBJ_ENTRY]  = { .type = NLA_BINARY,
				     .len = sizeof (struct madcap_obj_entry)},
};

static int
madcap_cmd_llt_offset_cfg (struct sk_buff *skb, struct genl_info *info)
{
	u32 ifindex;
	struct net_device *dev;
	struct madcap_obj_offset obj_ofs;
	struct net *net = sock_net (skb->sk);

	if (!info->attrs[MADCAP_ATTR_IFINDEX]) {
		pr_debug ("%s: none ifindex", __func__);
		return -EINVAL;
	}
	ifindex = nla_get_u32 (info->attrs[MADCAP_ATTR_IFINDEX]);

	dev = __dev_get_by_index (net, ifindex);
	if (!dev) {
		pr_debug ("%s: device not found for %u", __func__, ifindex);
		return -ENODEV;
	}

	if (!info->attrs[MADCAP_ATTR_OBJ_OFFSET]) {
		pr_debug ("%s: none offset object", __func__);
		return -EINVAL;
	}
	nla_memcpy (&obj_ofs, info->attrs[MADCAP_ATTR_OBJ_OFFSET],
		    sizeof (obj_ofs));

	return madcap_llt_offset_cfg (dev, MADCAP_OBJ (obj_ofs));
}

static int
madcap_cmd_llt_length_cfg (struct sk_buff *skb, struct genl_info *info)
{
	u32 ifindex;
	struct net_device *dev;
	struct madcap_obj_length obj_len;
	struct net *net = sock_net (skb->sk);

	if (!info->attrs[MADCAP_ATTR_IFINDEX]) {
		pr_debug ("%s: none ifindex", __func__);
		return -EINVAL;
	}
	ifindex = nla_get_u32 (info->attrs[MADCAP_ATTR_IFINDEX]);

	dev = __dev_get_by_index (net, ifindex);
	if (!dev) {
		pr_debug ("%s: device not found for %u", __func__, ifindex);
		return -ENODEV;
	}

	if (!info->attrs[MADCAP_ATTR_OBJ_LENGTH]) {
		pr_debug ("%s: none length object", __func__);
		return -EINVAL;
	}
	nla_memcpy (&obj_len, info->attrs[MADCAP_ATTR_OBJ_LENGTH],
		    sizeof (obj_len));

	return madcap_llt_length_cfg (dev, MADCAP_OBJ (obj_len));
}

static int
madcap_cmd_llt_entry_add (struct sk_buff *skb, struct genl_info *info)
{
	u32 ifindex;
	struct net_device *dev;
	struct madcap_obj_entry obj_ent;
	struct net *net = sock_net (skb->sk);

	if (!info->attrs[MADCAP_ATTR_IFINDEX]) {
		pr_debug ("%s: none ifindex", __func__);
		return -EINVAL;
	}
	ifindex = nla_get_u32 (info->attrs[MADCAP_ATTR_IFINDEX]);

	dev = __dev_get_by_index (net, ifindex);
	if (!dev) {
		pr_debug ("%s: device not found for %u", __func__, ifindex);
		return -ENODEV;
	}

	if (!info->attrs[MADCAP_ATTR_OBJ_ENTRY]) {
		pr_debug ("%s: none entry object", __func__);
		return -EINVAL;
	}
	nla_memcpy (&obj_ent, info->attrs[MADCAP_ATTR_OBJ_ENTRY],
		    sizeof (obj_ent));

	return madcap_llt_entry_add (dev, MADCAP_OBJ (obj_ent));
}

static int
madcap_cmd_llt_entry_del (struct sk_buff *skb, struct genl_info *info)
{
	u32 ifindex;
	struct net_device *dev;
	struct madcap_obj_entry obj_ent;
	struct net *net = sock_net (skb->sk);

	if (!info->attrs[MADCAP_ATTR_IFINDEX]) {
		pr_debug ("%s: none ifindex", __func__);
		return -EINVAL;
	}
	ifindex = nla_get_u32 (info->attrs[MADCAP_ATTR_IFINDEX]);

	dev = __dev_get_by_index (net, ifindex);
	if (!dev) {
		pr_debug ("%s: device not found for %u", __func__, ifindex);
		return -ENODEV;
	}

	if (!info->attrs[MADCAP_ATTR_OBJ_ENTRY]) {
		pr_debug ("%s: none entry object", __func__);
		return -EINVAL;
	}
	nla_memcpy (&obj_ent, info->attrs[MADCAP_ATTR_OBJ_ENTRY],
		    sizeof (obj_ent));

	return madcap_llt_entry_del (dev, MADCAP_OBJ (obj_ent));
}

static int
madcap_cmd_llt_entry_get (struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

static int
madcap_cmd_llt_entry_dump (struct sk_buff *skb, struct netlink_callback *cb)
{
	return 0;
}

static struct genl_ops madcap_nl_ops[] = {
	{
		.cmd	= MADCAP_CMD_LLT_OFFSET_CFG,
		.doit	= madcap_cmd_llt_offset_cfg,
		.policy	= madcap_nl_policy,
	},
	{
		.cmd	= MADCAP_CMD_LLT_LENGTH_CFG,
		.doit	= madcap_cmd_llt_length_cfg,
		.policy	= madcap_nl_policy,
	},
	{
		.cmd	= MADCAP_CMD_LLT_ENTRY_ADD,
		.doit	= madcap_cmd_llt_entry_add,
		.policy	= madcap_nl_policy,
	},
	{
		.cmd	= MADCAP_CMD_LLT_ENTRY_DEL,
		.doit	= madcap_cmd_llt_entry_del,
		.policy	= madcap_nl_policy,
	},
	{
		.cmd	= MADCAP_CMD_LLT_ENTRY_GET,
		.doit	= madcap_cmd_llt_entry_get,
		.dumpit	= madcap_cmd_llt_entry_dump,
		.policy	= madcap_nl_policy,
	},
};




/* XXX: Per net namespace subsystem, is
 * ONLY used for finding corresponding madcap_ops for a net_device.
 * NOT needed if madcap_ops was a member of struct net_device.
 * DIRTY hack in order to avoid mainline kernel modification.
 *    because make-kpkg is too slow...
 */

static unsigned int madcap_net_id;
#define MADCAPDEV_PERNET_NUM	16

struct madcap_net {
	rwlock_t	lock;
	struct madcap_ops *ops[MADCAPDEV_PERNET_NUM];
	struct net_device *dev[MADCAPDEV_PERNET_NUM];
};

static __net_init int
madcap_init_net (struct net *net)
{
	struct madcap_net *madnet = net_generic (net, madcap_net_id);

	memset (madnet, 0, sizeof (*madnet));
	rwlock_init (&madnet->lock);

	return 0;
}

static __net_exit void
madcap_exit_net (struct net *net)
{
	return;
}

static struct pernet_operations madcap_net_ops = {
	.init	= madcap_init_net,
	.exit	= madcap_exit_net,
	.id	= &madcap_net_id,
	.size	= sizeof (struct madcap_net),
};

struct madcap_ops *
get_madcap_ops (struct net_device *dev)
{
	/* XXX: if madcap_ops was a member of struct net_device,
	 * This did "return dev->madcap_ops;".
	 */
	
	int n;
	struct madcap_net *madnet = net_generic (dev_net (dev), madcap_net_id);

	for (n = 0; n < MADCAPDEV_PERNET_NUM; n++) {
		if (madnet->dev[n] == dev) {
			return madnet->ops[n];
		}
	}

	return NULL;
}
EXPORT_SYMBOL (get_madcap_ops);

int
madcap_register_device (struct net_device *dev, struct madcap_ops *mc_ops)
{
	int n;
	struct madcap_net *madnet = net_generic (dev_net (dev), madcap_net_id);

	/*XXX: get_madcap_ops should lock? */
	if (get_madcap_ops (dev))
		return -EEXIST;
	
	write_lock_bh (&madnet->lock);
	for (n = 0; n < MADCAPDEV_PERNET_NUM; n++) {
		if (madnet->dev[n] == NULL) {
			madnet->ops[n] = mc_ops;
			madnet->dev[n] = dev;
			break;
		}
	}
	write_unlock_bh (&madnet->lock);

	if (!(n < MADCAPDEV_PERNET_NUM)) {
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL (madcap_register_device);

int
madcap_unregister_device (struct net_device *dev)
{
	int n;
	struct madcap_net *madnet = net_generic (dev_net (dev), madcap_net_id);

	write_lock_bh (&madnet->lock);
	for (n = 0; n < MADCAPDEV_PERNET_NUM; n++) {
		if (madnet->dev[n] == dev) {
			madnet->dev[n] = NULL;
			madnet->ops[n] = NULL;
		}
	}
	write_unlock_bh (&madnet->lock);

	if (!(n < MADCAPDEV_PERNET_NUM)) {
		return -ENOENT;
	}

	return 0;
}
EXPORT_SYMBOL (madcap_unregister_device);


static int
__init madcap_init (void)
{
	int rc;

	rc = register_pernet_subsys (&madcap_net_ops);
	if (rc < 0)
		return rc;

	rc = genl_register_family_with_ops (&madcap_nl_family, madcap_nl_ops);
	if (rc < 0)
		goto genl_failed;

	pr_info ("madcap (%s) is loaded", MADCAP_VERSION);
	return 0;

genl_failed:
	unregister_pernet_subsys (&madcap_net_ops);
	return rc;
}
module_init (madcap_init);

static void
__exit madcap_exit (void)
{
	genl_unregister_family (&madcap_nl_family);
	unregister_pernet_subsys(&madcap_net_ops);

	pr_info ("madcap (%s) is unloaded", MADCAP_VERSION);
	return;
}
module_exit (madcap_exit);
