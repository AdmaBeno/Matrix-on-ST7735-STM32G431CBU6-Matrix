#ifndef ST7735_H
#define ST7735_H

#include "stm32g4xx_hal.h"

#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160

// Пины подключения
#define ST7735_RES_PORT GPIOB
#define ST7735_RES_PIN  GPIO_PIN_0
#define ST7735_DC_PORT  GPIOB
#define ST7735_DC_PIN   GPIO_PIN_1
#define ST7735_CS_PORT  GPIOA
#define ST7735_CS_PIN   GPIO_PIN_15

// Команды ST7735
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_INVON   0x21
// Цвета RGB565
#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_BLUE    0x001F
#define ST7735_YELLOW  0xFFE0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_GRAY    0x8410



// Функции
void ST7735_Init(SPI_HandleTypeDef *hspi);
void ST7735_Clear(uint16_t color);
void ST7735_FillScreen(uint16_t color);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7735_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
void ST7735_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);
void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_WriteData16(uint16_t data);

#endif
