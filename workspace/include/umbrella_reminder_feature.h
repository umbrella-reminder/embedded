/* define umbrella features */
#ifndef __UMBRELLA_REMINDER_H__
#define __UMBRELLA_REMINDER_H__

/* Enable ESP32 */
#define ESP32_BOARD_ENABLE

/* Enable PIR Sensor */
#define PIR_SENSOR_ENABLE

/* Enable speaker */
#define SPEAKER_ENABLE

/* Enable WIFI AP Mode */
#define WIFI_AP_ENABLE

#ifdef WIFI_AP_ENABLE
/* Enable WIFI Indicator LED */
#define WIFI_INDICATOR_ENABLE
#endif

#endif /* __UMBRELLA_REMINDER_H__ */