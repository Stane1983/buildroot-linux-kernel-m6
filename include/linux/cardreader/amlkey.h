#ifndef AMLKEY_H
#define AMLKEY_H

#define MINIKEY_PART_SIZE				0x800000
#define CONFIG_KEYSIZE         		0x2000
#define ENV_KEY_MAGIC					"keyx"
#define AML_KEY_DEVICE_NAME	"nand_key"


struct key_oobinfo_t {
	char name[4];
    int16_t  ec;
    unsigned        timestamp: 15;
    unsigned       status_page: 1;
};

struct key_valid_node_t {
	int16_t  ec;
	int16_t	phy_blk_addr;
	int16_t	phy_page_addr;
	int timestamp;
};

struct key_free_node_t {
	int16_t  ec;
	int16_t	phy_blk_addr;
	int dirty_flag;
	struct key_free_node_t *next;
};

struct aml_key_info_t {
	void   *owner;
	struct key_valid_node_t *env_valid_node;
	struct key_free_node_t *env_free_node;
	u_char key_valid;
	u_char key_init;
//	int key_phy_addr;
//	int key_phy_size;
};


#define KEYSIZE (CONFIG_KEYSIZE - (sizeof(uint32_t)))
typedef	struct  {
	uint32_t	crc;		/* CRC32 over data bytes	*/
	unsigned char	data[KEYSIZE]; /* Environment data		*/
} mesonkey_t;



#endif