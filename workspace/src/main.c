#include "board.h"

#ifdef WIFI_AP_ENABLE
#include "wifi_ap.h"
#endif /*WIFI_AP_ENABLE*/



/* global variable for board specific data */
struct board_mgmt board;

/* global variable for pin infomation */
struct board_pin_mgmt pin[PIN_MAX] =
{
#ifdef PIR_SENSOR_ENABLE
  {
    .num = PIR_SENSOR_PIN_ID,
    .enable = TRUE,
    .def_val = TRUE
  },
#endif /* PIR_SENSOR_ENABLE */
#ifdef WIFI_INDICATOR_ENABLE
  {
    .num = WIFI_INDICATOR_PIN_ID,
    .enable = TRUE,
    .def_val = FALSE
  },
#endif /* WIFI_INDICATOR_ENABLE */
};

void detect()
{
    // sdfsdfd

    return;
}

void setup()
{

//setup begin

/* pin configurion ~~*/

  /* pin */
//setup end
}

void loop()
{
//loop begin

//loop end
}


/* entry point for starting */
int
app_main(void)
{
  ESP_LOGI ("SYS", "Boot Start");
#ifdef WIFI_AP_ENABLE
  wifi_ap_init ();
#endif /* WIFI_AP_ENABLE*/
  return 0;
}

