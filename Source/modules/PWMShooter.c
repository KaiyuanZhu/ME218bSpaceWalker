#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BITDEFS.H"
#include "termio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ssi.h"
#include "ES_DeferRecall.h"
#include <math.h>
#include "PWMShooter.h"

#define ALL_BITS (0xff<<2)
#define PWMTicksPerMS 1250 
#define PeriodInUs 20000 
#define BitsPerNibble 4

void InitPWMShooter(void);
void SetShooterSpeed(uint16_t DutyCycle);


void InitPWMShooter(void){

  HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R0;
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT6HI);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT6HI);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT6HI); 
	
	printf("\n\r PWMShooter Inited \n\r");
}

void SetShooterSpeed(uint16_t DutyCycle)
{
			if (DutyCycle == 0) {
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT6HI); 
		} else if (DutyCycle == 100) {
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT6HI); 

		}
}
