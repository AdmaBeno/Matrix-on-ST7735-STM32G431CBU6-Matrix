#include "st7735.h"
#include "font.h"

static SPI_HandleTypeDef *st7735_hspi;

// Отправка команды
static void ST7735_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ST7735_DC_PORT, ST7735_DC_PIN, GPIO_PIN_RESET); // DC = 0
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_RESET); // CS = 0
    HAL_SPI_Transmit(st7735_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_SET);   // CS = 1
}

// Отправка данных
static void ST7735_WriteData(uint8_t *data, uint32_t len) {
    HAL_GPIO_WritePin(ST7735_DC_PORT, ST7735_DC_PIN, GPIO_PIN_SET); // DC = 1
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(st7735_hspi, data, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_SET);
}

// Отправка одного 16-битного цвета
void ST7735_WriteData16(uint16_t data) {
    uint8_t buf[2] = { data >> 8, data & 0xFF };
    ST7735_WriteData(buf, 2);
}

// Установка адресного окна (область для отрисовки)
void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    // Добавляем смещение для WeAct ST7735S: +2 по X, +1 по Y
    uint8_t x_start = x0 + 2;
    uint8_t x_end = x1 + 2;
    uint8_t y_start = y0 + 1;
    uint8_t y_end = y1 + 1;

    ST7735_WriteCommand(ST7735_CASET);
    uint8_t data_x[] = {0x00, x_start, 0x00, x_end};
    ST7735_WriteData(data_x, 4);

    ST7735_WriteCommand(ST7735_RASET);
    uint8_t data_y[] = {0x00, y_start, 0x00, y_end};
    ST7735_WriteData(data_y, 4);

    ST7735_WriteCommand(ST7735_RAMWR);
}

// Инициализация
void ST7735_Init(SPI_HandleTypeDef *hspi) {
    st7735_hspi = hspi;

    // Сброс дисплея
    HAL_GPIO_WritePin(ST7735_RES_PORT, ST7735_RES_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ST7735_RES_PORT, ST7735_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    // Программный сброс
    ST7735_WriteCommand(ST7735_SWRESET);
    HAL_Delay(150);

    // Выход из спящего режима
    ST7735_WriteCommand(ST7735_SLPOUT);
    HAL_Delay(255);

    // Настройка Frame Rate
    ST7735_WriteCommand(0xB1);
    uint8_t frmctr1[] = {0x05, 0x3C, 0x3C};
    ST7735_WriteData(frmctr1, 3);

    ST7735_WriteCommand(0xB2);
    uint8_t frmctr2[] = {0x05, 0x3C, 0x3C};
    ST7735_WriteData(frmctr2, 3);

    ST7735_WriteCommand(0xB3);
    uint8_t frmctr3[] = {0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C};
    ST7735_WriteData(frmctr3, 6);

    // Настройка питания
    ST7735_WriteCommand(0xC0);
    uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    ST7735_WriteData(pwctr1, 3);

    ST7735_WriteCommand(0xC1);
    uint8_t pwctr2[] = {0xC5};
    ST7735_WriteData(pwctr2, 1);

    ST7735_WriteCommand(0xC2);
    uint8_t pwctr3[] = {0x0A, 0x00};
    ST7735_WriteData(pwctr3, 2);

    ST7735_WriteCommand(0xC5);
    uint8_t vmctr1[] = {0x3C, 0x38};
    ST7735_WriteData(vmctr1, 2);

    // Установка цветового формата (16-бит RGB565)
    ST7735_WriteCommand(ST7735_COLMOD);
    uint8_t color_mode = 0x05;
    ST7735_WriteData(&color_mode, 1);
    HAL_Delay(10);

    // Настройка управления памятью
    ST7735_WriteCommand(ST7735_MADCTL);
    uint8_t madctl = 0xC8; // Пробуем ориентацию: MY=0, MX=1, MV=0, RGB
    ST7735_WriteData(&madctl, 1);

    // Настройка адреса столбцов (CASET): 2–129
    ST7735_WriteCommand(ST7735_CASET);
    uint8_t caset_data[] = {0x00, 0x02, 0x00, 0x81};
    ST7735_WriteData(caset_data, 4);

    // Настройка адреса строк (RASET): 1–160
    ST7735_WriteCommand(ST7735_RASET);
    uint8_t raset_data[] = {0x00, 0x01, 0x00, 0xA0};
    ST7735_WriteData(raset_data, 4);

    // Применение настроек
    ST7735_WriteCommand(ST7735_RAMWR);

    // Настройка гаммы
    ST7735_WriteCommand(0xE0);
    uint8_t gamma_p[] = {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    ST7735_WriteData(gamma_p, 16);

    ST7735_WriteCommand(0xE1);
    uint8_t gamma_n[] = {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
    ST7735_WriteData(gamma_n, 16);

    // Включение дисплея
    ST7735_WriteCommand(ST7735_DISPON);
    HAL_Delay(100);
}


// Заливка экрана
void ST7735_Clear(uint16_t color) {
    ST7735_SetAddressWindow(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
    for (uint16_t y = 0; y < ST7735_HEIGHT; y++) {
        for (uint16_t x = 0; x < ST7735_WIDTH; x++) {
            ST7735_WriteData16(color);
        }
    }
}

// Быстрая заливка
void ST7735_FillScreen(uint16_t color) {
    ST7735_Clear(color);
}

// Пиксель
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;

    ST7735_SetAddressWindow(x, y, x, y);
    ST7735_WriteData16(color);
}

// Символ
void ST7735_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
    if (x > ST7735_WIDTH - 6 || y > ST7735_HEIGHT - 8 || c < 32) return;

    const uint8_t *glyph = &font[(c - 32) * 5];
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                ST7735_DrawPixel(x + i, y + j, color);
            } else {
                ST7735_DrawPixel(x + i, y + j, bg_color);
            }
        }
    }
    // Заполнение 6-го столбца (отступ)
    for (uint8_t j = 0; j < 8; j++) {
        ST7735_DrawPixel(x + 5, y + j, bg_color);
    }
}

// Строка
void ST7735_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color) {
    while (*str) {
        ST7735_DrawChar(x, y, *str++, color, bg_color);
        x += 6;
        if (x > ST7735_WIDTH - 6) break;
    }
}
