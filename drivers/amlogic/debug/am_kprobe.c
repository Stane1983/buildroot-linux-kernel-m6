/*
 * Copyright (C) 2012 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author: amlogic SW [platform  BJ]	
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/debugfs.h>
#include <linux/kmod.h>
#include "am_kprobe.h"

static  am_kpb_mgr_t  am_kpb_mgr;
/* For each probe you need to allocate a kprobe structure */


static ssize_t  symbol_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb) return 0;     
       return sprintf(buf,"[symbol name]:%s  [addr]:0x%p\r\n",am_kpb->kp.symbol_name,am_kpb->kp.addr);
}
static int register_symbol(am_kpb_t *am_kpb,const char *buf,int count)
{
       int ret;
            
       if(NULL!=am_kpb->kp.addr)
       {
            unregister_kprobe(&am_kpb->kp);
            am_kpb->kp.addr=NULL;
            am_kpb->kp.flags=0;
       }
      if(NULL!=buf)
      {
            memset(am_kpb->symbol,0, KSYM_NAME_LEN);
            strncpy(am_kpb->symbol,buf,count-1);
            strcpy(am_kpb->symbol,strim(am_kpb->symbol));
            am_kpb->kp.symbol_name=(const char*)am_kpb->symbol;
      }
      if((ret=register_kprobe(&am_kpb->kp))<0)
      {
            printk("register kprobe failed,%d\n",ret);
            am_kpb->kp.addr=NULL;
      }
      return 0;
}
static ssize_t symbol_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  
  
       if(NULL==am_kpb) return 0;
       if(NULL==buf) return 0;
       register_symbol(am_kpb,buf,count);
       return count;
}

static ssize_t offset_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  
       unsigned  offset;

       if(NULL==am_kpb) return 0;
       offset=simple_strtoul(buf,NULL,0);
       printk("[offset]:%d\n",offset);
       am_kpb->kp.offset=offset;

       if(am_kpb->kp.symbol_name)
       register_symbol(am_kpb,NULL,0);

       return count;
}
static ssize_t  pre_msg_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  

       if(NULL==am_kpb) return 0;
       return sprintf(buf,"pre_msg:%s\r\n",am_kpb->pre_msg);
}
static ssize_t pre_msg_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb) return 0;
       if(NULL!=am_kpb->pre_msg) kfree(am_kpb->pre_msg);
       am_kpb->pre_msg=kzalloc(count-1,GFP_KERNEL);
       strncpy(am_kpb->pre_msg,buf,count-1 );
       am_kpb->pre_msg=strim(am_kpb->pre_msg);
       return count;
}
static ssize_t  post_msg_show(struct device *dev, struct device_attribute *attr,char *buf)
{
        am_kpb_t  *am_kpb=dev_get_drvdata(dev); 

        if(NULL==am_kpb) return 0;
        return sprintf(buf,"post_msg:%s\r\n",am_kpb->post_msg);
}
static ssize_t post_msg_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  

       if(NULL==am_kpb) return 0;
       if(NULL!=am_kpb->post_msg) kfree(am_kpb->post_msg);
       am_kpb->post_msg=kzalloc(count-1,GFP_KERNEL);
       strncpy(am_kpb->post_msg,buf,count-1 );
       am_kpb->post_msg=strim(am_kpb->post_msg);
       return count;
}

static ssize_t  pre_enable_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb) return 0;
       return sprintf(buf,"pre_enable:%d\r\n",am_kpb->pre_enable);
}
static ssize_t pre_enable_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev); 

       if(NULL==am_kpb) return 0;
       sscanf(buf, "%d", &am_kpb->pre_enable);    
       return count;
}
static ssize_t  post_enable_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb) return 0;  
       return sprintf(buf,"post_enable:%d\r\n",am_kpb->post_enable);
}
static ssize_t post_enable_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  

       if(NULL==am_kpb) return 0;
       sscanf(buf, "%d", &am_kpb->post_enable);            
       return count;
}
static ssize_t  hwm_enable_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb) return 0; 
       return sprintf(buf,"pre_enable:%d\r\n",am_kpb->hw_msg_enable);
}
static ssize_t  hwm_enable__store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  

       if(NULL==am_kpb) return 0;
       sscanf(buf, "%d", &am_kpb->hw_msg_enable);            
       return count;
}
static ssize_t  uhelper_show(struct device *dev, struct device_attribute *attr,char *buf)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);   

       if(NULL==am_kpb || NULL==am_kpb->uhelper ) return 0; 
       return sprintf(buf,"user mode helper:%s\r\n",am_kpb->uhelper );
}
static ssize_t  uhelper_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
       am_kpb_t  *am_kpb=dev_get_drvdata(dev);  

       if(NULL==am_kpb) return 0;
       printk("%s---%d\n",buf,count);
       if(am_kpb->uhelper) kfree(am_kpb->uhelper);
       am_kpb->uhelper=kzalloc(count-1,GFP_KERNEL);
       strncpy(am_kpb->uhelper,buf,count-1);
       strcpy(am_kpb->uhelper,strim(am_kpb->uhelper));
       if(strcmp(am_kpb->uhelper,"")==0 || count==1) 
       {
            am_kpb->uhelper=NULL;
            return count;
       }
       printk("tmpbuffer1:%s",am_kpb->uhelper);
       return count;
}
static struct device_attribute    kpb_attr[]={
__ATTR(offset, S_IRUGO|S_IWUSR, NULL, offset_store),
__ATTR(pre_msg, S_IRUGO|S_IWUSR, pre_msg_show, pre_msg_store),    
__ATTR(post_msg, S_IRUGO|S_IWUSR, post_msg_show, post_msg_store),       
__ATTR(symbol, S_IRUGO|S_IWUSR, symbol_show, symbol_store),
__ATTR(pre_enable, S_IRUGO|S_IWUSR, pre_enable_show, pre_enable_store),
__ATTR(post_enable, S_IRUGO|S_IWUSR, post_enable_show, post_enable_store),
__ATTR(hw_msg_enable, S_IRUGO|S_IWUSR, hwm_enable_show, hwm_enable__store),
__ATTR(usr_helper, S_IRUGO|S_IWUSR, uhelper_show, uhelper_store),
__ATTR_NULL
};

static ssize_t  max_pb_num_show(struct class *class, struct class_attribute *attr,char *buf)
{
       return sprintf(buf,"probe num:%d\r\n", am_kpb_mgr.pb_num);
}
static ssize_t max_pb_num_store(struct class *class, struct class_attribute *attr,const char *buf, size_t count)
{
       int number,i;
       sscanf(buf, "%d", &number); 
       if(MAX_PB_NUM < number) 
       {
            printk("cant create breakpoints more than %d\n",MAX_PB_NUM);
            return count;
       }
       /*create new probe point*/
       if(number==am_kpb_mgr.pb_num) return count;
       for(i=am_kpb_mgr.pb_num+1;i<=number;i++) //add extra new ones
       {
             
             am_kpb_mgr.pb_points[i]=init_one_pb_point(i,am_kpb_mgr.pb_points[i]);
       }
       for(i=am_kpb_mgr.pb_num;i>number;i--)    //delete unused ones
       {
            deinit_one_pb_point(i,am_kpb_mgr.pb_points[i]);
            am_kpb_mgr.pb_points[i]=NULL;
       }
       am_kpb_mgr.pb_num=number;
       return count;
}
static ssize_t addr2sym_store(struct class *class, struct class_attribute *attr,const char *buf, size_t count)
{
       ulong  addr;
       char symbol[KSYM_NAME_LEN];

       addr=simple_strtoul(buf,NULL,0);
       sprint_symbol(symbol,addr);
       printk("[addr]:0x%lx --[symbol]:%s\n",addr,symbol);
       return count;
}

static  struct  class_attribute   kpb_mgr_attr[]={
__ATTR(max_pb_num, S_IRUGO|S_IWUSR,max_pb_num_show,max_pb_num_store),
__ATTR(addr2sym, S_IRUGO|S_IWUSR, NULL, addr2sym_store),      
};
static inline void dump_message(am_kpb_t  *am_kpb,struct pt_regs *regs)
{
       struct kprobe *p=&am_kpb->kp;
       
       if(0==am_kpb->hw_msg_enable) return ;
       printk("dump cpu regs:\n");
       printk("pc: 0x%lx\t lr:0x%lx\n",regs->ARM_pc,regs->ARM_lr);
       printk("sp: 0x%lx\t ip:0x%lx\n",regs->ARM_sp,regs->ARM_ip);
       printk("fp:0x%lx\t r10:0x%lx\n",regs->ARM_fp,regs->ARM_r10);
       printk("r9:0x%lx\t r8:0x%lx\n",regs->ARM_r9,regs->ARM_r8);
       printk("r7:0x%lx\t r6:0x%lx\n",regs->ARM_r7,regs->ARM_r6);
       printk("r5:0x%lx\t r4:0x%lx\n",regs->ARM_r5,regs->ARM_r4);
       printk("r3:0x%lx\t r2:0x%lx\n",regs->ARM_r3,regs->ARM_r2);
       printk("r1:0x%lx\t r0:0x%lx\n",regs->ARM_r1,regs->ARM_r0);
       printk("dump kprobe msg:\n");
       printk("addr:0x%p \t symbol:%s\n",p->addr,p->symbol_name);
       printk("saved op code(rpl by bp):0x%x\n",p->opcode);
       printk("origin instr opcode:0x%p  handler:0x%p\n",p->ainsn.insn,p->ainsn.insn_handler);
}
static void call_uhelper(am_kpb_t  *am_kpb,char  pos)
{
       char *argv [3];
       int ret;
       struct kobj_uevent_env *env;
       const char  *subsystem;
       const char *devpath;
       
       subsystem = am_kpb->dev->kobj.name;
       //kobject_uevent(&am_kpb->dev->kobj, KOBJ_ADD);
       if(NULL== am_kpb->uhelper) return ;

       
       argv [0] = am_kpb->uhelper;
	argv [1] = (char *)subsystem;
	argv [2] = NULL;
       env = kzalloc(sizeof(struct kobj_uevent_env), GFP_KERNEL);
       //add_uevent_var(env, "ACTION=%s","add");
       add_uevent_var(env, "PREV_POST=%s", (pos==POS_POST)?"post":"prev");
       add_uevent_var(env, "SUBSYSTEM=%s", subsystem);
       devpath = kobject_get_path(&am_kpb->dev->kobj, GFP_KERNEL);
       add_uevent_var(env, "DEVPATH=%s", devpath);
       add_uevent_var(env, "SUBSYSTEM=%s", subsystem);
        ret = add_uevent_var(env, "HOME=/");
	if (ret)
	goto exit_helper;

       ret= add_uevent_var(env,
	            		"PATH=/sbin:/bin:/usr/sbin:/usr/bin");
      
	if (ret)
	goto exit_helper;
	ret = call_usermodehelper(argv[0], argv,
					     env->envp, UMH_WAIT_EXEC);
    exit_helper:
        kfree(env);
        return ;
}
/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
        am_kpb_t  *am_kpb=kprobe_to_am_kpb(p);

        if(NULL==am_kpb) return 0;
        if(0==am_kpb->pre_enable) return  0;
        if(am_kpb->pre_msg)
        printk("%s",am_kpb->pre_msg);
        dump_message(am_kpb,regs);
        call_uhelper(am_kpb,POS_PRE);
        return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
        am_kpb_t  *am_kpb=kprobe_to_am_kpb(p);

        if(NULL==am_kpb) return ;
        if(0==am_kpb->post_enable) return ;
        if(am_kpb->post_msg)
        printk("%s",am_kpb->post_msg);
        dump_message(am_kpb,regs);
        call_uhelper(am_kpb,POS_POST);           
        return ;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
       am_kpb_t  *am_kpb=kprobe_to_am_kpb(p);
       
       int  origin_hw_msg_enable=am_kpb->hw_msg_enable;
	printk(KERN_INFO "fault_handler: p->addr = 0x%p, trap #%dn",
		p->addr, trapnr);
	/*enable hw messge enable to see it*/
       am_kpb->hw_msg_enable=1;
       dump_message(am_kpb,regs);
       am_kpb->hw_msg_enable=origin_hw_msg_enable;
       /* Return 0 because we don't handle the fault. */
	return 0;
}
static int   deinit_one_pb_point(int index,am_kpb_t *am_kpb)
{
       int i ;
       const dev_t dev_id = MKDEV(MAJOR(am_kpb_mgr.kbp_devid), index);

       if(NULL==am_kpb) return -1;
       for(i=0;NULL!=kpb_attr[i].attr.name;i++)
       {
           device_remove_file(am_kpb->dev,&kpb_attr[i]);
       }
       device_destroy(am_kpb_mgr.base_class, dev_id);
       kfree(am_kpb);
       return 0;
}
static  am_kpb_t* init_one_pb_point(int index,am_kpb_t *am_kpb)
{
       am_kpb_t* p_am_kpb=NULL;
       int i=0;
       struct kobject *kobj;
        const dev_t dev_id = MKDEV(MAJOR(am_kpb_mgr.kbp_devid), index);
        
       if(am_kpb && NULL!=am_kpb->dev) return NULL;

       if(am_kpb) kfree(am_kpb);
       p_am_kpb=(am_kpb_t*)kmalloc(sizeof(am_kpb_t),GFP_KERNEL);
       if(NULL==p_am_kpb) 
       {
            printk("no memory for new kprobe obj");
            return NULL;  
       }
       p_am_kpb->kp.pre_handler = handler_pre;
	p_am_kpb->kp.post_handler = handler_post;
	p_am_kpb->kp.fault_handler = handler_fault;
       p_am_kpb->kp.addr=NULL;
       p_am_kpb->pre_enable=0;
       p_am_kpb->post_enable=0;
       p_am_kpb->hw_msg_enable=0;
       p_am_kpb->pre_msg=NULL;
       p_am_kpb->post_msg=NULL;
       p_am_kpb->index=index;
       
       kobj =&p_am_kpb->kobj ;
       if(NULL==kobj)
       {
            printk("no memory for create probe object\n");
            goto exit;
       }
        p_am_kpb->dev = device_create(am_kpb_mgr.base_class, NULL, dev_id, NULL, "breakpoint%d",index); //kernel>=2.6.27 
        for(i=0;NULL!=kpb_attr[i].attr.name;i++)
        {
            if(0>device_create_file(p_am_kpb->dev,&kpb_attr[i]))
            {
                printk("create file attr %s failed\n",kpb_attr[i].attr.name);
            }
        } 
        dev_set_drvdata(p_am_kpb->dev, p_am_kpb);
exit:       
       return p_am_kpb;
}
static int __init kprobe_init(void)
{
	int i;
       struct dentry *fd;

       printk("insert am kprobe module ok\n");
       /*create kprobe class*/
       i=alloc_chrdev_region(&am_kpb_mgr.kbp_devid, 0,1, KPB_CLASS_NAME);
       printk("dev_t for aml_kprobe:[%d:%d]\n",MAJOR(am_kpb_mgr.kbp_devid),MINOR(am_kpb_mgr.kbp_devid));
       if(i<0)
       {
            printk("can't alloc char device region for aml_kprobe\n");
            return -1;
       }
       am_kpb_mgr.base_class=class_create(THIS_MODULE,KPB_CLASS_NAME);
       if(NULL==am_kpb_mgr.base_class)
       {
            printk("can not create kpb base class \n");
            return -1;
       }
       am_kpb_mgr.pb_num=0;
       for(i=0;i<ARRAY_SIZE(kpb_mgr_attr);i++)
	{
		if ( class_create_file(am_kpb_mgr.base_class,&kpb_mgr_attr[i]))
		{
			printk("create kpb attribute %s fail\r\n",kpb_mgr_attr[i].attr.name);
		}
	}
       /*create debugfs interface here*/
       am_kpb_mgr.dbg_dir=debugfs_create_dir(KPB_CLASS_NAME,NULL);
       	fd = debugfs_create_u32("max_pb_num", S_IRUGO, am_kpb_mgr.dbg_dir,
		(u32 *)&am_kpb_mgr.pb_num);
        if(0 > fd )
        {
              printk("cant create debufs file max_pb_num\n ");
        }
       return 0;
}

static void __exit kprobe_exit(void)
{
       int i;
	printk(KERN_INFO "am  kprobe exit\n");
       debugfs_remove_recursive(am_kpb_mgr.dbg_dir);
       for(i=1;i <= am_kpb_mgr.pb_num;i++)    //delete unused ones
       {
            deinit_one_pb_point(i,am_kpb_mgr.pb_points[i]);
       }
       if(NULL==am_kpb_mgr.base_class) return ;
       	for(i=0;i<ARRAY_SIZE(kpb_mgr_attr);i++)
	{
		class_remove_file(am_kpb_mgr.base_class,&kpb_mgr_attr[i]) ;
	}
		
	class_destroy(am_kpb_mgr.base_class);
       unregister_chrdev_region(am_kpb_mgr.kbp_devid,1);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");


