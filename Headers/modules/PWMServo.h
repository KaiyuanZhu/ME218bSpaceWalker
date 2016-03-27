/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PWMSERVO_H
#define PWMSERVO_H


#include "ES_Configure.h" 
#include "ES_Types.h"     


void InitPWMServo(void);
void SetServoPosition(uint16_t deg);


#endif /* PWMSERVO_H */