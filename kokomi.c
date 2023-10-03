// SPDX-License-Identifier: GPL-2.0+
/******************************************************************************
 *  speedtch.c  -  Alcatel SpeedTouch NETLINK xDSL modem driver
 *
 *  Copyright (C) 2001, Alcatel
 *  Copyright (C) 2003, Duncan Sands
 *  Copyright (C) 2004, David Woodhouse
 *
 *  Based on "modem_run.c", copyright (C) 2001, Benoit Papillault
 ******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <net/sock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KOKOMI");
MODULE_DESCRIPTION("DESC MODULE");
MODULE_VERSION("0.1");

#define NETLINK_CUSTOM_PROTOCOL 31

struct process_info {
    pid_t pid; // 进程ID
    size_t virt_addr; // 虚拟地址
    size_t len; // 数据长度
};

struct sock *nl_sk = NULL; // Netlink套接字

struct process_info *process_info_data; // 进程信息数据

// 根据进程ID获取进程的内存管理结构体
static inline struct mm_struct *get_mm_by_pid(pid_t nr)
{
    struct task_struct *task;

    task = pid_task(find_vpid(nr), PIDTYPE_PID);
    if (!task)
        return NULL;

    return get_task_mm(task);
}

phys_addr_t translate_linear_address(struct mm_struct* mm, uintptr_t va) {

    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pte_t *pte;
    pud_t *pud;

    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if(pgd_none(*pgd) || pgd_bad(*pgd)) {
        return 0;
    }
    p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        return 0;
    }
    pud = pud_offset(p4d,va);
    if(pud_none(*pud) || pud_bad(*pud)) {
        return 0;
    }
    pmd = pmd_offset(pud,va);
    if(pmd_none(*pmd)) {
        return 0;
    }
    pte = pte_offset_kernel(pmd,va);
    if(pte_none(*pte)) {
        return 0;
    }
    if(!pte_present(*pte)) {
        return 0;
    }
    //页物理地址
    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    //页内偏移
    page_offset = va & (PAGE_SIZE-1);

    return page_addr + page_offset;
}

// 接收Netlink消息的回调函数
static void nl_recv_msg(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    int pid;
    int send_pid;
    struct sk_buff *skb_out;
    int res;
    size_t virt_addr;
    phys_addr_t phys_addr;
    void* kernel_addr;
    size_t len;
    struct mm_struct *mm;
    int err = 0;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *)skb->data;

    send_pid = nlh->nlmsg_pid; // 发送方进程的PID

    // 解析虚拟地址和偏移量
    process_info_data = (struct process_info *)nlmsg_data(nlh);
    virt_addr = process_info_data->virt_addr;
    pid = process_info_data->pid;
    len = process_info_data->len;

    printk(KERN_ERR "v2p: send pid: %d,pid: %d,virt_addr: %lx,len: %ld\n",send_pid,pid,virt_addr,len);
    mm = get_mm_by_pid(pid);

    if (!mm) {
        printk(KERN_ERR "v2p: 获取mm 出错\n");
        return;
    }

    phys_addr = translate_linear_address(mm,virt_addr);

    mmput(mm); // 移到这里

    if (!phys_addr) {
        printk(KERN_ERR "v2p: 获取phys_addr 出错\n");
        return;
    }

    if (!pfn_valid(__phys_to_pfn(phys_addr))) {
        return;
    }
    kernel_addr = ioremap_cache(phys_addr, len);

    if(!kernel_addr){
        printk(KERN_ERR "v2p: 获取read_physical_address 出错\n");
        return;
    }

    printk(KERN_ERR "v2p: kernel_addr: %d\n",*(int *)kernel_addr);

    skb_out = nlmsg_new(len, 0);
    if (!skb_out) {
        err = -ENOMEM;
        goto out_free;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, len, 0);
    if (!nlh) {
        err = -ENOMEM;
        goto out_free_skb;
    }

    memcpy(nlmsg_data(nlh), kernel_addr, len);
    iounmap(kernel_addr);

    res = nlmsg_unicast(nl_sk, skb_out, send_pid);
    if (res < 0) {
        err = res;
        goto out_free_skb;
    }
out_free:
    return;

out_free_skb:
    nlmsg_free(skb_out);
}

// 模块初始化函数
static int __init dev_init(void) {

    printk(KERN_INFO "Loading netlink_virt_to_phys module...\n");

    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    }; // Netlink内核配置

    nl_sk = netlink_kernel_create(&init_net, NETLINK_CUSTOM_PROTOCOL, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Failed to create netlink socket.\n");
        return -10;
    }
    printk(KERN_INFO "Netlink socket created.\n");
    return 0;
}

// 模块卸载函数
static void __exit dev_exit(void) {
    if (nl_sk) {
        netlink_kernel_release(nl_sk);
        nl_sk = NULL;  // 将指针设为 NULL，避免重复释放
    }
    printk(KERN_INFO "Unloading netlink_virt_to_phys module...\n");
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_DESCRIPTION("DRIVER_DESC");
MODULE_INFO(intree, "Y");
MODULE_INFO(scmversion, "g35fcd5da188a");
