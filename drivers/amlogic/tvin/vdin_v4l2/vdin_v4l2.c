#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/mm.h>

#include <linux/tvin/tvin_v4l2.h>
#include "vdin_v4l2.h"

static struct vdin_v4l2_ops_s ops = {NULL};

int vdin_reg_v4l2(vdin_v4l2_ops_t *v4l2_ops)
{
        void * ret = 0;
        if(!v4l2_ops)
                return -1;
        ret = memcpy(&ops,v4l2_ops,sizeof(vdin_v4l2_ops_t));
        if(ret)
                return 0;
        return -1;        
}
EXPORT_SYMBOL(vdin_reg_v4l2);

int v4l2_vdin_ops_init(vdin_v4l2_ops_t *vdin_v4l2p)
{
        void * ret = 0;
        if(!vdin_v4l2p)
                return -1;
        ret = memcpy(vdin_v4l2p,&ops,sizeof(vdin_v4l2_ops_t));
        if(ret)
                return 0;
        return -1;
}

vdin_v4l2_ops_t *get_vdin_v4l2_ops()
{
        if((ops.start_tvin_service != NULL) && (ops.stop_tvin_service != NULL))
                return &ops;
        else{
                pr_err("[vdin..]%s: vdin v4l2 operation haven't registered.",__func__);
                return NULL;
        }
}
