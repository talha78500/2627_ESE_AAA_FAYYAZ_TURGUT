/*
 * motor.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "motor_control/motor.h"

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"




void motor_init(void){

	/* Rapports cycliques : U = alpha, V = 1 - alpha (complementaire decalee) */
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_DUTY_TEST);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_PERIOD - PWM_DUTY_TEST);

	/* Bras U : CH1 (haut) + CH1N (bas) */
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

	/* Bras V : CH2 (haut) + CH2N (bas) */
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
}
