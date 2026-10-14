/*
 * motor.h
 *
 *  Created on: Nov 11, 2025
 *      Author: nicolas
 */

#ifndef INC_MOTOR_CONTROL_MOTOR_H_
#define INC_MOTOR_CONTROL_MOTOR_H_



#define PWM_PERIOD     4250   // ARR + 1
#define PWM_DUTY_TEST  2550   // 60 % de PWM_PERIOD


void motor_init(void);



#endif /* INC_MOTOR_CONTROL_MOTOR_H_ */
