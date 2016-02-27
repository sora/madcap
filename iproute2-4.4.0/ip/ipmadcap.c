/* ipmad.c */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>

#include <linux/genetlink.h>
#include "utils.h"
#include "ip_common.h"
#include "rt_names.h"
#include "libgenl.h"


#include "../../include/madcap.h" /* XXX: need makefile magic */

/* netlink socket */
static struct rtnl_handle genl_rth;
static int genl_family = -1;

struct madcap_param {
	__u32 ifindex;
	__u16 offset;
	__u16 length;
	__u8 proto;
	__u64 id;
	__u32 dst, src;

	int udp;
	int enable, disable, src_port_hash;
	__u16 dst_port, src_port;

	int config;

	int f_offset, f_length;	/* offset and length may become 0 correctly */
};

static void
usage (void)
{
	fprintf (stderr,
		 "usage:  ip madcap { add | del } "
		 "[ id ID ] [ dst IPADDR ] [ dev DEVICE ]\n"
		 "\n"
		 "        ip madcap set [ dev DEVICE ] "
		 "[ offset OFFSET ] [ length LENGTH ]\n"
		 "                      [ src IPADDR ] [ proto IPPROTO ]\n"
		 "                      [ udp [ [ dst-port [ PORT ] ]\n"
		 "                              [ src-port [ PORT | hash ] ]\n"
		 "                              [ enable | disable ] ]\n"
		 "\n"
		 "        ip madcap show [ config | udp ] [ dev DEVICE ]\n"
		);

	exit (-1);
}


static int
parse_args (int argc, char ** argv, struct madcap_param *p)
{

	memset (p, 0, sizeof (*p));

	while (argc > 0) {
		if (strcmp (*argv, "dev") == 0) {
			NEXT_ARG ();
			p->ifindex = if_nametoindex (*argv);
			if (!p->ifindex) {
				invarg ("invalid device", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "offset") == 0) {
			NEXT_ARG ();
			if (get_u16 (&p->offset, *argv, 0)) {
				invarg ("invalid offset", *argv);
				exit (-1);
			}
			p->f_offset = 1;
		} else if (strcmp (*argv, "length") == 0) {
			NEXT_ARG ();
			if (get_u16 (&p->length, *argv, 0)) {
				invarg ("invalid length", *argv);
				exit (-1);
			}
			p->f_length = 1;
		} else if (strcmp (*argv, "proto") == 0) {
			NEXT_ARG ();
			if (strcmp (*argv, "udp") == 0) {
				p->proto = IPPROTO_UDP;
			} else if (strcmp (*argv, "ipip") == 0) {
				p->proto = IPPROTO_IPIP;
			} else if (strcmp (*argv, "gre") == 0) {
				p->proto = IPPROTO_GRE;
			} else 	if (get_u8 (&p->proto, *argv, 0)) {
				invarg ("proto", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "id") == 0) {
			NEXT_ARG ();
			if (get_u64 (&p->id, *argv, 0)) {
				invarg ("invalid id", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "dst") == 0) {
			NEXT_ARG ();
			if (inet_pton (AF_INET, *argv, &p->dst) < 1) {
				invarg ("invalid dst address", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "src") == 0) {
			NEXT_ARG ();
			if (inet_pton (AF_INET, *argv, &p->src) < 1) {
				invarg ("invalid src address", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "udp") == 0)
			p->udp = 1;
		else if (strcmp (*argv, "enable") == 0)
			p->enable = 1;
		else if (strcmp (*argv, "disable") == 0)
			p->disable = 1;
		else if (strcmp (*argv, "src-port") == 0) {
			NEXT_ARG ();
			if (strcmp (*argv, "hash") == 0) {
				p->src_port_hash = 1;
			} else {
				if (get_u16 (&p->src_port, *argv, 0)) {
					invarg ("invalid src-port", *argv);
					exit (-1);
				}
			}
		} else if (strcmp (*argv, "dst-port") == 0) {
			NEXT_ARG ();
			if (get_u16 (&p->dst_port, *argv, 0)) {
				invarg ("invalid dst-port", *argv);
				exit (-1);
			}
		} else if (strcmp (*argv, "config") == 0) {
			p->config = 1;
		}

		argc--;
		argv++;
	}

	return 0;
}

static int
do_add (int argc, char **argv)
{
	struct madcap_param p;
	struct madcap_obj_entry oe;

	parse_args (argc, argv, &p);

	if ((p.id == 0 && p.dst == 0) || p.ifindex == 0) {
		fprintf (stderr, "id, dst and dev must be specified\n");
		exit (-1);
	}

	memset (&oe, 0, sizeof (oe));
	oe.obj.id	= MADCAP_OBJ_ID_LLT_ENTRY;
	oe.obj.tb_id	= 0;	/* XXX */
	oe.id		= p.id;
	oe.dst		= p.dst;

	GENL_REQUEST (req, 1024, genl_family, 0, MADCAP_GENL_VERSION,
		      MADCAP_CMD_LLT_ENTRY_ADD, NLM_F_REQUEST | NLM_F_ACK);

	addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
	addattr_l (&req.n, 1024, MADCAP_ATTR_OBJ_ENTRY, &oe, sizeof (oe));

	if (rtnl_talk (&genl_rth, &req.n, NULL, 0) < 0)
		return -2;

	return 0;
}

static int
do_del (int argc, char **argv)
{
	struct madcap_param p;
	struct madcap_obj_entry oe;

	parse_args (argc, argv, &p);

	if ((p.id == 0 && p.dst == 0) || p.ifindex == 0) {
		fprintf (stderr, "id, dst and dev must be specified\n");
		exit (-1);
	}

	memset (&oe, 0, sizeof (oe));
	oe.obj.id	= MADCAP_OBJ_ID_LLT_ENTRY;
	oe.obj.tb_id	= 0;	/* XXX */
	oe.id		= p.id;
	oe.dst		= p.dst;

	GENL_REQUEST (req, 1024, genl_family, 0, MADCAP_GENL_VERSION,
		      MADCAP_CMD_LLT_ENTRY_DEL, NLM_F_REQUEST | NLM_F_ACK);

	addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
	addattr_l (&req.n, 1024, MADCAP_ATTR_OBJ_ENTRY, &oe, sizeof (oe));

	if (rtnl_talk (&genl_rth, &req.n, NULL, 0) < 0)
		return -2;

	return 0;
}

static int
do_set_udp (struct madcap_param p)
{
	struct madcap_obj_udp ou;

	memset (&ou, 0, sizeof (ou));
	ou.obj.id = MADCAP_OBJ_ID_UDP;

	if (p.enable)
		ou.encap_enable = 1;

	if (p.disable)
		ou.encap_enable = 0;

	if (p.src_port_hash)
		ou.src_hash_enable = 1;

	if (p.src_port)
		ou.src_port = htons (p.src_port);

	if (p.dst_port)
		ou.dst_port = htons (p.dst_port);

	GENL_REQUEST (req, 1024, genl_family, 0, MADCAP_GENL_VERSION,
		      MADCAP_CMD_UDP_CONFIG, NLM_F_REQUEST | NLM_F_ACK);

	if (p.ifindex == 0) {
		fprintf (stderr, "device must be specified\n");
		exit (-1);
	} else {
		addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
	}

	addattr_l (&req.n, 1024, MADCAP_ATTR_OBJ_UDP, &ou, sizeof (ou));

	if (rtnl_talk (&genl_rth, &req.n, NULL, 0) < 0)
		return -2;

	return 0;
}

static int
do_set (int argc, char **argv)
{
	struct madcap_param p;
	struct madcap_obj_config oc;

	parse_args (argc, argv, &p);

	if (p.udp) {
		return do_set_udp (p);
	}

	GENL_REQUEST (req, 1024, genl_family, 0, MADCAP_GENL_VERSION,
		      MADCAP_CMD_LLT_CONFIG, NLM_F_REQUEST | NLM_F_ACK);

	if (p.ifindex == 0) {
		fprintf (stderr, "device must be specified\n");
		exit (-1);
	} else {
		addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
	}

	if (p.f_offset && p.f_length) {
		/* config offset */
		memset (&oc, 0, sizeof (oc));
		oc.obj.id	= MADCAP_OBJ_ID_LLT_CONFIG;
		oc.obj.tb_id	= 0;	/* XXX */
		oc.offset	= p.offset;
		oc.length	= p.length;
		oc.proto	= p.proto;
		oc.src		= p.src;
		addattr_l (&req.n, 1024, MADCAP_ATTR_OBJ_CONFIG,
			   &oc, sizeof (oc));
	} else {
		fprintf (stderr, "offset and length must be specified\n");
		exit (-1);
	}

	if (rtnl_talk (&genl_rth, &req.n, NULL, 0) < 0)
		return -2;

	return 0;
}

static int
obj_entry_nlmsg (const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	int len;
	__u32 ifindex;
	char dev[IF_NAMESIZE] = { 0, };
	char dst[16] = { 0, };
	struct genlmsghdr *ghdr;
	struct rtattr *attrs[MADCAP_ATTR_MAX + 1];
	struct madcap_obj_entry oe;
	
	ghdr = NLMSG_DATA (n);
	len = n->nlmsg_len - NLMSG_LENGTH (sizeof (*ghdr));
	if (len < 0)
		return -1;
	parse_rtattr (attrs, MADCAP_ATTR_MAX, (void *)ghdr + GENL_HDRLEN, len);

	if (!attrs[MADCAP_ATTR_OBJ_ENTRY])
		return -1;

	if (!attrs[MADCAP_ATTR_IFINDEX])
		return -1;

	ifindex = rta_getattr_u32 (attrs[MADCAP_ATTR_IFINDEX]);
	if_indextoname (ifindex, dev);

	memcpy (&oe, RTA_DATA (attrs[MADCAP_ATTR_OBJ_ENTRY]), sizeof (oe));
	inet_ntop (AF_INET, &oe.dst, dst, sizeof (dst));
	
	fprintf (stdout, "dev %s id %llu dst %s\n", dev, oe.id, dst);
	return 0;
}

static int
obj_config_nlmsg (const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	int len;
	__u32 ifindex;
	char dev[IF_NAMESIZE] = { 0, }, addr[16] = { 0, };
	struct genlmsghdr *ghdr;
	struct rtattr *attrs[MADCAP_ATTR_MAX + 1];
	struct madcap_obj_config oc;

	ghdr = NLMSG_DATA (n);
	len = n->nlmsg_len - NLMSG_LENGTH (sizeof (*ghdr));
	if (len < 0)
		return -1;

	parse_rtattr (attrs, MADCAP_ATTR_MAX, (void *)ghdr + GENL_HDRLEN, len);

	if (!attrs[MADCAP_ATTR_OBJ_CONFIG])
		return -1;

	if (!attrs[MADCAP_ATTR_IFINDEX])
		return -1;

	ifindex = rta_getattr_u32 (attrs[MADCAP_ATTR_IFINDEX]);
	if_indextoname (ifindex, dev);

	memcpy (&oc, RTA_DATA (attrs[MADCAP_ATTR_OBJ_CONFIG]), sizeof (oc));
	inet_ntop (AF_INET, &oc.src, addr, sizeof (addr));

	fprintf (stdout, "dev %s offset %u length %u proto %u src %s\n",
		 dev, oc.offset, oc.length, oc.proto, addr);

	return 0;
}

static int
obj_udp_nlmsg (const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	int len;
	__u32 ifindex;
	char dev[IF_NAMESIZE] = { 0, };
	struct genlmsghdr *ghdr;
	struct rtattr *attrs[MADCAP_ATTR_MAX + 1];
	struct madcap_obj_udp ou;

	ghdr = NLMSG_DATA (n);
	len = n->nlmsg_len - NLMSG_LENGTH (sizeof (*ghdr));
	if (len < 0)
		return -1;

	parse_rtattr (attrs, MADCAP_ATTR_MAX, (void *)ghdr + GENL_HDRLEN, len);

	if (!attrs[MADCAP_ATTR_OBJ_UDP])
		return -1;

	if (!attrs[MADCAP_ATTR_IFINDEX])
		return -1;

	ifindex = rta_getattr_u32 (attrs[MADCAP_ATTR_IFINDEX]);
	if_indextoname (ifindex, dev);

	memcpy (&ou, RTA_DATA (attrs[MADCAP_ATTR_OBJ_UDP]), sizeof (ou));

	if (!ou.encap_enable) {
		fprintf (stdout, "dev %s udp disable\n", dev);
	} else {
		char dst_port[8], src_port[8];
		sprintf (dst_port, "%d", ntohs (ou.dst_port));
		if (ou.src_hash_enable)
			sprintf (src_port, "hash");
		else
			sprintf (src_port, "%d", ntohs (ou.src_port));

		fprintf (stdout, "dev %s udp enable dst-port %s src-port %s\n",
			 dev, dst_port, src_port);
	}

	return 0;
}

static int
do_show_config (struct madcap_param p)
{
	int ret;

	GENL_REQUEST (req, 2048, genl_family, 0,
		      MADCAP_GENL_VERSION, MADCAP_CMD_LLT_CONFIG_GET,
		      NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST);

	if (p.ifindex) {
		addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
		req.n.nlmsg_seq = genl_rth.dump = ++genl_rth.seq;
	}
	ret = rtnl_send (&genl_rth, &req.n, req.n.nlmsg_len);
	if (ret < 0) {
		fprintf (stderr, "%s:%d: error\n", __func__, __LINE__);
		return -2;
	}

	if (rtnl_dump_filter (&genl_rth, obj_config_nlmsg, NULL) < 0) {
		fprintf (stderr, "Dump terminated\n");
		exit (1);
	}

	return 0;
}

static int
do_show_udp (struct madcap_param p)
{
	int ret;

	GENL_REQUEST (req, 2048, genl_family, 0,
		      MADCAP_GENL_VERSION, MADCAP_CMD_UDP_CONFIG_GET,
		      NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST);

	if (p.ifindex) {
		addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
		req.n.nlmsg_seq = genl_rth.dump = ++genl_rth.seq;
	}
	ret = rtnl_send (&genl_rth, &req.n, req.n.nlmsg_len);
	if (ret < 0) {
		fprintf (stderr, "%s:%d: error\n", __func__, __LINE__);
		return -2;
	}

	if (rtnl_dump_filter (&genl_rth, obj_udp_nlmsg, NULL) < 0) {
		fprintf (stderr, "Dump terminated\n");
		exit (1);
	}

	return 0;
}

static int
do_show (int argc, char **argv)
{
	int ret;
	struct madcap_param p;

	/* XXX: I have to know abount current usage of dump API.
	 * addattr to dump request can be handled in .dumpit ?
	 * show config ? or ip -d link show ?
	 */

	parse_args (argc, argv, &p);
	
	if (p.config)
		return do_show_config (p);

	if (p.udp)
		return do_show_udp (p);


	GENL_REQUEST (req, 2048, genl_family, 0,
		      MADCAP_GENL_VERSION, MADCAP_CMD_LLT_ENTRY_GET,
		      NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST);

	if (p.ifindex) {
		addattr32 (&req.n, 1024, MADCAP_ATTR_IFINDEX, p.ifindex);
		req.n.nlmsg_seq = genl_rth.dump = ++genl_rth.seq;
	}

	ret = rtnl_send (&genl_rth, &req.n, req.n.nlmsg_len);
	if (ret < 0) {
		fprintf (stderr, "%s:%d: error\n", __func__, __LINE__);
		return -2;
	}

	if (rtnl_dump_filter (&genl_rth, obj_entry_nlmsg, &p) < 0) {
		fprintf (stderr, "Dump terminated\n");
		exit (1);
	}

	return 0;
}

int
do_ipmadcap (int argc, char **argv)
{
	if (genl_family < 0) {
		if (rtnl_open_byproto (&genl_rth, 0, NETLINK_GENERIC) < 0) {
			fprintf (stderr, "Can't open genetlink socket\n");
			exit (1);
		}

		genl_family = genl_resolve_family (&genl_rth,
						   MADCAP_GENL_NAME);
		if (genl_family < 0)
			exit (1);
	}

	if (argc < 1)
		usage ();

	if (matches (*argv, "add") == 0)
		return do_add (argc - 1, argv + 1);
	if (matches (*argv, "del") == 0 || matches (*argv, "delete") == 0)
		return do_del (argc - 1, argv + 1);
	if (matches (*argv, "set") == 0)
		return do_set (argc - 1, argv + 1);
	if (matches (*argv, "show") == 0)
		return do_show (argc -1, argv + 1);
	if (matches (*argv, "help") == 0)
		usage ();
		
	fprintf (stderr,
		 "Command \"%s\" is unknown, try \"ip madcap help\".\n",
		 *argv);

	return -1;
}
