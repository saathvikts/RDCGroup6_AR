/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd/lcd.h"
#include "lcd/lcd_graphics.h"
#include "elcp_motor/elcp_motor.h"
#include "rm_motor.h"
#include "controller_receiver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RM_TARGET_RPM 300.0f
#define RM_SPEED_KP 0.001f          // Initial, untuned gain: amperes per RPM.
#define RM_TEST_CURRENT_LIMIT 1.5f
#define RM_FEEDBACK_TIMEOUT_MS 100U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float SpeedCurrent(float target_rpm, float measured_rpm)
{
  float current = RM_SPEED_KP * (target_rpm - measured_rpm);
  if (current > RM_TEST_CURRENT_LIMIT) current = RM_TEST_CURRENT_LIMIT;
  if (current < -RM_TEST_CURRENT_LIMIT) current = -RM_TEST_CURRENT_LIMIT;
  return current;
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */
  if (ControllerReceiver_Start(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  // Initialize and stop motors on startup
  ELCP_Motor_Init(MOTOR_J1);
  ELCP_Motor_Init(MOTOR_J3);
  ELCP_Motor_Stop(MOTOR_J1);
  ELCP_Motor_Stop(MOTOR_J3);

  tft_init(PIN_ON_TOP, BLACK, WHITE, CYAN, DARK_GREY);
  rm_can_init(&hcan1);
  RM_Motor motor = RM_MOTOR_1;  // ESC ID 1; enum index 0.
  rm_set_current_setpoint(&hcan1, motor, 0.0f);
  uint32_t led_last_tick = HAL_GetTick();
  uint32_t rm_last_tick = HAL_GetTick();
  uint32_t display_last_tick = HAL_GetTick() - 50U;
  float target_rpm = 0.0f;
  float current_command = 0.0f;
  int16_t measured_rpm = 0;
  bool feedback_fresh = false;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ControllerReceiver_Process();

    HAL_GPIO_TogglePin(LED_D6_GPIO_Port, LED_D6_Pin);
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - led_last_tick) >= 20U)
    {
      gpio_toggle(LED_D5);
      led_last_tick = now;
    }
    if ((uint32_t)(now - rm_last_tick) >= 1U)
    {
      if (btn_read(BTN1))
      {
        target_rpm = RM_TARGET_RPM;
      }
      else if (btn_read(BTN2))
      {
        target_rpm = -RM_TARGET_RPM;
      }
      else
      {
        target_rpm = 0.0f;
      }

      feedback_fresh = rm_feedback_is_fresh(&hcan1, motor, RM_FEEDBACK_TIMEOUT_MS);
      measured_rpm = rm_get_actual_vel(&hcan1, motor);
      current_command = feedback_fresh ? SpeedCurrent(target_rpm, measured_rpm) : 0.0f;
      rm_set_current_setpoint(&hcan1, motor, current_command);
      rm_ctrl_loop(&hcan1);
      rm_last_tick = now;
    }

    // if ((uint32_t)(now - display_last_tick) >= 50U)
    // {
    //   int32_t milliamps = (int32_t)(rm_get_actual_current(&hcan1, motor) * 1000.0f);
    //   unsigned magnitude = (unsigned)(milliamps < 0 ? -milliamps : milliamps);
    //   int32_t command_ma = (int32_t)(current_command * 1000.0f);
    //   unsigned command_magnitude = (unsigned)(command_ma < 0 ? -command_ma : command_ma);
    //   tft_prints(0, 0, "RM1 SPEED P");
    //   tft_prints(0, 1, "tgt:%d RPM", (int)target_rpm);
    //   tft_prints(0, 2, "vel:%d RPM", (int)measured_rpm);
    //   tft_prints(0, 3, "err:%d RPM", (int)target_rpm - (int)measured_rpm);
    //   tft_prints(0, 4, "cmd:%c%u.%03uA", command_ma < 0 ? '-' : '+',
    //              command_magnitude / 1000U, command_magnitude % 1000U);
    //   tft_prints(0, 5, "cur:%c%u.%03uA", milliamps < 0 ? '-' : '+',
    //              magnitude / 1000U, magnitude % 1000U);
    //   tft_prints(0, 6, "enc:%u", (unsigned)rm_get_encoder(&hcan1, motor));
    //   tft_prints(0, 7, "BTN1:+300 RPM");
    //   tft_prints(0, 8, "BTN2:-300 RPM");
    //   tft_prints(0, 9, "CAN:%s", feedback_fresh ? "OK" : "NO FEEDBACK");
    //   tft_update(0);
    //   display_last_tick = HAL_GetTick();
    // }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
