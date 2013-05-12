/***************************************************************************************
*					             
*			(c) Copyright 2008, LegendSilicon, beijing, China
*
*				         All Rights Reserved
*
* Description : Legend Silicon Demod Library header file
*
* Notice: DO NOT change the file in your project
*
***************************************************************************************/
#ifndef _LGS_TYPES_H_
#define _LGS_TYPES_H_

/////////////////////////////////////////////////////////////////
/// Definition of basic data type
/////////////////////////////////////////////////////////////////
typedef unsigned char	LGS_RESULT;
typedef unsigned char	LGS_HANDLE;

#define MAX_NAME_LEN		128
#define TIMEOUT			100

#ifndef INT8
	#define INT8	char
#endif

#ifndef INT16
	#define INT16	short
#endif

#ifndef INT32
	#define INT32	long
#endif

#ifndef UINT8
	#define UINT8	unsigned char
#endif

#ifndef UINT16
	#define UINT16	unsigned short
#endif

#ifndef UINT32
	#define UINT32	unsigned long
#endif

#ifndef BOOL
	#define BOOL	int
#endif

#ifndef NULL
	#define NULL    0
#endif

#ifndef _TCHAR_DEFINED
	#define	TCHAR short int
#endif

#ifndef _T
	#define _T(x)      L ## x
#endif

/////////////////////////////////////////////////////////////////
/// Definition of function's return value
/////////////////////////////////////////////////////////////////
#define LGS_NO_ERROR				0
#define LGS_REGISTER_ERROR			0xFF	//�Ĵ�������
#define LGS_I2C_OPEN_ERROR			0xFE	//��I2C����
#define LGS_I2C_READ_ERROR			0xFD	//��I2C����
#define LGS_I2C_WRITE_ERROR			0xFC	//дI2C����
#define LGS_I2C_CLOSE_ERROR			0xFB	//�ر�I2C����
#define LGS_NO_LOCKED				0xFA	//�ź�δ����
#define LGS_AUTO_DETECT_FAILED		0xF9    //�Զ����ʧ��
#define LGS_FREQUENCY_ERROR			0xF8	//Ƶ�ʴ���
#define LGS_NO_DEFINE				0xF7	//�޶���
#define LGS_NO_MAGIC				0xF6	//MAGICֵ��ƥ��,����������
#define LGS_NO_DEVICE				0xF5
#define LGS_DETECT_ERROR			0xF4
#define LGS_OPENECHO_ERROR			0xF3
#define LGS_CLOSEECHO_ERROR			0xF2
#define LGS_SPI_WRITE_ERROR			0xF1
#define LGS_SET_FREQ_ERROR			0xF0    //��Ƶ����
#define LGS_PARA_ERROR				0xEF	//��������

#define LGS_ERROR				0xE0

#define LGS_CHANGE_MODE_ERROR			0xDF
#define LGS_SET_MANUAL_ERROR			0xDE
#define LGS_USB_READ_ERROR			0xDD
#define LGS_USB_WRITE_ERROR			0xDC
#define LGS_TIMEOUT_ERROR			0xDB

////////////////////////////////////////////////////////////////
// �Ĵ�����д����ԭ�Ͷ���
////////////////////////////////////////////////////////////////
typedef LGS_RESULT (*LGS_REGISTER_READ)(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal);
typedef LGS_RESULT (*LGS_REGISTER_WRITE)(UINT8 secAddr, UINT8 regAddr, UINT8 regVal);

typedef LGS_RESULT (*LGS_REGISTER_READ_MULTIBYTE)(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len);
typedef LGS_RESULT (*LGS_REGISTER_WRITE_MULTIBYTE)(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len);

typedef void (*LGS_WAIT)(UINT16 millisecond);

#endif //_TYPES_C631FEF2_89A8_4AE5_B22A_04122BA8B12D_H_
