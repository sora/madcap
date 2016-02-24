
/* raven.h */

#ifndef _RAVEN_H_
#define _RAVEN_H_

/* raven IFLA parameter */
enum {
	IFLA_RAVEN_UNSPEC,
	IFLA_RAVEN_PHYSICAL_DEV,	/* ifindex of physical device
					 * to TX ip encaped packet */
	__IFLA_RAVEN_MAX
};
#define IFLA_RAVEN_MAX (__IFLA_RAVEN_MAX -1)


#endif /* _RAVEN_H_ */
