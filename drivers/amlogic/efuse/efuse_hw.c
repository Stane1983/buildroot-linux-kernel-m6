#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <mach/am_regs.h>
#include <plat/io.h>

#include <linux/efuse.h>
#include "efuse_regs.h"


static void __efuse_write_byte( unsigned long addr, unsigned long data );
static void __efuse_read_dword( unsigned long addr, unsigned long *data);

extern int efuseinfo_num;
extern int efuse_active_version;
///extern unsigned efuse_active_customerid;
extern pfn efuse_getinfoex;
extern pfn efuse_getinfoex_byPos;
extern efuseinfo_t efuseinfo[];


#ifdef EFUSE_DEBUG

static unsigned long efuse_test_buf_32[EFUSE_DWORDS] = {0};
static unsigned char* efuse_test_buf_8 = (unsigned char*)efuse_test_buf_32;

static void __efuse_write_byte_debug(unsigned long addr, unsigned char data)
{
	efuse_test_buf_8[addr] = data;		
}

static void __efuse_read_dword_debug(unsigned long addr, unsigned long *data)    
{
	*data = efuse_test_buf_32[addr >> 2];		
}

void __efuse_debug_init(void)
{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6	
	/*__efuse_write_byte_debug(0, 0xbf);	
	__efuse_write_byte_debug(1, 0xff);	
	__efuse_write_byte_debug(2, 0x00);
	
	__efuse_write_byte_debug(3, 0x02);
	__efuse_write_byte_debug(4, 0x81);
	__efuse_write_byte_debug(5, 0x0f);
	__efuse_write_byte_debug(6, 0x00);
	__efuse_write_byte_debug(7, 0x00);

	__efuse_write_byte_debug(8, 0xaf);
	__efuse_write_byte_debug(9, 0x32);
	__efuse_write_byte_debug(10, 0x76);
	__efuse_write_byte_debug(135, 0xb2);*/
#endif	
#ifdef CONFIG_ARCH_MESON3	
	/*__efuse_write_byte_debug(0, 0x02);
	__efuse_write_byte_debug(60, 0x02);
	__efuse_write_byte_debug(1, 0x03);
	__efuse_write_byte_debug(61, 0x03);
	__efuse_write_byte_debug(2, 0x00);
	__efuse_write_byte_debug(62, 0x00);
	__efuse_write_byte_debug(3, 0xA3);
	__efuse_write_byte_debug(63, 0xA3);
	__efuse_write_byte_debug(380, 0x01);
	__efuse_write_byte_debug(381, 0x8e);
	__efuse_write_byte_debug(382, 0x0b);
	__efuse_write_byte_debug(383, 0x66);*/
#endif	
}
#endif


static void __efuse_write_byte( unsigned long addr, unsigned long data )
{
	unsigned long auto_wr_is_enabled = 0;

	if (aml_read_reg32( P_EFUSE_CNTL1) & (1 << CNTL1_AUTO_WR_ENABLE_BIT)) {                                                                                
		auto_wr_is_enabled = 1;
	} else {                                                                                
		/* temporarily enable Write mode */
		aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_ON,
		CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

	/* write the address */
	aml_set_reg32_bits( P_EFUSE_CNTL1, addr,
			CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );
	/* set starting byte address */
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
			CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
			CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

	/* write the byte */
	aml_set_reg32_bits( P_EFUSE_CNTL1, data,
			CNTL1_BYTE_WR_DATA_BIT, CNTL1_BYTE_WR_DATA_SIZE );
	/* start the write process */
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
			CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
			CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	/* dummy read */
	aml_read_reg32( P_EFUSE_CNTL1 );

	while ( aml_read_reg32(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_BUSY_BIT ) ) {                                                                                
		udelay(1);
	}

	/* if auto write wasn't enabled and we enabled it, then disable it upon exit */
	if (auto_wr_is_enabled == 0 ) {                                                                                
		aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_OFF,
				CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

	//printk(KERN_INFO "__efuse_write_byte: addr=%ld, data=0x%ld\n", addr, data);
}

static void __efuse_read_dword( unsigned long addr, unsigned long *data )
{
	//unsigned long auto_rd_is_enabled = 0;
	

	//if( aml_read_reg32(EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ) ){                                                                               
	//	auto_rd_is_enabled = 1;
	//} else {                                                                               
		/* temporarily enable Read mode */
	//aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
	//	CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	//}

	/* write the address */
	aml_set_reg32_bits( P_EFUSE_CNTL1, addr,
			CNTL1_BYTE_ADDR_BIT,  CNTL1_BYTE_ADDR_SIZE );
	/* set starting byte address */
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
			CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
			CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

	/* start the read process */
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
			CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
	aml_set_reg32_bits(P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
			CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
	/* dummy read */
	aml_read_reg32( P_EFUSE_CNTL1 );

	while ( aml_read_reg32(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) ) {                                                                               
		udelay(1);
	}
	/* read the 32-bits value */
	( *data ) = aml_read_reg32( P_EFUSE_CNTL2 );

	/* if auto read wasn't enabled and we enabled it, then disable it upon exit */
	//if ( auto_rd_is_enabled == 0 ){                                                                               
		//aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
		//	CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	//}

	//printk(KERN_INFO "__efuse_read_dword: addr=%ld, data=0x%lx\n", addr, *data);
}

static ssize_t __efuse_read( char *buf, size_t count, loff_t *ppos )
{
	unsigned long* contents = (unsigned long*)kzalloc(sizeof(unsigned long)*EFUSE_DWORDS, GFP_KERNEL);
	unsigned pos = *ppos;
	unsigned long *pdw;
	char* tmp_p;
		
	/*pos may not align to 4*/
	unsigned int dwsize = (count + 3 +  pos%4) >> 2;	
	
	if (!contents) {
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}	
	if (pos >= EFUSE_BYTES)
		return 0;
	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;
	
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
		CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
		
	for (pdw = contents + pos/4; dwsize-- > 0 && pos < EFUSE_BYTES; pos += 4, ++pdw) {
		#ifdef EFUSE_DEBUG     				
		__efuse_read_dword_debug(pos, pdw);
		#else		
		/*if pos does not align to 4,  __efuse_read_dword read from next dword, so, discount this un-aligned partition*/
		__efuse_read_dword((pos - pos%4), pdw);
		#endif
	}     
	
	aml_set_reg32_bits( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
			CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
		
	tmp_p = (char*)contents;
    tmp_p += *ppos;                           

	memcpy(buf, tmp_p, count);

	*ppos += count;
	
	if (contents)
		kfree(contents);
	return count;
}

static ssize_t __efuse_write(const char *buf, size_t count, loff_t *ppos )
{
	unsigned pos = *ppos;
	//loff_t *readppos = ppos;
	unsigned char *pc;	

	if (pos >= EFUSE_BYTES)
		return 0;       /* Past EOF */
	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -EFAULT;
	
	for (pc = (char*)buf; count--; ++pos, ++pc){
		#ifdef EFUSE_DEBUG    
         __efuse_write_byte_debug(pos, *pc);  
         #else                             
          __efuse_write_byte(pos, *pc);   
         #endif
    }

	*ppos = pos;

	return (const char *)pc - buf;
}

//=================================================================================================
static int cpu_is_before_m6(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c0, c0, 5	@ get MPIDR" : "=r" (val) : : "cc");
		
	return ((val & 0x40000000) == 0x40000000);
}

static void efuse_set_versioninfo(efuseinfo_item_t *info)
{
	strcpy(info->title, "version");		
	info->id = EFUSE_VERSION_ID;
	info->bch_reverse = 0;
	if(cpu_is_before_m6()){
			info->offset = EFUSE_VERSION_OFFSET; //380;		
			info->data_len = EFUSE_VERSION_DATA_LEN; //3;	
			info->enc_len = EFUSE_VERSION_ENC_LEN; //4;
			info->bch_en = EFUSE_VERSION_BCH_EN; //1;		
		}
		else{
			info->offset = V2_EFUSE_VERSION_OFFSET; //3;		
			info->data_len = V2_EFUSE_VERSION_DATA_LEN; //1;		
			info->enc_len = V2_EFUSE_VERSION_ENC_LEN; //1;
			info->bch_en = V2_EFUSE_VERSION_BCH_EN; //0;
		}
}


static int efuse_readversion(void)
{

	char ver_buf[4], buf[4];
	efuseinfo_item_t info;
	
	if(efuse_active_version != -1)
		return efuse_active_version;
	
	efuse_set_versioninfo(&info);
	memset(ver_buf, 0, sizeof(ver_buf));		
	memset(buf, 0, sizeof(buf));
	
	__efuse_read(buf, info.enc_len, &info.offset);
	if(info.bch_en){
		if(efuse_bch_dec(buf, info.enc_len, ver_buf, info.bch_reverse)<0)	
			return -ENOMEM;
	}
	else
		memcpy(ver_buf, buf, sizeof(buf));
	
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6   //version=2
	if(ver_buf[0] == 2){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
	else
		return -1;

#elif MESON_CPU_TYPE == MESON_CPU_TYPE_MESON3   //version=1
	if(ver_buf[0] == 1){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
	else
		return -1;
		
#elif MESON_CPU_TYPE == MESON_CPU_TYPE_MESON1   //version=0
	if(ver_buf[0] == 0){
		efuse_active_version = ver_buf[0];
		return ver_buf[0];
	}
	else
		return -1;
#else
	return -1;
#endif
}

static int efuse_getinfo_byPOS(unsigned pos, efuseinfo_item_t *info)
{
	int ver;
	int i;
	efuseinfo_t *vx = NULL;
	efuseinfo_item_t *item = NULL;
	int size;
	int ret = -1;		
	
	unsigned versionPOS;
	if(cpu_is_before_m6())
		versionPOS = EFUSE_VERSION_OFFSET; //380;
	else
		versionPOS = V2_EFUSE_VERSION_OFFSET; //3;
	if(pos == versionPOS){
		efuse_set_versioninfo(info);
		return 0;
	}
	
	ver = efuse_readversion();
		if(ver < 0){
			printk("efuse version is not selected.\n");
			return -1;
		}
		
		for(i=0; i<efuseinfo_num; i++){
			if(efuseinfo[i].version == ver){
				vx = &(efuseinfo[i]);
				break;
			}				
		}
		if(!vx){
			printk("efuse version %d is not supported.\n", ver);
			return -1;
		}	
		
		// BSP setting priority is higher than version table
		if((efuse_getinfoex != NULL)){
			ret = efuse_getinfoex_byPos(pos, info);			
			if(ret >=0 )
				return ret;
		}
		
		item = vx->efuseinfo_version;
		size = vx->size;		
		ret = -1;		
		for(i=0; i<size; i++, item++){			
			if(pos == item->offset){
				strcpy(info->title, item->title);				
				info->offset = item->offset;
				info->id = item->id;				
				info->data_len = item->data_len;			
				info->enc_len = item->enc_len;
				info->bch_en = item->bch_en;
				info->bch_reverse = item->bch_reverse;///what's up ? typo error?
				ret = 0;
				break;
			}
		}
		
		//if((ret < 0) && (efuse_getinfoex != NULL))
		//	ret = efuse_getinfoex(id, info);		
		if(ret < 0)
			printk("POS:%d is not found.\n", pos);
			
		return ret;
}

//=================================================================================================
// public interface
//=================================================================================================
int efuse_getinfo_byID(unsigned id, efuseinfo_item_t *info)
{
	int ver;
	int i;
	efuseinfo_t *vx = NULL;
	efuseinfo_item_t *item = NULL;
	int size;
	int ret = -1;		
	
	if(id == EFUSE_VERSION_ID){
		efuse_set_versioninfo(info);
		return 0;		
	}	
	
	ver = efuse_readversion();
	if(ver < 0){
		printk("efuse version is not selected.\n");
		return -1;
	}		
	for(i=0; i<efuseinfo_num; i++){
		if(efuseinfo[i].version == ver){
			vx = &(efuseinfo[i]);
			break;
		}				
	}
	if(!vx){
		printk("efuse version %d is not supported.\n", ver);
		return -1;
	}	
		
		// BSP setting priority is higher than versiontable
		if(efuse_getinfoex != NULL){
			ret = efuse_getinfoex(id, info);		
			if(ret >= 0)
				return ret;
		}
			
		item = vx->efuseinfo_version;
		size = vx->size;
		ret = -1;		
		for(i=0; i<size; i++, item++){			
			if(id == item->id){
				strcpy(info->title, item->title);				
				info->offset = item->offset;
				info->id = item->id;				
				info->data_len = item->data_len;			
				info->enc_len = item->enc_len;
				info->bch_en = item->bch_en;
				info->bch_reverse = item->bch_reverse;
				ret = 0;
				break;
			}
		}
		
		if(ret < 0)
			printk("ID:%d is not found.\n", id);
			
		return ret;
}


int check_if_efused(loff_t pos, size_t count)
{
	loff_t local_pos = pos;	
	int i;
	unsigned char* buf = NULL;
	efuseinfo_item_t info;
	unsigned enc_len ;		
	
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printk("not found the position:%lld.\n", pos);
		return -1;
	}
	 if(count>info.data_len){
		printk("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printk("data length: 0 is error!\n");
		return -1;
	}
	
	enc_len = info.enc_len;			
	buf = (unsigned char*)kzalloc(sizeof(char)*enc_len, GFP_KERNEL);
	if (buf) {
		if (__efuse_read(buf, enc_len, &local_pos) == enc_len) {
			for (i = 0; i < enc_len; i++) {
				if (buf[i]) {
					printk("pos %d value is %d", (size_t)(pos + i), buf[i]);
					return 1;
				}
			}
		}
	} else {
		printk("no memory \n");
		return -ENOMEM;
	}
	kfree(buf);
	buf = NULL;
	return 0;
}

int efuse_read_item(char *buf, size_t count, loff_t *ppos)
{	

	unsigned enc_len;			
	char* enc_buf = NULL;
	char* data_buf=NULL;
	
	char *penc = NULL;
	char *pdata = NULL;		
	int reverse = 0;
	unsigned pos = (unsigned)*ppos;
	efuseinfo_item_t info;	
		
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printk("not found the position:%d.\n", pos);
		return -1;
	}		
	
	if(count>info.data_len){
		printk("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printk("data length: 0 is error!\n");
		return -1;
	}
	
	enc_len = info.enc_len;
	reverse=info.bch_reverse;					
	enc_buf = (char*)kzalloc(sizeof(char)*EFUSE_BYTES, GFP_KERNEL);
	if (!enc_buf) {
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}		
	data_buf = (char*)kzalloc(sizeof(char)*EFUSE_BYTES, GFP_KERNEL);
	if(!data_buf){
		if(enc_buf)
			kfree(enc_buf);
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}		
		
	penc = enc_buf;
	pdata = data_buf;			
	if(info.bch_en){						
		__efuse_read(enc_buf, enc_len, ppos);		
		while(enc_len >= 31){
			if(efuse_bch_dec(penc, 31, pdata, reverse)<0){
				if(data_buf)
					kfree(data_buf);
				if(enc_buf)
					kfree(enc_buf);
				return -ENOMEM;
			}
			penc += 31;
			pdata += 30;
			enc_len -= 31;
		}
		if((enc_len > 0))
			if(efuse_bch_dec(penc, enc_len, pdata, reverse)<0){
				if(data_buf)
					kfree(data_buf);
				if(enc_buf)
					kfree(enc_buf);
			}
				return -ENOMEM;
	}	
	else
		__efuse_read(pdata, enc_len, ppos);	
		
	memcpy(buf, data_buf, count);		
		
	if(enc_buf)
		kfree(enc_buf);
	if(data_buf)
		kfree(data_buf);
	return count;	
}

int efuse_write_item(char *buf, size_t count, loff_t *ppos)
{
	char* enc_buf = NULL;
	char* data_buf=NULL;
	char *pdata = NULL;
	char *penc = NULL;			
	unsigned enc_len,data_len, reverse;
	unsigned pos = (unsigned)*ppos;	
	efuseinfo_item_t info;
		
	if(efuse_getinfo_byPOS(pos, &info) < 0){
		printk("not found the position:%d.\n", pos);
		return -1;
	}
	
	if(count>info.data_len){
		printk("data length: %d is out of EFUSE layout!\n", count);
		return -1;
	}
	if(count == 0){
		printk("data length: 0 is error!\n");
		return -1;
	}	
		
	enc_buf = (char*)kzalloc(sizeof(char)*EFUSE_BYTES, GFP_KERNEL);
	if (!enc_buf) {
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}		
	data_buf = (char*)kzalloc(sizeof(char)*EFUSE_BYTES, GFP_KERNEL);
	if(!data_buf){
		if(enc_buf)
			kfree(enc_buf);
		printk(KERN_INFO "memory not enough\n"); 
		return -ENOMEM;
	}		
	
	memcpy(data_buf, buf, count)	;	
	pdata = data_buf;
	penc = enc_buf;			
	enc_len=info.enc_len;
	data_len=info.data_len;
	reverse = info.bch_reverse;
	
	if(info.bch_en){				
		while(data_len >= 30){
			if(efuse_bch_enc(pdata, 30, penc, reverse)<0){
				if(enc_buf)
					kfree(enc_buf);
				if(data_buf)
					kfree(data_buf);
				return -ENOMEM;
			}
			data_len -= 30;
			pdata += 30;
			penc += 31;		
		}
		if(data_len > 0)
			if(efuse_bch_enc(pdata, data_len, penc, reverse)<0){
				if(enc_buf)
					kfree(enc_buf);
				if(data_buf)
					kfree(data_buf);
				return -ENOMEM;
			}
	}	
	else
		memcpy(penc, pdata, enc_len);
	
	__efuse_write(enc_buf, enc_len, ppos);
	
	if(enc_buf)
		kfree(enc_buf);
	if(data_buf)
		kfree(data_buf);
		
	return enc_len ;		
}
static uint32_t __v3_get_gap_start(uint32_t id)
{

    if(cpu_is_before_m6())///M3
    {

        return id*36 + 112;
    }
    return id*36 + 136; ///M6

}

static uint32_t __v3_check_dirty(size_t count, loff_t pos )
{
	unsigned char* buf = NULL;
	loff_t local_pos=pos;
	int i, error=0; //efuse was not write

	buf = (char*)kzalloc(count, GFP_KERNEL);
	if (!buf) {
		printk(KERN_INFO "memory not enough,%s:%d\n",__FILE__,__LINE__);
		return -ENOMEM;
	}

	if (__efuse_read(buf, count, &local_pos) == count) {
		for (i = 0; i < count; i++) {
			if (buf[i]) {
				printk("pos %d value is %d", (size_t)(pos + i), buf[i]);
				error =1; //efuse was write
				//return 1;
				break;
			}
		}
	}

	if(buf)
		kfree(buf);
	return error;
}

int32_t __v3_read_hash(uint32_t id,char * buf)
{
    loff_t off;
    char  temp[36];
#ifdef CONFIG_EFUSE_LAYOUT_VERSION
    if((CONFIG_EFUSE_LAYOUT_VERSION >= efuseinfo_num)||(CONFIG_EFUSE_LAYOUT_VERSION != efuseinfo[CONFIG_EFUSE_LAYOUT_VERSION].version))
    {
	printk("efuse layout version error,%s:%d\n",__FILE__,__LINE__);
	return -EINVAL;
    }
#else
    if(efuse_readversion()<3)
        return -EINVAL;
#endif
    off=__v3_get_gap_start(id);
    if(cpu_is_before_m6())
    {
        __efuse_read(temp,36,&off);
        if(efuse_bch_dec(temp,18,buf,0)<0)
			return -ENOMEM;
        if(efuse_bch_dec(&temp[18],18,&buf[17],0)<0)
			return -ENOMEM;

    }else{
        __efuse_read(buf,34,&off);
    }
    /**
     * @todo EFUSE Not Implement
     */
    return 0;
}
int32_t __v3_write_hash(uint32_t id,char * buf)
{
    loff_t off;
    char temp[36];
    int error=0;
#ifdef CONFIG_EFUSE_LAYOUT_VERSION
    if((CONFIG_EFUSE_LAYOUT_VERSION >= efuseinfo_num)||(CONFIG_EFUSE_LAYOUT_VERSION != efuseinfo[CONFIG_EFUSE_LAYOUT_VERSION].version))
    {
	printk("efuse layout version error,%s:%d\n",__FILE__,__LINE__);
	return -EINVAL;
    }
#else
    if (efuse_readversion() < 3)
        return -EINVAL;
#endif
    /**
     * @todo EFUSE Not Implement
     */
    off=__v3_get_gap_start(id);
    if (cpu_is_before_m6()) {
        if(efuse_bch_enc(buf, 17, temp, 0)<0)
			return -ENOMEM;
        if(efuse_bch_enc(&buf[17], 17, &temp[18], 0)<0)
			return -ENOMEM;
        if(!__v3_check_dirty(36, off ))
        {
            __efuse_write(temp, 36, &off);
        }
        else
        {
            printk("efuse write fail\n");
            error = -1;
        }
    } else {
        if(!__v3_check_dirty(34, off ))
        {
            __efuse_write(buf, 34, &off);
        }
        else
        {
            printk("efuse write fail\n");
            error = -1;
        }
    }
    return error;
}
int32_t __v3_check_key_installed(uint32_t id)
{
    char temp[36];
    int i;
#ifdef CONFIG_EFUSE_LAYOUT_VERSION
    if((CONFIG_EFUSE_LAYOUT_VERSION >= efuseinfo_num)||(CONFIG_EFUSE_LAYOUT_VERSION != efuseinfo[CONFIG_EFUSE_LAYOUT_VERSION].version))
    {
	printk("efuse layout version error,%s:%d\n",__FILE__,__LINE__);
	return -EINVAL;
    }
#else
    if(efuse_readversion()<3)
        return -EINVAL;
#endif
    memset(temp,0,sizeof(temp));
    __v3_read_hash(id,temp);
    for(i=0;i<sizeof(temp);i++)
    {
        if(temp[i]!=0)
            return 1;
    }

    return 0;
}

/*void efuse_dump(char* pbuffer)
{
	loff_t pos = 0;	
	__efuse_read(pbuffer, 512, &pos);
}*/
