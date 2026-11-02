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
HAL_StatusTypeDef motor_set_ccr(uint32_t ccr)
{
	if (ccr > PWM_CCR_MAX)
	{
		return HAL_ERROR;                       // hors plage : on ne touche à rien
	}
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_PERIOD - ccr);
	return HAL_OK;
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

int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv)
{
	int size;

	if (argc != 2)
	{
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Usage : SETCCR <%d..%d>\r\n", PWM_CCR_MIN, PWM_CCR_MAX);
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return -1;
	}

	char * end;
	long value = strtol(argv[1], &end, 10);     // conversion chaine -> entier

	if (*end != '\0' || value < PWM_CCR_MIN || value > PWM_CCR_MAX)
	{
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Erreur : valeur hors plage <%d..%d>, rien n'est modifie\r\n", PWM_CCR_MIN, PWM_CCR_MAX);
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return -1;
	}

	motor_set_ccr((uint32_t)value);
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "CCR = %ld\r\n", value);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
