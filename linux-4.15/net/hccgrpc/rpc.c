/*
 *  Copyright (C) 2019 Innogrid
 */

/*  writen by cgs 2019 */

#include <linux/module.h>
#include <net/hccgrpc/rpc.h>

int init_rpc(void){

    int res;

    printk("HCC: init_rpc");

    res = comlayer_init();
    if(res)
        return res;

    return 0;
}