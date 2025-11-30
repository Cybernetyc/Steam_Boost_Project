//
// Created by Dmitry on 29.11.2025.
//
#include "AppFlashConfig.h"
#include <string.h>
#include "tim.h"

/** Глобальная RAM копия данных */
AppFlashConfig_t GlobalAppConfig;

/**
 * @brief Возвращает указатель на структуру конфигурации\n
 * расположенную во Flash - памяти.
 * @details Static inline — встраивается компилятором для оптимизации производительности
 * @details Так же производится приведение типов адреса памяти к структуре
 * @retval константный Указатель (данные во Flash нельзя менять напрямую)
 */
static inline const AppFlashConfig_t* APP_Get_CFG_Addr(void)
{
  return (const AppFlashConfig_t*)FLASH_CFG_ADDR;
}

/**
 * @brief Проверка предоставленной конфигурационной структуры на валидность:\n
 *        соответствие полей структуры заранее заданным константам.\n
 *        Требуется для обеспечения целостности конфигурационных данных.
 * @param config Указатель на конфигурационную структуру для проверки.
 *               Не должно быть NULL и содержать действительные данные.
 * @retval Валидность если данные проходят проверку, иначе инвалидность.
 */
static validate APP_Check_CFG_Valid(const AppFlashConfig_t *config)
{
  /// Проверка контрольных значений:
  /// Магическое число
  if (config->magic   != APP_CFG_MAGIC)
  {
    return INVALID;
  }
  /// Версия
  if (config->version != APP_CFG_VERSION)
  {
    return INVALID;
  }
  /// Диапазон минимального значения
  if (config->cfg_sec  < APP_CFG_SEC_MIN)
  {
    return INVALID;
  }
  /// Диапазон максимального значения
  if (config->cfg_sec  > APP_CFG_SEC_MAX)
  {
    return INVALID;
  }
  /// Инвернсая копия для защиты от повреждённых данных
  if (~config->cfg_sec != config->cfg_sec_inv)
  {
    return INVALID;
  }
  /// Вернуть валидность при успешной проверке
  return VALID;
}

/// Стирание памяти 5-го сектора. Всё в 0xFF
static HAL_StatusTypeDef APP_Erase_CFG_Flash(void)
{
  uint32_t Error = 0;
  FLASH_EraseInitTypeDef EraseInitStruct = {0};

  /// Настройка параметров стирания сектора Flash памяти
  EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;  /// Тип стирания - секторами
  EraseInitStruct.Sector       = FLASH_CFG_SECTOR;         /// Номер сектора ( определён в хедере )
  EraseInitStruct.NbSectors    = 1;                        /// Стираем только 1 сектор
  EraseInitStruct.VoltageRange = FLASH_CFG_VRANGE;         /// Диапазон напряжений

  __HAL_FLASH_CLEAR_FLAG( FLASH_FLAG_EOP    | FLASH_FLAG_OPERR  |
                          FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                          FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR )
  ;

  return HAL_FLASHEx_Erase(&EraseInitStruct, &Error);
}

/**
 * @brief Записывает конфигурационную структуру во Flash-память машинными словами.
 * @details Выполняет последовательную запись 32-битных слов конфигурационной
 *          структуры в заданный адрес Flash-памяти
 * @param input_config Указатель на структуру с данными для записи
 * @retval HAL_StatusTypeDef Статус операции: HAL_OK при успехе, HAL_ERROR при ошибке
 */
static HAL_StatusTypeDef APP_Write_CFG_Flash(const AppFlashConfig_t *input_config)
{
  /// Преобразуем указатель на структуру в указатель на массив 32-битных слов
  /// Это необходимо т.к. Flash память программируется словами (32 бита)
  const uint32_t *local_config_pointer = (const uint32_t *)input_config;

  /// Адрес во Flash-памяти, куда будет производиться запись
  uint32_t address = FLASH_CFG_ADDR;

  /// Вычисляем количество 32-битных слов в структуре конфигурации
  /// Деление на 4u (sizeof(uint32_t)) - т.к. работаем с 32-битными словами
  const uint32_t numbers_of_word = (uint32_t)((sizeof(*input_config)+3u)/4u);

  /// Последовательная запись каждого слова конфигурации во Flash
  for (uint32_t i = 0; i < numbers_of_word; i++)
  {
    /**
     * Программируем очередное 32-битное слово во Flash-память
     * FLASH_TYPEPROGRAM_WORD - указывает на программирование одного слова
     * address - базовый адрес
     * local_config_pointer[i] - текущее слово данных для записи
     */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, local_config_pointer[i]) != HAL_OK)
    {
      return HAL_ERROR;  /// При любой ошибке программирования - немедленный выход
    }
    address += 4u;
  }
  /// Все слова успешно записаны
  return HAL_OK;
}


/**
 * @brief Сохраняет конфигурацию во Flash-память
 * @details Выполняет полный цикл сохранения:\n
 *  -- подготовка данных\n
 *  -- проверка необходимости записи\n
 *  -- стирание сектора\n
 *  -- запись и верификация\n
 *  -- Операция защищена от прерываний.\n
 *
 * @retval HAL_StatusTypeDef Статус операции сохранения
 */
HAL_StatusTypeDef APP_Save_CFG_Flash(void)
{
  /// 0. Защита диапазона и инверсия - на всякий случай
  if (GlobalAppConfig.cfg_sec < APP_CFG_SEC_MIN)
  {
      GlobalAppConfig.cfg_sec = APP_CFG_SEC_MIN;
  }
  if (GlobalAppConfig.cfg_sec > APP_CFG_SEC_MAX)
  {
    GlobalAppConfig.cfg_sec = APP_CFG_SEC_MAX;
  }
  GlobalAppConfig.cfg_sec_inv = ~GlobalAppConfig.cfg_sec;
  GlobalAppConfig.magic       = APP_CFG_MAGIC;
  GlobalAppConfig.version     = APP_CFG_VERSION;
  GlobalAppConfig.reserved_1  = 0u;
  GlobalAppConfig.reserved_2  = 0u;

  /// 1. Если данные уже лежат во Flash, то ничего не копируем
  AppFlashConfig_t const *CurFlashConfig = APP_Get_CFG_Addr();

  if (APP_Check_CFG_Valid(CurFlashConfig) == VALID &&
      memcmp(CurFlashConfig, &GlobalAppConfig, sizeof(GlobalAppConfig)) == 0)
  {
    return HAL_OK;
  }

  /// 2. На время erase программ выключаем TIM3-IRQ (мультиплекс) и делаем участок атомарным*/
  // extern TIM_HandleTypeDef htim3;
  HAL_TIM_Base_Stop_IT(&htim3);
  __disable_irq();

  /// 3. Разблокируем FLASH для последующей операции над ним */
  HAL_StatusTypeDef App_CurrStatus = HAL_FLASH_Unlock();

  /// В случае неудачи восстанавливаем прерывания и запускаем таймер для мультиплексирования
  if (App_CurrStatus != HAL_OK)
  {
    __enable_irq();
    HAL_TIM_Base_Start_IT(&htim3);
    return App_CurrStatus;
  }

  App_CurrStatus = APP_Erase_CFG_Flash();
  if (App_CurrStatus == HAL_OK)
  {
    App_CurrStatus = APP_Write_CFG_Flash(&GlobalAppConfig);
  }

  /// Если успешно стёрли и записали
  (void)HAL_FLASH_Lock();
  __enable_irq();
  HAL_TIM_Base_Start_IT(&htim3);

  /// Быстрая верификация
  if (App_CurrStatus == HAL_OK && APP_Check_CFG_Valid(APP_Get_CFG_Addr())!= VALID)
  {
    App_CurrStatus = HAL_ERROR;
  }

  return App_CurrStatus; /// Вернули текущий статус
}

/**
 * @brief Загружает конфигурационные данные из Flash.\n\n
 *
 * Если данные конфигурации во Flash памяти валидны,\n
 * они копируются в глобальную переменную `GlobalAppConfig`\n
 *
 * В случае невалидных данных выполняется инициализация\n
 * конфигурации значениями по умолчанию. Затем конфигурация\n
 * сохраняется во флеш для последующего использования.\n
 *
 * Алгоритм включает следующие этапы:
 * - Извлечение указателя на текущую конфигурацию из Flash.
 * - Проверка валидности извлеченных данных.
 * - В случае валидности    - копирование данных в глобальную переменную.
 * - В случае не валидности - инициализация конфигурации значениями по умолчанию
 *   и сохранение в память.
 */
void APP_Load_CFG(void)
{
    const AppFlashConfig_t *flashConfig = APP_Get_CFG_Addr();

  if (APP_Check_CFG_Valid(flashConfig) == VALID)
  {
    GlobalAppConfig = *flashConfig; /// Почему сразу не использовать глобальную переменную ? В чём прикол ? Спросить у GPT
  }
  else
  {
    GlobalAppConfig.magic       = APP_CFG_MAGIC;
    GlobalAppConfig.version     = APP_CFG_VERSION;
    GlobalAppConfig.cfg_sec     = APP_CFG_SEC_DEFAULT;
    GlobalAppConfig.cfg_sec_inv = ~GlobalAppConfig.cfg_sec;
    GlobalAppConfig.reserved_1  = 0u;
    GlobalAppConfig.reserved_2  = 0u;
    (void)APP_Save_CFG_Flash();       /// Первый старт прошивки или битый блок - записали дефолтное значение
  }
}