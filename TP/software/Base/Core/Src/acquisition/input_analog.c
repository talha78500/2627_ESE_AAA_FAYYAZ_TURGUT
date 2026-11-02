/*
 * analog_input.c
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#include "acquisition/input_analog.h"
#include "adc.h"

static int32_t v0_u_mv = CURRENT_V0_MV;   // tension à 0 A du capteur U
static int32_t v0_v_mv = CURRENT_V0_MV;   // tension à 0 A du capteur V

void input_analog_init(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);   // calibration interne de l'ADC

	shell_add(&hshell1, "current", sh_current, "current : courants U et V");
	shell_add(&hshell1, "izero", sh_current_zero, "izero : zero du courant a 0 A");
}

/* Mesure en polling : choisit la voie, lance une conversion et attend le résultat */
HAL_StatusTypeDef input_analog_read_raw(uint32_t channel, uint32_t * raw)
{
	ADC_ChannelConfTypeDef config = {0};

	config.Channel = channel;
	config.Rank = ADC_REGULAR_RANK_1;
	config.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
	config.SingleDiff = ADC_SINGLE_ENDED;
	config.OffsetNumber = ADC_OFFSET_NONE;

	if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK) return HAL_ERROR;
	if (HAL_ADC_Start(&hadc1) != HAL_OK) return HAL_ERROR;
	if (HAL_ADC_PollForConversion(&hadc1, ADC_POLL_TIMEOUT_MS) != HAL_OK)
	{
		HAL_ADC_Stop(&hadc1);
		return HAL_ERROR;
	}
	*raw = HAL_ADC_GetValue(&hadc1);
	return HAL_ADC_Stop(&hadc1);
}

/* Moyenne de ADC_NB_SAMPLES mesures pour réduire le bruit */
static HAL_StatusTypeDef input_analog_read_avg(uint32_t channel, uint32_t * raw)
{
	uint32_t sum = 0, sample;

	for (int i = 0; i < ADC_NB_SAMPLES; i++)
	{
		if (input_analog_read_raw(channel, &sample) != HAL_OK) return HAL_ERROR;
		sum += sample;
	}
	*raw = sum / ADC_NB_SAMPLES;
	return HAL_OK;
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

/* Commande shell : current -> affiche les courants des bras U et V */
int sh_current(h_shell_t * h_shell, int argc, char ** argv)
{
	uint32_t raw_u, raw_v;
	int size;

	if (input_analog_read_avg(ADC_CHANNEL_I_U, &raw_u) != HAL_OK
			|| input_analog_read_avg(ADC_CHANNEL_I_V, &raw_v) != HAL_OK)
	{
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Erreur ADC\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, size);
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

/* Commande shell : izero -> mesure V0 des 2 capteurs (à faire moteur à l'arrêt, I = 0) */
int sh_current_zero(h_shell_t * h_shell, int argc, char ** argv)
{
	uint32_t raw_u, raw_v;
	int size;

	if (input_analog_read_avg(ADC_CHANNEL_I_U, &raw_u) != HAL_OK
			|| input_analog_read_avg(ADC_CHANNEL_I_V, &raw_v) != HAL_OK)
	{
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Erreur ADC\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, size);
		return -1;
	}
	v0_u_mv = input_analog_raw_to_mv(raw_u);
	v0_v_mv = input_analog_raw_to_mv(raw_v);
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "V0_U = %ld mV, V0_V = %ld mV\r\n", (long)v0_u_mv, (long)v0_v_mv);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return 0;
}
