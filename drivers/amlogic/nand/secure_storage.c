#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/crc32.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <mach/nand.h>

static unsigned default_environment_size = 0;

static int aml_nand_read_secure (struct mtd_info *mtd, loff_t offset, u_char * buf)
{
	struct secure_oobinfo_t *secure_oobinfo;
	struct mtd_oob_ops	* aml_oob_ops;
	int error = 0, err;
	loff_t addr = 0, len, amount_loaded=0;
	unsigned char *data_buf;
	unsigned char secure_oob_buf[sizeof(struct secure_oobinfo_t)];

	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	if (!aml_chip->aml_nandsecure_info->secure_valid)
		return 1;

	addr = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
	addr *= mtd->erasesize;
	addr += aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr * mtd->writesize;
	printk("aml_nand_read_secure:  read from valid addr: %llx  at block %d page %d\n", (uint64_t)addr, \
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr, \
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr);
	
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}

	secure_oobinfo = (struct secure_oobinfo_t *)secure_oob_buf;
	while (amount_loaded < CONFIG_SECURE_SIZE ) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct secure_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = secure_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

		error = mtd->read_oob(mtd, addr, aml_oob_ops);
		if ((error != 0) && (error != -EUCLEAN)) {
			printk("blk check good but read failed: %llx, %d\n", (uint64_t)addr, error);
			err = 1;
			goto exit;
		}

				    //if (memcmp(secure_oobinfo->name, SECURE_STORE_MAGIC, 4))
		if ((secure_oobinfo->name != SECURE_STORE_MAGIC))
			printk("invalid nand secure info magic: %llx\n", (uint64_t)addr);

		addr += mtd->writesize;
		len = min(mtd->writesize, CONFIG_SECURE_SIZE - amount_loaded);
		memcpy(buf + amount_loaded, data_buf, len);
		amount_loaded += mtd->writesize;
	}
	if (amount_loaded < CONFIG_SECURE_SIZE){
		err = 1;
		goto exit;
	}

	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;

exit:
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	return err;
}

static int aml_nand_write_secure(struct mtd_info *mtd, loff_t offset, u_char *buf)
{
	struct secure_oobinfo_t *secure_oobinfo;
	struct mtd_oob_ops	* aml_oob_ops;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	unsigned char secure_oob_buf[sizeof(struct secure_oobinfo_t)];
	size_t len, amount_saved = 0;
	unsigned char *data_buf;
	int error = 0, err;
	loff_t addr = 0;

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}

	addr = offset;
	secure_oobinfo = (struct secure_oobinfo_t *)secure_oob_buf;
	//memcpy(secure_oobinfo->name, SECURE_STORE_MAGIC, 4);
	secure_oobinfo->name = SECURE_STORE_MAGIC;
	secure_oobinfo->timestamp = aml_chip->aml_nandsecure_info->secure_valid_node->timestamp;

	while (amount_saved < CONFIG_SECURE_SIZE ) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct secure_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = secure_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		len = min(mtd->writesize, CONFIG_SECURE_SIZE - amount_saved);
		memcpy((unsigned char *)aml_oob_ops->datbuf, buf + amount_saved, len);

		error = mtd->write_oob(mtd, addr, aml_oob_ops);
		if (error) {
			printk("blk check good but write failed: %llx, %d\n", (uint64_t)addr, error);
			err = 1;
			goto exit;
		}

		addr += mtd->writesize;;
		amount_saved += mtd->writesize;
	}

	if (amount_saved < CONFIG_SECURE_SIZE){
		err = 1;
		goto exit;
	}

	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;

exit:
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	return err;
}

int aml_nand_save_secure(struct mtd_info *mtd, u_char *buf, unsigned int size)
{
	struct env_free_node_t *secure_free_node, *secure_tmp_node;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct erase_info  *nand_erase_info;
	int error = 0, pages_per_blk, i = 1;
	loff_t addr = 0;
	secure_t *secure_ptr = (secure_t *)buf;

	if (!aml_chip->aml_nandsecure_info->secure_init)
		return 1;

	nand_erase_info = kzalloc(sizeof(struct erase_info), GFP_KERNEL);
	if (nand_erase_info == NULL){
		printk("%s %d no mem for nand_erase_info\n", __func__, __LINE__);
		error = -ENOMEM;
		goto exit;
	}

	pages_per_blk = mtd->erasesize / mtd->writesize;
	if ((mtd->writesize < CONFIG_SECURE_SIZE) && (aml_chip->aml_nandsecure_info->secure_valid == 1))
		i = (CONFIG_SECURE_SIZE + mtd->writesize - 1) / mtd->writesize;

	if (aml_chip->aml_nandsecure_info->secure_valid) {
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr += i;
		if ((aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr + i) > pages_per_blk) {

			secure_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
			if (secure_free_node == NULL){
				printk("%s %d no mem for secure_free_node\n", __func__, __LINE__);
				error = -ENOMEM;
				goto exit;
			}

            secure_tmp_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
			if (secure_tmp_node == NULL){
				printk("%s %d no mem for secure_tmp_node\n", __func__, __LINE__);
				error = -ENOMEM;
				goto exit;
			}

			secure_tmp_node = aml_chip->aml_nandsecure_info->secure_valid_node;

			secure_free_node = aml_chip->aml_nandsecure_info->secure_free_node;

            aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr = secure_free_node->phy_blk_addr;
			aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr = 0;
			aml_chip->aml_nandsecure_info->secure_valid_node->timestamp += 1;
			aml_chip->aml_nandsecure_info->secure_free_node = secure_tmp_node;
		}
	}
	else {

		secure_tmp_node = aml_chip->aml_nandsecure_info->secure_free_node;
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr = secure_tmp_node->phy_blk_addr;
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr = 0;
		aml_chip->aml_nandsecure_info->secure_valid_node->timestamp += 1;
		aml_chip->aml_nandsecure_info->secure_free_node = secure_tmp_node->next;
		kfree(secure_tmp_node);
	}

	addr = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
	addr *= mtd->erasesize;
	addr += aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr * mtd->writesize;

	if (aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr == 0) {

		memset(nand_erase_info, 0, sizeof(struct erase_info));
		nand_erase_info->mtd = mtd;
		nand_erase_info->addr = addr;
		nand_erase_info->len = mtd->erasesize;

		error = mtd->erase(mtd, nand_erase_info);
		if (error) {
			printk("secure free blk erase failed %d\n", error);
			mtd->block_markbad(mtd, addr);
			goto exit;
		}
	}

//	secure_ptr->crc = (crc32((0 ^ 0xffffffffL), secure_ptr->data, SECURE_SIZE) ^ 0xffffffffL);

	if (aml_nand_write_secure(mtd, addr, (u_char *) secure_ptr)) {
		printk("nand secure info update FAILED!\n");
		error = 1;
		goto exit;
	}
    printk("nand secure info save Ok\ns");
	kfree(nand_erase_info);
	return error;
exit:
	if (nand_erase_info) {
		kfree(nand_erase_info);
		nand_erase_info = NULL;
	}
	if (secure_free_node) {
		kfree(secure_free_node);
		secure_free_node = NULL;
	}
	if (secure_tmp_node) {
		kfree(secure_tmp_node);
		secure_tmp_node = NULL;
	}
	return error;
}


static int aml_nand_secure_init(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = &aml_chip->chip;
	struct secure_oobinfo_t *secure_oobinfo;
	struct env_free_node_t *secure_free_node, *secure_tmp_node, *secure_prev_node;
	int error = 0, err, start_blk, tmp_blk, secure_blk, i, j, pages_per_blk, bad_blk_cnt = 0, max_env_blk, phys_erase_shift;
	loff_t offset;
	unsigned char *data_buf;
	unsigned int remain_start_block,remain_tatol_block,remain_block,total_blk;
	
	struct mtd_oob_ops	*aml_oob_ops;
	unsigned char secure_oob_buf[sizeof(struct secure_oobinfo_t)];

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	aml_chip->aml_nandsecure_info = kzalloc(sizeof(struct aml_nandsecure_info_t), GFP_KERNEL);
	if (aml_chip->aml_nandsecure_info == NULL){
		printk("%s %d no mem for aml_chip->aml_nandsecure_info \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}

	aml_chip->aml_nandsecure_info->mtd = mtd;
	aml_chip->aml_nandsecure_info->secure_valid_node = kzalloc(sizeof(struct env_valid_node_t), GFP_KERNEL);
	if (aml_chip->aml_nandsecure_info->secure_valid_node == NULL){
		printk("%s %d no mem for aml_chip->aml_nandsecure_info->secure_valid_node \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr = -1;

	phys_erase_shift = fls(mtd->erasesize) - 1;
	max_env_blk = NAND_SECURE_BLK;

	offset = mtd->size - mtd->erasesize;
	total_blk = (int)(offset >> phys_erase_shift);

	pages_per_blk = (1 << (chip->phys_erase_shift - chip->page_shift));
	secure_oobinfo = (struct secure_oobinfo_t *)secure_oob_buf;
#ifdef CONFIG_AML_NAND_KEY
	remain_tatol_block = REMAIN_BLOCK_NUM;
	remain_block = 0;
	remain_start_block = aml_chip->aml_nandkey_info->start_block;
	do{
		offset = mtd->erasesize;
		offset *= remain_start_block;
		error = mtd->block_isbad(mtd, offset);
		if (error == FACTORY_BAD_BLOCK_ERROR) {			
			remain_start_block++;
			continue;
		}
		remain_start_block++;
	}while(++remain_block< remain_tatol_block);

	aml_chip->aml_nandsecure_info->start_block = remain_start_block;
	printk("%s,%d : secure start blk %d \n",__func__,__LINE__,aml_chip->aml_nandsecure_info->start_block);	
#else
	offset = mtd->size - mtd->erasesize;
	remain_start_block = (int)(offset >> phys_erase_shift);
	remain_block = 0;
	remain_tatol_block = REMAIN_BLOCK_NUM;
	aml_chip->aml_nandsecure_info->start_block=remain_start_block;
	aml_chip->aml_nandsecure_info->end_block=remain_start_block;
	bad_blk_cnt=0;
	do{
		offset = mtd->erasesize;
		offset *= remain_start_block;
		error = mtd->block_isbad(mtd, offset);
		if (error == FACTORY_BAD_BLOCK_ERROR) {			
			aml_chip->aml_nandsecure_info->start_block--;
			remain_start_block--;
			continue;
		}
		remain_start_block--;
	}while(++remain_block< remain_tatol_block);
	aml_chip->aml_nandsecure_info->start_block -= (remain_block-1);
	printk("key start_blk=%d,end_blk=%d,%s:%d\n",aml_chip->aml_nandsecure_info->start_block,aml_chip->aml_nandsecure_info->end_block,__func__,__LINE__);
#endif

	tmp_blk = start_blk = aml_chip->aml_nandsecure_info->start_block;
	secure_blk = 0;
	do {

		offset = mtd->erasesize;
		offset *= start_blk;
		error = mtd->block_isbad(mtd, offset);
		if (error) {
			continue;
		}

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct secure_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = secure_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

		error = mtd->read_oob(mtd, offset, aml_oob_ops);
		if ((error != 0) && (error != -EUCLEAN)) {
			printk("blk check good but read failed: %llx, %d\n", (uint64_t)offset, error);
			continue;
		}

		aml_chip->aml_nandsecure_info->secure_init = 1;
				    //if (secure_oobinfo->name == SECURE_STORE_MAGIC)) {
                //if (!memcmp(secure_oobinfo->name, SECURE_STORE_MAGIC, 4)) {
		if ((secure_oobinfo->name == SECURE_STORE_MAGIC)) {
			aml_chip->aml_nandsecure_info->secure_valid = 1;
			if (aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr >= 0) {
				secure_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
				if (secure_free_node == NULL){
					printk("%s %d no mem for secure_free_node\n", __func__, __LINE__);
					err = -ENOMEM;
					goto exit;
				}
				secure_free_node->dirty_flag = 1;
				if (secure_oobinfo->timestamp > aml_chip->aml_nandsecure_info->secure_valid_node->timestamp) {

					secure_free_node->phy_blk_addr = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
					aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr = start_blk;
					aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr = 0;
					aml_chip->aml_nandsecure_info->secure_valid_node->timestamp = secure_oobinfo->timestamp;
				}
				else {
					secure_free_node->phy_blk_addr = start_blk;
				}
				if (aml_chip->aml_nandsecure_info->secure_free_node == NULL)
					aml_chip->aml_nandsecure_info->secure_free_node = secure_free_node;
				else {
					secure_tmp_node = aml_chip->aml_nandsecure_info->secure_free_node;
					while (secure_tmp_node->next != NULL) {
						secure_tmp_node = secure_tmp_node->next;
					}
					secure_tmp_node->next = secure_free_node;
				}
			}
			else {

				aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr = start_blk;
				aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr = 0;
				aml_chip->aml_nandsecure_info->secure_valid_node->timestamp = secure_oobinfo->timestamp;
			}
		}
		else if (secure_blk < max_env_blk) {
			secure_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
			if (secure_free_node == NULL){
					printk("%s %d no mem for secure_free_node\n", __func__, __LINE__);
					err = -ENOMEM;
					goto exit;
			}
			secure_free_node->phy_blk_addr = start_blk;
			if (aml_chip->aml_nandsecure_info->secure_free_node == NULL){
				aml_chip->aml_nandsecure_info->secure_free_node = secure_free_node;
			}
			else {
				secure_tmp_node = aml_chip->aml_nandsecure_info->secure_free_node;
				secure_prev_node = secure_tmp_node;
				while (secure_tmp_node != NULL) {
					if (secure_tmp_node->dirty_flag == 1)
						break;
					secure_prev_node = secure_tmp_node;
					secure_tmp_node = secure_tmp_node->next;
				}
				if (secure_prev_node == secure_tmp_node) {
					secure_free_node->next = secure_tmp_node;
					aml_chip->aml_nandsecure_info->secure_free_node = secure_free_node;
				}
				else {
					secure_prev_node->next = secure_free_node;
					secure_free_node->next = secure_tmp_node;
				}
			}
		}
		secure_blk++;

		if ((secure_blk >= max_env_blk) && (aml_chip->aml_nandsecure_info->secure_valid == 1))
			break;
	} while ((++start_blk) <= total_blk);

	if (aml_chip->aml_nandsecure_info->secure_valid == 1) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct secure_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = secure_oob_buf;

		for (i=0; i<pages_per_blk; i++) {

			memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
			memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

			offset = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
			offset *= mtd->erasesize;
			offset += i * mtd->writesize;
			error = mtd->read_oob(mtd, offset, aml_oob_ops);
			if ((error != 0) && (error != -EUCLEAN)) {
				printk("blk check good but read failed: %llx, %d\n", (uint64_t)offset, error);
				continue;
			}
   //if (!memcmp(secure_oobinfo->name, SECURE_STORE_MAGIC, 4))
			if ((secure_oobinfo->name == SECURE_STORE_MAGIC))
				aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr = i;
			else
				break;
		}
	}
	if ((mtd->writesize < CONFIG_SECURE_SIZE) && (aml_chip->aml_nandsecure_info->secure_valid == 1)) {
		i = (CONFIG_SECURE_SIZE + mtd->writesize - 1) / mtd->writesize;
		aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr -= (i - 1);
	}

	printk("secure_valid_node->add =%d\n",aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr);
	printk("secure_free_node->add =%d\n",aml_chip->aml_nandsecure_info->secure_free_node->phy_blk_addr);

	offset = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
	offset *= mtd->erasesize;
	offset += aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr * mtd->writesize;
	printk("aml nand secure info valid addr: %llx \n", (uint64_t)offset);

	printk(KERN_DEBUG "CONFIG_SECURE_SIZE=0x%x; \n",CONFIG_SECURE_SIZE);

	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;

exit:
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (aml_chip->aml_nandsecure_info) {
		kfree(aml_chip->aml_nandsecure_info);
		aml_chip->aml_nandsecure_info = NULL;
	}
	if (aml_chip->aml_nandsecure_info->secure_valid_node ) {
		kfree(aml_chip->aml_nandsecure_info->secure_valid_node );
		aml_chip->aml_nandsecure_info->secure_valid_node = NULL;
	}
	if (secure_free_node ) {
		kfree(secure_free_node );
		secure_free_node = NULL;
	}
	return err;

}

static int secure_info_check(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct platform_nand_chip *chip = &plat->platform_nand_data.chip;
	secure_t *secure_ptr;
	int error = 0, start_blk, total_blk, i, j, nr, phys_erase_shift, update_secure_flag = 0;
	loff_t offset;

	error = aml_nand_secure_init(mtd);
	if (error)
		return error;

	secure_ptr = kzalloc(sizeof(secure_t), GFP_KERNEL);
	if (secure_ptr == NULL)
		return -ENOMEM;

    memset(secure_ptr, 0xa5, sizeof(secure_t)); //set default data a5

	if (aml_chip->aml_nandsecure_info->secure_valid == 1){

            goto exit;
#if 0
		offset = aml_chip->aml_nandsecure_info->secure_valid_node->phy_blk_addr;
		offset *= mtd->erasesize;
		offset += aml_chip->aml_nandsecure_info->secure_valid_node->phy_page_addr * mtd->writesize;

		error = aml_nand_read_secure(mtd, offset, (u_char *)secure_ptr);
		if (error) {
			printk("nand secure info read failed: %llx, %d\n", (uint64_t)offset, error);
			goto exit;
		}
#endif
	}else{
		error = aml_nand_save_secure(mtd, (u_char *)secure_ptr, CONFIG_SECURE_SIZE);
		if(error){
			printk("nand secure info save failed\n");
		}
	}


exit:
	kfree(secure_ptr);
	return error;

}

#ifdef CONFIG_SECURE_NAND

struct device *devp;
static dev_t nand_secure_devno;
struct mtd_info *nand_secure_mtd = NULL;
#define NAND_SECURE_DEVICE_NAME	"nand_secure"

static int nand_secure_open(struct inode * inode, struct file * filp)
{
	return 0;
}

static ssize_t nand_secure_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	secure_t *secure_ptr = NULL;
	ssize_t read_size;
	int error = 0,ret=0;
	if(*ppos == CONFIG_SECURE_SIZE)
	{
		return 0;
	}

	if(*ppos >= CONFIG_SECURE_SIZE)
	{
		printk(KERN_ERR "nand secure: data access violation!\n");
		return -EFAULT;
	}

	secure_ptr = kzalloc(sizeof(secure_t), GFP_KERNEL);
	if (secure_ptr == NULL)
	{
		return -ENOMEM;
	}

	error = aml_nand_read_secure(nand_secure_mtd, 0, (u_char *)secure_ptr);

	if (error)
	{
		printk("nand_secure_read: nand secure read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(secure_ptr);
		return -EFAULT;
	}

	if((*ppos + count) > CONFIG_SECURE_SIZE)
	{
		read_size = CONFIG_SECURE_SIZE - *ppos;
	}
	else
	{
		read_size = count;
	}

	ret=copy_to_user(buf, (secure_ptr + *ppos), read_size);

	*ppos += read_size;

	kfree(secure_ptr);
	return read_size;
}


static ssize_t nand_secure_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	u_char *secure_ptr = NULL;
	ssize_t write_size;
	int error = 0,ret;
	if(*ppos == CONFIG_SECURE_SIZE)
	{
		return 0;
	}

	if(*ppos >= CONFIG_SECURE_SIZE)
	{
		printk(KERN_ERR "nand secure: data access violation!\n");
		return -EFAULT;
	}

	secure_ptr = kzalloc(sizeof(secure_t), GFP_KERNEL);
	if (secure_ptr == NULL)
	{
		return -ENOMEM;
	}

	error = aml_nand_read_secure (nand_secure_mtd, 0, (u_char *)secure_ptr);

	if (error)
	{
		printk("nand_secure_read: nand secure info read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(secure_ptr);
		return -EFAULT;
	}

	if((*ppos + count) > CONFIG_SECURE_SIZE)
	{
		write_size = CONFIG_SECURE_SIZE - *ppos;
	}
	else
	{
		write_size = count;
	}

	ret=copy_from_user((secure_ptr + *ppos), buf, write_size);

	error = aml_nand_save_secure(nand_secure_mtd, secure_ptr, CONFIG_SECURE_SIZE);

	if (error)
	{
		printk("nand_secure_read: nand secure info read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(secure_ptr);
		return -EFAULT;
	}

	*ppos += write_size;

	kfree(secure_ptr);
	return write_size;
}


static int nand_secure_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int nand_secure_cls_suspend(struct device *dev, pm_message_t state)
{
		return 0;
}

static int nand_secure_cls_resume(struct device *dev)
{
	return 0;
}


static struct class nand_secure_class = {

	.name = "nand_secure",
	.owner = THIS_MODULE,
	.suspend = nand_secure_cls_suspend,
	.resume = nand_secure_cls_resume,
};

static struct file_operations nand_secure_fops = {
    .owner	= THIS_MODULE,
    .open	= nand_secure_open,
    .read	= nand_secure_read,
    .write	= nand_secure_write,
    .release	= nand_secure_close,
};

#endif


int secure_device_init(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct platform_nand_chip *chip = &plat->platform_nand_data.chip;
	struct aml_nand_bbt_info *nand_bbt_info;
	struct aml_nand_part_info *aml_nand_part;
	struct mtd_partition *parts;
	env_t *env_ptr;
	int ret = 0, start_blk, total_blk, i, j, nr, phys_erase_shift;
	loff_t offset;

	ret = secure_info_check(mtd);
	if(ret){
		printk("invalid secure info\n");
	}

#ifdef CONFIG_SECURE_NAND
	printk("nand secure store: nand_secure probe. \n");

	nand_secure_mtd = mtd;

	ret = alloc_chrdev_region(&nand_secure_devno, 0, 1, NAND_SECURE_DEVICE_NAME);

	if (ret < 0) {
		pr_err("nand_secure: failed to allocate chrdev. \n");
		goto exit_erro;
	}

	cdev_init(&aml_chip->nand_secure_cdev, &nand_secure_fops);
	aml_chip->nand_secure_cdev.owner = THIS_MODULE;

	ret = cdev_add(&aml_chip->nand_secure_cdev, nand_secure_devno, 1);
	if (ret) {
		pr_err("nand secure: failed to add device. \n");
		goto exit_erro;
	}

	ret = class_register(&nand_secure_class);
	if (ret < 0) {
		printk(KERN_NOTICE "class_register(&nand_secure_class) failed!\n");
		goto exit_erro;
	}

	devp = device_create(&nand_secure_class, NULL, nand_secure_devno, NULL, "nand_secure");
	if (IS_ERR(devp)) {
		printk(KERN_ERR "nand_secure: failed to create device node\n");
		ret = PTR_ERR(devp);
		goto exit_erro;
	}

#endif

	return ret;

exit_erro:
	return ret;

}


