/*
 * OpenBSC mobile network call control client
 * Nicolas Bouliane <nicboul@gmail.com>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mncc.h"

int fd;

void send_mncc(void *data)
{
        sendto(fd, data, sizeof(struct gsm_mncc), 0, 0, 0);
}

struct gsm_mncc *create_mncc(int msg_type, unsigned int callref)
{
        struct gsm_mncc *mncc;

        mncc = (struct gsm_mncc *)malloc(sizeof(struct gsm_mncc));
        mncc->msg_type = msg_type;
        mncc->callref = callref;

        return mncc;
}

void setup_ind(struct gsm_mncc *mncc_prim)
{
        struct gsm_mncc *mode = NULL;
        struct gsm_mncc *proceeding = NULL;
        struct gsm_mncc *frame = NULL;
        struct gsm_mncc *reject = NULL;

        if (0) { /* if something wrong happen */
                reject = create_mncc(MNCC_REJ_REQ, mncc_prim->callref);
                reject->fields |= MNCC_F_CAUSE;
                reject->cause.coding = 3;
                reject->cause.location = 1;
                reject->cause.value = 47;
                send_mncc(reject);

                return;
        }

        mode = create_mncc(MNCC_LCHAN_MODIFY, mncc_prim->callref);
        mode->lchan_mode = 0x01; /* GSM V1 */
        send_mncc(mode);
        proceeding = create_mncc(MNCC_CALL_PROC_REQ, mncc_prim->callref);
        proceeding->fields |= MNCC_F_PROGRESS;
        proceeding->progress.coding = 3; /* GSM */
        proceeding->progress.location = 1;
        proceeding->progress.descr = 8;
        send_mncc(proceeding);

        frame = create_mncc(MNCC_FRAME_RECV, mncc_prim->callref);
        send_mncc(frame);
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

                sleep(1);
                memset(buf, 0, sizeof(buf));
                rc = recv(fd, buf, sizeof(buf), 0);

                printf("recv: %d\n", rc);
                printf("callref: %u\n", mncc_prim->callref);

                switch (mncc_prim->msg_type) {
                case MNCC_F_SSVERSION:
                        printf("ssversion: %d:%s\n", mncc_prim->ssversion.len, mncc_prim->ssversion.info);
                        break;
                case MNCC_SETUP_IND:
                        setup_ind(mncc_prim);
                        break;
                case MNCC_START_DTMF_IND:
                        printf("MNCC_START_DTMF_IND\n");
                        break;
                case MNCC_STOP_DTMF_IND:
                        printf("MNCC_STOP_DTMF_IND\n");
                        break;
                case MNCC_CALL_CONF_IND:
                        printf("MNCC_CALL_CONF_IND\n");
                        break;
                case MNCC_ALERT_IND:
                        printf("MNCC_ALERT_IND\n");
                        break;
                case MNCC_SETUP_CNF:
                        printf("MNCC_SETUP_CNF\n");
                        break;
                case MNCC_SETUP_COMPL_IND:
                        printf("MNCC_SETUP_COMPL_IND\n");
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

