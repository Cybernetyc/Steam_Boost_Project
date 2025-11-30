/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "7_seg_driver.h"
#include "State_Machine.h"
#include "AppFlashConfig.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum{
  RELEASED = 0,
  PUSH = 1
} BUTTON_STATE_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIME (2)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile Seg7_Handle_t seg7_handle = {0};

GPIO_TypeDef* digit_ports[NUMBER_OF_DIG] = {
  [0] = Q1_GPIO_Port,
  [1] = Q2_GPIO_Port,
  [2] = Q3_GPIO_Port
};

uint16_t digit_pins[NUMBER_OF_DIG] = {
  [0] = Q1_Pin,
  [1] = Q2_Pin,
  [2] = Q3_Pin
};

/* Порт сегментов: весь порт A отдан под сегменты (как ты описывал) */
GPIO_TypeDef* segment_port = GPIOA;


MachineState_Context_t Machine_State = {
  .machine_state = STATE_READY,
  .valve_state   = CLOSED,
  .cfg_sec       = DEFAULT_TIME,
  .cur_sec       = 0
};

MachineEvent_t Machine_Event = EVENT_NONE;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

//--- Настройка сканера ---
#define BTN_DEBOUNCE_MS (20)    /// Антридребезг: сколько мс подряд уровень должен быть неизменным
#define BTN_LONG_MS     (1000)  /// Порог длительного нажатия МС

//--- Контекст кнопки ---
typedef struct {
  BUTTON_STATE_t  state     ; /// Текущее подтверждённое (после дебаунса) состояние кнопки: 1 - нажата. 0 - отпущена
  uint8_t         long_state ; /// Флаг - длинное нажатие уже создано в этом удержании, чтобы не дублировать.
  uint16_t        stable_time_ms ; /// Время мс подряд "сырое" чтение не менялось (доказательство стабильности)
  uint16_t        hold_ms   ; /// Длительность текущего удержания (считаем только когда state == 1)
} BtnCtx;

static BtnCtx Button = {0}; /// Инициализировали переменную кнопка


/// СЫРОЕ чтение GPIO: Возвращаем состояние кнопки, НАЖАТА или ОТПУЩЕНА.
/// Если кнопка у меня активный НУЛЬ - инвертируем (== GPIO_PIN_RESET)
static inline BUTTON_STATE_t Butt_State_Read(void)
{
  return HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin) == SET ? PUSH : RELEASED;
}

/// Опрос раз в 1 мс. Возвращаем одно событие либо EVENT_NONE
/// Логика. Ключевые моменты
/// Prev_raw : "что было в прошлую мс" (для подсчёта подряд идущих одинаковых выборок)
/// stable_time_ms : Счётчик - растёт, если raw НЕ меняется; сбрасывается при смене raw
/// Как только stable_time_ms ДОШЁЛ до порога BUTTON_DEBOUNCE_MS - фиксируем новое state-состояние
/// и генерируем переход (SHORT на отпускании без LONG, или старт удержания).
///
/// Чтобы не триггерить повторно на каждом тике (вызове функции), сразу после срабатывания
/// проталкиваем счётчик ЗА порог (stable_time_ms++) - это делает событие одноразовым на данный фронт.
///
static MachineEvent_t Button_Poll_1ms(void)
{
  static BUTTON_STATE_t Prev_State;
  const  BUTTON_STATE_t Curr_State = Butt_State_Read();

  static BUTTON_STATE_t Init_State = 0;

  if (!Init_State){                      /// Инициализация начального состояния кнопки
    Prev_State            = Curr_State;
    Button.state          = Curr_State;   ///Стартовое подтверждённое состояние такое как сейчас
    Button.stable_time_ms = 0;
    Button.hold_ms        = 0;
    Button.long_state     = 0;
    Init_State            = 1;            /// Init State off
    return EVENT_NONE;
  }

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

      if (Button.state == PUSH)          /// Если кнопка стабильно нажата
      {
        Button.hold_ms = 0;              /// Занулили счётчик удержания для последующего использования
        Button.long_state = 0;           /// Занулили состояние длинного удержания
      }
      else                               /// Иначе кнопка стабильно отпущена
      {
        if (!Button.long_state)          /// Если состояние длинного удержания нет
          event = EVENT_BTN_SHRT_PRESS;  /// Значит удержание короткое

        Button.hold_ms = 0;              /// Занулили счётчик удержания
        Button.long_state = 0;
      }
    }
    Button.stable_time_ms++;           /// Проталкиваем счётчика за порог,
  }                                    /// чтобы не перегенерировать событие


  /// 3) Пока подтверждено НАЖАТО - копим удержание, и при достижении порога, выдаём LONG - 1 раз
  if (Button.state == PUSH)
  {
    if (Button.hold_ms < 0xFFFF)
      Button.hold_ms++;

    if ( Button.hold_ms >= BTN_LONG_MS && Button.long_state == 0)
    {
      Button.long_state = 1;
      event = EVENT_BTN_LONG_PRESS; /// Сработает ровно один раз за удержание
    }
  }

  return event;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  APP_Load_CFG_Flash();

  Machine_State.cfg_sec = GlobalAppConfig.cfg_sec;

  Seg7_Init(&seg7_handle, digit_ports, digit_pins, segment_port, 0xFF);
  Seg7_SetNumber(&seg7_handle, Machine_State.cfg_sec);
  Seg7_UpdateIndicator(&seg7_handle);
  HAL_TIM_Base_Start_IT(&htim3);

  uint32_t last_tick_ms = 0U; /// Служебная переменная время последнего обновления

  uint32_t last_ms =     HAL_GetTick();  /// Для 1мс - сканера
  uint32_t last_tick1s = HAL_GetTick();  /// Для 1с - тика автомата

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    /// --- Кнопка:  строгие 1 мс шаги; "догоняем', если главный цикл задержался ---///
    while (last_ms != now)
    {
      last_ms++;
      const MachineEvent_t current_event = Button_Poll_1ms();

      if (current_event != EVENT_NONE)
      {
        Machine_Process(&Machine_State, current_event);
      }

    }

    /// --- Секундный тик автомата ---
    if ((now - last_tick1s) >= 1000){
      last_tick1s += 1000;  /// Не приравнять к NOW, а прибавить 1000
      Machine_Process(&Machine_State, EVENT_TICK_1S);
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
    Seg7_UpdateIndicator(&seg7_handle);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
