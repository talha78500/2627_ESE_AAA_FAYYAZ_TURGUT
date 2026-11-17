/*
 * analog_input.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "acquisition/input_analog.h"
#include "adc.h"
#include "main.h"

static uint16_t adc_dma_buffer[ADC_NB_CHANNELS];    // rempli par le DMA à chaque déclenchement TIM1

static uint32_t sum_u = 0, sum_v = 0;                // sommes pour la moyenne
static uint32_t nb_samples = 0;
static volatile uint32_t avg_u_raw = 0;              // dernières moyennes (valeurs ADC)
static volatile uint32_t avg_v_raw = 0;
static volatile uint32_t nb_averages = 0;            // nombre de moyennes disponibles

static int32_t v0_u_mv = CURRENT_V0_MV;              // tension à 0 A du capteur U
static int32_t v0_v_mv = CURRENT_V0_MV;              // tension à 0 A du capteur V

void input_analog_init(void)
{
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler();
	}
	/* L'ADC attend maintenant les déclenchements de TIM1 (TRGO) :
	 * les mesures démarrent dès que les PWM tournent (commande start) */
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_NB_CHANNELS) != HAL_OK)
	{
		Error_Handler();
	}

	shell_add(&hshell1, "current", sh_current, "current : courants U et V");
	shell_add(&hshell1, "izero", sh_current_zero, "izero : zero du courant a 0 A");
}

/* Appelée par le DMA quand les 2 conversions sont terminées (1 fois par période PWM) */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef * hadc)
{
	if (hadc->Instance != ADC1)
	{
		return;
	}
	HAL_GPIO_WritePin(ADC_SYNC_GPIO_Port, ADC_SYNC_Pin, GPIO_PIN_SET);     // impulsion de vérification

	sum_u += adc_dma_buffer[ADC_RANK_I_U];
	sum_v += adc_dma_buffer[ADC_RANK_I_V];
	nb_samples++;

	if (nb_samples >= ADC_NB_SAMPLES)
	{
		avg_u_raw = sum_u / ADC_NB_SAMPLES;
		avg_v_raw = sum_v / ADC_NB_SAMPLES;
		nb_averages++;
		sum_u = 0;
		sum_v = 0;
		nb_samples = 0;
	}

	HAL_GPIO_WritePin(ADC_SYNC_GPIO_Port, ADC_SYNC_Pin, GPIO_PIN_RESET);
}

/* Valeur ADC -> tension (mV) */
static int32_t input_analog_raw_to_mv(uint32_t raw)
{
	return (int32_t)(raw * ADC_VREF_MV / ADC_FULL_SCALE);
}

/* Valeur ADC -> courant (mA) : I = (V - V0) / G */
int32_t input_analog_raw_to_ma(uint32_t raw, int32_t v0_mv)
{
	return (input_analog_raw_to_mv(raw) - v0_mv) * 1000 / CURRENT_SENSITIVITY_MV_PER_A;
}

/* Vérifie qu'au moins une moyenne a été calculée (PWM démarrées) */
static int input_analog_ready(h_shell_t * h_shell)
{
	if (nb_averages == 0)
	{
		int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Pas de mesure : taper start\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return 0;
	}
	return 1;
}

/* Commande shell : current -> affiche les courants des bras U et V */
int sh_current(h_shell_t * h_shell, int argc, char ** argv)
{
	uint32_t raw_u = avg_u_raw;
	uint32_t raw_v = avg_v_raw;
	int size;

	if (!input_analog_ready(h_shell))
	{
		return -1;
	}
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "I_U : ADC = %lu, %ld mV, %ld mA\r\n",
			(unsigned long)raw_u, (long)input_analog_raw_to_mv(raw_u), (long)input_analog_raw_to_ma(raw_u, v0_u_mv));
	h_shell->drv.transmit(h_shell->print_buffer, size);
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "I_V : ADC = %lu, %ld mV, %ld mA\r\n",
			(unsigned long)raw_v, (long)input_analog_raw_to_mv(raw_v), (long)input_analog_raw_to_ma(raw_v, v0_v_mv));
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}

/* Commande shell : izero -> enregistre V0 des 2 capteurs (moteur à l'arrêt, I = 0) */
int sh_current_zero(h_shell_t * h_shell, int argc, char ** argv)
{
	if (!input_analog_ready(h_shell))
	{
		return -1;
	}
	v0_u_mv = input_analog_raw_to_mv(avg_u_raw);
	v0_v_mv = input_analog_raw_to_mv(avg_v_raw);
	int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "V0_U = %ld mV, V0_V = %ld mV\r\n", (long)v0_u_mv, (long)v0_v_mv);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
