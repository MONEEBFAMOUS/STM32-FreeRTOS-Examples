#include <stdio.h>
#include "main.h"
#include "cmsis_os.h"

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int __io_putchar(int ch);

void SenderTask(void *pvParameters);
void ReceiverTask(void *pvParameters);

//Humidity sensor is the first data type and pressure sensor is the second data type in
//our DataSource structure
typedef enum
{
	humidity_sensor,
	pressure_sensor

} DataSource_t;

//Lets create a structure.
//First member will be the value of our sensor
//Second member is of type data source
//This structure type will be passed to the queue
typedef struct
{
	uint8_t ucValue;
	DataSource_t sDataSource;
} Data_t;

//Declare two "Data_t" variables that will be passed to the queue
static const Data_t xStructsToSend[ 2 ] =
{
		{ 77, humidity_sensor }, //Used by humidity sensor
		{ 63, pressure_sensor }  //Used by pressure sensor
};

TaskHandle_t hum_task_handle, press_task_handle, receiver_task_handle, receiver_handle;

QueueHandle_t xQueue;



//------------------------------------------------------------------------------

int main(void)
{

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  //Create a queue to hold a maximum of three structures

  xQueue = xQueueCreate	(3, sizeof(Data_t));

  //Create a receiver task with a priority of 1
  xTaskCreate(ReceiverTask,
		  "Receiver Task",
		  400,
		  NULL,
		  1,
		  &receiver_handle);

  //Create task to send humidity data with a priority of 2
  xTaskCreate(SenderTask,
  		  "Humidity Sender Task",
  		  400,
  		  (void *)&(xStructsToSend[0]),
  		  2,
  		  &hum_task_handle);

  //Create task to send pressure data with a priority of 2
  xTaskCreate(SenderTask,
		  "Pressure Sender Task",
		  400,
		  (void *)&(xStructsToSend[1]),
		  2,
		  &press_task_handle);

  vTaskStartScheduler();



  while (1)
  {

  }
}

//-----------------------------------------------------------------------------
void SenderTask(void *pvParameters)
{
	BaseType_t qStatus;

	//Enter the blocked state for 200ms for space to become available in the queue
	//each time the queue is full

	const TickType_t wait_time = pdMS_TO_TICKS(200);

	while(1)
	{
		qStatus = xQueueSend(xQueue, pvParameters, wait_time);

		if (qStatus != pdPASS)
		{
			//dO Something...
		}
		for (int i=0; i < 100000; i++){}

	}
}

void ReceiverTask(void *pvParameters)
{
	Data_t xReceivedStructure;

	BaseType_t qStatus;

	while(1)
	{
		qStatus = xQueueReceive(xQueue, &xReceivedStructure, 0);
		if(qStatus == pdPASS)
		{
			if( xReceivedStructure.sDataSource == humidity_sensor )
			{
				printf( "Humidity sensor value = %d \n\r", xReceivedStructure.ucValue );
			}
			else
			{
				printf( "Pressure sensor value = %d \n\r", xReceivedStructure.ucValue );
			}
		}
		else
		{
			//Do something...
		}
	}
}

//------------------------------------------------------------------------------

int uart2_write(int ch)
{
	while(!(USART2->SR &0x0080)){}
	USART2 -> DR = (ch & 0XFF);

	return ch;

}

int __io_putchar(int ch)
{
  //HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  uart2_write(ch);
  return ch;
}

//------------------------------------------------------------------------------
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;  // Changed to HSI
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;                   // Changed to HSI_ON
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;  // Added
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;       // Changed to HSI
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;  // Changed from 336 to 168 for HSI
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
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

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  __HAL_RCC_USART2_CLK_ENABLE();

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
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
