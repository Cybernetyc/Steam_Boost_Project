//
// Created by Dmitry on 09.11.2025.
//

#ifndef INC_7_SEG_STATE_MACHINE_H
#define INC_7_SEG_STATE_MACHINE_H

#include "stdint.h"

#define DEFAULT_TIME  (3) /// This is the default time value for cfg_sec in Machine_State_Context

typedef enum {
  STATE_READY     = 0,   /// The device is waiting for the button to be pressed
  STATE_COUNTDOWN = 1,   /// Countdown after pressing the button
  STATE_CONFIG    = 2    /// Config for countdown seconds
} MachineState_t;        /// Machine condition

typedef enum {
  CLOSED = 0,            /// Valve is closed
  OPEN   = 1             /// Valve is open
} Valve_State_t;         /// Valve condition

typedef struct {
  MachineState_t machine_state;  /// Current condition of the machine
  Valve_State_t  valve_state  ;  /// Valve flag state
  uint8_t cfg_sec ;              /// Start value of seconds
  uint8_t cur_sec ;              /// Current value of seconds
} MachineState_Context_t;        /// Context of Machine State

typedef enum {
  EVENT_NONE = 0,             /// None event (default)
  EVENT_BTN_SHRT_PRESS = 1,   /// Short button press
  EVENT_BTN_LONG_PRESS = 2,   /// Пока что закомментировал, но использую позже
  EVENT_TICK_1S        = 3    /// One-second TIMER TICK
} MachineEvent_t;

void Machine_Process (MachineState_Context_t* ctx, const MachineEvent_t event);

#endif //INC_7_SEG_STATE_MACHINE_H