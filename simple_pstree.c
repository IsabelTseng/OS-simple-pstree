#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 20024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

int main(int argc, char* argv[])
{
    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
        return -1;

    // printf("before test\n");
    // char* test="c1";
    // char* pid="";
    char* option = "";
    if(argc == 1) {
        option = "c 1";
    } else if(argc == 2) {
        char msg_to_kernal[12];  //in order not to overflow when sprintf
        if(argv[1][0] == '-') {
            // char* delim = "-";
            if(strlen(argv[1])==2) {
                // option = strtok(argv[1], delim);
                if(argv[1][1] == 'c') {
                    sprintf(msg_to_kernal, "c %d", 1);
                } else {
                    int defaultPid = getpid();
                    sprintf(msg_to_kernal, "%c %d",argv[1][1], defaultPid);
                }
            } else {
                sprintf(msg_to_kernal, "%c %s",argv[1][1], argv[1]+2);
            }
        } else if(argv[1][0]>='0' && argv[1][0]<='9') {
            sprintf(msg_to_kernal, "c %s", argv[1]);
        }
        option = msg_to_kernal;
    }
    // printf("option: %s\n",option);

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy(NLMSG_DATA(nlh), option);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);
    // printf("Waiting for message from kernel\n");

    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    char* testresult = (char *)NLMSG_DATA(nlh);
    int i;
    int flag = 1;
    for(i = 0; i<strlen(option); ++i) {
        if(strlen(testresult)!=strlen(option))break;
        flag = 0;
        if(testresult[i]!=option[i]) {
            flag = 1;
            break;
        }
    }
    if(flag==1) {
        printf("%s", (char *)NLMSG_DATA(nlh));
    }
    close(sock_fd);
}
