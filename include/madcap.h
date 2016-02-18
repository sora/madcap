
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
	u64	id;
	__be32	dst;	/* dst ipv4 address (locator) */
};


struct madcap_ops {
	struct net_device *physical_dev;
	/* physical_dev is key for confainter_of in order to get ptr
	 * of madcap_ops.  It is unsafe approach, but this avoids any
	 * modifications to mainline kernel. struct madcap_ops should
	 * be a member of struct net_device...
	 */

	netdev_tx_t	(*mco_start_xmit) (struct sk_buff *skb,
					   struct net_device *dev);
	int		(*mco_if_rx) (struct sk_buff *skb);	/* ??? */
	

	int		(*mco_allocate_tonic) (struct net_device *dev,
					       struct net_device *vdev);
	int		(*mco_release_tonic) (struct net_device *dev,
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

int madcap_allocate_tonic (struct net_device *dev, struct net_device *vdev);
int madcap_release_tonic (struct net_device *dev, struct net_device *vdev);

int madcap_llt_offset_cfg (struct net_device *dev, struct madcap_obj *obj);
int madcap_llt_length_cfg (struct net_device *dev, struct madcap_obj *obj);

int madcap_llt_entry_add (struct net_device *dev, struct madcap_obj *obj);
int madcap_llt_entry_del (struct net_device *dev, struct madcap_obj *obj);



/* XXX: functions should be defined in madcap.c, and many features
 * such as driver/device lock and resource allocation like switchdev
 * trans.ph_prepare phasing are needed. but not implemented.
 */

int
madcap_allocate_tonic (struct net_device *dev, struct net_device *vdev)
{
	struct madcap_ops *mc_ops;

	mc_ops = container_of (dev, struct madcap_ops, physical_dev);

	if (mc_ops->madcap_allocate_tonic)
		return mc_ops->madcap_allocate_tonic (dev, vdev);

	return -EOPNOTSUPP;
}

int
madcap_release_tonic (struct net_device *dev, struct net_device *vdev)
{
	struct madcap_ops *mc_ops;

	mc_ops = container_of (dev, struct madcap_ops, physical_dev);

	if (mc_ops->madcap_release_tonic)
		return mc_ops->madcap_release_tonic (dev, vdev);

	return -EOPNOTSUPP;
}



/* XXX: */
#define __MADCAP_OBJ_DEFUN(funcname)					\
	int (funcname) (struct net_device *dev, struct madcap_obj *obj)	\
	{								\
		struct madcap_ops *mc_ops;				\
		mc_ops = container_of (dev, struct madcap_ops, physical_dev); \
		if (mc_ops->(funcname))					\
			return mc_ops->(funcname) (dev, vdev);		\
									\
		return -EOPNOTSUPP;					\
	}								\

__MADCAP_OBJ_DEFUN(madcap_llt_offset_cfg);
__MADCAP_OBJ_DEFUN(madcap_llt_length_cfg);
__MADCAP_OBJ_DEFUN(madcap_llt_entry_add);
__MADCAP_OBJ_DEFUN(madcap_llt_entry_del);

#endif /* _MADCAP_H_ */
