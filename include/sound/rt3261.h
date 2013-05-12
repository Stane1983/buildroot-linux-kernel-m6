/**
 * struct rt3261_platform_data - platform-specific RT3261 data
 */

#ifndef _RT3261_H_
#define _RT3261_H_

/* platform speaker watt */
#define RT3261_SPK_1_0W     0
#define RT3261_SPK_0_5W     1
#define RT3261_SPK_1_5W     2

/* platform speaker mode */
#define RT3261_SPK_STEREO   0
#define RT3261_SPK_LEFT     1
#define RT3261_SPK_RIGHT    2

/* platform mic input mode */
#define RT3261_MIC_DIFFERENTIAL     0
#define RT3261_MIC_SINGLEENDED      1

struct rt3261_platform_data {
    int (*hp_detect)(void);
    void (*device_init)(void);
    void (*device_uninit)(void); 

    int  spk_output;
    int  mic_input;
};

#endif
