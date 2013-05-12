/*
 * Amlogic general gpio driver
 *
 *
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
 *  docment for gpio usage  :
 *        http://openlinux.amlogic.com/wiki/index.php/GPIO
 */


#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/uio_driver.h>  //mmap to user space . to raise frequency.

#include <linux/io.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/vout/vinfo.h>
#include <mach/am_regs.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <mach/gpio_data.h>
#include <mach/pinmux.h>
#include "am_gpio.h"
#include "am_gpio_log.h"
#include <linux/amlog.h>
MODULE_AMLOG(AMLOG_DEFAULT_LEVEL, 0xff, LOG_LEVEL_DESC, LOG_MASK_DESC);
int gpio_out_status(char *name,int bit,uint32_t ghigh)
{
	bool high;
		uint32_t pin;
		char p[5] = "GPIO";
		char cmd[10];	
		sprintf(&cmd,"%s%s_%d",p,name,bit);
		pin = get_pin_num(&cmd);
		printk(KERN_DEBUG "cmd=%s pin = %d ghigh = %d\n",cmd,pin,ghigh);
		if(pin != -1){
			if(ghigh == 1){
				high = true;
					return gpio_out(pin,high);
			}
			else if(ghigh == 0){
				high = false;
					return gpio_out(pin,high);
			}
			else if(ghigh == -1)
			{
				return gpio_get_val(pin);
			}
			else{
				printk(KERN_ERR "enter error!\n");
			}
		}
		else{
			printk(KERN_ERR "you input error!\n");
			return -1;	
		}
}
static inline int _gpio_run_cmd(const char *ex_cmd, cmd_t *op)
{

	char *cmd;
//	if((strlen(ex_cmd) < 6)||(strlen(ex_cmd) > 10))
//	{
//		printk(KERN_ERR " YOU ENTER ERROR!!\n");
//			return -1;
//	}
	//trim space out
	cmd = skip_spaces(ex_cmd);
		if (isdigit(cmd[4])) 
		{
			if(isdigit(cmd[5])){
				op->bit = (cmd[4] - '0') * 10 + cmd[5] - '0';
					op->val = cmd[7] - '0';
			}
			else{
				op->bit = cmd[4] - '0';
					op->val = cmd[6] - '0';
			}
			//op->name,cmd[2]);
			op->name[0] = cmd[2];
			op->name[1] = '\0';
				switch (cmd[0]|0x20)
				{
					case 'w':
					case 'W':
							op->cmd_flag = 0;
						 return gpio_out_status(op->name,op->bit,op->val);
					case 'r':
					case 'R':
						    op->cmd_flag = 1;
						 return gpio_out_status(op->name,op->bit,-1);
					default:
						printk(KERN_ERR "_gpio_run_cmd cmd error \n");
							return -1;
				}
		}
		else if(cmd[4] == '_'){
			if(isdigit(cmd[6]))
			{
				op->bit = (cmd[5] - '0') * 10 + cmd[6] - '0';
					op->val = cmd[8] - '0';
			}
			else{
				op->bit = cmd[5] - '0';
					op->val = cmd[7] - '0';
			}
			op->name[0] = cmd[2];
			op->name[1] = cmd[3];
			op->name[2] = '\0';
				switch (cmd[0]|0x20)
				{
					case 'w':
					case 'W':
							op->cmd_flag = 0;
						 return gpio_out_status(op->name,op->bit,op->val);
					case 'r':
					case 'R':
						    op->cmd_flag = 1;
						 return gpio_out_status(op->name,op->bit,-1);
					default:
						printk(KERN_ERR "_gpio_run_cmd cmd error \n");
							return -1;
				}
		}
	return 0;
}
static int am_gpio_open(struct inode * inode, struct file * file)
{
	cmd_t *op;
		
		op = (cmd_t*)kmalloc(sizeof(cmd_t),GFP_KERNEL);
		if (IS_ERR(op)) {
			amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"cant alloc an op_target struct\n");;
				return -1;
		}
	file->private_data = op;
		return 0;
}
static ssize_t am_gpio_read(struct file *file, char __user *buf, size_t size,loff_t*off)
{
	cmd_t *op=file->private_data;
		char tmp[10];
		char cmd[10];
		int val;
		
		if (buf == NULL || op == NULL)
			return -1;
				copy_from_user(tmp, buf, size);
				
				strcpy(cmd, "r ");
				strcat(cmd, tmp);
				val = _gpio_run_cmd(cmd, op);
				if (0 > val)
					return -1;
						sprintf(tmp, ":%d", val);
						strcat(cmd, tmp);
						return copy_to_user(buf, cmd, strlen(cmd));
}

static ssize_t am_gpio_write(struct file *file, const char __user *buf,
		size_t bytes, loff_t *off)
{
	cmd_t *op = file->private_data;
		char tmp[10];
		char cmd[10];
		int val;
		
		if(buf == NULL || op == NULL)
			return -1;
				copy_from_user(tmp, buf, bytes);
				
				strcpy(cmd,"w ");
				strcat(cmd,tmp);
				val = _gpio_run_cmd(cmd,op);
				if(0 > val)
					return -1;
						return strlen(cmd);
}

static int am_gpio_ioctl(struct file *file, unsigned int ctl_cmd, unsigned long arg)
{
	cmd_t *op=file->private_data;
		char cmd[10];
		iocmd_t op_target;
		iocmd_t ret_target;
		
		switch(ctl_cmd)
		{
			case GPIO_CMD_OP_W:
					 if(copy_from_user(&op_target, (iocmd_t*)arg, sizeof(iocmd_t)))
					 {
						 return -EFAULT;
					 }
					 memcpy(&ret_target, &op_target, sizeof(iocmd_t));
					 if(op_target.nameb != ' ')
						  sprintf(cmd,"w %c%c_%d %d",op_target.namef,op_target.nameb,op_target.bit, op_target.val);
					 else
						  sprintf(cmd,"w %c_%d %d",op_target.namef,op_target.bit, op_target.val);
					 if(0 > _gpio_run_cmd(cmd, op))
							 return -EFAULT;
								 ret_target.val = op_target.val;
								 if(copy_to_user((iocmd_t*)arg, &ret_target, sizeof(iocmd_t)))
								 {
									 return -EFAULT;
								 }
					 break;
			case GPIO_CMD_OP_R:
					 if(copy_from_user(&op_target, (iocmd_t*)arg, sizeof(iocmd_t)))
					 {
						 return -EFAULT;
					 }
					 memcpy(&ret_target, &op_target, sizeof(iocmd_t));
					 if(op_target.nameb != ' ')
						  sprintf(cmd,"r %c%c_%d %d",op_target.namef,op_target.nameb,op_target.bit, op_target.val);
					 else
						  sprintf(cmd,"r %c_%d %d",op_target.namef,op_target.bit, op_target.val);
					 if(0 > _gpio_run_cmd(cmd, op))
							 return -EFAULT;
								 ret_target.val = op_target.val;
								 if(copy_to_user((iocmd_t*)arg, &ret_target, sizeof(iocmd_t)))
								 {
									 return -EFAULT;
								 }
					 break;
			default:
				break;
		}
	return 0;
}
static int am_gpio_release(struct inode *inode, struct file *file)
{
	cmd_t *op=file->private_data;
		
		if(op)
		{
			kfree(op);
		}
	return 0;
}
static const struct file_operations am_gpio_fops = {
	.open       = am_gpio_open,
		.read       = am_gpio_read,
		.write      = am_gpio_write,
		.unlocked_ioctl      = am_gpio_ioctl,
		//.ioctl      = am_gpio_ioctl,
		.release    = am_gpio_release,
		.poll       = NULL,
};

ssize_t gpio_cmd_restore(struct class *cla, struct class_attribute *attr, const char *buf, size_t count)
{
	        cmd_t op;
		int val;
		val=_gpio_run_cmd(buf, &op);
		if(val != -1){
		if(op.cmd_flag == 1)
			printk(KERN_DEBUG "READ [GPIO%s_%d]  %d \n",op.name,op.bit,val);
		else
			printk(KERN_DEBUG "WRITE [GPIO%s_%d] %d \n",op.name,op.bit,op.val);
		}
		else
			printk(KERN_ERR "you enter error!\n %s",help_cmd);
		return strlen(buf);
}
int create_gpio_device(gpio_t *gpio)
{
	int ret;
		
		ret = class_register(&gpio_class);
		if (ret < 0)
		{
			amlog_level(LOG_LEVEL_HIGH,"error create gpio class\r\n");
				return ret;
		}
	gpio->cla = &gpio_class ;
		gpio->dev = device_create(gpio->cla,NULL,MKDEV(gpio->major,0),NULL,strcat(gpio->name,"_dev"));
		if (IS_ERR(gpio->dev)) {
			amlog_level(LOG_LEVEL_HIGH,"create gpio device error\n");
				class_unregister(gpio->cla);
				gpio->cla = NULL;
				gpio->dev = NULL;
				return -1 ;
		}
	
		if (uio_register_device(gpio->dev, &gpio_uio_info)) {
			amlog_level(LOG_LEVEL_HIGH,"gpio UIO device register fail.\n");
				return -1;
		}
	
		amlog_level(LOG_LEVEL_HIGH,"create gpio device success\n");
		return  0;
}
/*****************************************************************
 **
 **  module   entry and exit port
 **
 ******************************************************************/
static int __init gpio_init_module(void)
{
	sprintf(am_gpio.name, "%s", GPIO_DEVCIE_NAME);
		am_gpio.major = register_chrdev(0, GPIO_DEVCIE_NAME, &am_gpio_fops);
		if(am_gpio.major < 0) {
			amlog_mask_level(LOG_MASK_INIT, LOG_LEVEL_HIGH,"register char dev gpio error\r\n");
		}
		else {
			amlog_mask_level(LOG_MASK_INIT, LOG_LEVEL_HIGH, "gpio dev major number:%d\r\n", am_gpio.major);
				if(0 > create_gpio_device(&am_gpio))
				{
					unregister_chrdev(am_gpio.major, am_gpio.name);
						am_gpio.major = -1;
				}
		}
	
		return am_gpio.major;
}
static __exit void gpio_remove_module(void)
{
	if(0 > am_gpio.major )
		return;
			uio_unregister_device(&gpio_uio_info);
			device_destroy(am_gpio.cla,MKDEV(am_gpio.major,0));
			class_unregister(am_gpio.cla);
			return;
}
/****************************************/
module_init(gpio_init_module);
module_exit(gpio_remove_module);

MODULE_DESCRIPTION("AMLOGIC gpio driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Beijing Platform");

