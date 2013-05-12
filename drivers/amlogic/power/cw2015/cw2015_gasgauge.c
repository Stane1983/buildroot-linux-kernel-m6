
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/power_supply.h>

//#include <linux/power/ns115-battery.h>
#include <linux/string.h>
#include <asm/irq.h>

struct ns115_battery_gauge  {
	int (*get_battery_mvolts)(void);
	int (*get_battery_capacity)(int, int);
};

/*----- driver defines -----*/
#define MYDRIVER "cw2015"


#define REG_VERSION     0x0
#define REG_VCELL       0x2
#define REG_SOC         0x4
#define REG_RRT_ALERT   0x6
#define REG_CONFIG      0x8
#define REG_MODE        0xA
#define REG_BATINFO     0x10

#define SIZE_BATINFO    64 

#define MODE_SLEEP_MASK (0x3<<6)
#define MODE_SLEEP      (0x3<<6)
#define MODE_NORMAL     (0x0<<6)
#define MODE_QUICK_START (0x3<<4)
#define MODE_RESTART    (0xf<<0)

#define CONFIG_UPDATE_FLG (0x1<<1)
#define ATHD (0xa<<3)   //ATHD =10%

/*----- config part for battery information -----*/
#if 0
#undef dev_info
#define dev_info dev_err
#endif

#define FORCE_WAKEUP_CHIP 1

/* battery info: GS, 3.7v, 1600mAh */
static char cw2015_bat_config_info[SIZE_BATINFO] = {
	0x15,0x50,0x6A,0x59,0x50,0x4C,0x4B,0x4A,0x48,0x47,0x46,0x49,0x49,0x3A,0x31,0x27,0x21,0x1D,0x1E,0x1D,0x2B,0x2E,0x3E,0x49,0x27,0x4D,0x0C,0x29,0x29,0x49,0x4C,0x62,0x71,0x6B,0x6A,0x6E,0x41,0x1C,0x46,0x2A,0x12,0x1A,0x77,0x87,0x8F,0x91,0x8F,0x1D,0x59,0x85,0x93,0xA3,0x30,0x9F,0xCA,0xCB,0x2F,0x7D,0x72,0xA5,0xB5,0xC1,0x46,0xAE
};


struct cw2015_info {
	struct i2c_client  *client;
	struct device      *dev;
};

static struct cw2015_info *g_cw2015;

static int cw2015_verify_update_battery_info(void)
{
	int ret = 0;
	int i;
	char value = 0;
	short value16 = 0;
	char buffer[SIZE_BATINFO*2];
	struct i2c_client *client = NULL;
	char reg_mode_value = 0;


	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

	/* make sure not in sleep mode */
	ret = i2c_smbus_read_byte_data(client, REG_MODE);
	if(ret < 0) {
		dev_err(&client->dev, "Error read mode\n");
		return ret;
	}

	value = ret;
	reg_mode_value = value; /* save MODE value for later use */
	if((value & MODE_SLEEP_MASK) == MODE_SLEEP) {
		dev_err(&client->dev, "Error, device in sleep mode, cannot update battery info\n");
		return -1;
	}

	/* update new battery info */
	for(i=0; i<SIZE_BATINFO; i++) {
		ret = i2c_smbus_write_byte_data(client, REG_BATINFO+i, cw2015_bat_config_info[i]);
		if(ret < 0) {
			dev_err(&client->dev, "Error update battery info @ offset %d, ret = 0x%x\n", i, ret);
			return ret;
		}
	}

	/* readback & check */
	for(i=0; i<SIZE_BATINFO; i++) {
		ret = i2c_smbus_read_byte_data(client, REG_BATINFO+i);
		if(ret < 0) {
			dev_err(&client->dev, "Error read origin battery info @ offset %d, ret = 0x%x\n", i, ret);
			return ret;
		}

		buffer[i] = ret;
	}

	if(0 != memcmp(buffer, cw2015_bat_config_info, SIZE_BATINFO)) {
		dev_info(&client->dev, "battery info NOT matched, after readback.\n");
		return -1;
	} else {
		dev_info(&client->dev, "battery info matched, after readback.\n");
	}

	/* set 2015 to use new battery info */
	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
	if(ret < 0) {
		dev_err(&client->dev, "Error to read CONFIG\n");
		return ret;
	}
	value = ret;
	  
	value |= CONFIG_UPDATE_FLG;/* set UPDATE_FLAG */
	value &= 0x7;  /* clear ATHD */
	value |= ATHD; /* set ATHD */
	
	ret = i2c_smbus_write_byte_data(client, REG_CONFIG, value);
	if(ret < 0) {
		dev_err(&client->dev, "Error to update flag for new battery info\n");
		return ret;
	}
	
	/* check 2015 for ATHD&update_flag */
	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
	if(ret < 0) {
		dev_err(&client->dev, "Error to read CONFIG\n");
		return ret;
	}
	value = ret;
	
	if (!(value & CONFIG_UPDATE_FLG)) {
	  dev_info(&client->dev, "update flag for new battery info have not set\n");
	}
	if ((value & 0xf8) != ATHD) {
	  dev_info(&client->dev, "the new ATHD have not set\n");
	}	  

	reg_mode_value &= ~(MODE_RESTART);  /* RSTART */
	ret = i2c_smbus_write_byte_data(client, REG_MODE, reg_mode_value|MODE_RESTART);
	if(ret < 0) {
		dev_err(&client->dev, "Error to restart battery info1\n");
		return ret;
	}
	ret = i2c_smbus_write_byte_data(client, REG_MODE, reg_mode_value|0);
	if(ret < 0) {
		dev_err(&client->dev, "Error to restart battery info2\n");
		return ret;
	}
	return 0;
}

static int cw2015_init_charger(void)
{
  int cnt,i =0;
	int ret = 0;
	char value = 0;
	char buffer[SIZE_BATINFO*2];
	short value16 = 0;
	struct i2c_client *client = NULL;

	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

#if FORCE_WAKEUP_CHIP
	value = MODE_SLEEP;
#else
	/* check if sleep mode, bring up */
	ret = i2c_smbus_read_byte_data(client, REG_MODE);
	if(ret < 0) {
		dev_err(&client->dev, "read mode\n");
		return ret;
	}

	value = ret;
#endif

	if((value & MODE_SLEEP_MASK) == MODE_SLEEP) {

		/* do wakeup cw2015 */
		ret = i2c_smbus_write_byte_data(client, REG_MODE, MODE_NORMAL);
		if(ret < 0) {
			dev_err(&client->dev, "Error update mode\n");
			return ret;
		}

    /* check 2015 if not set ATHD */
  	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
  	if(ret < 0) {
		  dev_err(&client->dev, "Error to read CONFIG\n");
		  return ret;
  	}
  	value = ret;

	  if ((value & 0xf8) != ATHD) {
	    dev_info(&client->dev, "the new ATHD have not set\n");
  	 	value &= 0x7;  /* clear ATHD */
	    value |= ATHD; 
	  /* set ATHD */
	    ret = i2c_smbus_write_byte_data(client, REG_CONFIG, value);
	    if(ret < 0) {
		    dev_err(&client->dev, "Error to set new ATHD\n");
	  	  return ret;
	    }
	  }
	  
	  /* check 2015 for update_flag */
  	ret = i2c_smbus_read_byte_data(client, REG_CONFIG);
	  if(ret < 0) {
	  	dev_err(&client->dev, "Error to read CONFIG\n");
	  	return ret;
	  }
  	value = ret;  	    	 
    /* not set UPDATE_FLAG,do update_battery_info  */
	  if (!(value & CONFIG_UPDATE_FLG)) {
	    dev_info(&client->dev, "update flag for new battery info have not set\n");
	    cw2015_verify_update_battery_info();
	  }
	
  	/* read origin info */
	  for(i=0; i<SIZE_BATINFO; i++) {
		ret = i2c_smbus_read_byte_data(client, REG_BATINFO+i);
		if(ret < 0) {
			dev_err(&client->dev, "Error read origin battery info @ offset %d, ret = 0x%x\n", i, ret);
			return ret;
		}

		buffer[i] = ret;
	  }

  	if(0 != memcmp(buffer, cw2015_bat_config_info, SIZE_BATINFO)) {
	  	dev_info(&client->dev, "battery info NOT matched.\n");
	 /* battery info not matched,do update_battery_info  */
	  	cw2015_verify_update_battery_info();
	  } else {
	  	dev_info(&client->dev, "battery info matched.\n");
	  }

		/* do wait valide SOC, if the first time wakeup after poweron */
		ret = i2c_smbus_read_word_data(client, REG_SOC);
		if(ret < 0) {
			dev_err(&client->dev, "Error read init SOC\n");
			return ret;
		}   
		value16 = ret;
#if 0		    
		
		while ((value16 == 0xff)&&(cnt < 10000)) {     //SOC value is not valide or time is not over 3 seconds
		  ret = i2c_smbus_read_word_data(client, REG_SOC);		  
		  if(ret < 0) {
		  	dev_err(&client->dev, "Error read init SOC\n");
		  	return ret;
		  }   
		  value16 = ret;
		  cnt++;
		}
		
#endif
/*		
		if(value16 == 0) {     //do QUICK_START
			ret = i2c_smbus_write_byte_data(client, REG_MODE, MODE_QUICK_START|MODE_NORMAL);
			if(ret < 0) {
				dev_err(&client->dev, "Error quick start1\n");
				return ret;
			}

			ret = i2c_smbus_write_byte_data(client, REG_MODE, MODE_NORMAL);
			if(ret < 0) {
				dev_err(&client->dev, "Error quick start2\n");
				return ret;
			}
		}
*/
	}

	return 0;
}

int cw2015_gasgauge_get_mvolts(void)
{
	int ret = 0;
	short ustemp =0,ustemp1 =0,ustemp2 =0,ustemp3 =0;
	int voltage;
	struct i2c_client *client = NULL;
	printk("%s\n", __FUNCTION__);
	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	ustemp = ret;
	ustemp = cpu_to_be16(ustemp);
	
	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	ustemp1 = ret;
	ustemp1 = cpu_to_be16(ustemp1);
	
	ret = i2c_smbus_read_word_data(client, REG_VCELL);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	ustemp2 = ret;
	ustemp2 = cpu_to_be16(ustemp2);
	
	if(ustemp >ustemp1)
	{	 
	   ustemp3 =ustemp;
		  ustemp =ustemp1;
		 ustemp1 =ustemp3;
  }
	if(ustemp1 >ustemp2)
	{
	   ustemp3 =ustemp1;
		 ustemp1 =ustemp2;
		 ustemp2 =ustemp3;
	}	
	if(ustemp >ustemp1)
	{	 
	   ustemp3 =ustemp;
		  ustemp =ustemp1;
		 ustemp1 =ustemp3;
  }			

	/* 1 voltage LSB is 305uV, ~312/1024mV */
	// voltage = value16 * 305 / 1000;
	voltage = ustemp1 * 312 / 1024;

	dev_info(&client->dev, "read Vcell %dmV. value16 0x%x\n", voltage, voltage);
	return voltage;
}

int cw2015_gasgauge_get_capacity(void)
{
	int ret = 0;
	short value16 = 0;
	int soc;
	struct i2c_client *client = NULL;
	printk("%s\n", __FUNCTION__);
	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

	ret = i2c_smbus_read_word_data(client, REG_SOC);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	soc = value16 >> 8;

	dev_info(&client->dev, "read SOC %d%%. value16 0x%x\n", soc, value16);
	return soc;
}

int cw2015_gasgauge_get_rrt(void)
{
	int ret = 0;
	short value16 = 0;
	int rrt;
	struct i2c_client *client = NULL;
	printk("%s\n", __FUNCTION__);
	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

	ret = i2c_smbus_read_word_data(client, REG_RRT_ALERT);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	rrt = value16 & 0x1fff;

	dev_info(&client->dev, "read RRT %d%%. value16 0x%x\n", rrt, value16);
	return rrt;
}

int cw2015_gasgauge_get_alt(void)
{
	int ret = 0;
	short value16 = 0;
	int alt;
	char value=0;
	struct i2c_client *client = NULL;

	if(NULL == g_cw2015)
		return -1;

	client = to_i2c_client(g_cw2015->dev);

	ret = i2c_smbus_read_word_data(client, REG_RRT_ALERT);
	if(ret < 0) {
		dev_err(&client->dev, "Error read SOC\n");
		return 0;
	}
	value16 = ret;
	value16 = cpu_to_be16(value16);
	alt = value16 >>15;

	dev_info(&client->dev, "read RRT %d%%. value16 0x%x\n", alt, value16);
	
	value = (char)(value16 >>8);
	value = value&0x7f;
	ret = i2c_smbus_write_byte_data(client, REG_RRT_ALERT, value);
	if(ret < 0) {
		 dev_err(&client->dev, "Error to clear ALT\n");
	   return ret;
	}	
		
	return alt;
}


static struct ns115_battery_gauge cw2015_battery_gauge = {
	.get_battery_mvolts = cw2015_gasgauge_get_mvolts,
	.get_battery_capacity = cw2015_gasgauge_get_capacity,
	
};


/*-------------------------------------------------------------------------*/

static enum power_supply_property axp_battery_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_TECHNOLOGY,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
  POWER_SUPPLY_PROP_CAPACITY,
};


static int cw2015_battery_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  int ret = 0;
  switch (psp) {
  	case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = "CW2015 Battery";
    break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
    val->intval= POWER_SUPPLY_TECHNOLOGY_LiFe;
    break;
/*  	
  case POWER_SUPPLY_PROP_STATUS:
  	*val = cw2015_gasgauge_get_mvolts();
    break;
*/
case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = cw2015_gasgauge_get_mvolts() * 1000;
    break;
  case POWER_SUPPLY_PROP_CAPACITY:
    val->intval = cw2015_gasgauge_get_capacity;
    break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
    val->intval = cw2015_gasgauge_get_rrt();
    break;
	
/*  
  case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
    if(charger->bat_det && charger->is_on)
      val->intval = charger->rest_time;
    else
      val->intval = 0;
    break;
*/
  default:
    ret = -EINVAL;
    break;
  }

  return ret;
}

/*-------------------------------------------------------------------------*/

static int cw2015_detect(struct i2c_client *client, int kind,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA
				     | I2C_FUNC_SMBUS_READ_BYTE))
		return -ENODEV;

	strlcpy(info->type, MYDRIVER, I2C_NAME_SIZE);
	return 0;
}

static irqreturn_t cw2015_interrupt(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}

static int cw2015_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret, irq, irq_flag;
	struct cw2015_info *data;

	printk("%s %d +++\n",__FUNCTION__,__LINE__);

	if (!(data = kzalloc(sizeof(struct cw2015_info), GFP_KERNEL)))
		return -ENOMEM;

	// Init real i2c_client
	i2c_set_clientdata(client, data);

	g_cw2015 = data;
	data->client = client;
	data->dev    = &client->dev;

	ret = cw2015_init_charger();
	if(ret < 0)
		return ret;
	
	//ns115_battery_gauge_register(&cw2015_battery_gauge);

	struct power_supply battery;

	battery.properties = axp_battery_props;
	battery.num_properties = ARRAY_SIZE(axp_battery_props);

	battery.name = "bat";
//	battery.use_for_apm = info->use_for_apm;
	battery.type = POWER_SUPPLY_TYPE_BATTERY;
	battery.get_property = cw2015_battery_get_property;
/*
	ret = power_supply_register(&client->dev, &battery);
	if (ret)
		return -1;
*/
	printk("%s ok. %d ---\n",__FUNCTION__,__LINE__);
	return 0;					//return Ok
}

static int cw2015_remove(struct i2c_client *client)
{
	struct cw2015_info *data = i2c_get_clientdata(client);
	kfree(data);
	g_cw2015 = NULL;
	return 0;
}

static int cw2015_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct cw2015_info *data = i2c_get_clientdata(client);
	return 0;
}

static int cw2015_resume(struct i2c_client *client)
{
	struct cw2015_info *data = i2c_get_clientdata(client);
	int ret = 0;

	return 0;
}

/*-------------------------------------------------------------------------*/
static const struct i2c_device_id cw2015_id[] = {
	{ MYDRIVER, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cw2015_id);

static struct i2c_driver cw2015_driver = {
	.driver = {
		.name	= MYDRIVER,
	},
	.probe			= cw2015_probe,
	.remove			= cw2015_remove,
	.suspend		= cw2015_suspend,
	.resume			= cw2015_resume,
	.id_table		= cw2015_id,
};

/*-------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>

int HELLO_MAJOR = 0;
int HELLO_MINOR = 0;
int NUMBER_OF_DEVICES = 2;
 
struct class *my_class;
struct cdev cdev;
dev_t devno;

static int test_open (struct inode* inode, struct file* filp) {
	return 0;
}

static int test_release (struct inode* inode, struct file* filp) {
	return 0;
}

static ssize_t test_read(struct file* filp, char __user *buf, size_t size, loff_t * ppos) {
	int volt = cw2015_gasgauge_get_mvolts();
	int soc = cw2015_gasgauge_get_capacity();\
	int rrt = cw2015_gasgauge_get_rrt();
	printk(KERN_INFO "volt ------ %d\n", volt);
	printk(KERN_INFO "soc ------ %d\n", soc);
	printk(KERN_INFO "rrt ------ %d\n", rrt);
	return 0;
}

struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.open = test_open,
	.release = test_release,
	.read = test_read,
};

static int __init cw2015_init(void)
{
	int result;
    devno = MKDEV(HELLO_MAJOR, HELLO_MINOR);
    if (HELLO_MAJOR)
        result = register_chrdev_region(devno, 2, "memdev");
    else
    {
        result = alloc_chrdev_region(&devno, 0, 2, "memdev");
        HELLO_MAJOR = MAJOR(devno);
    }  
    printk("MAJOR IS %d\n",HELLO_MAJOR);
    my_class = class_create(THIS_MODULE,"hello_char_class");  //类名为hello_char_class
    if(IS_ERR(my_class)) 
    {
        printk("Err: failed in creating class.\n");
        return -1; 
    }
    device_create(my_class,NULL,devno,NULL,"memdev");      //设备名为memdev
    if (result<0) 
    {
        printk (KERN_WARNING "hello: can't get major number %d\n", HELLO_MAJOR);
        return result;
    }
 
    cdev_init(&cdev, &hello_fops);
    cdev.owner = THIS_MODULE;
    cdev_add(&cdev, devno, NUMBER_OF_DEVICES);
    printk (KERN_INFO "Character driver Registered\n");
 
	return i2c_add_driver(&cw2015_driver);
}

static void __exit cw2015_exit(void)
{
	i2c_del_driver(&cw2015_driver);

	cdev_del (&cdev);
    device_destroy(my_class, devno);         //delete device node under /dev//必须先删除设备，再删除class类
    class_destroy(my_class);                 //delete class created by us
    unregister_chrdev_region (devno,NUMBER_OF_DEVICES);
    printk (KERN_INFO "char driver cleaned up\n");
}

/*-------------------------------------------------------------------------*/

MODULE_DESCRIPTION("cw2015 i2c battery gasgauge driver");
MODULE_LICENSE("public");

module_init(cw2015_init);
module_exit(cw2015_exit);

