//
// Created by Dmitry on 26.10.2025.
//

#include "../Inc/7_seg_driver.h"
#include <string.h>

/* Segment codes for digits (generic pattern; actual bit mapping depends on PCB wiring) */
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

/* Примеры символов (если понадобятся позже) /
static const uint8_t symbols_code[] = {
  [0] = 0x77, // A
  [1] = 0x7C, // b
  [2] = 0x39, // C
  [3] = 0x5E, // d
  [4] = 0x79, // E
  [5] = 0x71, // F
  [6] = 0x74, // h
}; */

/**
 * @brief Initializes the structure that describes the 7-segment indicator.
 * @param seg7_handle      Pointer to the 7-segment indicator handle structure.
 * @param digit_ports      Array of GPIO port pointers controlling the digit transistors.
 * @param digit_pins       Array of GPIO pins controlling the digit transistors.
 * @param segment_port     GPIO port used for the indicator segments.
 * @param segment_pin_mask Bitmask specifying active bits of the segment port.
 * @retval None
 */
void Seg7_Init(
  Seg7_Handle_t* seg7_handle,
  GPIO_TypeDef*  digit_ports[],
  const uint16_t digit_pins[],
  GPIO_TypeDef*  segment_port,
  uint16_t       segment_pin_mask )
{
  memset(seg7_handle, 0, sizeof(*seg7_handle));     /// Set input struct in 0

  seg7_handle->segment_port     = segment_port;
  seg7_handle->segment_pin_mask = segment_pin_mask;

  for (int8_t i = 0; i < NUMBER_OF_DIG; ++i) {
    seg7_handle->digit_ports[i] = digit_ports[i];  /// Rewrite digit ports
    seg7_handle->digit_pins [i] = digit_pins [i];  /// Rewrite digit pins
  }
}

/**
 * @brief Converts a numeric value into segment patterns to be displayed on the 7-segment indicator.
 * @param seg7_handle   - Pointer to the 7-segment indicator handle structure.
 * @param input_number  - Number to convert
 */
void Seg7_SetNumber(Seg7_Handle_t* seg7_handle, uint16_t input_number)
{
  /* Clear the digit buffer (turn all segments off) */ /// Тут попробовать убрать
  memset(seg7_handle->digit_buf, 0, sizeof(seg7_handle->digit_buf));

  /// Special case: input value is zero
  if (input_number == 0)
  {
    seg7_handle->digit_buf[NUMBER_OF_DIG - 1] = digits_code[0];
    return;
  }

  /* Decompose the number from right to left (least significant digit goes to the rightmost position)  */
  for (int8_t i = (int8_t)NUMBER_OF_DIG - 1; i >= 0 && input_number != 0; --i)
  {
    seg7_handle->digit_buf[i] = digits_code[input_number % 10];
    input_number /= 10;
  }

}


void Seg7_UpdateIndicator(Seg7_Handle_t *seg7_handle)
{
  /// Off all digits а нужно ли выключать все цифры ?
  for (int8_t i = 0; i < NUMBER_OF_DIG; ++i)
  {
    seg7_handle->digit_ports[i]->BSRR = (uint32_t)seg7_handle->digit_pins[i] << 16;
  }

  /// Перезапишу в отдельную переменную чтобы проще было работать.
  uint8_t current_digit = seg7_handle->current_digit;

  /// Очистим биты порта , отвечающие за вывод символа через маску.
  seg7_handle->segment_port->ODR &= ~seg7_handle->segment_pin_mask;

  /// Запишем биты порта, отвечающие за вывод символа, требуемый символ через маску
  seg7_handle->segment_port->ODR |= seg7_handle->digit_buf[current_digit] & seg7_handle->segment_pin_mask;

  /// Включаем текущий транзистор на отображение
  seg7_handle->digit_ports[current_digit]->BSRR = (uint32_t)(seg7_handle->digit_pins[current_digit]);

  /// Увеличивем значение символа в структуре в пределах количества символов для последующих вызовов
  seg7_handle->current_digit++;
  if (seg7_handle->current_digit >= NUMBER_OF_DIG)
  {
    seg7_handle->current_digit = 0;
  }

}

void Seg7_SetDP (Seg7_Handle_t * seg7_handle,const uint8_t digit_index, uint8_t on)
{
  if (digit_index >= NUMBER_OF_DIG)
  {
    return;
  }

  if (on)
  {
    seg7_handle->digit_buf[digit_index] |= SEG7_DP_BIT;
  }
  else
  {
    seg7_handle->digit_buf[digit_index] &= (uint8_t)~SEG7_DP_BIT;
  }

}