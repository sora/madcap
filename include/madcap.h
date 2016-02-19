
/* madcap.h */

#ifndef _MADCAP_H_
#define _MADCAP_H_

/*
 * Encapsulation Madness!!
 * 
 * - TX protocol specified encaped packet.
 * - RX encaped packet to protocol driver pseudo interface.
 * - allocate ToNIC device (with specified queue?)
 * - release ToNIC device (with specified queue?)
 * - Config locator look-up table :
 *   - set offset/length.
 *   - add entry.
 *   - delete entry.
 */


#include <linux/netdevice.h>

/* common prefix used by pr_<> macros */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

enum madcap_obj_id {
	MADCAP_OBJ_ID_UNDEFINED,
	MADCAP_OBJ_ID_LLT_OFFSET,
	MADCAP_OBJ_ID_LLT_LENGTH,
	MADCAP_OBJ_ID_LLT_ENTRY,
};

struct madcap_obj {
	enum madcap_obj_id id;
	u16	tb_id;	/* table id (queue?) */
};

struct madcap_obj_offset {
	struct madcap_obj obj;
	u16	offset;
};

struct madcap_obj_length {
	struct madcap_obj obj;
	u16	length;
};

struct madcap_obj_entry {
	struct madcap_obj obj;
	u64	id;	/* identifier of dst */
	__be32	dst;	/* dst ipv4 address (locator) */
};

#define MADCAP_OBJ(obj_)	&(obj_.obj)
#define MADCAP_OBJ_OFFSET(obj)	\
	container_of (obj, struct madcap_obj_offset, obj)
#define MADCAP_OBJ_LENGTH(obj)	\
	container_of (obj, struct madcap_obj_length, obj)
#define MADCAP_OBJ_ENTRY(obj)	\
	container_of (obj, struct madcap_obj_entry, obj)



struct madcap_ops {
	netdev_tx_t	(*mco_start_xmit) (struct sk_buff *skb,
					   struct net_device *dev);
	int		(*mco_if_rx) (struct sk_buff *skb);	/* ??? */
	

	/* vdev (pseudo NIC) acquire/release dev (physical tunnel device) */
	int		(*mco_acquire_dev) (struct net_device *dev,
					     struct net_device *vdev);
	int		(*mco_release_dev) (struct net_device *dev,
					    struct net_device *vdev);

	int		(*mco_llt_offset_cfg) (struct net_device *dev,
					       struct madcap_obj *obj);
	int		(*mco_llt_length_cfg) (struct net_device *dev,
					       struct madcap_obj *obj);
	int		(*mco_llt_entry_add) (struct net_device *dev,
					      struct madcap_obj *obj);
	int		(*mco_llt_entry_del) (struct net_device *dev,
					      struct madcap_obj *obj);
};


/* prototypes for madcap operations */

int madcap_acquire_dev (struct net_device *dev, struct net_device *vdev);
int madcap_release_dev (struct net_device *dev, struct net_device *vdev);

int madcap_llt_offset_cfg (struct net_device *dev, struct madcap_obj *obj);
int madcap_llt_length_cfg (struct net_device *dev, struct madcap_obj *obj);

int madcap_llt_entry_add (struct net_device *dev, struct madcap_obj *obj);
int madcap_llt_entry_del (struct net_device *dev, struct madcap_obj *obj);


/* dev<->madcap_ops mappings are maintained in a table in madcap.ko
 * in order to eliminate any modifications to mainline kernel.
 */
struct madcap_ops * get_madcap_ops (struct net_device *dev);
int madcap_regsiter_device (struct net_device *dev, struct madcap_ops *mc_ops);
int madcap_unregister_device (struct net_device *dev);



/* Generic Netlink, madcap family definition. */

/*
 * XXX: Allocate/release tonic device are called when overlay pseudo
 * device is created/destroyed. It will be implmeneted as a
 * modification for protocol drivers. Otherwise, notifier like
 * switchdev is needed ?
 */

/* genl commands */
enum {
	MADCAP_CMD_LLT_OFFSET_CFG,
	MADCAP_CMD_LLT_LENGTH_CFG,
	MADCAP_CMD_LLT_ENTRY_ADD,
	MADCAP_CMD_LLT_ENTRY_DEL,
	MADCAP_CMD_LLT_ENTRY_GET,

	__MADCAP_CMD_MAX,
};
#define MADCAP_CMD_MAX	(__MADCAP_CMD_MAX - 1)

/* genl attr types */
enum {
	MADCAP_ATTR_NONE,	/* none */
	MADCAP_ATTR_IFINDEX,	/* ifindex of tonic device */
	MADCAP_ATTR_OBJ_OFFSET,	/* struct madcap_obj_offset */
	MADCAP_ATTR_OBJ_LENGTH,	/* struct madcap_obj_length */
	MADCAP_ATTR_OBJ_ENTRY,	/* struct madcap_obj_entry */

	__MADCAP_ATTR_MAX,
};
#define MADCAP_ATTR_MAX	(__MADCAP_ATTR_MAX - 1)


#endif /* _MADCAP_H_ */
