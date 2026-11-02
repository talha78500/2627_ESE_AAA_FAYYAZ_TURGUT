/*
 * motor.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_MOTOR_CONTROL_MOTOR_H_
#define INC_MOTOR_CONTROL_MOTOR_H_



#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "user_interface/shell.h"

#define PWM_PERIOD      4250                    // ARR + 1 (ARR = 4249, 20 kHz center-aligned)
#define PWM_CCR_MAX     PWM_PERIOD              // alpha = 100 %
#define PWM_CCR_MIN     0
#define PWM_CCR_STOP    (PWM_PERIOD / 2)        // alpha = 50 % -> tension moyenne nulle
#define PWM_DUTY_TEST   2550                    // 60 %

/* Rampe (7.1) : +/- RAMP_STEP sur le CCR toutes les RAMP_PERIOD_MS
 * 50 % -> 70 % (850 pas de CCR) : 850 / 5 x 10 ms = 1,7 s */
#define RAMP_STEP           5                   // pas de CCR
#define RAMP_PERIOD_MS      10                  // ms entre deux pas

void motor_init(void);
HAL_StatusTypeDef motor_set_ccr(uint32_t ccr);
void motor_ramp_update(void);


int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv);
int sh_start(h_shell_t * h_shell, int argc, char ** argv);
int sh_stop(h_shell_t * h_shell, int argc, char ** argv);
int sh_speed(h_shell_t * h_shell, int argc, char ** argv);

#endif /* INC_MOTOR_CONTROL_MOTOR_H_ */
