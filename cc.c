/*
 * OpenBSC mobile network call control client
 * Nicolas J. Bouliane <nicboul@gmail.com>
 */

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mncc.h"

int fd;

struct gsm_call {
	uint32_t callref;
	uint32_t remote_ref;
} ms1, ms2;

static uint32_t new_callref = 0x00000001;

void send_mncc(void *data)
{
	sendto(fd, data, sizeof(struct gsm_mncc), 0, 0, 0);
	free(data);
}

struct gsm_mncc *create_mncc(int msg_type, unsigned int callref)
{
	struct gsm_mncc *mncc;

	mncc = (struct gsm_mncc *)malloc(sizeof(struct gsm_mncc));
	mncc->msg_type = msg_type;
	mncc->callref = callref;

	return mncc;
}

void setup_compl_ind(struct gsm_mncc *mncc_prim)
{
	struct gsm_mncc *mncc;

	/* modify mode */
	mncc = create_mncc(MNCC_LCHAN_MODIFY, mncc_prim->callref);
	mncc->lchan_mode = 0x01; /* GSM V1 */
	mncc->lchan_type = 0x02;
	send_mncc(mncc);
}

void mncc_alert_ind(struct gsm_mncc *mncc_prim)
{
	struct gsm_mncc *mncc;

	/* send alerting to remote */
	if (mncc_prim->callref == ms1.callref)
		mncc = create_mncc(MNCC_ALERT_REQ, ms2.callref);
	else
		mncc = create_mncc(MNCC_ALERT_REQ, ms1.callref);

	send_mncc(mncc);
}

void setup_cnf(struct gsm_mncc *mncc_prim)
{
	struct gsm_mncc *mncc = NULL;

	mncc = create_mncc(MNCC_SETUP_COMPL_REQ, mncc_prim->callref);
	send_mncc(mncc);

	mncc = create_mncc(MNCC_SETUP_RSP, ms1.callref);
	send_mncc(mncc);

	/* XXX That should bridge them ? */
	mncc = create_mncc(MNCC_FRAME_RECV, mncc_prim->callref);
	send_mncc(mncc);

	mncc = create_mncc(MNCC_FRAME_RECV, ms1.callref);
	send_mncc(mncc);
}

void call_conf_ind(struct gsm_mncc *mncc_prim)
{
	struct gsm_mncc *mncc = NULL;
	/* modify logical channel to GSM v1 */
	mncc = create_mncc(MNCC_LCHAN_MODIFY, mncc_prim->callref);
	mncc->lchan_mode = 0x01;
	send_mncc(mncc);
}

void tchf_recv(struct gsm_data_frame *mncc_prim)
{
	printf("GSM TCHF Frame from %u\n", mncc_prim->callref);
}

void setup_ind(struct gsm_mncc *mncc_prim)
{
	struct gsm_mncc *mncc = NULL;

	printf("called.type: %x\n", mncc_prim->called.type);
	printf("called.plan: %x\n", mncc_prim->called.plan);
	printf("called.present: %x\n", mncc_prim->called.present);
	printf("called.screen: %x\n", mncc_prim->called.screen);
	printf("called.number: %s\n", mncc_prim->called.number);

	printf("calling.type: %x\n", mncc_prim->calling.type);
	printf("calling.plan: %x\n", mncc_prim->calling.plan);
	printf("calling.present: %x\n", mncc_prim->calling.present);
	printf("calling.screen: %x\n", mncc_prim->calling.screen);
	printf("calling.number: %s\n", mncc_prim->calling.number);

	ms1.callref = mncc_prim->callref;
	ms2.callref = new_callref++;

	ms1.remote_ref = ms2.callref;
	ms2.remote_ref = ms1.callref;

	printf("ms1.callref %d\n", ms1.callref);
	printf("ms2.callref %d\n", ms2.callref);

	/* modify logical channel to GSM v1 */
	mncc = create_mncc(MNCC_LCHAN_MODIFY, ms1.callref);
	mncc->lchan_mode = 0x01;
	send_mncc(mncc);

	/* 5.2.1.1.3 */
	mncc = create_mncc(MNCC_CALL_PROC_REQ, ms1.callref);
	send_mncc(mncc);

	/* setup the called party */
	mncc = create_mncc(MNCC_SETUP_REQ, ms2.callref);
	send_mncc(mncc);
}

int main()
{
	struct sockaddr_un sunaddr;
	int rc;

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

		memset(buf, 0, sizeof(buf));
		rc = recv(fd, buf, sizeof(buf), 0);

		printf("recv: %d\n", rc);
		printf("callref: %u\n", mncc_prim->callref);

		switch (mncc_prim->msg_type) {
		case MNCC_F_SSVERSION:
			printf("ssversion: %d:%s\n", mncc_prim->ssversion.len, mncc_prim->ssversion.info);
			break;
		case MNCC_SETUP_IND:
			printf("MCC_SETUP_IND\n");
			setup_ind(mncc_prim);
			break;
		case MNCC_CALL_CONF_IND:
			printf("MNCC_CALL_CONF_IND\n");
			call_conf_ind(mncc_prim);
			break;
		case MNCC_SETUP_CNF:
			printf("MNCC_SETUP_CNF\n");
			setup_cnf(mncc_prim);
			break;
		case GSM_TCHF_FRAME:
			printf("GSM_TCHF_FRAME\n");
			tchf_recv((struct gsm_data_frame *)mncc_prim);
			break;
		case MNCC_START_DTMF_IND:
			printf("MNCC_START_DTMF_IND\n");
			break;
		case MNCC_STOP_DTMF_IND:
			printf("MNCC_STOP_DTMF_IND\n");
			break;
		case MNCC_ALERT_IND:
			printf("MNCC_ALERT_IND\n");
			mncc_alert_ind(mncc_prim);
			break;
		case MNCC_SETUP_COMPL_IND:
			printf("MNCC_SETUP_COMPL_IND\n");
			setup_compl_ind(mncc_prim);
			break;
		case MNCC_DISC_IND:
			printf("MNCC_DISC_IND\n");
			break;
		case MNCC_REL_IND:
			printf("MNCC_REL_IND\n");
			break;
		case MNCC_REL_CNF:
			printf("MNCC_REL_CNF\n");
			break;
		case MNCC_REJ_IND:
			printf("MNCC_REJ_IND\n");
			break;
		case MNCC_NOTIFY_IND:
			printf("MNCC_NOTIFY_IND\n");
			break;
		case MNCC_HOLD_IND:
			printf("MNCC_HOLD_IND\n");
			break;
		case MNCC_RETRIEVE_IND:
			printf("MNCC_RETRIEVE_IND\n");
			break;
		default:
			printf("MNCC unknown [%u]\n", mncc_prim->msg_type);
		}
	}
}
