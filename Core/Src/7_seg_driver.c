//
// Created by Dmitry on 26.10.2025.
//

#include "../Inc/7_seg_driver.h"
#include <string.h>

/* Коды сегментов для цифр 0..9 (общий шаблон; соответствие битам зависит от разводки) */
static const uint8_t digits_code[] = {
  [0] = 0x3F,
  [1] = 0x06,
  [2] = 0x5B,
  [3] = 0x4F,
  [4] = 0x66,
  [5] = 0x6D,
  [6] = 0x7D,
  [7] = 0x07,
  [8] = 0x7F,
  [9] = 0x6F
};

/* Примеры символов (если понадобятся позже) */
static const uint8_t symbols_code[] = {
  [0] = 0x77, // A
  [1] = 0x7C, // b
  [2] = 0x39, // C
  [3] = 0x5E, // d
  [4] = 0x79, // E
  [5] = 0x71, // F
  [6] = 0x74, // h
};

void Seg7_Init(
  Seg7_Handle_t* seg7_handle,
  GPIO_TypeDef*  digit_ports[],
  const uint16_t digit_pins[],
  GPIO_TypeDef*  segment_port,
  uint16_t       segment_pin_mask
)
{
  /* Обнуляем весь хэндл (на случай если он не был = {0}) */
  memset(seg7_handle, 0, sizeof(*seg7_handle));

  seg7_handle->segment_port     = segment_port;
  seg7_handle->segment_pin_mask = segment_pin_mask;

  for (size_t i = 0; i < NUMBER_OF_DIG; ++i) {
    seg7_handle->digit_ports[i] = digit_ports[i];
    seg7_handle->digit_pins[i]  = digit_pins[i];
  }
}

/* Выравнивание по правому краю. При input_number == 0 показываем '0' в младшем разряде. */
void Seg7_SetNumber(Seg7_Handle_t* seg7_handle, uint16_t input_number)
{
  /* Чистим буфер шаблонов (все разряды гасим) */
  memset(seg7_handle->digit_buf, 0, sizeof(seg7_handle->digit_buf));

  /* Отдельный случай: число = 0 */
  if (input_number == 0) {
    seg7_handle->digit_buf[NUMBER_OF_DIG - 1] = digits_code[0];
    return;
  }

  /* Разложение числа справа-налево (младшие -> правые разряды) */
  for (int8_t i = (int8_t)NUMBER_OF_DIG - 1; i >= 0 && input_number != 0; --i) {
    seg7_handle->digit_buf[i] = digits_code[input_number % 10];
    input_number /= 10;
  }
}
