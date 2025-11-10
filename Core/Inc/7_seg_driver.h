//
// Created by Dmitry on 26.10.2025.
//

#ifndef INC_7_SEG_7_SEG_DRIVER_H
#define INC_7_SEG_7_SEG_DRIVER_H

#include <stdint.h>
#include "stm32f401xc.h"

#define NUMBER_OF_DIG 3  // Кол-во разрядов

/**
 * @brief Структура для описания семисегментного индикатора
 * @param digit_ports      - Порты для разрядов (ключей)
 * @param digit_pins       - Пины для разрядов (ключей)
 * @param digit_buf        - Буфер шаблонов сегментов для каждого разряда
 * @param current_digit    - Текущий активный разряд (для динамики)
 * @param segment_port     - Порт для сегментов (A..G + точка)
 * @param segment_pin_mask - Маска задействованных бит сегментов в ODR
 */
typedef struct {
  GPIO_TypeDef* digit_ports [NUMBER_OF_DIG];
  uint16_t      digit_pins  [NUMBER_OF_DIG];
  uint8_t       digit_buf   [NUMBER_OF_DIG];
  uint8_t       current_digit;
  GPIO_TypeDef* segment_port;
  uint16_t      segment_pin_mask;
} Seg7_Handle_t;

/// Инициализация дескриптора индикатора
void Seg7_Init(
  Seg7_Handle_t* seg7_handle,
  GPIO_TypeDef*  digit_ports[],     // массив указателей на порты разрядов
  const uint16_t digit_pins[],      // массив пинов разрядов
  GPIO_TypeDef*  segment_port,      // порт сегментов (общий)
  uint16_t       segment_pin_mask   // маска задействованных бит сегментов
);

/// Заполнение буфера сегментов по числу (выравнивание по правому краю)
void Seg7_SetNumber(Seg7_Handle_t* seg7_handle, uint16_t input_number);
void Seg7_UpdateIndicator(Seg7_Handle_t *seg7_handle);

#endif // INC_7_SEG_7_SEG_DRIVER_H
