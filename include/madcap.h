
/* madcap.h */

#ifndef _MADCAP_H_
#define _MADCAP_H_

/*
 * Encapsulation Madness!!
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
	MADCAP_OBJ_ID_LLT_ENTRY_ADD,
	MADCAP_OBJ_ID_LLT_NETRY_DEL,
};

struct madcap_obj {
	enum madcap_obj_id id;
};

/* madcap_obj_xxx will be defined here. */


struct madcap_ops {
	netdev_tx_t	(*mco_start_xmit)(struct sk_buff *skb,
					  struct net_device *dev);
	int		(*mco_if_rx)(struct sk_buff *skb);	/* ??? */
	
	int		(*mco_alloc_tonic)(struct net_device *dev,
					   struct net_device *link);
	int		(*mco_release_tonic)(struct net_device *dev,
					     struct net_device *link);

	int		(*mco_cfg_llt_offset)(struct net_device *dev,
					      struct madcap_obj *obj);
	int		(*mco_cfg_llt_length)(struct net_device *dev,
					      struct madcap_obj *obj);
	int		(*mco_cfg_llt_entry_add)(struct net_device *dev,
						 struct madcap_obj *obj);
	int		(*mco_cfg_llt_entry_del)(struct net_device *dev,
						 struct madcap_obj *obj);
};


#endif /* _MADCAP_H_ */
