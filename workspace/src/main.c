
#include "board.h"

struct board_pin_mgmt pin[PIN_MAX] =
{
#ifdef PIR_SENSOR_ENABLE
  {
    .num = PIR_SENSOR_PIN_ID,
    .enable = TRUE,
    .def_val = TRUE
  },
#endif /* PIR_SENSOR_ENABLE */
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

