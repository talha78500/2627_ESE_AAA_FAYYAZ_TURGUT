/*
 * motor.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_MOTOR_CONTROL_MOTOR_H_
#define INC_MOTOR_CONTROL_MOTOR_H_



#include <stdint.h>
#include "user_interface/shell.h"

#define PWM_PERIOD      4250                    // ARR + 1 (ARR = 4249, 20 kHz center-aligned)
#define PWM_CCR_MAX     PWM_PERIOD              // alpha = 100 %
#define PWM_CCR_STOP    (PWM_PERIOD / 2)        // alpha = 50 % -> tension moyenne nulle
#define PWM_DUTY_TEST   2550                    // 60 %

void motor_init(void);
void motor_set_ccr(uint32_t ccr);


int sh_set_ccr(h_shell_t * h_shell, int argc, char ** argv);

#endif /* INC_MOTOR_CONTROL_MOTOR_H_ */
