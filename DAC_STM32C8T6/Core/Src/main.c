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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "OLED.h"
#include "ES9018K2M.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LINE_INPUT_SHOWN 1+2
#define LINE_LEVEL_SHOWN 2+2
#define LINE_FILTR_SHOWN 3+2
#define LINE_MODE_SHOWN  4+2

// 编码器引脚定义 (原按键定义现在用于编码器)
#define ENCODER_SW_PIN     GPIO_PIN_1   // 按键 (EXTI1)
#define ENCODER_CLK_PIN    GPIO_PIN_2   // A相/CLK (EXTI2) - 用于触发旋转中断
#define ENCODER_DT_PIN     GPIO_PIN_3   // B相/DT (EXTI3) - 仅作为输入读取电平

#define DEBOUNCE_TIME_MS 200     // 按钮消抖时间
#define TIMEOUT_MS       3000    // 超时时间3秒

// 添加串口命令相关定义
#define UART_RX_BUFFER_SIZE 64
#define UART_CMD_MAX_LENGTH 32

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef enum {
    MODE_LEVEL = 0,
    MODE_INPUT,
    MODE_FILTER,
    MODE_COUNT // 用于循环计数
} ControlMode_t;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// 编码器状态变量
ES9018K2M_Handle dac_chip;
uint8_t atten_half_dB;
uint8_t DAC_INPUT_SEL;
uint8_t DAC_FILTR_SEL;

volatile ControlMode_t current_mode = MODE_LEVEL; // 当前控制模式，默认为音量调节
volatile uint32_t last_rotation_time = 0;         // 记录最后一次旋转操作的时间
volatile uint32_t last_switch_press_time = 0;     // 按键消抖用

// 标记 DAC 寄存器是否需要更新（用于在主循环中执行I2C，避免在中断中执行阻塞操作）
volatile uint8_t dac_update_flag = 0;

// 串口接收相关变量
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
uint8_t uart_cmd_buffer[UART_CMD_MAX_LENGTH];
uint16_t uart_rx_index = 0;
uint8_t uart_cmd_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
#ifdef __GNUC__                                    //串口重定向
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1 , (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

// 串口命令处理函数声明
void UART_Command_Process(void);
void UART_Process_ReadCommand(char *cmd);
void UART_Process_WriteCommand(char *cmd);
void UART_Send_Help(void);
void UART_Send_Response(const char *format, ...);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  ES9018K2M_InitHandle(&dac_chip, &hi2c1, 0x90);
  printf("Try to reset ES9018K2M\n");
//  ES9018K2M_SoftReset(&dac_chip);

  // 启动串口接收中断
  HAL_UART_Receive_IT(&huart1, uart_rx_buffer, 1);

  atten_half_dB=0;
  if (ES9018K2M_GetAttenuation(&dac_chip, &atten_half_dB) == HAL_OK) {
      printf("Current attenuation: %d *0.5dB\n", atten_half_dB);
  }
//  ES9018K2M_SetAttenuation(&dac_chip, atten_half_dB);
  DAC_INPUT_SEL=0;
  DAC_FILTR_SEL=0;
  OLED_ShowString(LINE_INPUT_SHOWN, 1, "Input:");
  OLED_ShowString(LINE_FILTR_SHOWN, 1, "FLT:");

  // 打印帮助信息
  printf("\r\nES9018K2M Debug Console Ready!\r\n");
  printf("Available commands:\r\n");
  printf("  r [addr]          - Read register (hex)\r\n");
  printf("  w [addr] [value]  - Write register (hex)\r\n");
  printf("  level [value]     - Set attenuation (0-255)\r\n");
  printf("  input [0-2]       - Set input source\r\n");
  printf("  filter [0-2]      - Set filter type\r\n");
  printf("  status            - Show current status\r\n");
  printf("  help              - Show this help\r\n");
  printf("Example: r 01, w 01 FF, level 128\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t current_time = HAL_GetTick();

        // ============== 1. 模式超时检查 (非 LEVEL 模式下) ==============
        if (current_mode != MODE_LEVEL) {
            if (current_time - last_rotation_time > TIMEOUT_MS) {
                current_mode = MODE_LEVEL;
                printf("Mode Timeout, back to Level\n");
            }
        }

        // ============== 2. DAC I2C 写入 (在主循环中执行，非中断) ==============
        if (dac_update_flag) {
            printf("Applying DAC settings...\n");
            if (current_mode == MODE_LEVEL) {
                ES9018K2M_SetAttenuation(&dac_chip, atten_half_dB);
            } else if (current_mode == MODE_INPUT) {
                switch (DAC_INPUT_SEL) {
                    case 0: // USB
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_INPUT_CONFIG, 0b10001100);
                        break;
                    case 1: // SPDIF Optical
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GPIO_CONFIG, 0b10001000);
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_INPUT_CONFIG, 0b10000001);
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_CH_MAPPING, 0b00110010);
                        break;
                    case 2: // SPDIF Coaxial
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GPIO_CONFIG, 0b10001000);
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_INPUT_CONFIG, 0b10000001);
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_CH_MAPPING, 0b01000010);
                        break;
                }
            } else if (current_mode == MODE_FILTER) {
                switch (DAC_FILTR_SEL) {
                    case 0: // Fast RO
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GENERAL_SETTINGS, 0b10000000);
                        break;
                    case 1: // Slow RO
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GENERAL_SETTINGS, 0b10010000);
                        break;
                    case 2: // Min Phase
                        ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GENERAL_SETTINGS, 0b10100000);
                        break;
                }
            }
            dac_update_flag = 0; // 重置标志位
        }

        // ============== 3. 串口命令处理 ==============
            if (uart_cmd_ready) {
                UART_Command_Process();
                uart_cmd_ready = 0;
            }

  	  // ============== 4. OLED 屏幕刷新 ==============

  	  // Line1: Input
  	  switch (DAC_INPUT_SEL) {
  	  		  case 0:
  	  			  OLED_ShowString(LINE_INPUT_SHOWN, 8, "USB      ");
  	  			  break;
  	  		  case 1:
  	  			  OLED_ShowString(LINE_INPUT_SHOWN, 8, "SPDIF Cox");
  	  			  break;
  	  		  case 2:
  	  			  OLED_ShowString(LINE_INPUT_SHOWN, 8, "SPDIF Opt");
  	  			  break;
  	  		  default:
  	  			  OLED_ShowString(LINE_INPUT_SHOWN, 8, "Unknown");
  	  			  break;
  	  	}

  	  	  	  // Line2: Level
  	  	  	  OLED_ShowString(LINE_LEVEL_SHOWN, 1, "Level: -");
  	  	  	  OLED_ShowNum(LINE_LEVEL_SHOWN, 9, atten_half_dB/2, 3);
  	  	  	  if (atten_half_dB%2) OLED_ShowString(LINE_LEVEL_SHOWN, 12, ".5 dB");
  	  	  	  else OLED_ShowString(LINE_LEVEL_SHOWN, 12, ".0 dB");

  	  	  	  // Line3: Filter
  	  	  	  switch (DAC_FILTR_SEL) {
  	  	  		  case 0:
  	  	  			OLED_ShowString(LINE_FILTR_SHOWN, 6, "Fast RO  ");
   	  	  			break;
  	  	  		  case 1:
  	  	  			OLED_ShowString(LINE_FILTR_SHOWN, 6, "Slow RO  ");
  	  	  			break;
  	  	  		  case 2:
  	  	  			OLED_ShowString(LINE_FILTR_SHOWN, 6, "Min Phase");
  	  	  			break;
  	  	  		  default:
  	  	  			OLED_ShowString(LINE_FILTR_SHOWN, 6, "Unknown");
  	  	  			break;
  	  	  		}

            // Line4: Current Mode (新增)
            OLED_ShowString(LINE_MODE_SHOWN, 1, "Mode: ");
            switch (current_mode) {
                case MODE_LEVEL:
                    OLED_ShowString(LINE_MODE_SHOWN, 7, "**LEVEL** ");
                    break;
                case MODE_INPUT:
                    OLED_ShowString(LINE_MODE_SHOWN, 7, "**INPUT** ");
                    break;
                case MODE_FILTER:
                    OLED_ShowString(LINE_MODE_SHOWN, 7, "**FILTER** ");
                    break;
                default:
                    OLED_ShowString(LINE_MODE_SHOWN, 7, "UNKNOWN    ");
                    break;
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PA1 PA2 PA3 PA4 */
  GPIO_InitStruct.Pin = ENCODER_CLK_PIN|ENCODER_SW_PIN; // PA1(SW) 和 PA2(CLK)
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure Input GPIO pin: PA3 (DT) */
  GPIO_InitStruct.Pin = ENCODER_DT_PIN; // PA3(DT)
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 仅作为输入读取电平
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  // EXTI1 (PA1 - SW)
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  // EXTI2 (PA2 - CLK)
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// 外部中断回调函数 - 只在下降沿触发
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_time = HAL_GetTick();

    // 1. 编码器按键 (SW) 处理 - 模式切换
    if (GPIO_Pin == ENCODER_SW_PIN)
    {
        // 关键：确认是真正的下降沿（当前为低电平）
        if (HAL_GPIO_ReadPin(GPIOA, ENCODER_SW_PIN) != GPIO_PIN_RESET) {
            return; // 忽略
        }

        // 按键消抖检查
        if (current_time - last_switch_press_time < DEBOUNCE_TIME_MS) {
            return; // 忽略抖动
        }
        last_switch_press_time = current_time;

        // 切换模式
        current_mode++;
        if (current_mode >= MODE_COUNT) {
            current_mode = MODE_LEVEL;
        }
        printf("Mode Switched to: %d\n", current_mode);

        // 每次模式切换，都重置旋转操作时间，避免立即超时
        last_rotation_time = current_time;
    }

    // 2. 编码器旋转 (CLK) 处理 - 参数调节
    // **仅处理 CLK 引脚的中断 (ENCODER_CLK_PIN)**
    else if (GPIO_Pin == ENCODER_CLK_PIN)
    {
        // 关键：仅处理下降沿（这是 EXT_FALLING 的要求）
        if (HAL_GPIO_ReadPin(GPIOA, ENCODER_CLK_PIN) != GPIO_PIN_RESET) {
            return; // 忽略
        }

        // 记录旋转时间，用于超时判断
        last_rotation_time = current_time;

        // 获取 B (DT) 的当前电平
        uint8_t PinB = HAL_GPIO_ReadPin(GPIOA, ENCODER_DT_PIN);

        int8_t direction = 0; // 1: 顺时针(CW), -1: 逆时针(CCW)

        // 方向判断逻辑 (基于 A/CLK 下降沿，保持不变)
        if (PinB == GPIO_PIN_SET) {
            // A↓, B↑ -> 逆时针 (CCW)
            direction = -1;
        } else { // PinB == GPIO_PIN_RESET
            // A↓, B↓ -> 顺时针 (CW)
            direction = 1;
        }

        if (direction != 0)
        {
            switch (current_mode)
            {
                case MODE_LEVEL: // 衰减调节 (逻辑已反转)
                    if (direction == 1) { // CW: 减少衰减 (音量增大)
                        if (atten_half_dB > 0x00) atten_half_dB--;
                    } else { // CCW: 增加衰减 (音量减小)
                        if (atten_half_dB < 0xFF) atten_half_dB++;
                    }
                    printf("Level: %d *0.5dB\n", atten_half_dB);
                    // 设置标志位，让主循环执行 I2C 写入
                    dac_update_flag = 1;
                    break;

                case MODE_INPUT: // 输入调节 (保持不变，CW=下一个, CCW=上一个)
                    if (direction == 1) { // CW: 循环下一个
                        DAC_INPUT_SEL = (DAC_INPUT_SEL + 1) % 3;
                    } else { // CCW: 循环上一个
                        DAC_INPUT_SEL = (DAC_INPUT_SEL == 0) ? 2 : (DAC_INPUT_SEL - 1);
                    }
                    printf("Input SEL: %d\n", DAC_INPUT_SEL);
                    dac_update_flag = 1;
                    break;

                case MODE_FILTER: // 滤波调节 (保持不变，CW=下一个, CCW=上一个)
                    if (direction == 1) { // CW: 循环下一个
                        DAC_FILTR_SEL = (DAC_FILTR_SEL + 1) % 3;
                    } else { // CCW: 循环上一个
                        DAC_FILTR_SEL = (DAC_FILTR_SEL == 0) ? 2 : (DAC_FILTR_SEL - 1);
                    }
                    printf("Filter SEL: %d\n", DAC_FILTR_SEL);
                    dac_update_flag = 1;
                    break;
            }
        }
    }
    // **注意: PA3 (DT) 的中断被忽略**
}

// 串口接收完成回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t received_char = uart_rx_buffer[0];

        // 处理回车或换行作为命令结束符
        if (received_char == '\r' || received_char == '\n') {
            if (uart_rx_index > 0) {
                uart_cmd_buffer[uart_rx_index] = '\0'; // 添加字符串结束符
                uart_cmd_ready = 1;
                uart_rx_index = 0;
            }
        }
        // 处理退格键
        else if (received_char == '\b' || received_char == 0x7F) {
            if (uart_rx_index > 0) {
                uart_rx_index--;
                printf("\b \b"); // 回显退格
            }
        }
        // 普通字符处理
        else if (uart_rx_index < (UART_CMD_MAX_LENGTH - 1)) {
            uart_cmd_buffer[uart_rx_index++] = received_char;
            printf("%c", received_char); // 回显字符
        }

        // 重新启动接收
        HAL_UART_Receive_IT(&huart1, uart_rx_buffer, 1);
    }
}

// 在命令处理中添加新命令
void UART_Command_Process(void)
{
    char *cmd = (char *)uart_cmd_buffer;
    printf("\r\n");

    while (*cmd == ' ') cmd++;
    if (strlen(cmd) == 0) return;

    // 命令解析
    if (strncmp(cmd, "r ", 2) == 0) {
        UART_Process_ReadCommand(cmd + 2);
    }
    else if (strncmp(cmd, "w ", 2) == 0) {
        UART_Process_WriteCommand(cmd + 2);
    }
    else if (strncmp(cmd, "level ", 6) == 0) {
        UART_Process_LevelCommand(cmd + 6);
    }
    else if (strncmp(cmd, "input ", 6) == 0) {
        UART_Process_InputCommand(cmd + 6);
    }
    else if (strncmp(cmd, "filter ", 7) == 0) {
        UART_Process_FilterCommand(cmd + 7);
    }
    else if (strcmp(cmd, "bassboost") == 0) {
        UART_Process_BassBoostCommand(cmd);
    }
    else if (strcmp(cmd, "lowpass") == 0) {
        UART_Process_LowpassCommand(cmd);
    }
    else if (strcmp(cmd, "safevolume") == 0) {  // 新增安全音量命令
        UART_Process_SafeVolume();
    }
    else if (strcmp(cmd, "resetfilter") == 0) { // 新增重置滤波器命令
        UART_Process_ResetFilter();
    }
    else if (strcmp(cmd, "disablecustom") == 0) {
        UART_Process_DisableCustomFilter();
    }
    else if (strcmp(cmd, "filterstatus") == 0) {
        UART_Process_FilterStatusCommand();
    }
    else if (strcmp(cmd, "status") == 0) {
        UART_Process_StatusCommand();
    }
    else if (strcmp(cmd, "help") == 0) {
        UART_Send_Help();
    }
    else {
        UART_Send_Response("Unknown command: %s", cmd);
        UART_Send_Response("Type 'help' for available commands");
    }

    printf("> ");
}

// 读取寄存器命令
void UART_Process_ReadCommand(char *cmd)
{
    uint8_t reg_addr;
    uint8_t reg_value;

    if (sscanf(cmd, "%hhx", &reg_addr) == 1) {
        if (ES9018K2M_ReadReg(&dac_chip, reg_addr, &reg_value) == HAL_OK) {
            UART_Send_Response("Register 0x%02X = 0x%02X", reg_addr, reg_value);
        } else {
            UART_Send_Response("Error reading register 0x%02X", reg_addr);
        }
    } else {
        UART_Send_Response("Invalid read command format");
        UART_Send_Response("Usage: r [addr]");
        UART_Send_Response("Example: r 01");
    }
}

// 写入寄存器命令
void UART_Process_WriteCommand(char *cmd)
{
    unsigned int reg_addr, reg_value;  // 使用 unsigned int 而不是 uint8_t

    UART_Send_Response("Debug: Processing write command: '%s'", cmd);

    // 方法1：使用 %x 解析到 unsigned int
    int parsed_count = sscanf(cmd, "%x %x", &reg_addr, &reg_value);
    printf("Parsed %d parameters: addr=0x%X, value=0x%X\n", parsed_count, reg_addr, reg_value);

    if (parsed_count == 2) {
        // 检查值是否在有效范围内
        if (reg_addr > 0xFF || reg_value > 0xFF) {
            UART_Send_Response("Error: Address and value must be 8-bit (0x00-0xFF)");
            UART_Send_Response("Received: addr=0x%X, value=0x%X", reg_addr, reg_value);
            return;
        }

        uint8_t addr_byte = (uint8_t)reg_addr;
        uint8_t value_byte = (uint8_t)reg_value;

        UART_Send_Response("Writing 0x%02X to register 0x%02X", value_byte, addr_byte);

        // 继续执行写入操作...
        HAL_Delay(10);
        HAL_StatusTypeDef status = ES9018K2M_WriteReg(&dac_chip, addr_byte, value_byte);

        if (status == HAL_OK) {
            UART_Send_Response("Write OK: 0x%02X = 0x%02X", addr_byte, value_byte);
        } else {
            UART_Send_Response("Error writing register 0x%02X, HAL Status: %d", addr_byte, status);
        }
    } else {
        UART_Send_Response("Invalid write command format (parsed %d parameters)", parsed_count);
        UART_Send_Response("Usage: w [addr] [value]");
        UART_Send_Response("Example: w 01 81 or w 0x01 0x81");
    }
}

// 检查滤波器状态
void UART_Process_FilterStatusCommand(void)
{
    uint8_t filter_control, filter_addr;

    ES9018K2M_ReadReg(&dac_chip, 0x1E, &filter_control);
    ES9018K2M_ReadReg(&dac_chip, 0x1A, &filter_addr);

    UART_Send_Response("=== Filter Status ===");
    UART_Send_Response("Filter Control (0x1E): 0x%02X", filter_control);
    UART_Send_Response("Filter Addr (0x1A): 0x%02X", filter_addr);

    UART_Send_Response("Custom Filter: %s", (filter_control & 0x01) ? "ENABLED" : "DISABLED");
    UART_Send_Response("Coeff Write: %s", (filter_control & 0x02) ? "ENABLED" : "DISABLED");
    UART_Send_Response("Stage2 Symmetry: %s", (filter_control & 0x04) ? "COSINE(28-tap)" : "SINE(27-tap)");

    UART_Send_Response("Current Stage: %s", (filter_addr & 0x80) ? "STAGE2" : "STAGE1");
    UART_Send_Response("Current Addr: 0x%02X", filter_addr & 0x7F);
}

// 设置音量命令
void UART_Process_LevelCommand(char *cmd)
{
    uint16_t level;

    if (sscanf(cmd, "%hu", &level) == 1) {
        if (level <= 255) {
            atten_half_dB = (uint8_t)level;
            dac_update_flag = 1;
            UART_Send_Response("Attenuation set to %d (%.1f dB)",
                             atten_half_dB, atten_half_dB * 0.5f);
        } else {
            UART_Send_Response("Level must be 0-255");
        }
    } else {
        UART_Send_Response("Invalid level command format");
        UART_Send_Response("Usage: level [0-255]");
    }
}

// 设置输入命令
void UART_Process_InputCommand(char *cmd)
{
    uint8_t input;

    if (sscanf(cmd, "%hhu", &input) == 1) {
        if (input < 3) {
            DAC_INPUT_SEL = input;
            dac_update_flag = 1;
            const char *input_names[] = {"USB", "SPDIF Coaxial", "SPDIF Optical"};
            UART_Send_Response("Input set to %d (%s)", input, input_names[input]);
        } else {
            UART_Send_Response("Input must be 0-2");
        }
    } else {
        UART_Send_Response("Invalid input command format");
        UART_Send_Response("Usage: input [0-2]");
    }
}

// 设置滤波器命令
void UART_Process_FilterCommand(char *cmd)
{
    uint8_t filter;

    if (sscanf(cmd, "%hhu", &filter) == 1) {
        if (filter < 3) {
            DAC_FILTR_SEL = filter;
            dac_update_flag = 1;
            const char *filter_names[] = {"Fast RO", "Slow RO", "Min Phase"};
            UART_Send_Response("Filter set to %d (%s)", filter, filter_names[filter]);
        } else {
            UART_Send_Response("Filter must be 0-2");
        }
    } else {
        UART_Send_Response("Invalid filter command format");
        UART_Send_Response("Usage: filter [0-2]");
    }
}

// 状态查询命令
void UART_Process_StatusCommand(void)
{
    UART_Send_Response("=== ES9018K2M Status ===");
    UART_Send_Response("Attenuation: %d (%.1f dB)", atten_half_dB, atten_half_dB * 0.5f);

    const char *input_names[] = {"USB", "SPDIF Coaxial", "SPDIF Optical"};
    UART_Send_Response("Input: %d (%s)", DAC_INPUT_SEL, input_names[DAC_INPUT_SEL]);

    const char *filter_names[] = {"Fast RO", "Slow RO", "Min Phase"};
    UART_Send_Response("Filter: %d (%s)", DAC_FILTR_SEL, filter_names[DAC_FILTR_SEL]);

    const char *mode_names[] = {"LEVEL", "INPUT", "FILTER"};
    UART_Send_Response("Current Mode: %s", mode_names[current_mode]);
}

// 更新帮助信息
void UART_Send_Help(void)
{
    UART_Send_Response("Available commands:");
    UART_Send_Response("  r [addr]          - Read register (hex)");
    UART_Send_Response("  w [addr] [value]  - Write register (hex)");
    UART_Send_Response("  level [value]     - Set attenuation (0-255)");
    UART_Send_Response("  input [0-2]       - Set input source");
    UART_Send_Response("  filter [0-2]      - Set built-in filter type");
    UART_Send_Response("  bassboost         - Enable true bass boost");
    UART_Send_Response("  lowpass           - Enable simple lowpass filter");
    UART_Send_Response("  safevolume        - Set safe volume to prevent clipping");
    UART_Send_Response("  resetfilter       - Reset to default filter and safe volume");
    UART_Send_Response("  disablecustom     - Disable custom filter");
    UART_Send_Response("  filterstatus      - Show filter status");
    UART_Send_Response("  status            - Show current status");
    UART_Send_Response("  help              - Show this help");
    UART_Send_Response("Important:");
    UART_Send_Response("  - Use 'safevolume' after enabling custom filters");
    UART_Send_Response("  - Use 'resetfilter' if experiencing issues");
}

// 通用响应发送函数
void UART_Send_Response(const char *format, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s\r\n", buffer);
}

// 真正的低音增强滤波器系数计算
int32_t calculate_bass_boost_coeff(int index, int total_coeffs, float bass_gain)
{
    int center = total_coeffs / 2;
    int distance = abs(index - center);

    // 基础低通滤波器系数
    float cutoff = 0.2f; // 200Hz左右的低通
    float pi = 3.14159265358979323846f;
    float coeff = 0.0f;

    if (distance == 0) {
        // 中心抽头
        coeff = 2.0f * cutoff;
    } else {
        // sinc函数
        coeff = sinf(2.0f * pi * cutoff * distance) / (pi * distance);

        // 应用汉宁窗
        float window = 0.5f - 0.5f * cosf(2.0f * pi * index / (total_coeffs - 1));
        coeff *= window;
    }

    // 应用低音增强
    if (distance < total_coeffs / 4) {
        // 对低频部分进行增强
        coeff *= bass_gain;
    }

    // 缩放到合适的范围，避免爆音
    int32_t result = (int32_t)(coeff * 0x3FFFFF); // 使用较小的缩放因子

    // 限制范围
    if (result > 0x3FFFFF) result = 0x3FFFFF;
    if (result < -0x3FFFFF) result = -0x3FFFFF;

    return result;
}

// 改进的低音增强滤波器命令
void UART_Process_BassBoostCommand(char *cmd)
{
    UART_Send_Response("Enabling True Bass Boost Filter...");

    // 0. 首先读取当前状态
    uint8_t current_ctrl;
    ES9018K2M_ReadReg(&dac_chip, 0x1E, &current_ctrl);
    UART_Send_Response("Current Filter Control: 0x%02X", current_ctrl);

    // 1. 启用系数写入
    UART_Send_Response("Step 1: Enabling coefficient write mode...");
    if (ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x02) != HAL_OK) {
        UART_Send_Response("ERROR: Failed to enable coefficient write!");
        return;
    }
    HAL_Delay(50);

    // 2. 编程阶段1系数 - 真正的低音增强
    UART_Send_Response("Step 2: Programming Stage 1 bass boost coefficients...");

    for (int i = 0; i < 128; i++) {
        ES9018K2M_WriteReg(&dac_chip, 0x1A, i);

        // 使用真正的低音增强系数，增益1.5倍
        int32_t coefficient = calculate_bass_boost_coeff(i, 128, 1.5f);

        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coefficient >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coefficient >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coefficient & 0xFF);

        if ((i % 32) == 0) {
            UART_Send_Response("  Stage 1 progress: %d/128", i);
        }
    }

    // 3. 编程阶段2系数
    UART_Send_Response("Step 3: Programming Stage 2 coefficients...");

    for (int i = 0; i < 14; i++) {
        ES9018K2M_WriteReg(&dac_chip, 0x1A, 0x80 | i);

        // 阶段2使用相同的低音增强
        int32_t coefficient = calculate_bass_boost_coeff(i, 16, 1.3f);

        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coefficient >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coefficient >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coefficient & 0xFF);
    }

    // 4. 启用自定义滤波器
    UART_Send_Response("Step 4: Enabling custom filter...");

    uint8_t filter_control = 0x01; // prog_coeff_en=1, 正弦对称

    if (ES9018K2M_WriteReg(&dac_chip, 0x1E, filter_control) != HAL_OK) {
        UART_Send_Response("ERROR: Failed to enable custom filter!");
        return;
    }

    HAL_Delay(50);

    // 验证最终状态
    ES9018K2M_ReadReg(&dac_chip, 0x1E, &current_ctrl);
    UART_Send_Response("Final Filter Control: 0x%02X", current_ctrl);

    UART_Send_Response("True Bass Boost Filter Enabled!");
    UART_Send_Response("Bass frequencies are now enhanced.");
}

// 简单的低通滤波器（用于对比测试）
void UART_Process_LowpassCommand(char *cmd)
{
    float cutoff = 0.25f; // 默认截止频率

    UART_Send_Response("Setting up Lowpass Filter (cutoff: %.2f)...", cutoff);

    // 启用系数写入
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x02);
    HAL_Delay(20);

    // 编程阶段1系数 - 简单的低通
    for (int i = 0; i < 128; i++) {
        ES9018K2M_WriteReg(&dac_chip, 0x1A, i);

        int center = 64;
        int distance = abs(i - center);
        int32_t coeff = 0;

        if (distance == 0) {
            coeff = 0x200000; // 中心值较小
        } else if (distance < 10) {
            coeff = 0x100000; // 靠近中心
        } else if (distance < 30) {
            coeff = 0x080000; // 中等距离
        } else {
            coeff = 0x020000; // 远离中心
        }

        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coeff >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coeff >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coeff & 0xFF);
    }

    // 编程阶段2系数
    for (int i = 0; i < 14; i++) {
        ES9018K2M_WriteReg(&dac_chip, 0x1A, 0x80 | i);

        int32_t coeff = 0x100000; // 统一的较小值

        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coeff >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coeff >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coeff & 0xFF);
    }

    // 启用滤波器
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x01);

    UART_Send_Response("Lowpass Filter Enabled - High frequencies attenuated");
}

// 新增：音量保护命令
void UART_Process_SafeVolume(void)
{
    UART_Send_Response("Setting safe volume levels to prevent clipping...");

    // 设置较低的衰减值以避免爆音
    atten_half_dB = 150; // -75dB，中等音量
    dac_update_flag = 1;

    UART_Send_Response("Volume set to safe level: %d (%.1f dB)", atten_half_dB, atten_half_dB * 0.5f);
    UART_Send_Response("Adjust volume gradually to find optimal level.");
}

// 简化的低通滤波器系数 (矩形窗)
int32_t calculate_simple_lowpass(int n, int N, float cutoff)
{
    int center = N / 2;
    int x = n - center;

    if (x == 0) {
        return (int32_t)(0x7FFFFF * 2.0f * cutoff);
    }

    // 简单的sinc函数
    float pi = 3.14159265358979323846f;
    float value = sin(2.0f * pi * cutoff * x) / (pi * x);

    // 直接缩放
    return (int32_t)(value * 0x7FFFFF);
}

// 低通滤波器配置函数
void setup_lowpass_filter(void)
{
    // 1. 启用系数写入模式
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x02); // prog_coeff_we = 1
    HAL_Delay(10);

    // 2. 配置阶段1低通滤波器系数 (128个系数)
    for (int i = 0; i < 128; i++) {
        ES9018K2M_WriteReg(&dac_chip, 0x1A, i); // 设置系数地址

        // 计算低通滤波器系数
        int32_t coeff = calculate_lowpass_coeff(i, 128, 0.3); // 截止频率比0.3

        // 写入24位系数
        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coeff >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coeff >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coeff & 0xFF);
    }

    // 3. 配置阶段2低通滤波器系数 (16个系数)
    for (int i = 0; i < 14; i++) { // 只使用前14个系数
        ES9018K2M_WriteReg(&dac_chip, 0x1A, 0x80 | i); // 阶段2，地址i

        // 计算阶段2低通系数
        int32_t coeff = calculate_lowpass_coeff(i, 16, 0.4); // 稍高的截止频率

        // 写入24位系数
        ES9018K2M_WriteReg(&dac_chip, 0x1B, (coeff >> 16) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1C, (coeff >> 8) & 0xFF);
        ES9018K2M_WriteReg(&dac_chip, 0x1D, coeff & 0xFF);
    }

    // 4. 禁用系数写入，启用自定义滤波器
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x01); // prog_coeff_en=1, 正弦对称
}

// 新增：重置滤波器到默认状态
void UART_Process_ResetFilter(void)
{
    UART_Send_Response("Resetting filter to default state...");

    // 禁用自定义滤波器
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x00);

    // 设置Fast RO内置滤波器
    ES9018K2M_WriteReg(&dac_chip, ES9018K2M_REG_GENERAL_SETTINGS, 0b10000000);

    // 设置安全音量
    atten_half_dB = 120;
    ES9018K2M_SetAttenuation(&dac_chip, atten_half_dB);

    UART_Send_Response("Filter reset to default (Fast RO)");
    UART_Send_Response("Volume set to safe level");
}

// 禁用自定义滤波器，返回内置滤波器
void UART_Process_DisableCustomFilter(void)
{
    UART_Send_Response("Disabling custom filter, returning to built-in filters...");
    ES9018K2M_WriteReg(&dac_chip, 0x1E, 0x00); // 禁用自定义滤波器
    UART_Send_Response("Custom filter disabled");
}
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
