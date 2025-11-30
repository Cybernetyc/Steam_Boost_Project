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

/**
 * @brief   Стирает сектор Flash памяти, где хранится конфигурация
 * @details Выполняет полное стирание указанного сектора Flash-памяти.\n
 * Перед операцией сбрасывает флаги ошибок предыдущих операций Flash.
 * @retval HAL_StatusTypedef - статус операции стирания.
 */
static HAL_StatusTypeDef APP_Erase_CFG_Flash(void)
{
  uint32_t Error = 0;                                      // Переменная для хранения кода ошибки
  FLASH_EraseInitTypeDef EraseInitStruct = {0};            // Структура инициализации стирания

  // Настройка параметров стирания сектора Flash памяти
  EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;  // Тип стирания - секторами (не страницами)
  EraseInitStruct.Sector       = FLASH_CFG_SECTOR;         // Номер сектора (определён в заголовке)
  EraseInitStruct.NbSectors    = 1;                        // Стираем только 1 сектор
  EraseInitStruct.VoltageRange = FLASH_CFG_VRANGE;         // Диапазон напряжений

  /**
   * @brief Очистка флагов статуса Flash перед началом новой операции
   * @details Критически важный шаг для предотвращения ложного определения ошибок:
   * гарантия, что мы начинаем операцию с "чистого листа"
   */
  __HAL_FLASH_CLEAR_FLAG(
    FLASH_FLAG_EOP    |  // Флаг окончания операции ....................... (End Of Operation)
    FLASH_FLAG_OPERR  |  // Флаг ошибки операции .......................... (Operation Error)
    FLASH_FLAG_WRPERR |  // Флаг ошибки защиты от записи .................. (Write Protection Error)
    FLASH_FLAG_PGAERR |  // Флаг ошибки выравнивания программирования ..... (Programming Alignment Error)
    FLASH_FLAG_PGPERR |  // Флаг ошибки программирования .................. (Programming Error)
    FLASH_FLAG_PGSERR )  // Флаг ошибки последовательности программирования (Programming Sequence Error)
  ;

  /**
   * @brief Выполнение операции стирания Flash-памяти
   * @param &EraseInitStruct - указатель на структуру с параметрами стирания
   * @param &Error - указатель на переменную для сохранения детального кода ошибки.
   * @retval Hal_StatusTypeDef - Общий статус операции
   */
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
  // 1. Подготовка данных: установка защитных полей и граничных значений.
  //    Обеспечим корректный диапазон для основного параметра конфигурации.
  if (GlobalAppConfig.cfg_sec < APP_CFG_SEC_MIN)
  {
    GlobalAppConfig.cfg_sec = APP_CFG_SEC_MIN;  // Защита нижней границы
  }
  if (GlobalAppConfig.cfg_sec > APP_CFG_SEC_MAX)
  {
    GlobalAppConfig.cfg_sec = APP_CFG_SEC_MAX;  // Защита верхней границы
  }
  //Установка защитных и служебных полей структуры
  GlobalAppConfig.cfg_sec_inv = ~GlobalAppConfig.cfg_sec; // Инверсная копия для контроля целостности данных
  GlobalAppConfig.magic       = APP_CFG_MAGIC;            // Версия структуры конфигурации
  GlobalAppConfig.version     = APP_CFG_VERSION;          // Обнуление резервных полей
  GlobalAppConfig.reserved_1  = 0u;
  GlobalAppConfig.reserved_2  = 0u;

  // 2. Проверка необходимости записи: избегаем избыточного программирования Flash.
  //    Получаем указатель на текущую конфигурацию во Flash-памяти.
  AppFlashConfig_t const *CurFlashConfig = APP_Get_CFG_Addr();

  // Если данные во Flash идентичны подготовленным - пропускаем запись.
  // Тем самым продлевая срок службы Flash-памяти.
  if (APP_Check_CFG_Valid(CurFlashConfig) == VALID &&
      memcmp(CurFlashConfig, &GlobalAppConfig, sizeof(GlobalAppConfig)) == 0)
  {
    return HAL_OK; // Данные актуальные - запись не требуется.
  }

  // 3. Подготовка критической секции: атомарный участок кода
  // На время erase программ выключаем TIM3-IRQ (мультиплекс) и запрещаем прерывания
  HAL_TIM_Base_Stop_IT(&htim3);
  __disable_irq();

  // 4.   Работа с памятью
  // 4.1. Разблокировка Flash для последующих операций стирания и записи данных
  HAL_StatusTypeDef App_CurrStatus = HAL_FLASH_Unlock();

  // Обработка ошибки разблокировки: восстанавливаем систему и выходим
  if (App_CurrStatus != HAL_OK)
  {
    __enable_irq();                // Разрешаем прерывания
    HAL_TIM_Base_Start_IT(&htim3); // Запускаем таймер мультиплексирования
    return App_CurrStatus;         // Возвращаем статус ошибки
  }

  // 4.2. Стираем целевой сектор Flash-памяти
  App_CurrStatus = APP_Erase_CFG_Flash();

  // Если стирание прошло без ошибок - записываем новую конфигурацию
  if (App_CurrStatus == HAL_OK)
  {
    App_CurrStatus = APP_Write_CFG_Flash(&GlobalAppConfig);
  }

  // 4.3. Завершение работы с памятью: блокировка и восстановление системы
  (void)HAL_FLASH_Lock();         // Блокируем Flash для защиты от случайных изменений
  __enable_irq();                 // Восстанавливаем прерывания
  HAL_TIM_Base_Start_IT(&htim3);  // Запускаем таймер для мультиплексирования

  // 5. Финальная верификация: проверка валидности записанных данных
  if (App_CurrStatus == HAL_OK &&
      APP_Check_CFG_Valid(APP_Get_CFG_Addr())!= VALID)
  {
    App_CurrStatus = HAL_ERROR; // Данные повреждены или не прошли проверку валидности
  }

  return App_CurrStatus; /// Вернули итоговой статус операции
}

/**
 * @brief Загрузка конфигурационных данных из Flash-памяти.
 *
 * @details В случае валидности данных -
 * загрузка из Flash в глобальную переменную структуру GlobalAppConfig.\n\n
 * Иначе инициализация конфигурации значениями по умолчанию в глобальную переменную GlobalAppConfig.\n
 * И после её запись во Flash-память.
 *
 * Функция включает следующие этапы:
 * - Извлечение указателя на текущую конфигурацию из Flash.
 * - Проверка валидности извлеченных данных.
 * - В случае валидности    - копирование данных в глобальную переменную.
 * - В случае не валидности - инициализация конфигурации значениями по умолчанию
 *   и сохранение в память.
 */
void APP_Load_CFG_Flash(void)
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