/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f3xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f3xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dictionary.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
int j = 0;
int isSfx = 0;
extern int game_paused;
extern int is_dead;
extern int paused;
extern int in_menu;

enum GameState {
	MENU, OPTIONS, GAME, GAMEOVER
};
extern enum GameState gameState;
enum OptionsEnumerator {
	ABOUT, SETTINGS, MOD, START, NONE
};
extern enum OptionsEnumerator currentOption;

enum Direction {
	UP, DOWN, LEFT, RIGHT
};

extern enum Direction direction;

extern struct Dictionary *playlist;
extern TIM_HandleTypeDef htim2;
TIM_HandleTypeDef *pwm_timer = &htim2; // Point to PWM Timer configured in CubeMX
uint32_t pwm_channel = TIM_CHANNEL_1;   // Select configured PWM channel number

const struct Tone *volatile melody_ptr;
volatile uint16_t melody_tone_count;
volatile uint16_t current_tone_number;
volatile uint32_t current_tone_end;
volatile uint16_t volume = 5;          // (0 - 1000)
volatile uint32_t last_button_press;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void PWM_Start() {
	HAL_TIM_PWM_Start(pwm_timer, pwm_channel);
}

void PWM_Change_Tone(uint16_t pwm_freq, uint16_t volume) // pwm_freq (1 - 20000), volume (0 - 1000)
{
	if (pwm_freq == 0 || pwm_freq > 20000) {
		__HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, 0);
	} else {
		const uint32_t internal_clock_freq = HAL_RCC_GetSysClockFreq();
		const uint16_t prescaler = 1;
		const uint32_t timer_clock = internal_clock_freq / prescaler;
		const uint32_t period_cycles = timer_clock / pwm_freq;
		const uint32_t pulse_width = volume * period_cycles / 1000 / 2;

		pwm_timer->Instance->PSC = prescaler - 1;
		pwm_timer->Instance->ARR = period_cycles - 1;
		pwm_timer->Instance->EGR = TIM_EGR_UG;
		__HAL_TIM_SET_COMPARE(pwm_timer, pwm_channel, pulse_width); // pwm_timer->Instance->CCR2 = pulse_width;
	}
}

void Change_Melody(const struct Tone *melody, uint16_t tone_count) {
	melody_ptr = melody;
	melody_tone_count = tone_count;
	current_tone_number = 0;
}

struct Tone sfx[] = { { 1047, 50 }, { 82, 200 } , { 82, 1000 }};

void Update_Melody() {
	if (!paused) {
		if ((HAL_GetTick() > current_tone_end)
				&& (current_tone_number < melody_tone_count)) {
			if (isSfx) {
				PWM_Change_Tone(sfx[isSfx - 1].frequency, 5);
				current_tone_end = HAL_GetTick() + sfx[isSfx - 1].duration;
				isSfx = 0;
			} else {
				const struct Tone active_tone = *(melody_ptr
						+ current_tone_number);
				PWM_Change_Tone(active_tone.frequency, volume);
				current_tone_end = HAL_GetTick() + active_tone.duration;
				current_tone_number++;
			}
		} else if (current_tone_number >= melody_tone_count) {
			current_tone_number = 0;
		}
	} else {
		PWM_Change_Tone(0, 0);
	}
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
	/* USER CODE BEGIN NonMaskableInt_IRQn 0 */

	/* USER CODE END NonMaskableInt_IRQn 0 */
	/* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1) {
	}
	/* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {
	/* USER CODE BEGIN HardFault_IRQn 0 */

	/* USER CODE END HardFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_HardFault_IRQn 0 */
		/* USER CODE END W1_HardFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
	/* USER CODE BEGIN MemoryManagement_IRQn 0 */

	/* USER CODE END MemoryManagement_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
		/* USER CODE END W1_MemoryManagement_IRQn 0 */
	}
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void) {
	/* USER CODE BEGIN BusFault_IRQn 0 */

	/* USER CODE END BusFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_BusFault_IRQn 0 */
		/* USER CODE END W1_BusFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {
	/* USER CODE BEGIN UsageFault_IRQn 0 */

	/* USER CODE END UsageFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_UsageFault_IRQn 0 */
		/* USER CODE END W1_UsageFault_IRQn 0 */
	}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
	/* USER CODE BEGIN SVCall_IRQn 0 */

	/* USER CODE END SVCall_IRQn 0 */
	/* USER CODE BEGIN SVCall_IRQn 1 */

	/* USER CODE END SVCall_IRQn 1 */
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
	/* USER CODE BEGIN DebugMonitor_IRQn 0 */

	/* USER CODE END DebugMonitor_IRQn 0 */
	/* USER CODE BEGIN DebugMonitor_IRQn 1 */

	/* USER CODE END DebugMonitor_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
	/* USER CODE BEGIN PendSV_IRQn 0 */

	/* USER CODE END PendSV_IRQn 0 */
	/* USER CODE BEGIN PendSV_IRQn 1 */

	/* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
	/* USER CODE BEGIN SysTick_IRQn 0 */

	/* USER CODE END SysTick_IRQn 0 */
	HAL_IncTick();
	/* USER CODE BEGIN SysTick_IRQn 1 */
	Update_Melody();
	/* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F3xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f3xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles EXTI line0 interrupt.
 */
void EXTI0_IRQHandler(void) {
	/* USER CODE BEGIN EXTI0_IRQn 0 */

	/* USER CODE END EXTI0_IRQn 0 */
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
	/* USER CODE BEGIN EXTI0_IRQn 1 */
	for (int i = 0; i < 4; i++) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
		if (i == 0) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) {
				j = 77;
			}
		} else if (i == 1) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) {

				j = 1;
			}
		} else if (i == 2) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) {
				j = 2;
			}
		} else if (i == 3) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) {
				if (direction != RIGHT)
					direction = LEFT;
				j = 3;
			}

		}
		HAL_Delay(10);
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0))
		;
	/* USER CODE END EXTI0_IRQn 1 */
}

/**
 * @brief This function handles EXTI line1 interrupt.
 */
void EXTI1_IRQHandler(void) {
	/* USER CODE BEGIN EXTI1_IRQn 0 */

	/* USER CODE END EXTI1_IRQn 0 */
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
	/* USER CODE BEGIN EXTI1_IRQn 1 */
	for (int i = 0; i < 4; i++) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
		if (i == 0) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1)) {

			}
		} else if (i == 1) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1)) {

			}
		} else if (i == 2) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1)) {
				if (direction != DOWN)
					direction = UP;
			}
		} else if (i == 3) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1)) {
				if (direction != UP)
					direction = DOWN;
			}

		}
		HAL_Delay(10);
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1))
		;
	/* USER CODE END EXTI1_IRQn 1 */
}

/**
 * @brief This function handles EXTI line2 and Touch Sense controller.
 */
void EXTI2_TSC_IRQHandler(void) {
	/* USER CODE BEGIN EXTI2_TSC_IRQn 0 */

	/* USER CODE END EXTI2_TSC_IRQn 0 */
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
	/* USER CODE BEGIN EXTI2_TSC_IRQn 1 */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
	for (int i = 0; i < 4; i++) {
		if (i == 0) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)) {
				if (gameState == OPTIONS && currentOption != NONE) {
					show_options();
				}
			}
		} else if (i == 1) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)) {

			}
		} else if (i == 2) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)) {
			}
		} else if (i == 3) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)) {
				if (direction != LEFT)
					direction = RIGHT;
			}

		}
		HAL_Delay(10);
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2))
		;
	/* USER CODE END EXTI2_TSC_IRQn 1 */
}

/**
 * @brief This function handles EXTI line3 interrupt.
 */
void EXTI3_IRQHandler(void) {
	/* USER CODE BEGIN EXTI3_IRQn 0 */

	/* USER CODE END EXTI3_IRQn 0 */
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
	/* USER CODE BEGIN EXTI3_IRQn 1 */
	for (int i = 0; i < 4; i++) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
		if (i == 0) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3)) {
				if (gameState == OPTIONS && currentOption != NONE
						&& currentOption != ABOUT) {
					handle_menu_choice(1);
				} else {
					clear();
					gameState = MENU;
					menu();
				}
			}
		} else if (i == 1) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3)) {
				if (gameState == OPTIONS && currentOption != NONE
						&& currentOption != ABOUT) {
					handle_menu_choice(-1);
				} else {
					if (gameState == MENU) {
						show_options();
					} else if (gameState == OPTIONS) {
						handle_menu_choice(0);
					}
				}
			}
		} else if (i == 2) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3)) {
				if (!is_dead)
					game_paused = 1;
				if (gameState == OPTIONS)
					decrement_choice();
			}
		} else if (i == 3) {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3)) {
				if (gameState == OPTIONS)
					increment_choice();

			}

		}
		HAL_Delay(10);
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
		;
	/* USER CODE END EXTI3_IRQn 1 */
}

/**
 * @brief This function handles TIM1 update and TIM16 interrupts.
 */
void TIM1_UP_TIM16_IRQHandler(void) {
	/* USER CODE BEGIN TIM1_UP_TIM16_IRQn 0 */

	/* USER CODE END TIM1_UP_TIM16_IRQn 0 */
	HAL_TIM_IRQHandler(&htim1);
	/* USER CODE BEGIN TIM1_UP_TIM16_IRQn 1 */

	/* USER CODE END TIM1_UP_TIM16_IRQn 1 */
}

/**
 * @brief This function handles TIM3 global interrupt.
 */
void TIM3_IRQHandler(void) {
	/* USER CODE BEGIN TIM3_IRQn 0 */

	/* USER CODE END TIM3_IRQn 0 */
	HAL_TIM_IRQHandler(&htim3);
	/* USER CODE BEGIN TIM3_IRQn 1 */

	/* USER CODE END TIM3_IRQn 1 */
}

/**
 * @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
 */
void USART1_IRQHandler(void) {
	/* USER CODE BEGIN USART1_IRQn 0 */

	/* USER CODE END USART1_IRQn 0 */
	HAL_UART_IRQHandler(&huart1);
	/* USER CODE BEGIN USART1_IRQn 1 */

	/* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
