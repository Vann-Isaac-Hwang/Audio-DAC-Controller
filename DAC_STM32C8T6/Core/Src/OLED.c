/*
 * OLED.c
 *
 *  Created on: Oct 6, 2025
 *      Author: huang
 */

#include "main.h"
#include "OLED.h"
#include "OLED_Font.h"

// I2C句柄声明（需要在main.c中定义）
extern I2C_HandleTypeDef hi2c2;  // 根据实际使用的I2C修改

// OLED I2C地址
#define OLED_ADDRESS 0x78  // 0x78 << 1 = 0xF0

/**
  * @brief  OLED写命令
  */
void OLED_WriteCommand(uint8_t Command)
{
    uint8_t buf[2] = {0x00, Command};  // 0x00表示命令模式
    HAL_I2C_Master_Transmit(&hi2c2, OLED_ADDRESS, buf, 2, 100);
}

/**
  * @brief  OLED写数据
  */
void OLED_WriteData(uint8_t Data)
{
    uint8_t buf[2] = {0x40, Data};  // 0x40表示数据模式
    HAL_I2C_Master_Transmit(&hi2c2, OLED_ADDRESS, buf, 2, 100);
}

/**
  * @brief  OLED设置光标位置 (针对SH1107 128x128)
  */
void OLED_SetCursor(uint8_t Page, uint8_t Column)
{
    if(Page >= OLED_PAGE_COUNT) Page = OLED_PAGE_COUNT - 1;
    if(Column >= OLED_WIDTH) Column = OLED_WIDTH - 1;

    OLED_WriteCommand(0xB0 + Page);        // 设置页地址 (0xB0 ~ 0xBF)
    OLED_WriteCommand(0x00 + (Column & 0x0F));     // 设置列地址低4位
    OLED_WriteCommand(0x10 + ((Column >> 4) & 0x0F)); // 设置列地址高4位
}

/**
  * @brief  OLED清屏
  */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < OLED_PAGE_COUNT; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < OLED_WIDTH; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
  * @brief  OLED清除指定页
  */
void OLED_ClearPage(uint8_t page)
{
    uint8_t i;
    if(page >= OLED_PAGE_COUNT) return;

    OLED_SetCursor(page, 0);
    for(i = 0; i < OLED_WIDTH; i++)
    {
        OLED_WriteData(0x00);
    }
}

/**
  * @brief  OLED显示一个字符 (8x16字体)
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    uint8_t page = (Line - 1) * 2;  // 每行字符占用2页(16像素)

    if(Line < 1 || Line > 8) return;  // SH1107支持8行字符 (128/16=8)
    if(Column < 1 || Column > 16) return;  // 每行16个字符 (128/8=16)

    // 显示字符上半部分 (前8字节)
    OLED_SetCursor(page, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }

    // 显示字符下半部分 (后8字节)
    OLED_SetCursor(page + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

/**
  * @brief  OLED显示字符串
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief  OLED次方函数
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED初始化
  */
void OLED_Init(void)
{
    HAL_Delay(100);  // 上电延时

    OLED_WriteCommand(0xAE);    // 关闭显示

    // 基本显示设置
    OLED_WriteCommand(0xA8);    // 设置多路复用比率
    OLED_WriteCommand(0x7F);    // 128 MUX

    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);    // 无偏移

    OLED_WriteCommand(0x40);    // 设置显示起始行

    OLED_WriteCommand(0xA0);    // 设置段重映射 (A0通常是反向)
    OLED_WriteCommand(0xC0);    // 设置COM扫描方向 (C0通常是正常)

    OLED_WriteCommand(0xDA);    // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);    // 顺序, 禁用左右COM翻转

    OLED_WriteCommand(0x81);    // 设置对比度
    OLED_WriteCommand(0x80);    // 对比度值

    OLED_WriteCommand(0xA4);    // 整个显示打开
    OLED_WriteCommand(0xA6);    // 设置正常显示

    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比
    OLED_WriteCommand(0x50);    // 默认值

    OLED_WriteCommand(0x8D);    // 设置电荷泵
    OLED_WriteCommand(0x14);    // 启用电荷泵

    OLED_WriteCommand(0xAF);    // 开启显示

    OLED_Clear();               // 清屏
}
