/*
 * raven is a madcap capable dummy driver.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rculist.h>
#include <linux/hash.h>
#include <linux/etherdevice.h>
#include <net/net_namespace.h>
#include <net/rtnetlink.h>
#include <net/ip_tunnels.h>



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



struct raven_dev {
	struct list_head	list;
	struct rcu_head		rcu;
	struct net_device	*dev;
};

struct raven_net {
	struct list_head	dev_list;	/* per netns device list */
};

static int raven_net_id;



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
		netdev_err (dev, "failed to register netdevice\n");
		return err;
	}

	list_add_tail_rcu (&rdev->list, &rnet->dev_list);
	return 0;
}

static void
raven_dellink (struct net_device *dev, struct list_head *head)
{
	struct raven_dev *rdev = netdev_priv (dev);

	list_del_rcu (&rdev->list);
	unregister_netdevice_queue (dev, head);
}

static void
raven_setup (struct net_device *dev)
{
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
	rdev->dev = dev;
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
