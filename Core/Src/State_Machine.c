//
// Created by Dmitry on 09.11.2025.
//

#include <State_Machine.h>
#include <7_seg_driver.h>

#include "AppFlashConfig.h"

/**
  * @brief Дескриптор структуры для управления 7-сегментным индикатором.
  *
  * @details Эта структура используется для хранения и управления
  * конфигурацией и состоянием 7-сегментного дисплея,
  * включая цифровые порты, выводы, сегментный порт,
  * активную маску контактов и текущее состояние цифр.
  * Он позволяет взаимодействовать с дисплеем через соответствующие функции драйвера.
  */
extern Seg7_Handle_t seg7_handle;

/**
  * @brief Устанавливает состояние клапана и обновляет контекст машины.
  *
  * @details Эта функция устанавливает контакт GPIO,
  * связанный с клапаном, в желаемое состояние и
  * обновляет состояние клапана в указанном контексте состояния машины,
  * чтобы отразить изменение.
  *
  * @param ctx Указатель на структуру контекста состояния машины,
  *            содержащую текущее состояние машины.
  * @param Valve_state_set Желаемое состояние для установки клапана
  *                        (ОТКРЫТО или ЗАКРЫТО).
  */
static inline void Valve_Set (MachineState_Context_t* ctx, const Valve_State_t Valve_state_set)
{
  HAL_GPIO_WritePin(VALVE_GPIO_PORT, VALVE_PIN, (Valve_state_set ? GPIO_PIN_RESET : GPIO_PIN_SET));
  ctx->valve_state = Valve_state_set;
}

/**
  * @brief Retrieves the next value based on a predefined sequence of input.
  *
  * This function determines the next value in a circular sequence
  * depending on the provided input value, with a default fall-through value
  * if the input does not match any of the specific cases.
  *
  * @param input_value The current value to evaluate and determine its next sequence value.
  * @return The next value in the sequence. If the input value matches:
  *         - DEFAULT_TIME, returns DEFAULT_TIME+1.
  *         - DEFAULT_TIME+1, returns DEFAULT_TIME+2.
  *         - 5, returns 6.
  *         - Any other value, returns 3 as the default.
  */
static inline uint8_t cfg_next_3_6 (const uint8_t input_value)
{
  switch (input_value)
  {
    case DEFAULT_TIME:
      return DEFAULT_TIME+1;
    case DEFAULT_TIME+1:
      return DEFAULT_TIME+2; /// Спросить у GPT можно ли так
    case 5:
      return 6;
    default:
      return 3; /// До 6 или любых иных значений
  }
}


/**
  * @brief Processes the state machine transitions and actions based on the current state and events.
  *
  * This function manages state transitions for a state machine, which can operate in three primary
  * modes: READY, COUNTDOWN, and CONFIG. Actions are performed depending on the current state and
  * received event, including updating the machine's state, opening or closing a valve, modifying
  * configuration values, and updating a seven-segment display.
  *
  * @param ctx Pointer to the machine state context structure, which contains information
  *        about the machine's current state and configuration.
  * @param event The current event triggering the state machine to process, such as a button press
  *        or a timer tick.
  */
void Machine_Process (MachineState_Context_t* ctx, const MachineEvent_t event)
{
  switch (ctx->machine_state)
  {
    case STATE_READY:
      if (event == EVENT_BTN_SHRT_PRESS)
      {
        ctx->machine_state = STATE_COUNTDOWN; /// 1. Перейти в состояние обратного отсчёта

        ctx->cur_sec       = ctx->cfg_sec;    /// 2. Установить cur_sec = cfg_sec
        Valve_Set(ctx, OPEN);  /// 3. Изменить контекст состояния и открыть клапан
      }
      else if (event == EVENT_BTN_LONG_PRESS)
      {
        ctx->machine_state = STATE_CONFIG;     /// 1. Перейти в состояние настройки

        ctx->cur_sec       = ctx->cfg_sec;     /// 2. Редактируем с текущего.
      }
    break;


    case STATE_COUNTDOWN:

      if (event == EVENT_BTN_SHRT_PRESS)
      {
        ctx->machine_state  = STATE_READY;
        Valve_Set(ctx, CLOSED);
        //ctx->valve_state   = CLOSED;
      }

      if (event == EVENT_TICK_1S)            /// Событие - 1 секунда
      {
        if (ctx->cur_sec > 0)
        {
          ctx->cur_sec--;                    /// Уменьшить значение текущего времени если одно не 0
          if (ctx->cur_sec == 0)             /// Стало нулём ?
          {
            Valve_Set(ctx, CLOSED);
            //ctx->valve_state = CLOSED;       /// Закрыли клапан
          }
        }
        else
        {
          ctx->machine_state = STATE_READY;  /// Если текушее время 0 то перешли в состояние готов
        }
      }
    break;

    case STATE_CONFIG:
      if (event == EVENT_BTN_SHRT_PRESS)
      {
        ctx->cur_sec = cfg_next_3_6(ctx->cur_sec); /// Шаг по кругу
      }
      else if (event == EVENT_BTN_LONG_PRESS)
      {
        if (ctx->cfg_sec != ctx->cur_sec)
        {
          ctx->cfg_sec = ctx->cur_sec;
          GlobalAppConfig.cfg_sec = ctx->cfg_sec; /// Обновили RAM-копию
          APP_Save_CFG_Flash();
        }
        ctx->machine_state = STATE_READY;
      }
    break;


    default: /// Страховка - сброс автомата в READY
      ctx->machine_state = STATE_READY;
    break;
  }

  Seg7_SetNumber(&seg7_handle,        /// Установить текущее значение числа секунд
    (ctx->machine_state == STATE_READY) ? ctx->cfg_sec : ctx -> cur_sec);

  if (ctx->machine_state == STATE_CONFIG)
  {
    Seg7_SetDP(&seg7_handle, NUMBER_OF_DIG-1, 1);
  }
  else
  {
    Seg7_SetDP(&seg7_handle, NUMBER_OF_DIG-1, 0);
  }
}

