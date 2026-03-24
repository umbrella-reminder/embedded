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

#ifdef PIR_SENSOR_ENABLE
#define PIR_SENSOR_PIN_ID   33      /* PIN ID is TEMP */
#endif

#define TRUE 1
#define FALSE 0

struct board_pin_mgmt
{
    unsigned int num;   /* pin number */

    bool enable;           /* pin enable */

    bool def_val;          /* default value */
};

#endif /* __BOARD_H__ */