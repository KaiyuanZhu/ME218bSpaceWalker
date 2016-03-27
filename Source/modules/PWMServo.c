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
#include "SendByte.h"
#include "MasterSM.h"
#include "PACSM.h"
#include "TOPPAC.h"
#include "ES_DeferRecall.h"
#include <math.h>
#include "PWMServo.h"
#include "MasterSM.h"


#define ALL_BITS (0xff<<2)
#define PWMTicksPerMS 1250 
#define PeriodInUs 20000 
#define BitsPerNibble 4

#define DEGZERO (2625) 
#define DEGONEEGIHTY (1125) 


void InitPWMServo(void);
void SetServoPosition(uint16_t deg);
static uint16_t deg2ticks(uint16_t deg);

void InitPWMServo() // Init PB4 as PWM Servo
	
{
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1; // Enables Port B
while((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
;
// Enable Servo Pin
HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT4HI); // PB4
HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT4HI); // PB4
HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT4HI);
volatile uint32_t Dummy;
	// start by enabling the clock to the PWM Module (PWM0)
HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
// enable the clock to Port B
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
// Select the PWM clock as System Clock/32
HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
// make sure that the PWM module clock has gotten going
while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
;

// disable the PWM while initializing
HWREG( PWM0_BASE+PWM_O_2_CTL ) = 0;
// program generator A to go to 1 at rising compare A, 0 on falling compare A
	HWREG(PWM0_BASE + PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO); 
// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period
HWREG(PWM0_BASE + PWM_O_1_LOAD) = (((PeriodInUs*PWMTicksPerMS)/1000)-1)>>1;

// Set the initial Duty cycle on A to 50% by programming the compare value
// to 1/2 the period to count up (or down)
HWREG(PWM0_BASE + PWM_O_1_CMPA) = (HWREG(PWM0_BASE+PWM_O_1_LOAD)>>1); 
HWREG( PWM0_BASE+PWM_O_2_CMPB) = HWREG( PWM0_BASE+PWM_O_2_LOAD);
// enable the PWM output
HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM2EN); // tricky! this is the PWM line num PWM0-7 per module
HWREG(PWM0_BASE + PWM_O_ENUPD) = (HWREG( PWM0_BASE+PWM_O_ENUPD) & ~(PWM_ENUPD_ENUPD2_M)) | (PWM_ENUPD_ENUPD2_LSYNC);
// Select alternate function for PB4
	HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= (BIT4HI);

// now configure the Port B pins to be PWM outputs
// start by selecting the alternate function for Pe4 & 5
HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT4HI);
// now choose to map PWM to those pins, this is a mux value of 4 that we
// want to use for specifying the function on bits 4 & 5
HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) & 0xfff0ffff) + (4<<(4*BitsPerNibble)); 
HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= BIT4HI; 
HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= BIT4HI; 
// set the up/down count mode and enable the PWM generator, both generator updates locally synchronized to zero count
	HWREG(PWM0_BASE + PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE | PWM_1_CTL_GENAUPD_LS);
}


void SetServoPosition (uint16_t deg) {
	uint16_t ticks = deg2ticks(deg);
	HWREG(PWM0_BASE + PWM_O_1_GENA) =	(PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO); 
	HWREG(PWM0_BASE+PWM_O_1_CMPA) = HWREG(PWM0_BASE+PWM_O_1_LOAD) - (((ticks>>1)));	// Sets Pulse Width	accordingly
}


static uint16_t deg2ticks(uint16_t deg) {
	uint16_t tick;
	tick = (int)round(deg*(((float)DEGONEEGIHTY-(float)DEGZERO)/(float)180) + DEGZERO);
	return tick;
}



