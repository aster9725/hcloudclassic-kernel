#include <linux/module.h>
#include <net/hccgrpc/rpc.h>

int init_rpc(void){

    int res = 0;

    printk("HCC: init_rpc");

    res = comlayer_init();
    if(res)
        return res;

    return 0;
}