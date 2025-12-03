///
/// Created by Dmitry on 09.11.2025.
///

/** Подключение заголовочных файлов */
#include <State_Machine.h>
#include <7_seg_driver.h>
#include <AppFlashConfig.h>

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
  * @brief Функция для установки состояния клапана и обновления контекста машины состояний.
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
  * @brief Функция изменения текущего числа из заранее заданной последовательности.
  *
  * @details Эта функция определяет следующее значение в круговой последовательности
  *          в зависимости от предоставленного входного значения,
  *          при этом по умолчанию придаётся значение fall-through,
  *          если вход не совпадает ни с одним из конкретных
  *
  *
  * @param input_value Текущее значение для оценки и определения следующего значения последовательности.
  * @return Следующее значение в последовательности.
  *         Если входное значение совпадает:
  *         - DEFAULT_TIME+0, возвращает DEFAULT_TIME+1.
  *         - DEFAULT_TIME+1, возвращает DEFAULT_TIME+2.
  *         - DEFAULT_TIME+2, возвращает DEFAULT_TIME+3.
  *         - Любое другое значение возвращает 3 по умолчанию.
  */
static inline uint8_t cfg_next_3_6 (const uint8_t input_value)
{
  switch (input_value)
  {
    case DEFAULT_TIME+0:
      return DEFAULT_TIME+1;
    case DEFAULT_TIME+1:
      return DEFAULT_TIME+2;
    case DEFAULT_TIME+3:
      return DEFAULT_TIME+4;
    default:
      return DEFAULT_TIME+0;
  }
}

/**
  * @brief Функция обработки переходов и действий машины состояний на основе текущего состояния и событий.
  *
  * @details Эта функция управляет переходами состояний для машины состояний,
  *          которая может работать в трёх основных режимах:
  *          - READY
  *          - COUNTDOWN
  *          - CONFIG.
  *           Действия выполняются в зависимости от: текущего состояния и полученного события,
  *           включая обновление состояния машины, открытие или закрытие клапана,
  *           изменение конфигурационных значений и обновление семи сегментного индикатора.
  *
  * @param ctx Указатель на структуру контекста состояния машины,
  *            которая содержит информацию о текущем состоянии и конфигурации машины.
  * @param event Текущее событие, запускающее обработку в машине состояний,
  *              например, нажатие кнопки или тик-таймер.
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
        //ctx->valve_state = CLOSED;
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

