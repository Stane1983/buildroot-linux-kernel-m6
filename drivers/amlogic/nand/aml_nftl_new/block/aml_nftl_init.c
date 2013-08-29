
/*
 * Aml nftl init
 *
 * (C) 2012 8
 */


#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

#include "aml_nftl_block.h"

extern int aml_nftl_start(void* priv,void* cfg,struct aml_nftl_part_t ** ppart,uint64_t size,unsigned erasesize,unsigned writesize,unsigned oobavail,char* name,int no,char type);
extern uint32 gc_all(struct aml_nftl_part_t* part);
extern uint32 gc_one(struct aml_nftl_part_t* part);
extern void print_nftl_part(struct aml_nftl_part_t * part);
extern int part_param_init(struct aml_nftl_part_t *part,uint16 start_block,uint32_t logic_sects,uint32_t backup_cap_in_sects);
extern uint32 is_no_use_device(struct aml_nftl_part_t * part,uint32 size);
extern uint32 create_part_list_first(struct aml_nftl_part_t * part,uint32 size);
extern uint32 create_part_list(struct aml_nftl_part_t * part);
extern int nand_test(struct aml_nftl_blk_t *aml_nftl_blk,unsigned char flag,uint32 blocks);
extern int part_param_exit(struct aml_nftl_part_t *part);
extern int cache_init(struct aml_nftl_part_t *part);
extern int cache_exit(struct aml_nftl_part_t *part);
extern uint32 get_vaild_blocks(struct aml_nftl_part_t * part,uint32 start_block,uint32 blocks);
extern uint32 __nand_read(struct aml_nftl_part_t* part,uint32 start_sector,uint32 len,unsigned char *buf);
extern uint32 __nand_write(struct aml_nftl_part_t* part,uint32 start_sector,uint32 len,unsigned char *buf);
extern uint32 __nand_flush_write_cache(struct aml_nftl_part_t* part);
extern uint32 __shutdown_op(struct aml_nftl_part_t* part);
extern void print_free_list(struct aml_nftl_part_t* part);
extern void print_block_invalid_list(struct aml_nftl_part_t* part);

uint32 _nand_read(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
uint32 _nand_write(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
uint32 _nand_flush_write_cache(struct aml_nftl_blk_t *aml_nftl_blk);
uint32 _shutdown_op(struct aml_nftl_blk_t *aml_nftl_blk);
void *aml_nftl_malloc(uint32 size);
void aml_nftl_free(const void *ptr);
//int aml_nftl_dbg(const char * fmt,args...);

static ssize_t show_part_struct(struct class *class,struct class_attribute *attr,char *buf);
static ssize_t show_list(struct class *class, struct class_attribute *attr, const char *buf);
static ssize_t do_gc_all(struct class *class, struct class_attribute *attr, const char *buf);
static ssize_t do_gc_one(struct class *class, struct class_attribute *attr, const char *buf);
static ssize_t do_flush(struct class *class, struct class_attribute *attr, const char *buf);
static ssize_t do_test(struct class *class, struct class_attribute *attr,	const char *buf, size_t count);

static struct class_attribute nftl_class_attrs[] = {
//    __ATTR(part_struct,  S_IRUGO | S_IWUSR, show_logic_block_table,    show_address_map_table),
    __ATTR(part,  S_IRUGO , show_part_struct,    NULL),
    __ATTR(list,  S_IRUGO , show_list,    NULL),
    __ATTR(gcall,  S_IRUGO , do_gc_all,    NULL),
    __ATTR(flush,  S_IRUGO , do_flush,    NULL),
    __ATTR(gcone,  S_IRUGO , do_gc_one,    NULL),
    __ATTR(test,  S_IRUGO | S_IWUSR , NULL,    do_test),
//    __ATTR(cache_struct,  S_IRUGO , show_logic_block_table,    NULL),
//    __ATTR(table,  S_IRUGO | S_IWUSR , NULL,    show_logic_page_table),

    __ATTR_NULL
};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void *aml_nftl_malloc(uint32 size)
{
    return kzalloc(size, GFP_KERNEL);
}

void aml_nftl_free(const void *ptr)
{
    kfree(ptr);
}

//int aml_nftl_dbg(const char * fmt,args...)
//{
//    //return printk(fmt,##__VA_ARGS__);
//    //return printk(KERN_WARNING "AML NFTL warning: %s: line:%d " fmt "\n",  __func__, __LINE__, ##__VA_ARGS__);
//    return printk( fmt,## args);
//    //return printk(KERN_ERR "AML NFTL error: %s: line:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
//}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int aml_nftl_initialize(struct aml_nftl_blk_t *aml_nftl_blk,int no)
{
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	int error = 0, phy_blk_num, oob_len;
	uint32_t phy_page_addr, size_in_blk,total_block,total_pages,temp,i;
	uint32_t phys_erase_shift;
	uint32_t ret;
	unsigned char nftl_oob_buf[mtd->oobavail];

	if (mtd->oobavail < MIN_BYTES_OF_USER_PER_PAGE)
		return -EPERM;

	aml_nftl_blk->nftl_cfg.nftl_use_cache = NFTL_DONT_CACHE_DATA;
	aml_nftl_blk->nftl_cfg.nftl_support_gc_read_reclaim = SUPPORT_GC_READ_RECLAIM;
	aml_nftl_blk->nftl_cfg.nftl_support_wear_leveling = SUPPORT_WEAR_LEVELING;
	aml_nftl_blk->nftl_cfg.nftl_support_fill_block = SUPPORT_FILL_BLOCK;
	aml_nftl_blk->nftl_cfg.nftl_need_erase = NFTL_ERASE;
	aml_nftl_blk->nftl_cfg.nftl_part_reserved_block_ratio = PART_RESERVED_BLOCK_RATIO;
	aml_nftl_blk->nftl_cfg.nftl_min_free_block_num = MIN_FREE_BLOCK_NUM;
	aml_nftl_blk->nftl_cfg.nftl_min_free_block = MIN_FREE_BLOCK;
	aml_nftl_blk->nftl_cfg.nftl_gc_threshold_free_block_num = GC_THRESHOLD_FREE_BLOCK_NUM ;
	aml_nftl_blk->nftl_cfg.nftl_gc_threshold_ratio_numerator = GC_THRESHOLD_RATIO_NUMERATOR;
	aml_nftl_blk->nftl_cfg.nftl_gc_threshold_ratio_denominator = GC_THRESHOLD_RATIO_DENOMINATOR;
	aml_nftl_blk->nftl_cfg.nftl_max_cache_write_num = MAX_CACHE_WRITE_NUM;

	ret = aml_nftl_start((void*)aml_nftl_blk,&aml_nftl_blk->nftl_cfg,&aml_nftl_blk->aml_nftl_part,mtd->size,mtd->erasesize,mtd->writesize,mtd->oobavail,mtd->name,no,0);
	if(ret != 0)
	    return ret;

	aml_nftl_blk->mbd.size = aml_nftl_get_part_cap(aml_nftl_blk->aml_nftl_part);
	aml_nftl_blk->read_data = _nand_read;
	aml_nftl_blk->write_data = _nand_write;
	aml_nftl_blk->flush_write_cache = _nand_flush_write_cache;
	aml_nftl_blk->shutdown_op = _shutdown_op;

    //setup class
    if(memcmp(mtd->name, "NFTL_Part", 9)==0)
    {
		aml_nftl_blk->debug.name = kzalloc(strlen((const char*)AML_NFTL_MAGIC)+1, GFP_KERNEL);
    	strcpy(aml_nftl_blk->debug.name, (char*)AML_NFTL_MAGIC);
    	aml_nftl_blk->debug.class_attrs = nftl_class_attrs;
   		error = class_register(&aml_nftl_blk->debug);
		if(error)
			printk(" class register nand_class fail!\n");
	}

	if(memcmp(mtd->name, "userdata", 8)==0)
    {
		aml_nftl_blk->debug.name = kzalloc(strlen((const char*)AML_USERDATA_MAGIC)+1, GFP_KERNEL);
    	strcpy(aml_nftl_blk->debug.name, (char*)AML_USERDATA_MAGIC);
    	aml_nftl_blk->debug.class_attrs = nftl_class_attrs;
   		error = class_register(&aml_nftl_blk->debug);
		if(error)
			printk(" class register nand_class fail!\n");
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 _nand_read(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    return __nand_read(aml_nftl_blk->aml_nftl_part,start_sector,len,buf);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 _nand_write(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret;
    ret = __nand_write(aml_nftl_blk->aml_nftl_part,start_sector,len,buf);
//    ktime_get_ts(&aml_nftl_blk->ts_write_start);
    aml_nftl_blk->time = jiffies;
    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 _nand_flush_write_cache(struct aml_nftl_blk_t *aml_nftl_blk)
{
    return __nand_flush_write_cache(aml_nftl_blk->aml_nftl_part);
}

uint32 _shutdown_op(struct aml_nftl_blk_t *aml_nftl_blk)
{
    return __shutdown_op(aml_nftl_blk->aml_nftl_part);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t show_part_struct(struct class *class,struct class_attribute *attr,char *buf)
{
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);

    print_nftl_part(aml_nftl_blk -> aml_nftl_part);

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t show_list(struct class *class, struct class_attribute *attr, const char *buf)
{
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);

    print_free_list(aml_nftl_blk -> aml_nftl_part);
    print_block_invalid_list(aml_nftl_blk -> aml_nftl_part);
//    print_block_count_list(aml_nftl_blk -> aml_nftl_part);

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t do_gc_all(struct class *class, struct class_attribute *attr, const char *buf)
{
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);

	gc_all(aml_nftl_blk -> aml_nftl_part);
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t do_gc_one(struct class *class, struct class_attribute *attr, const char *buf)
{
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);

	gc_one(aml_nftl_blk -> aml_nftl_part);
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t do_flush(struct class *class, struct class_attribute *attr, const char *buf)
{
    int error;
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);

	mutex_lock(aml_nftl_blk->aml_nftl_lock);
	error = aml_nftl_blk->flush_write_cache(aml_nftl_blk);
	mutex_unlock(aml_nftl_blk->aml_nftl_lock);

    return error;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t do_test(struct class *class, struct class_attribute *attr,	const char *buf, size_t count)
{
    struct aml_nftl_blk_t *aml_nftl_blk = container_of(class, struct aml_nftl_blk_t, debug);
    struct aml_nftl_part_t *part = aml_nftl_blk -> aml_nftl_part;

	uint32 num;

	sscanf(buf, "%x", &num);
	
	aml_nftl_set_part_test(part,num);

    return count;
}
