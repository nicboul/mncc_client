/*
 * OpenBSC mobile network call control client
 * Nicolas Bouliane <nicboul@gmail.com>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <unistd.h>

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

	while(1) {
		sleep(1);
	}
}
