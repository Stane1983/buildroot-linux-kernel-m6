#ifndef  AM_PROBE_H_
#define AM_PROBE_H_
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/device.h>

#define KPB_CLASS_NAME   "amkpb"
/*the must probe points that we can trace*/
#define MAX_PB_NUM              20
#define POS_PRE         0
#define POS_POST       1
typedef struct kobj_type  kobj_type_t;

typedef  struct {
struct class * base_class;
char  symbol[KSYM_NAME_LEN];
int     pre_enable;
int     post_enable;
int     hw_msg_enable;
char    *pre_msg;
char    *post_msg;
char    *uhelper;
int        index;
struct kprobe kp;
struct kobject  kobj;
struct device *dev;
}am_kpb_t;

typedef  struct{
int  pb_num;
struct class * base_class;
dev_t   kbp_devid;
am_kpb_t    *pb_points[MAX_PB_NUM+1];
struct dentry *dbg_dir;
}am_kpb_mgr_t;

#define   kprobe_to_am_kpb(x)  container_of(x,am_kpb_t,kp)

static am_kpb_t* init_one_pb_point(int index,am_kpb_t *am_kpb);
static int  deinit_one_pb_point(int index,am_kpb_t *am_kpb);
extern int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count,const char *name);
#endif
