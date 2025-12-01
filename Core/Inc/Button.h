//
// Created by Dmitry on 01.12.2025.
//

#ifndef INC_7_SEG_BUTTON_H
#define INC_7_SEG_BUTTON_H


#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "State_Machine.h"

typedef enum {
  BTN_RELEASED = 0,
  BTN_PRESSED  = 1
} ButtonState_t;

typedef enum {
  LOW  = 0,
  HIGH = 1
} ActiveLevel_t;

//--- Настройка сканера ---
#define BTN_DEBOUNCE_MS (20)     /// Антридребезг: сколько мс подряд уровень должен быть неизменным
#define BTN_LONG_MS     (1000)   /// Порог длительного нажатия МС

/** Контекст кнопки (внутренняя структура - не экспортируется напрямую) */
typedef struct {
  ButtonState_t state;           /// Текущее подтверждённое (после дебаунса) состояние кнопки: 1 - нажата. 0 - отпущена
  uint8_t       long_state_flag; /// Флаг - длинное нажатие уже создано в этом удержании, чтобы не дублировать.
  uint16_t      stable_time_ms;  /// Время мс подряд "сырое" чтение не менялось (доказательство стабильности)
  uint16_t      hold_ms;         /// Длительность текущего удержания (считаем только когда state == 1)

  /// Конфигурация GPIO
  GPIO_TypeDef* gpio_port;      /// Порт, к которому подключена кнопка
  uint16_t      gpio_pin;       /// Пин,  к которому подключена кнопка
  ActiveLevel_t level;          /// Активный уровень: 1 = активный HIGH, 0 = Активный LOW
} ButtonContext_t;

/**
 * @brief Инициализация модуля кнопки
 * @param gpio_port     Порт GPIO кнопки (например, GPIOB)
 * @param gpio_pin      Пин GPIO кнопки (например, GPIO_PIN_10)
 * @param active_level  Активный уровень: 1 = HIGH при нажатии, 0 = LOW при нажатии
 */
void Button_Init(GPIO_TypeDef* gpio_port, uint16_t gpio_pin, const ActiveLevel_t active_level);

/**
 * @brief Опрос кнопки (вызывать строго каждую 1 мс)
 * @retval MachineEvent_t - EVENT_NONE, EVENT_BTN_SHRT_PRESS или EVENT_BTN_LONG_PRESS
 */
MachineEvent_t Button_Poll_1ms(void);

#endif //INC_7_SEG_BUTTON_H