/*
 * input_encoder.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_ACQUISITION_INPUT_ENCODER_H_
#define INC_ACQUISITION_INPUT_ENCODER_H_

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "user_interface/shell.h"

/* Codeur incrémental : ENC_A -> PA6 (TIM3_CH1), ENC_B -> PA4 (TIM3_CH2), ENC_Z -> PC8
 * TIM3 en mode Encoder TI1 et TI2 : comptage x4 (fronts montants et descendants de A et B) */
#define ENCODER_PPR             1024            // impulsions par tour (voie A) : A VERIFIER (commande enc)
#define ENCODER_COUNTS_PER_REV  (4 * ENCODER_PPR)

/* Période de calcul de la vitesse = période de TIM7 (10 ms -> 100 Hz) */
#define SPEED_PERIOD_MS         10
#define MS_PER_MIN              60000

void input_encoder_init(void);
void input_encoder_update(void);                // à appeler toutes les SPEED_PERIOD_MS (interruption TIM7)
int32_t input_encoder_get_rpm(void);

int sh_enc(h_shell_t * h_shell, int argc, char ** argv);
int sh_rpm(h_shell_t * h_shell, int argc, char ** argv);

#endif /* INC_ACQUISITION_INPUT_ENCODER_H_ */
