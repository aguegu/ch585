#ifndef __SSD1306_H
#define __SSD1306_H

#include "CH58x_common.h"

void ssdInit(void);
void ssdPutString(const char * s, uint8_t row, uint8_t col);
void ssdRefresh();


#endif
