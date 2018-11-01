#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
// #include <lib/string.c>


#define NETLINK_USER 31

struct sock *nl_sk = NULL;
int buflen = 0;

int optionS(struct task_struct *task, char* buf);
int optionP(struct task_struct *task, char* buf);
int optionC(struct task_struct *task, char* buf, int tabCount);

static void hello_nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg="";
    char buf[10000] = "";
    int res;
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
    /*my var space*/

    long mypid = DUPLEX_UNKNOWN;
    struct pid *pid_struct;
    struct task_struct *task;
    char test[100] = {};
    char* user_to_kernal = "";
    //init
    msg="";
    sprintf(buf, "");
    buflen = 0;

    user_to_kernal = (char*)nlmsg_data(nlh);
    printk(KERN_INFO "user_to_kernal: %s\n", user_to_kernal);
    char option = 'n';
    option = user_to_kernal[0];
    kstrtol(user_to_kernal+2, 10, &mypid);

    printk(KERN_INFO "option: %c mypid: %d\n",option, mypid);

    pid_struct = find_get_pid((int)mypid);
    task = pid_task(pid_struct,PIDTYPE_PID);

    if(option == 's') {
        optionS(task, buf);
    } else if(option == 'p') {
        optionP(task, buf);
    } else if(option == 'c') {
        buflen += sprintf(buf, "%s(%d)\n",task->comm,task->pid);
        optionC(task, buf, 1);
    }

    // sprintf(test,"name %s\n ",task->comm);
    printk(KERN_INFO "PIDDDDDD: %d\n", (int)mypid);
    // printk(KERN_INFO "testlen: %d\n", (int)strlen(test));

    // sprintf(buf,"buf %s\n ",task->comm);
    // printk(KERN_INFO "buflen: %d\n", (int)strlen(buf));
    // sprintf(buf,"buf %s\n ","123");
    // printk(KERN_INFO "buf: %s\n", buf);
    msg = buf;
    printk(KERN_INFO "buf: %s\n", buf);
    printk(KERN_INFO "MSG: %s\n", msg);

    msg_size=strlen(msg);


    pid = nlh->nlmsg_pid; /*pid of sending process */

    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out) {

        printk(KERN_ERR "Failed to allocate new skb\n");
        return;

    }
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh),msg,msg_size);

    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

int optionS(struct task_struct *task, char* buf)
{
    // struct task_struct *parent = task->parent;
    // return 0;
    int len = 0;
    struct task_struct *task1;
    struct list_head *list=NULL;
    // printk(KERN_INFO "%s", task->comm);

    list_for_each(list,&task->parent->children) {
        task1=list_entry(list,struct task_struct,sibling);
        if(task1->pid > 0 && task1!=task ) {
            len += sprintf(buf+len, "%s(%d)\n",task1->comm,task1->pid);
        }
    }
    return len;
}

int optionP(struct task_struct *task, char* buf)
{
    struct task_struct *t;
    int len = 0;
    t = task;
    int parentPids[50]= {0};
    int countParents = 0;
    struct pid *pid_struct;
    do {
        parentPids[countParents++] = (int)(t->pid);
        t = t->parent;
    } while (t->pid != 0);
    int spaceCount = 0;
    do {
        pid_struct = find_get_pid(parentPids[--countParents]);
        t = pid_task(pid_struct,PIDTYPE_PID);
        int i;
        for(i=0; i<spaceCount; ++i) {
            len += sprintf(buf+len, "    ");
        }
        ++spaceCount;
        len += sprintf(buf+len, "%s(%d)\n",t->comm,t->pid);
        printk(KERN_INFO "buf: %s %n\n",t->comm,t->pid);
    } while (countParents > 0);

}
int optionC(struct task_struct *task, char* buf, int tabCount)
{
    struct task_struct *task1;
    struct list_head *list=NULL;

    list_for_each(list,&task->children) {
        task1=list_entry(list,struct task_struct,sibling);
        if(task1->pid > 0) {
            int i;
            for(i=0; i<tabCount; ++i) {
                buflen += sprintf(buf+buflen, "    ");
            }
            buflen += sprintf(buf+buflen, "%s(%d)\n",task1->comm,task1->pid);
            printk("buf: %s\n",buf);
            optionC(task1, buf, tabCount+1);
        }
    }
    return buflen;
}

static int __init hello_init(void)
{

    printk("Entering: %s\n",__FUNCTION__);
//This is for 3.6 kernels and above.
    struct netlink_kernel_cfg cfg = {
        .input = hello_nl_recv_msg,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
//nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg,NULL,THIS_MODULE);
    if(!nl_sk) {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }

    return 0;
}

static void __exit hello_exit(void)
{

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
