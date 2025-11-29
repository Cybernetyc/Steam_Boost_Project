//
// Created by Dmitry on 29.11.2025.
//
#include "AppFlashConfig.h"
#include <strings.h>

/** Глобальная RAM копия данных */
AppFlashConfig_t GlobalAppConfig;

/**
 * @brief Проверка предоставленной конфигурационной структуры на валидность:\n
 *        соответствие полей структуры заранее заданным константам.\n
 *        Требуется для обеспечения целостности конфигурационных данных.
 * @param config Указатель на конфигурационную структуру для проверки.
 *               Не должно быть NULL и содержать действительные данные.
 * @retval Валидность если данные проходят проверку, иначе инвалидность.
 */
static validate Config_Valid(const AppFlashConfig_t *config)
{
  if (config->magic   != APP_CFG_MAGIC   ||
      config->version != APP_CFG_VERSION ||
      config->cfg_sec  < APP_CFG_SEC_MIN ||
      config->cfg_sec  > APP_CFG_SEC_MAX ||
     ~config->cfg_sec != config->cfg_sec_inv)
  {
    return INVALID;
  }

  return VALID;
}

/**
 * @brief Возвращает указатель на структуру конфигурации\n
 * расположенную во Flash - памяти.
 * @details Static inline — встраивается компилятором для оптимизации производительности
 * @details Так же производится приведение типов адреса памяти к структуре
 * @retval константный Указатель (данные во Flash нельзя менять напрямую)
 */
static inline const AppFlashConfig_t* flash_cfg(void)
{
  return (const AppFlashConfig_t*)APP_CFG_ADDR;
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
void AppFlash_LoadConfig(void)
{
    const AppFlashConfig_t *flashConfig = flash_cfg();

  if (Config_Valid(flashConfig) == VALID)
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
    (void)AppFlash_SaveConfig();
  }
}