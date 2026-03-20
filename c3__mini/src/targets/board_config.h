
#pragma once

#if defined(BOARD_ESP32S3)
    #include "esp32s3/board_esp32s3.hpp"
#elif defined(BOARD_ESP32C3)
    #include "esp32c3/board_esp32c3.hpp"
//#elif defined(BOARD_NATIVE)
//    #include "board_native.h"
#else
    #error "No supported board selected"
#endif
