//
// Created by Dmitry on 09.11.2025.
//

#include <State_Machine.h>
#include <7_seg_driver.h>

extern Seg7_Handle_t seg7_handle;

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
      if (event == EVENT_BTN_SHRT_PRESS){
        ctx->machine_state = STATE_COUNTDOWN; /// 1. Перейти в состояние COUNTDOWN
        ctx->cur_sec       = ctx->cfg_sec;    /// 2. Установить cur_sec = cfg_sec
        ctx->valve_state   = OPEN;            /// 3. Включи клапан
      }
    break;

    case STATE_COUNTDOWN:
      if (event == EVENT_TICK_1S){           /// Событие - 1 секунда
        if (ctx->cur_sec > 0){
          ctx->cur_sec--;                    /// Уменьшить значение текущего времени если одно не 0
          if (ctx->cur_sec == 0)             /// Стало нулём ?
            ctx->valve_state = CLOSED;       /// Закрыли клапан
        }
        else
          ctx->machine_state = STATE_READY;  /// Если текушее время 0 то перешли в состояние готов
      }
    break;

    default: /// Страховка - сброс автомата в READY
      ctx->machine_state = STATE_READY;
    break;
  }

  Seg7_SetNumber(&seg7_handle,        /// Установить текущее значение числа секунд
    (ctx->machine_state == STATE_READY) ? ctx->cfg_sec : ctx -> cur_sec);
}

