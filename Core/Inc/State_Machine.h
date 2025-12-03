//
// Created by Dmitry on 09.11.2025.
//

#ifndef INC_7_SEG_STATE_MACHINE_H
#define INC_7_SEG_STATE_MACHINE_H

/** Подключение заголовочных файлов */
#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "main.h"

/** -- Частные макроопределения -- */
/** -- Значения по умолчанию -- */
#define DEFAULT_TIME  (3)               // Значение времени по умолчанию для cfg_sec в Machine_State_Context

/** -- Макроопределения для клапана -- */
#define VALVE_GPIO_PORT (VALVE_GPIO_Port)  // Порт клапана
#define VALVE_PIN       (VALVE_Pin)        // Пин клапана
/** --Окончание макроопределений --/

/** Перечисления */
/**
 * @brief Перечисление состояний машины
 *
 */
typedef enum {
  STATE_READY     = 0,   // Готовность. Ожидание внешнего события
  STATE_COUNTDOWN = 1,   // Обратного отсчёта.
  STATE_CONFIG    = 2    // Конфигурация.
} MachineState_t;        // Состояние машины

/**
 * @brief Перечисления событий машины
 *
 */
typedef enum {
  EVENT_NONE = 0,                 /// Нет события (по-умолчанию)
  EVENT_BTN_SHRT_PRESS = 1,       /// Короткое нажатие кнопки
  EVENT_BTN_LONG_PRESS = 2,       /// Долгое нажатие кнопки
  EVENT_TICK_1S        = 3        /// 1-секундный тик таймера
} MachineEvent_t;                 /// Событие для машины состояний

/**
 * @brief Перечисления состояний клапана
 *
 */
typedef enum {
  CLOSED = 0,                    /// Клапан закрыт
  OPEN   = 1                     /// Клапана открыт
} Valve_State_t;                 /// Состояние клапана
/** Окончание перечислений */

/** Структуры */
/**
 * @brief Структура контекста состояния машины
 */
typedef struct {
  MachineState_t machine_state;  /// Current condition of the machine
  Valve_State_t  valve_state  ;  /// Valve flag state
  uint8_t cfg_sec ;              /// Start value of seconds
  uint8_t cur_sec ;              /// Current value of seconds
}MachineState_Context_t;        /// Context of Machine State


/** -- Прототипы функций -- */
/**
 * @brief
 */
void Machine_Process (MachineState_Context_t* ctx, MachineEvent_t event);

#endif //INC_7_SEG_STATE_MACHINE_H