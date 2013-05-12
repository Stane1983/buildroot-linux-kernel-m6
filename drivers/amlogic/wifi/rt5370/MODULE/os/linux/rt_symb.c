#ifdef OS_ABL_SUPPORT

#include <linux/module.h>
#include "rt_config.h"

EXPORT_SYMBOL(RTMP_DRV_OPS_FUNCTION);

#ifdef RESOURCE_BOOT_ALLOC
EXPORT_SYMBOL(rtusb_tx_buf_len);
EXPORT_SYMBOL(rtusb_rx_buf_len);
EXPORT_SYMBOL(rtusb_tx_buf_cnt);
EXPORT_SYMBOL(rtusb_rx_buf_cnt);
#endif /* RESOURCE_BOOT_ALLOC */
#endif /* OS_ABL_SUPPORT */

/* End of rt_symb.c */
