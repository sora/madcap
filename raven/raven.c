/*
 * raven is a madcap capable dummy driver.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rculist.h>
#include <linux/hash.h>
#include <linux/rwlock.h>
#include <linux/etherdevice.h>
#include <net/net_namespace.h>
#include <net/rtnetlink.h>
#include <net/ip_tunnels.h>


#include "../include/madcap.h"


/* common prefix used by pr_<> macros */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#define RAVEN_VERSION	"0.0.0"
MODULE_VERSION (RAVEN_VERSION);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("upa@haeena.net");
MODULE_DESCRIPTION ("raven, madcap capable dummy driver");
MODULE_ALIAS_RTNL_LINK ("raven");

static u32 raven_salt __read_mostly;


struct raven_table {
	struct hlist_node	hlist;	/* raven_dev->raven_table[] */
	struct rcu_head		rcu;
	struct net_device	*dev;
	unsigned long		updated;	/* jiffies */

	u64	id;	/* key */
	__be32	dst;	/* ipv4 dst */
};

struct raven_dev {
	struct list_head	list;
	struct rcu_head		rcu;
	struct net_device	*dev;

	struct net_device	*vdev;	/* overlay virtual device acquiring 
					 * this raven device */

	u16	length;		/* madcap identifier length */
	u16	offset;		/* madcap identifier offset */

#define RAVEN_HASH_BITS	8
#define RAVEN_HASH_SIZE	(1 << RAVEN_HASH_BITS)
	struct hlist_head	raven_table[RAVEN_HASH_SIZE]; /* hash table */
	rwlock_t		lock;	/* table lock */
};

struct raven_net {
	struct list_head	dev_list;	/* per netns device list */
};

static int raven_net_id;


static void
raven_table_free (struct rcu_head *head)
{
	struct raven_table *rt = container_of (head, struct raven_table, rcu);
	kfree(rt);
}

static void
raven_table_delete (struct raven_table *rt)
{
	hlist_del_rcu (&rt->hlist);
	call_rcu (&rt->rcu, raven_table_free);
}

static void
raven_table_destroy (struct raven_dev *rdev)
{
	unsigned int n;

	for (n = 0; n < RAVEN_HASH_SIZE; n++) {
		struct hlist_node *ptr, *tmp;

		hlist_for_each_safe (ptr, tmp, &rdev->raven_table[n]) {
			struct raven_table *rt;

			rt = container_of (ptr, struct raven_table, hlist);
			raven_table_delete (rt);
		}
	}
}

static int
raven_acquire_dev (struct net_device *dev, struct net_device *vdev)
{
	return 0;
}

static int
raven_release_dev (struct net_device *dev, struct net_device *vdev)
{
	return 0;
}

static int
raven_llt_offset_cfg (struct net_device *dev, struct madcap_obj *obj)
{
	return 0;
}

static int
raven_llt_length_cfg (struct net_device *dev, struct madcap_obj *obj)
{
	return 0;
}

static int
raven_llt_entry_add (struct net_device *dev, struct madcap_obj *obj)
{
	return 0;
}

static int
raven_llt_entry_del (struct net_device *dev, struct madcap_obj *obj)
{
	return 0;
}

static int
raven_llt_entry_dump (struct net_device *dev, struct netlink_callback *cb)
{
	return 0;
}

static struct madcap_ops raven_madcap_ops = {
	.mco_acquire_dev	= raven_acquire_dev,
	.mco_release_dev	= raven_release_dev,
	.mco_llt_offset_cfg	= raven_llt_offset_cfg,
	.mco_llt_length_cfg	= raven_llt_length_cfg,
	.mco_llt_entry_add	= raven_llt_entry_add,
	.mco_llt_entry_del	= raven_llt_entry_del,
	.mco_llt_entry_dump	= raven_llt_entry_dump,
};


static netdev_tx_t
raven_xmit (struct sk_buff *skb, struct net_device *dev)
{
	struct pcpu_sw_netstats *tx_stats;

	/* As a dummy driver for measurement, raven device updates
	 * counters and drops packet immediately.
	 * 
	 * ToDo:
	 * emulate tonic device behavior in software layer.
	 * - encapsulate packet in accordance with the locator-lookup-table.
	 * - transmit encapsulated packet via physical NIC.
	 * - Should I implement this to existing drivers for normal NIC?
	 */

	tx_stats = this_cpu_ptr (dev->tstats);
	u64_stats_update_begin (&tx_stats->syncp);
	tx_stats->tx_packets++;
	tx_stats->tx_bytes += skb->len;
	u64_stats_update_end (&tx_stats->syncp);

	kfree_skb (skb);

	return NETDEV_TX_OK;
}

static int
raven_change_mtu (struct net_device *dev, int new_mtu)
{
#define MIN_MTU	46
#define MAX_MTU	65535

	if (!(new_mtu >= MIN_MTU && new_mtu <= MAX_MTU))
		return -EINVAL;

	dev->mtu = new_mtu;
	return 0;
}

static int
raven_init (struct net_device *dev)
{
	dev->tstats = netdev_alloc_pcpu_stats (struct pcpu_sw_netstats);
	if (!dev->tstats)
		return -ENOMEM;

	return 0;
}

static void
raven_uninit (struct net_device *dev)
{
	free_percpu (dev->tstats);
}

static const struct net_device_ops raven_netdev_ops = {
	.ndo_init		= raven_init,
	.ndo_uninit		= raven_uninit,
	.ndo_start_xmit		= raven_xmit,
	.ndo_get_stats64	= ip_tunnel_get_stats64,	/* XXX */
	.ndo_change_mtu		= raven_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= eth_mac_addr,
};

static int
raven_newlink (struct net *net, struct net_device *dev,
	       struct nlattr *tb[], struct nlattr *data[])
{
	int err;
	struct raven_net *rnet = net_generic (net, raven_net_id);
	struct raven_dev *rdev = netdev_priv (dev);

	err = register_netdevice (dev);
	if (err) {
		netdev_err (dev, "failed to register netdevice.\n");
		return err;
	}

	list_add_tail_rcu (&rdev->list, &rnet->dev_list);

	err = madcap_register_device (dev, &raven_madcap_ops);
	if (err < 0) {
		netdev_err (dev, "failed to register madcap_ops.\n");
		return err;
	}

	return 0;
}

static void
raven_dellink (struct net_device *dev, struct list_head *head)
{
	struct raven_dev *rdev = netdev_priv (dev);

	write_lock_bh (&rdev->lock);
	raven_table_destroy (rdev);
	write_unlock_bh (&rdev->lock);

	list_del_rcu (&rdev->list);
	unregister_netdevice_queue (dev, head);
}

static void
raven_setup (struct net_device *dev)
{
	int n;
	struct raven_dev *rdev = netdev_priv (dev);

	eth_hw_addr_random (dev);
	ether_setup (dev);
	dev->netdev_ops = &raven_netdev_ops;
	dev->destructor = free_netdev;

	/* XXX: qlene 0 causes special data path shortcut on __dev_queue_xmit. 
	 * More considerlation is needed for the research view.
	 */
	dev->tx_queue_len = 0;	
	dev->features	|= NETIF_F_LLTX;
	dev->features	|= NETIF_F_NETNS_LOCAL;
	dev->priv_flags	|= IFF_LIVE_ADDR_CHANGE;	/* XXX: phydev? */
	netif_keep_dst (dev);

	INIT_LIST_HEAD (&rdev->list);
	rwlock_init (&rdev->lock);
	rdev->dev = dev;
	rdev->vdev = NULL;	/* alloced by mco_acquire_dev */
	rdev->length = 0;	/* cfged by mco_llt_length_cfg */
	rdev->offset = 0;	/* cfged by mco_llt_offset_cfg */

	for (n = 0; n < RAVEN_HASH_SIZE; n++)
		INIT_HLIST_HEAD (&rdev->raven_table[n]);
}

static struct rtnl_link_ops raven_link_ops __read_mostly = {
	.kind		= "raven",
	.priv_size	= sizeof (struct raven_dev),
	.setup		= raven_setup,
	.newlink	= raven_newlink,
	.dellink	= raven_dellink,
};

static __net_init int
raven_init_net (struct net *net)
{
	struct raven_net *rnet = net_generic (net, raven_net_id);

	INIT_LIST_HEAD (&rnet->dev_list);
	return 0;
}

static void __net_exit
raven_exit_net (struct net *net)
{
	struct raven_net *rnet = net_generic (net, raven_net_id);
	struct raven_dev *rdev, *next;
	LIST_HEAD (list);

	rtnl_lock ();
	list_for_each_entry_safe (rdev, next, &rnet->dev_list, list) {
		unregister_netdevice_queue (rdev->dev, &list);
	}
	rtnl_unlock ();

	return;
}

static struct pernet_operations raven_net_ops = {
	.init  	= raven_init_net,
	.exit	= raven_exit_net,
	.id	= &raven_net_id,
	.size	= sizeof (struct raven_net),
};


static __init int
raven_init_module (void)
{
	int rc;

	get_random_bytes (&raven_salt, sizeof (raven_salt));

	rc = register_pernet_subsys (&raven_net_ops);
	if (rc < 0)
		goto netns_failed;

	rc = rtnl_link_register (&raven_link_ops);
	if (rc < 0)
		goto rtnl_failed;

	pr_info ("raven (%s) is loaded.", RAVEN_VERSION);
	return 0;

rtnl_failed:
	unregister_pernet_subsys (&raven_net_ops);
netns_failed:
	return rc;
}
module_init (raven_init_module)

static void __exit
raven_exit_module (void)
{
	rtnl_link_unregister (&raven_link_ops);
	unregister_pernet_subsys (&raven_net_ops);

	pr_info ("raven (%s) is unloaded.", RAVEN_VERSION);
}
module_exit (raven_exit_module);