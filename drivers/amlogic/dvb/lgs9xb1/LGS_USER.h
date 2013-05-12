
#ifndef _LGS_USER_H_
#define _LGS_USER_H_

#include "LGS_TYPES.h"

LGS_RESULT lgs_read(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal);
LGS_RESULT lgs_write(UINT8 secAddr, UINT8 regAddr, UINT8 regVal);
LGS_RESULT lgs_read_multibyte(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len);
LGS_RESULT lgs_write_multibyte(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len);
void lgs_wait(UINT16 millisecond);


#endif
