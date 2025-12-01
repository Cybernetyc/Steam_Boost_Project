//
// Created by Dmitry on 01.12.2025.
//

#include "Button.h"

/** Глобальная переменная контекста кнопки **/
static ButtonContext_t Button = {0};

/**
 * @brief Инициализация модуля кнопки
 * @param gpio_port    - Порт, на котором находится кнопка
 * @param gpio_pin     - Пин, на котором находится кнопка
 * @param active_level - Активный уровень (1 = HIGH при нажатии, 0 = LOW при нажатии)
 */
void Button_Init(GPIO_TypeDef* gpio_port, uint16_t gpio_pin, const ActiveLevel_t active_level)
{
  Button.gpio_port = gpio_port;
  Button.gpio_pin  = gpio_pin;
  Button.level     = active_level;

  /// Инициализируем начальное состояние из реального чтения GPIO
  const ButtonState_t init_state = (HAL_GPIO_ReadPin(gpio_port, gpio_pin) == GPIO_PIN_SET) ? BTN_PRESSED : BTN_RELEASED;

  /// Коррекция по активному уровню
  Button.state = (active_level == HIGH) ? init_state : ((init_state == BTN_PRESSED) ? BTN_RELEASED : BTN_PRESSED);

  Button.hold_ms          = 0;
  Button.stable_time_ms   = 0;
  Button.long_state_flag  = 0;
}

/**
 * @brief Сырое чтение состояния кнопки с GPIO
 * @retval ButtonState_t - BTN_PRESSED или BTN_RELEASED
 */

static inline ButtonState_t Button_Read_Raw(void)
{
  /// Прочли текущее состояние кнопки
  const GPIO_PinState pin_state = HAL_GPIO_ReadPin(Button.gpio_port, Button.gpio_pin);

  /// Eсли Active Level HIGH; HIGH - PRESSED,  LOW = REALESED
  /// Eсли Active Level LOW ; LOW  - PRESSED, HIGH = REALESED
  if (Button.level == HIGH)
  {
    return pin_state == GPIO_PIN_SET ? BTN_PRESSED : BTN_RELEASED;
  }
  else
  {
    return pin_state == GPIO_PIN_RESET ? BTN_PRESSED : BTN_RELEASED;
  }
}

/**
 * @brief   Опрос раз в 1 мс. Возвращаем одно событие либо EVENT_NONE
 * @details Логика. Ключевые моменты
 *
 * Prev_raw : "что было в прошлую мс" (для подсчёта подряд идущих одинаковых выборок),
 * stable_time_ms : Счётчик - растёт, если raw НЕ меняется; сбрасывается при смене raw
 *
 * Как только stable_time_ms ДОШЁЛ до порога BUTTON_DEBOUNCE_MS - фиксируем новое state-состояние
 * и генерируем переход (SHORT на отпускании без LONG, или старт удержания).
 *
 * Чтобы не триггерить повторно на каждом тике (вызове функции), сразу после срабатывания
 * проталкиваем счётчик ЗА порог (stable_time_ms++) - это делает событие одноразовым на данный фронт.
 *
 */
MachineEvent_t Button_Poll_1ms(void)
{
  static ButtonState_t Prev_State;
  const  ButtonState_t Curr_State = Button_Read_Raw();

  /// 1) Накопление "подряд одинаковых выборок" или сброс при изменении
  if (Curr_State == Prev_State)
  {
    if (Button.stable_time_ms < BTN_DEBOUNCE_MS)
      Button.stable_time_ms++;
  }
  else
  {                            /// Обнаружили переход состояния кнопки
    Prev_State = Curr_State;   /// Выровняли состояния
    Button.stable_time_ms = 0; /// Занулили счётчик подсчёт стабильного состояния кнопки
  }

  MachineEvent_t event = EVENT_NONE;

  /// 2) Отрабатываем программный антидребезг после фиксации состояния

  if (Button.stable_time_ms == BTN_DEBOUNCE_MS)     /// Если прошёл программынй антидребезг
  {
    if (Button.state != Curr_State)
    {
      Button.state = Curr_State;         /// Фиксируем текущий уровень как стабильный

      if (Button.state == BTN_PRESSED)   /// Кнопка стабильно нажата
      {
        Button.hold_ms = 0;              /// Занулили счётчик удержания для последующего использования
        Button.long_state_flag = 0;      /// Занулили состояние длинного удержания
      }
      else  /// Иначе кнопка стабильно отпущена
      {
        if (!Button.long_state_flag)     /// Если состояние длинного удержания нет
          event = EVENT_BTN_SHRT_PRESS;  /// Значит удержание короткое

        Button.hold_ms = 0;              /// Занулили счётчик удержания
        Button.long_state_flag = 0;
      }
    }
    Button.stable_time_ms++;           /// Проталкиваем счётчика за порог,
  }                                    /// чтобы не перегенерировать событие


  /// 3) Пока подтверждено НАЖАТО - копим удержание, и при достижении порога, выдаём LONG - 1 раз
  if (Button.state == BTN_PRESSED)
  {
    if (Button.hold_ms < 0xFFFF)
      Button.hold_ms++;

    if ( Button.hold_ms >= BTN_LONG_MS && Button.long_state_flag == 0)
    {
      Button.long_state_flag = 1;
      event = EVENT_BTN_LONG_PRESS; /// Сработает ровно один раз за удержание
    }
  }

  return event;
}