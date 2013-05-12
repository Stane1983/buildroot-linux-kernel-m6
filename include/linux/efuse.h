#ifndef __EFUSE_H
#define __EFUSE_H

#include <linux/ioctl.h>

#define EFUSE_ENCRYPT_DISABLE   _IO('f', 0x10)
#define EFUSE_ENCRYPT_ENABLE    _IO('f', 0x20)
#define EFUSE_ENCRYPT_RESET     _IO('f', 0x30)
#define EFUSE_INFO_GET				_IO('f', 0x40)

#ifdef __DEBUG
#define __D(fmt, args...) fprintf(stderr, "debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#ifdef __ERROR
#define __E(fmt, args...) fprintf(stderr, "error: " fmt, ## args)
#else
#define __E(fmt, args...)
#endif



#define BCH_T           1
#define BCH_M           8
#define BCH_P_BYTES     30

#ifdef CONFIG_ARCH_MESON6
#define EFUSE_BITS             4096
#define EFUSE_BYTES            512  //(EFUSE_BITS/8)
#define EFUSE_DWORDS            128  //(EFUSE_BITS/32)
#else
#define EFUSE_BITS             3072
#define EFUSE_BYTES            384  //(EFUSE_BITS/8)
#define EFUSE_DWORDS            96  //(EFUSE_BITS/32)
#endif

#define DOUBLE_WORD_BYTES        4
#define EFUSE_IS_OPEN           (0x01)

#define EFUSE_NONE_ID			0
#define EFUSE_VERSION_ID		1
#define EFUSE_LICENCE_ID		2
#define EFUSE_MAC_ID				3
#define EFUSE_MAC_WIFI_ID	4
#define EFUSE_MAC_BT_ID		5
#define EFUSE_HDCP_ID			6
#define EFUSE_USID_ID				7
#define EFUSE_RSA_KEY_ID		8
#define EFUSE_CUSTOMER_ID		9
#define EFUSE_MACHINEID_ID		10

int efuse_bch_enc(const char *ibuf, int isize, char *obuf, int reverse);
int efuse_bch_dec(const char *ibuf, int isize, char *obuf, int reverse);

struct efuse_platform_data {
	loff_t pos;
	size_t count;
	bool (*data_verify)(unsigned char *usid);
};

typedef struct efuseinfo_item{
	char title[40];	
	unsigned id;
	loff_t offset;    // write offset
	unsigned enc_len;
	unsigned data_len;			
	int bch_en;
	int bch_reverse;
} efuseinfo_item_t;


typedef struct efuseinfo{
	struct efuseinfo_item *efuseinfo_version;
	int size;
	int version;	
}efuseinfo_t;

typedef int (*pfn) (unsigned param, efuseinfo_item_t *info); 

#include <linux/cdev.h>

typedef struct 
{
    struct cdev cdev;
    unsigned int flags;
} efuse_dev_t;

#define EFUSE_READ_ONLY     


#include <linux/ioctl.h>

#define AML_KEYS_INSTALL_ID     _IO('f', 0x10)
#define AML_KEYS_INSTALL        _IO('f', 0x20)
#if 0
typedef struct {///exported structure
    char name[16];
    uint16_t size;
    uint16_t type;
    char key[1024];
}aml_install_key_t;
#endif
#define AML_KEYS_SET_VERSION    _IO('f', 0x30)
typedef struct aml_keybox_provider_s aml_keybox_provider_t;
struct aml_keybox_provider_s{
	char * name;
	int32_t flag;
	int32_t (* read)(aml_keybox_provider_t * provider,uint8_t * buf,int bytes,int flags);
	int32_t (* write)(aml_keybox_provider_t * provider,uint8_t * buf,int bytes);
	void * priv;
};
int32_t aml_keybox_provider_register(aml_keybox_provider_t * provider);
aml_keybox_provider_t * aml_keybox_provider_get(char * name);
#endif
