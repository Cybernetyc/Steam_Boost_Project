//
// Created by Dmitry on 09.11.2025.
//

#include <State_Machine.h>
#include <7_seg_driver.h>

#include "AppFlashConfig.h"

extern Seg7_Handle_t seg7_handle;

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
 * @brief Base machine process template
 * @param ctx
 * @param event
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
        ctx->valve_state   = OPEN;            /// 3. Контекст состояния клапана - ОТКРЫТ
        Valve_Set(OPEN);          /// 4. ОТКРЫТЬ клапан
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
        ctx->valve_state    = CLOSED;
      }

      if (event == EVENT_TICK_1S)            /// Событие - 1 секунда
      {
        if (ctx->cur_sec > 0)
        {
          ctx->cur_sec--;                    /// Уменьшить значение текущего времени если одно не 0
          if (ctx->cur_sec == 0)             /// Стало нулём ?
          {
            ctx->valve_state = CLOSED;       /// Закрыли клапан
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

