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
#include "adc.h"
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
#include "ir_app.h"
#include "ir_config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RM_TARGET_RPM 300.0f
/* Speed PI starting values (rotor RPM in, amperes out). Not final tuning:
   Kp 0.010 alone oscillated at 100 RPM, so Kp is lowered and the I term
   removes the remaining error. Integral time Kp/Ki = 60 ms. */
#define RM_SPEED_KP 0.006f          // Amperes per RPM.
#define RM_SPEED_KI 0.10f           // Amperes per (RPM * s).
#define RM_TEST_CURRENT_LIMIT 1.5f
#define RM_MAX_DT_S 0.01f           // Longest time step the integral may use (guards long loop stalls).
#define RM_FEEDBACK_TIMEOUT_MS 100U

/* Joystick -> motor 1 target speed. */
#define DRIVE_AXIS       1U       /* J1Y; axis order is J1X, J1Y, J2X, J2Y */
#define STICK_DEADZONE   50       /* out of +-1000; extra margin on top of the controller's deadband */
#define MAX_TARGET_RPM   2000.0f  /* rotor RPM (~104 RPM at the M3508 output, ~1.7 turns/s) */

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
/* Convert a joystick axis (-1000..+1000) into a target rotor RPM.
   The result can never exceed +-MAX_TARGET_RPM. */
static float StickToTargetRpm(int16_t stick)
{
  if (stick > -STICK_DEADZONE && stick < STICK_DEADZONE)
  {
    return 0.0f;                         /* stick near centre: stand still */
  }
  if (stick > 1000) stick = 1000;        /* guard against out-of-range values */
  if (stick < -1000) stick = -1000;
  return (float)stick / 1000.0f * MAX_TARGET_RPM;
}

/* Speed PI controller: target/actual in rotor RPM, output in amperes. */
typedef struct
{
  float kp;        /* A per RPM */
  float ki;        /* A per (RPM * s) */
  float limit;     /* output clamp, +-A */
  float integral;  /* accumulated error, RPM * s */
} SpeedPI;

static SpeedPI speed_pi = {
  .kp = RM_SPEED_KP,
  .ki = RM_SPEED_KI,
  .limit = RM_TEST_CURRENT_LIMIT,
  .integral = 0.0f,
};

/* Forget the accumulated error, e.g. after a stop or a lost signal. */
static void SpeedPI_Reset(SpeedPI *pi)
{
  pi->integral = 0.0f;
}

static float SpeedPI_Update(SpeedPI *pi, float target_rpm, float actual_rpm, float dt_s)
{
  float error = target_rpm - actual_rpm;

  /* What the integral would become if we integrate this step. */
  float candidate = pi->integral + error * dt_s;

  /* Extra safety net: the I part alone may never exceed the output limit. */
  if (pi->ki > 0.0f)
  {
    float max_integral = pi->limit / pi->ki;
    if (candidate > max_integral) candidate = max_integral;
    if (candidate < -max_integral) candidate = -max_integral;
  }

  float output = pi->kp * error + pi->ki * candidate;

  /* Anti-windup: while the output is clamped, do NOT keep integrating. */
  if (output > pi->limit)
  {
    output = pi->limit;
  }
  else if (output < -pi->limit)
  {
    output = -pi->limit;
  }
  else
  {
    pi->integral = candidate;
  }

  return output;
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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  IR_Init();

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
  uint32_t display_last_tick = HAL_GetTick() - 100U;
  int16_t measured_rpm = 0;
  uint16_t encoder = 0U;
  bool feedback_fresh = false;
  bool link_fresh = false;
  float target_rpm = 0.0f;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    IR_Update();
    HAL_GPIO_TogglePin(LED_D6_GPIO_Port, LED_D6_Pin);
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - led_last_tick) >= 20U)
    {
      gpio_toggle(LED_D5);
      led_last_tick = now;
    }

    /* Motor task, every 1 ms: joystick -> target RPM -> PI -> current. */
    if ((uint32_t)(now - rm_last_tick) >= 1U)
    {
      /* Real time since the last pass; the loop is not always exactly 1 ms. */
      float dt_s = (float)(now - rm_last_tick) / 1000.0f;
      if (dt_s > RM_MAX_DT_S) dt_s = RM_MAX_DT_S;

      link_fresh = ControllerReceiver_IsFresh(CONTROLLER_RECEIVER_TIMEOUT_MS);
      feedback_fresh = rm_feedback_is_fresh(&hcan1, motor, RM_FEEDBACK_TIMEOUT_MS);
      measured_rpm = rm_get_actual_vel(&hcan1, motor);
      encoder = rm_get_encoder(&hcan1, motor);

      /* Lost controller link -> target 0, whatever the last stick value was. */
      target_rpm = link_fresh
                     ? StickToTargetRpm(ControllerReceiver_GetAxis(DRIVE_AXIS))
                     : 0.0f;

      float current_command;
      if (!link_fresh || !feedback_fresh)
      {
        /* Lost controller or motor feedback: no current, forget old error. */
        SpeedPI_Reset(&speed_pi);
        current_command = 0.0f;
      }
      else
      {
        if (target_rpm == 0.0f)
        {
          /* Stick released: don't carry old accumulated error into the next move. */
          SpeedPI_Reset(&speed_pi);
        }
        current_command = SpeedPI_Update(&speed_pi, target_rpm,
                                         (float)measured_rpm, dt_s);
      }

      /* Only motor 1 is ever set; motors 2-8 stay at 0 A. */
      rm_set_current_setpoint(&hcan1, motor, current_command);
      rm_ctrl_loop(&hcan1);
      rm_last_tick = now;
    }

    /* Display task, every 100 ms. This is the only place that calls
       tft_update(), so the controller and motor rows never fight. */
    if ((uint32_t)(now - display_last_tick) >= 100U)
    {
      ControllerReceiver_Draw();                       /* rows 0-4 */
      tft_prints(0, 5, "M1 CAN:%-5s", feedback_fresh ? "OK" : "STALE");
      tft_prints(0, 6, "RPM:%6d", (int)measured_rpm);  /* rotor RPM */
      tft_prints(0, 7, "ENC:%5u", (unsigned)encoder);  /* 0-8191 per rotor turn */
      /* Target and error (target - actual), both in rotor RPM. */
      tft_prints(0, 8, "T:%5d E:%5d", (int)target_rpm,
                 (int)target_rpm - (int)measured_rpm);
      /* Current in mA: commanded (read back from the driver) / measured by
         the C620. Measured ~= commanded means the ESC is obeying. */
      tft_prints(0, 9, "I:%5d/%5d mA",
                 (int)(rm_get_current_setpoint(&hcan1, motor) * 1000.0f),
                 (int)(rm_get_actual_current(&hcan1, motor) * 1000.0f));
      tft_update(0);
      display_last_tick = now;
    }
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
