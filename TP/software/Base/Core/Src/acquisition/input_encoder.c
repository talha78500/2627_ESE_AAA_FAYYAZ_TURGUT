/*
 * input_encoder.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "acquisition/input_encoder.h"
#include "tim.h"
#include "main.h"

static uint16_t last_count = 0;                 // valeur du compteur à la mesure précédente
static volatile int32_t speed_rpm = 0;          // dernière vitesse calculée (tr/min)

void input_encoder_init(void)
{
	if (HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK)
	{
		Error_Handler();
	}
	last_count = __HAL_TIM_GET_COUNTER(&htim3);

	if (HAL_TIM_Base_Start_IT(&htim7) != HAL_OK)     // interruption toutes les SPEED_PERIOD_MS
	{
		Error_Handler();
	}

	shell_add(&hshell1, "enc", sh_enc, "enc : compteur du codeur");
	shell_add(&hshell1, "rpm", sh_rpm, "rpm : vitesse en tr/min");
}

/* Appelée toutes les SPEED_PERIOD_MS : vitesse = nombre de pas / temps */
void input_encoder_update(void)
{
	uint16_t count = __HAL_TIM_GET_COUNTER(&htim3);
	int16_t delta = (int16_t)(count - last_count);  // le cast gère le passage 65535 -> 0
	last_count = count;

	speed_rpm = (int32_t)delta * MS_PER_MIN / (ENCODER_COUNTS_PER_REV * SPEED_PERIOD_MS);
}

int32_t input_encoder_get_rpm(void)
{
	return speed_rpm;
}

/* Commande shell : enc -> valeur brute du compteur (pour vérifier le nombre de pas par tour) */
int sh_enc(h_shell_t * h_shell, int argc, char ** argv)
{
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Compteur = %lu\r\n",
			(unsigned long)__HAL_TIM_GET_COUNTER(&htim3));
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}

/* Commande shell : rpm -> vitesse en tr/min */
int sh_rpm(h_shell_t * h_shell, int argc, char ** argv)
{
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Vitesse = %ld tr/min\r\n",
			(long)input_encoder_get_rpm());
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
