/****************************************************************************
 Module
    AlignBeacon.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/gpio.h"
#include "PWM8Tiva.h"
#include "termio.h"
#include "ES_Port.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "ES_PostList.h"
#include "HallSensor.h"
#include "MasterSM.h"

/* Next level*/

/*----------------------------- Module Defines ----------------------------*/
#define PeriodInMS 1
#define BitsPerNibble 4
#define ALL_BITS (0xff<<2)  
#define aveNum 10
#define PWMTicksPerMS 40000
#define PeriodInMS 1
#define ONE_SEC  976
#define LEFT 1
#define RIGHT 2
#define MID 3
#define Period_Toler 5
/*---------------------------- Module Functions ---------------------------*/

void InitHallCapture(void);
void HallCaptureLeftResponse(void);
void HallCaptureRightResponse(void);
void HallCaptureMidResponse(void);


/*---------------------------- Module Variables ---------------------------*/


static uint32_t LastCaptureLeft;
static uint32_t LastCaptureRight;
static uint32_t LastCaptureMid;
static uint32_t PeriodL[aveNum];
static uint32_t PeriodR[aveNum];
static uint32_t PeriodM[aveNum];
static uint32_t avePeriodL;
static uint32_t avePeriodR;
static uint32_t avePeriodM;
static uint8_t counterL;
static uint8_t counterR;
static uint8_t counterM;



/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunAlignBeaconSM
****************************************************************************/
ES_Event RunHallSensor( ES_Event CurrentEvent )
{

   ES_Event ReturnEvent = CurrentEvent; // ????????????????? NO EVENT?
               ES_Event Event2Post;
	uint32_t period = 0;
   switch (CurrentEvent.EventType)
		 {
			 
       case HALL_DETECT :
            switch (CurrentEvent.EventParam)
            {
              case LEFT :
                for (uint8_t i=0; i<aveNum; i++){
                  avePeriodL += PeriodL[i];
                }
                avePeriodL = avePeriodL/aveNum;
                period = avePeriodL*1000/111*100/PWMTicksPerMS;
                printf("\r\n left measured period is %i", period);
								
                break;
              case RIGHT :
                for (uint8_t i=0; i<aveNum; i++){
                  avePeriodR += PeriodR[i];
                }
                avePeriodR = avePeriodR/aveNum;
                period = avePeriodR*1000/111*100/PWMTicksPerMS;
                printf("\r\n right measured period is %i", period);                
                break;

                case MID :
                for (uint8_t i=0; i<aveNum; i++){
                  avePeriodM += PeriodM[i];
                }
                avePeriodM = avePeriodM/aveNum;
                period = avePeriodM*1000/111*100/PWMTicksPerMS;
                printf("\r\n middle measured period is %i", period);              
                break;
            }

            if(period>(500-Period_Toler) && period<(500+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0f;
              PostMasterSM(Event2Post);
            }else if(period>(556-Period_Toler) && period<(556+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0e;
              PostMasterSM(Event2Post);
            }else if(period>(611-Period_Toler) && period<(611+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0d;
              PostMasterSM(Event2Post);
            }else if(period>(667-Period_Toler) && period<(667+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0c;
              PostMasterSM(Event2Post);
            }else if(period>(722-Period_Toler) && period<(722+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0b;
              PostMasterSM(Event2Post);
            }else if(period>(778-Period_Toler) && period<(778+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x0a;
              PostMasterSM(Event2Post);
            }else if(period>(833-Period_Toler) && period<(833+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x09;
              PostMasterSM(Event2Post); 
            }else if(period>(889-Period_Toler) && period<(889+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x08;
              PostMasterSM(Event2Post); 
             }else if(period>(944-Period_Toler) && period<(944+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x07;
              PostMasterSM(Event2Post);                                                         
             }else if(period>(1000-Period_Toler) && period<(1000+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x06;
              PostMasterSM(Event2Post);
             }else if(period>(1055-Period_Toler) && period<(1055+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x05;
              PostMasterSM(Event2Post);                                                         
             }else if(period>(1111-Period_Toler) && period<(1111+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x04;
              PostMasterSM(Event2Post);  
             }else if(period>(1166-Period_Toler) && period<(1166+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x03;
              PostMasterSM(Event2Post);  
             }else if(period>(1222-Period_Toler) && period<(1222+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x02;
              PostMasterSM(Event2Post);                                                                                                   
             }else if(period>(1277-Period_Toler) && period<(1277+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x01;
              PostMasterSM(Event2Post);
             }else if(period>(1333-Period_Toler) && period<(1333+Period_Toler) ){
              Event2Post.EventType = HallSensorRead;
              Event2Post.EventParam = 0x00;
              PostMasterSM(Event2Post);  
              }else{
                printf("\r\nfrequency not match");
              }                                                   
           ReturnEvent.EventType = ES_NO_EVENT;
       break;
						}

    return(ReturnEvent);
}
/****************************************************************************
 Function
     StartRunAlignBeaconSM
****************************************************************************/
void StartHallSensor ( ES_Event CurrentEvent )
{
  InitHallCapture();
  for (uint8_t i=0; i<aveNum; i++){
    PeriodL[i]=0;
    PeriodR[i]=0;
    PeriodM[i]=0;  
  }
  avePeriodL=0;
  avePeriodR=0;
  avePeriodM=0;
  counterL=0;
  counterR=0;
  counterM=0;
   RunHallSensor(CurrentEvent);
	printf("\r\n start hall sensor");
}



/***************************************************************************
 private functions
 ***************************************************************************/

//No during fcn, lowest level of the Hierarchy

/***************************************************************************
 Hardware Init/Response functions
 ***************************************************************************/

void InitHallCapture(void){
    //Enable the clock to the Wide timer 2 Wide timer 3
    HWREG(SYSCTL_RCGCWTIMER) |= (SYSCTL_RCGCWTIMER_R2 | SYSCTL_RCGCWTIMER_R3);
    //Enable the clock to Port D
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
    //make sure Timer A is disabled before configuring
    HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~(TIMER_CTL_TAEN|TIMER_CTL_TBEN);
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~(TIMER_CTL_TAEN);
    //Set the timer to 32bit wide individual mode
    HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    //Initialize the timer A to full 32 bit count
    HWREG(WTIMER2_BASE+TIMER_O_TAILR) = 0xffffffff;
    HWREG(WTIMER2_BASE+TIMER_O_TBILR) = 0xffffffff;
    HWREG(WTIMER3_BASE+TIMER_O_TAILR) = 0xffffffff;
    //Set the timer A to capture mode (TAMR = 3, TAAMS = 0), edge time (TACMR = 1), up-counting (TACDIR = 1)
    HWREG(WTIMER2_BASE+TIMER_O_TAMR) = (HWREG(WTIMER2_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
    HWREG(WTIMER2_BASE+TIMER_O_TBMR) = (HWREG(WTIMER2_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) | (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);
    HWREG(WTIMER3_BASE+TIMER_O_TAMR) = (HWREG(WTIMER3_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
    //Set the event to rising edge, modify the TAEVENT bits in GPTMCTL. Rising edge = 00
    HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~(TIMER_CTL_TAEVENT_M|TIMER_CTL_TBEVENT_M);
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
    //Set the port to capture, start by setting the alternate function for port C bit 4 (WT0CCP0)
    HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= (BIT0HI|BIT1HI|BIT2HI);
    //Map bit 4 alternate function to WT2CCP0 WT2CCP1 WT3CCP0
    HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffff000)+(7)+(7<<4)+(7<<8);
    //Enable PC4 for digital input
    HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= (BIT0HI|BIT1HI|BIT2HI);
    HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= (BIT0LO|BIT1LO|BIT2LO);
    //Enable a local capture interrupt
    HWREG(WTIMER2_BASE+TIMER_O_IMR) |= (TIMER_IMR_CAEIM|TIMER_IMR_CBEIM);
    HWREG(WTIMER3_BASE+TIMER_O_IMR) |= (TIMER_IMR_CAEIM);
    //Enable the WT2CCP0 98 WT2CCP1 99 WT2CCP0 100
    HWREG(NVIC_EN3) |= (BIT2HI|BIT3HI|BIT4HI);
    // make sure interrupts are enabled globally
    __enable_irq();
    //Start the timer and enable the timer to stall
    HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL |TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
    HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
		printf("\r\n finish init hall sensor module");
    
}

void HallCaptureLeftResponse(void){
    HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
    //Clear the source of the interrupt
    //Grab the capture value and calculate for the period
    uint32_t  ThisCapture;
    ThisCapture = HWREG(WTIMER2_BASE+TIMER_O_TAR);
    PeriodL[counterL]=ThisCapture - LastCaptureLeft;
    counterL++;
    // update LastCapture to prepare for the next edge
    if(counterL >= aveNum){
      counterL = 0;
    ES_Event Event2Post;
    Event2Post.EventType = HALL_DETECT;
    Event2Post.EventParam = LEFT;
    PostMasterSM(Event2Post);
  }
      LastCaptureLeft = ThisCapture;


}

void HallCaptureMidResponse(void){
    HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
    //Clear the source of the interrupt
    //Grab the capture value and calculate for the period
    uint32_t  ThisCapture;
    ThisCapture = HWREG(WTIMER2_BASE+TIMER_O_TBR);
    PeriodM[counterM]=ThisCapture-LastCaptureMid;
    counterM++;
    // update LastCapture to prepare for the next edge
    if(counterM >= aveNum){
    counterM = 0;
    ES_Event Event2Post;
    Event2Post.EventType = HALL_DETECT;
    Event2Post.EventParam = MID;
    PostMasterSM(Event2Post);
    }
        LastCaptureMid = ThisCapture;


}

void HallCaptureRightResponse(void){
    HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
    //Clear the source of the interrupt
    //Grab the capture value and calculate for the period
    uint32_t  ThisCapture;
    ThisCapture = HWREG(WTIMER3_BASE+TIMER_O_TAR);
    PeriodR[counterR]=ThisCapture - LastCaptureRight;
    counterR++;
    // update LastCapture to prepare for the next edge
    if(counterR >= aveNum){
      counterR = 0;
    ES_Event Event2Post;
    Event2Post.EventType = HALL_DETECT;
    Event2Post.EventParam = RIGHT;
    PostMasterSM(Event2Post);
  }
      LastCaptureRight = ThisCapture;


}



