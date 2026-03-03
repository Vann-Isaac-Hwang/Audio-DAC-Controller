/*
 * OLED.h
 *
 * Created on: Oct 6, 2025
 * Author: huang
 */

#ifndef INC_OLED_H_
#define INC_OLED_H_

#include "main.h"

// SH1107 屏幕参数
#define OLED_WIDTH   128
#define OLED_HEIGHT  128
#define OLED_PAGE_COUNT 16  // 128/8 = 16页

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif /* INC_OLED_H_ */
