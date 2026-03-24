#include <stdbool.h>
#include "umbrella_reminder_feature.h"

/* board define */
#ifndef __BOARD_H__
#define __BOARD_H__


#ifdef ESP32_BOARD_ENABLE
#define PIN_MAX         38
#else 
#define PIN_MAX         38    /* noting to match */
#endif


/*esp32 gpio pinmap*/
#define DAC_1           25      /*8-bit dac mp3 output */
#define DAC_2           26
/*GPIO 6 ~ 11 SPI flash pin*/

#ifdef PIR_SENSOR_ENABLE
#define PIR_SENSOR_PIN_ID   13      /* PIN ID is TEMP */
#endif

#ifdef WIFI_INDICATOR_ENABLE
#define WIFI_INDICATOR_PIN_ID   30
#endif

#define TRUE  true
#define FALSE false

struct board_pin_mgmt
{
    unsigned int num;   /* pin number */

    bool enable;           /* pin enable */

    bool def_val;          /* default value */
};

#endif /* __BOARD_H__ */
