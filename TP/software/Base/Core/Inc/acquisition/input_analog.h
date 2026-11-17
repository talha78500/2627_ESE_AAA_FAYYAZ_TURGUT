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

/* Chaîne d'acquisition TIM1 (TRGO) -> ADC1 (2 rangs) -> DMA (circulaire)
 * Rang 1 : ADC1_IN2  (PA1) : courant bras U (= courant moteur)
 * Rang 2 : ADC1_IN12 (PB1) : courant bras V */
#define ADC_NB_CHANNELS         2
#define ADC_RANK_I_U            0               // index dans le buffer DMA
#define ADC_RANK_I_V            1

/* Conversion ADC -> tension */
#define ADC_VREF_MV             3300            // tension de référence de l'ADC
#define ADC_FULL_SCALE          4096            // 12 bits

/* Capteur LEM GO 10-SME/SP3 : I = (V - V0) / G */
#define CURRENT_V0_MV           1650            // sortie à 0 A (recalibrée par la commande izero)
#define CURRENT_SENSITIVITY_MV_PER_A  50        // datasheet GO-SME/SP3 p.4

#define ADC_NB_SAMPLES          64              // moyenne sur 64 périodes PWM (3,2 ms)

void input_analog_init(void);
int32_t input_analog_raw_to_ma(uint32_t raw, int32_t v0_mv);

int sh_current(h_shell_t * h_shell, int argc, char ** argv);
int sh_current_zero(h_shell_t * h_shell, int argc, char ** argv);

#endif /* INC_INPUT_ANALOG_H_ */
