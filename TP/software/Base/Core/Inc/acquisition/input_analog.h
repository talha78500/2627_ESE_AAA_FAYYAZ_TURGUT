/*
 * input_analog.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_INPUT_ANALOG_H_
#define INC_INPUT_ANALOG_H_

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "user_interface/shell.h"

/* Voies ADC1 des capteurs de courant (schéma KiCad) */
#define ADC_CHANNEL_I_U         ADC_CHANNEL_2   // PA1 : courant bras U (= courant moteur)
#define ADC_CHANNEL_I_V         ADC_CHANNEL_12  // PB1 : courant bras V

/* Conversion ADC -> tension */
#define ADC_VREF_MV             3300            // tension de référence de l'ADC
#define ADC_FULL_SCALE          4096            // 12 bits

/* Capteur LEM GO 10-SME/SP3 : I = (V - V0) / G */
#define CURRENT_V0_MV           1650            // sortie à 0 A (recalibrée par la commande izero)
#define CURRENT_SENSITIVITY_MV_PER_A  50        // datasheet GO-SME/SP3 p.4

#define ADC_POLL_TIMEOUT_MS     10
#define ADC_NB_SAMPLES          64              // nombre de mesures moyennées

void input_analog_init(void);
HAL_StatusTypeDef input_analog_read_raw(uint32_t channel, uint32_t * raw);
int32_t input_analog_raw_to_ma(uint32_t raw, int32_t v0_mv);

int sh_current(h_shell_t * h_shell, int argc, char ** argv);
int sh_current_zero(h_shell_t * h_shell, int argc, char ** argv);

#endif /* INC_INPUT_ANALOG_H_ */
