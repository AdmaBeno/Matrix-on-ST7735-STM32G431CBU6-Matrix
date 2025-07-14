#include "main.h"
#include "st7735.h"
#include "font.h"
#include <stdlib.h>

// Определение констант для дисплея и анимации
#define SCREEN_WIDTH 128      // Ширина экрана в пикселях
#define SCREEN_HEIGHT 159     // Высота экрана в пикселях
#define CHAR_WIDTH 6          // Ширина символа в пикселях (с учетом отступа)
#define CHAR_HEIGHT 8         // Высота символа в пикселях
#define NUM_COLS (SCREEN_WIDTH / CHAR_WIDTH)   // Количество столбцов (21)
#define NUM_ROWS (SCREEN_HEIGHT / CHAR_HEIGHT) // Количество строк (20)
#define MAX_TAIL 10           // Максимальная длина шлейфа символов

// Дескриптор SPI для связи с дисплеем
SPI_HandleTypeDef hspi1;

// Макрос для преобразования RGB в формат RGB565 (16-битный цвет)
#define ST7735_COLOR565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// Структура для хранения состояния столбца в анимации "Матрицы"
typedef struct {
    int head_row;      // Текущая строка головы потока
    int tail_len;      // Длина шлейфа символов
    int speed;         // Скорость падения (кадры до обновления)
    int frame_counter; // Счетчик кадров для управления скоростью
} Column;

// Массив столбцов для анимации
Column columns[NUM_COLS];

// Набор символов для случайного отображения
char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&";

// Массив цветов для эффекта затухания шлейфа
uint16_t fade_colors[] = {
    ST7735_COLOR565(0, 255, 0), // Яркий зеленый
    ST7735_COLOR565(0, 200, 0), // Средний зеленый
    ST7735_COLOR565(0, 150, 0), // Тусклый зеленый
    ST7735_COLOR565(0, 100, 0), // Очень тусклый зеленый
    ST7735_COLOR565(0, 60, 0),  // Едва видимый зеленый
    ST7735_COLOR565(0, 30, 0),  // Почти черный зеленый
};

// Прототипы функций
void SystemClock_Config(void); // Конфигурация системного таймера
void MX_GPIO_Init(void);       // Инициализация GPIO
void MX_SPI1_Init(void);       // Инициализация SPI

// Функция выбора случайного символа из набора
char random_char() {
    return charset[rand() % (sizeof(charset) - 1)];
}

// Инициализация столбцов с случайными параметрами
void init_columns() {
    for (int i = 0; i < NUM_COLS; i++) {
        columns[i].head_row = rand() % NUM_ROWS; // Случайная начальная строка
        columns[i].tail_len = 4 + rand() % MAX_TAIL; // Случайная длина шлейфа (4–13)
        columns[i].speed = 1 + rand() % 3; // Случайная скорость (1–3 кадра)
        columns[i].frame_counter = rand() % 10; // Случайная задержка старта
    }
}

// Функция обновления матрицы (эффект "падающих символов", как в "Матрице")
void update_matrix() {
    // Проходим по всем колонкам матрицы
    for (int col = 0; col < NUM_COLS; col++) {
        Column *c = &columns[col];  // Получаем текущую колонку

        // Уменьшаем счетчик кадров и проверяем, нужно ли обновлять колонку
        if (--c->frame_counter <= 0) {
            c->frame_counter = c->speed;  // Сбрасываем счетчик кадров

            // Проходим по всем символам в хвосте колонки (от хвоста к голове)
            for (int i = c->tail_len; i >= 0; i--) {
                // Вычисляем строку для текущего символа в хвосте
                int row = c->head_row - i;

                // Проверяем, что строка находится в пределах экрана
                if (row >= 0 && row < NUM_ROWS) {
                    // Выбираем цвет символа: из градиента для хвоста или черный
                    uint16_t color = (i < sizeof(fade_colors)/sizeof(fade_colors[0]))
                                   ? fade_colors[i]  // Берем цвет из градиента
                                   : ST7735_BLACK;    // Иначе черный цвет

                    // Рисуем случайный символ в вычисленной позиции с выбранным цветом
                    ST7735_DrawChar(col * CHAR_WIDTH, row * CHAR_HEIGHT,
                                   random_char(), color, ST7735_BLACK);
                }
            }

            // Перемещаем голову колонки вниз
            c->head_row++;

            // Если голова вышла за пределы экрана (с учетом хвоста)
            if (c->head_row - c->tail_len > NUM_ROWS) {
                c->head_row = 0;  // Сбрасываем голову в верх экрана
                c->tail_len = 4 + rand() % MAX_TAIL;  // Случайная длина хвоста (4-MAX_TAIL)
                c->speed = 1 + rand() % 3;  // Случайная скорость (1-3)
            }
        }
    }
}

// Главная функция
int main(void) {
    HAL_Init(); // Инициализация HAL
    SystemClock_Config(); // Настройка системного таймера
    MX_GPIO_Init(); // Настройка GPIO
    MX_SPI1_Init(); // Настройка SPI

    ST7735_Init(&hspi1); // Инициализация дисплея ST7735

    ST7735_FillScreen(ST7735_BLACK); // Заливка экрана черным
    srand(HAL_GetTick()); // Инициализация генератора случайных чисел
    init_columns(); // Инициализация столбцов анимации

    while (1) {
        update_matrix(); // Обновление анимации
        HAL_Delay(30); // Задержка 40 мс (~25 FPS)
    }
}

// Настройка системного таймера для STM32
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Настройка регулятора напряжения
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    // Настройка внешнего осциллятора и PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2; // Делитель 2
    RCC_OscInitStruct.PLL.PLLN = 85; // Умножитель 85
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // Делитель для PLLP
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2; // Делитель для PLLQ
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2; // Делитель для PLLR
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler(); // Обработка ошибки
    }

    // Настройка системной частоты и делителей
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // Источник: PLL
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // Без деления AHB
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // Без деления APB1
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // Без деления APB2

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler(); // Обработка ошибки
    }
}

// Настройка интерфейса SPI для дисплея
void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1; // Используем SPI1
    hspi1.Init.Mode = SPI_MODE_MASTER; // Режим: мастер
    hspi1.Init.Direction = SPI_DIRECTION_2LINES; // Двухпроводной режим
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT; // 8-битные данные
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW; // Полярность: низкий уровень
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE; // Фаза: 1-й фронт
    hspi1.Init.NSS = SPI_NSS_SOFT; // Программное управление NSS
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Делитель частоты
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB; // Старший бит первым
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE; // Режим TI отключен
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // CRC отключен
    hspi1.Init.CRCPolynomial = 7; // Полином CRC (не используется)
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE; // Длина CRC
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE; // Импульс NSS
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler(); // Обработка ошибки
    }
}

// Настройка пинов GPIO для дисплея и светодиода
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Включение тактирования портов
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Установка начального состояния пинов
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

    // Настройка пинов PB0, PB1, PB2 (для дисплея: CS, DC, RES)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Выход с двухтактным выходом
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Без подтяжки
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Низкая скорость
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Настройка пина для светодиода
    GPIO_InitStruct.Pin = led_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

    // Настройка пина PA15 (возможно, дополнительный пин управления)
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Обработчик ошибок (бесконечный цикл при сбое)
void Error_Handler(void) {
    while (1) {
    }
}

// Обработчик ошибок для отладки (если включены ассерты)
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
}
#endif
