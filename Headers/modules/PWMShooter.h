/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PWMSHOOTER_H
#define PWMSHOOTER_H


#include "ES_Configure.h" 
#include "ES_Types.h"     


void InitPWMShooter(void);
void SetShooterSpeed(uint16_t deg);


#endif /* PWMSHOOTER_H */