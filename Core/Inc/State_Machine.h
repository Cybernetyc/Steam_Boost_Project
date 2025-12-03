//
// Created by Dmitry on 09.11.2025.
//

#ifndef INC_7_SEG_STATE_MACHINE_H
#define INC_7_SEG_STATE_MACHINE_H

/** Подключение заголовочных файлов */
#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "main.h"

/* -- Частные макроопределения -- */
/* -- Значения по умолчанию -- */
#define DEFAULT_TIME  (3) /// Значение времени по умолчанию для cfg_sec в Machine_State_Context

/** -- Макроопределения для клапана -- */
#define VALVE_GPIO_PORT (VALVE_GPIO_Port)  /// Порт клапана
#define VALVE_PIN       (VALVE_Pin)        /// Пин клапана
/** --Окончание макроопределений --/

/** Перечисления */
/**
 * @brief Перечисление состояний машины
 */
typedef enum {
  STATE_READY     = 0, /// Готовность. Ожидание внешнего события.
  STATE_COUNTDOWN = 1, /// Состояние временного исполнения. Обратного отсчёта по заданному таймеру.
  STATE_CONFIG    = 2  /// Состояние конфигурации параметров машины. (Времени исполнения)
} MachineState_t;

/**
 * @brief События для машины состояний
 *
 */
typedef enum {
  EVENT_NONE = 0,           /// Нет события (по-умолчанию)
  EVENT_BTN_SHRT_PRESS = 1, /// Короткое нажатие кнопки
  EVENT_BTN_LONG_PRESS = 2, /// Долгое нажатие кнопки
  EVENT_TICK_1S        = 3  /// 1-секундный тик таймера
} MachineEvent_t;           /// События для машины состояний

/**
 * @brief Перечисления состояний клапана
 *
 */
typedef enum {
  CLOSED = 0,    /// Клапан закрыт
  OPEN   = 1     /// Клапана открыт
} Valve_State_t; /// Состояние клапана

/** Окончание перечислений */

/** Структуры */
/**
 * @brief Контекст машины состояний
 */
typedef struct {
  MachineState_t machine_state; /// Текущее состояние машины
  Valve_State_t  valve_state  ; /// Текущее состояние клапана
  uint8_t cfg_sec ; /// Настроенное значение времени (секунд) отсчёта
  uint8_t cur_sec ; /// Текущее значение времени (секунд)
}MachineState_Context_t;


/** -- Прототипы функций -- */
/**
 * @brief
 */
void Machine_Process (MachineState_Context_t* ctx, MachineEvent_t event);

#endif //INC_7_SEG_STATE_MACHINE_H