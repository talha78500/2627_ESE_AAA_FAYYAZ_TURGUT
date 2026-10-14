/*
 * motor.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "motor_control/motor.h"
#include "user_interface/shell.h"

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdlib.h>

/* Applique un CCR aux 4 transistors : U = ccr, V = PWM_PERIOD - ccr */
void motor_set_ccr(uint32_t ccr)
{
	if (ccr > PWM_CCR_MAX)
	{
		ccr = PWM_CCR_MAX;                      // saturation
	}
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_PERIOD - ccr);
}

void motor_init(void)
{
	shell_add(&hshell1, "SETCCR", sh_set_ccr, "SETCCR he<0..4250> : rapport cyclique");

	motor_set_ccr(PWM_DUTY_TEST);

	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)    Error_Handler();
	if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
}

int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv)   // <-- même signature que la fonction de led.c
{
	if (argc != 2)
	{
		return -1;                              // usage : SET_CCR XXXX
	}

	char * end;
	long value = strtol(argv[1], &end, 10);     // conversion chaine -> entier

	if (*end != '\0' || value < 0)
	{
		return -1;                              // pas un nombre valide
	}

	motor_set_ccr((uint32_t)value);             // saturation a PWM_CCR_MAX incluse
	return 0;
}
