/*
 * OpenBSC mobile network call control client
 * Nicolas Bouliane <nicboul@gmail.com>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <unistd.h>

#include "mncc.h"

int main()
{
	struct sockaddr_un sunaddr;
	int fd, rc;

	fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (fd < 0) {
		fprintf(stderr, "Cannot create SEQPACKET socket\n");
		return fd;
	}

	sunaddr.sun_family = AF_LOCAL;
	strncpy(sunaddr.sun_path, "/tmp/bsc_mncc", sizeof(sunaddr.sun_path));

	rc = connect(fd, (struct sockaddr *)&sunaddr, sizeof(sunaddr));
	if (rc < 0) {
		fprintf(stderr, "Could not connect to mncc socket\n");
	}

	static char buf[sizeof(struct gsm_mncc)+1024];
	struct gsm_mncc *mncc_prim = (struct gsm_mncc *)buf;

	while(1) {
		sleep(1);
		memset(buf, 0, sizeof(buf));
		rc = recv(fd, buf, sizeof(buf), 0);
		printf("recv: %d\n", rc);
		printf("type: %d\n", mncc_prim->msg_type);
		printf("callref: %d\n", mncc_prim->callref);
		printf("fields: %d\n", mncc_prim->fields);

		switch (mncc_prim->msg_type) {
		case MNCC_F_SSVERSION:
			printf("ssversion: %d:%s\n", mncc_prim->ssversion.len, mncc_prim->ssversion.info);
		}
	}
}
