/*
 * motor.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "motor_control/motor.h"
#include "user_interface/shell.h"

#include "main.h"
#include "tim.h"
#include <stdlib.h>

static uint32_t ccr_current = PWM_CCR_STOP;              // CCR appliqué actuellement
static volatile uint32_t ccr_target = PWM_CCR_STOP;      // CCR visé par la rampe (modifié par le shell)
static volatile uint8_t motor_running = 0;               // 1 si les PWM sont actives

/* Écrit un CCR dans les registres : U = ccr, V = PWM_PERIOD - ccr */
static void motor_apply_ccr(uint32_t ccr)
{
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_PERIOD - ccr);
	ccr_current = ccr;
}

/* Envoie un message sur le shell */
static void motor_print(h_shell_t * h_shell, const char * msg)
{
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%s\r\n", msg);
	h_shell->drv.transmit(h_shell->print_buffer, size);
}

/* Lit un CCR dans argv[1]. Renvoie -1 si absent, non numérique ou hors plage */
static int motor_parse_ccr(int argc, char ** argv, uint32_t * ccr)
{
	if (argc != 2)
	{
		return -1;
	}
	char * end;
	long value = strtol(argv[1], &end, 10);
	if (*end != '\0' || value < PWM_CCR_MIN || value > PWM_CCR_MAX)
	{
		return -1;
	}
	*ccr = (uint32_t)value;
	return 0;
}

/* Applique un CCR immédiatement (sans rampe). Valeur hors plage refusée. */
HAL_StatusTypeDef motor_set_ccr(uint32_t ccr)
{
	if (ccr > PWM_CCR_MAX)
	{
		return HAL_ERROR;                       // hors plage : on ne touche à rien
	}
	motor_apply_ccr(ccr);
	ccr_target = ccr;                           // la rampe ne doit pas revenir à l'ancienne cible
	return HAL_OK;
}

void motor_init(void)
{
	shell_add(&hshell1, "SETCCR", sh_set_ccr, "SETCCR <0..4250> : CCR direct");
	shell_add(&hshell1, "start", sh_start, "start : PWM a 50 %");
	shell_add(&hshell1, "stop", sh_stop, "stop : arret des PWM");
	shell_add(&hshell1, "speed", sh_speed, "speed <0..4250> : rampe");

	motor_set_ccr(PWM_CCR_STOP);                // PWM arrêtées au démarrage : il faut taper start
}

/* À appeler dans loop() : rapproche le CCR de la cible d'un pas toutes les RAMP_PERIOD_MS */
void motor_ramp_update(void)
{
	static uint32_t last_tick = 0;
	uint32_t target = ccr_target;

	if (!motor_running || ccr_current == target)
	{
		return;
	}
	if (HAL_GetTick() - last_tick < RAMP_PERIOD_MS)
	{
		return;
	}
	last_tick = HAL_GetTick();

	if (ccr_current < target)
	{
		motor_apply_ccr((target - ccr_current > RAMP_STEP) ? ccr_current + RAMP_STEP : target);
	}
	else
	{
		motor_apply_ccr((ccr_current - target > RAMP_STEP) ? ccr_current - RAMP_STEP : target);
	}
}

int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv)
{
	uint32_t ccr;

	if (motor_parse_ccr(argc, argv, &ccr) != 0)
	{
		motor_print(h_shell, "Erreur : SETCCR <0..4250>, rien n'est modifie");
		return -1;
	}
	motor_set_ccr(ccr);
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "CCR = %lu\r\n", (unsigned long)ccr);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}

int sh_start(h_shell_t * h_shell, int argc, char ** argv)
{
	motor_set_ccr(PWM_CCR_STOP);                // 50 % : vitesse nulle

	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK
			|| HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK
			|| HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK
			|| HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
	{
		motor_print(h_shell, "Erreur : demarrage des PWM");
		return -1;
	}
	motor_running = 1;
	motor_print(h_shell, "PWM demarrees, CCR = 2125 (50 %)");
	return 0;
}

int sh_stop(h_shell_t * h_shell, int argc, char ** argv)
{
	motor_running = 0;
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
	motor_set_ccr(PWM_CCR_STOP);                // prochain start repart de 50 %
	motor_print(h_shell, "PWM arretees");
	return 0;
}

int sh_speed(h_shell_t * h_shell, int argc, char ** argv)
{
	uint32_t ccr;

	if (!motor_running)
	{
		motor_print(h_shell, "Erreur : moteur arrete, taper start");
		return -1;
	}
	if (motor_parse_ccr(argc, argv, &ccr) != 0)
	{
		motor_print(h_shell, "Erreur : speed <0..4250>, rien n'est modifie");
		return -1;
	}
	ccr_target = ccr;                           // la rampe fait le reste dans loop()
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Rampe vers CCR = %lu\r\n", (unsigned long)ccr);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
