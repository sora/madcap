/*
 * iplink_raven.c
 */

#include <stdio.h>
#include <string.h>
#include <net/if.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"
#include "../../include/raven.h"

static void
explain (void)
{
	fprintf (stderr, "Usage: ... nsh [ link DEVICE ]\n");
}

static int
raven_parse_opt (struct link_util *lu, int argc, char **argv,
		 struct nlmsghdr *n)
{
	__u32 ifindex = 0;

	while (argc > 0) {
		if (!matches (*argv, "help")) {
			explain ();
			exit (-1);
		} else if (!matches (*argv, "link")) {
			NEXT_ARG ();
			ifindex = if_nametoindex (*argv);
			if (ifindex < 1) {
				invarg ("invalid device", *argv);
				exit (-1);
			}
		}

		argc--;
		argv++;
	}

	if (ifindex)
		addattr32 (n, 1024, IFLA_RAVEN_PHYSICAL_DEV, ifindex);

	return 0;
}

static void
raven_print_opt (struct link_util *lu, FILE *f, struct rtattr *tb[])
{
	__u32 ifindex;
	char dev[IF_NAMESIZE];

	if (!tb)
		return;

	if (!tb[IFLA_RAVEN_PHYSICAL_DEV])
		return;

	ifindex = rta_getattr_u32 (tb[IFLA_RAVEN_PHYSICAL_DEV]);

	if (ifindex) {
		if_indextoname (ifindex, dev);
		fprintf (f, "link %s ", dev);
	} else
		fprintf (f, "link none ");

	return;
}

struct link_util raven_link_util = {
	.id		= "raven",
	.maxattr	= IFLA_RAVEN_MAX,
	.parse_opt	= raven_parse_opt,
	.print_opt	= raven_print_opt,
};
