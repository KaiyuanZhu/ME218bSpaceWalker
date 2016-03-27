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

#include "MasterSM.h"
#include "Lidar.h"
#include "AlignBeacon.h"
#include "MotorService.h"
#include "GotoShootingStationSM.h"

/* Next level*/

/*----------------------------- Module Defines ----------------------------*/
#define PeriodInMS 1
#define BitsPerNibble 4
#define ALL_BITS (0xff<<2)  
#define AlignPin GPIO_PIN_0
#define PI 3.14159265
#define IRTHRE
#define NW 2
#define NE 1
#define SE 4
#define SW 3
#define SEPeriod 32000
#define NEPeriod 23529
#define NWPeriod 27586
#define SWPeriod 20513
#define CountNumNE 5
#define CountNumNW 5
#define CountNumSE 5
#define CountNumSW 5
#define toler 300
#define tolerSE 400

/*---------------------------- Module Functions ---------------------------*/
//static ES_Event DuringAlign( ES_Event Event);
AlignBeaconState_t QueryAlignBeaconSM(void);
//static ES_Event DuringAligning( ES_Event Event);
void Init_IRCapture(void);
void Response_IRCapture(void);
float QueryX(void);
float QueryY(void);
float QueryAngle(void);


/*---------------------------- Module Variables ---------------------------*/
static AlignBeaconState_t CurrentState;
static uint8_t MyPriority;
static uint8_t numBeaconAligned;
static uint32_t ThisCapture;
static uint32_t Period;
static uint8_t NECount;
static uint8_t SECount;
static uint8_t SWCount;
static uint8_t NWCount;
static uint8_t DistanceCount = 0; // count up to three times and compare
//static bool TriggerLow;
static float Distance_1;
static float Distance_2;
static float Distance1 = 0;
static float Distance2 = 0;
static float Distance3 = 0;
//static uint32_t Distance[3];
static uint8_t lastBeacon;
static bool oneAlign;
static uint16_t firstBeacon;
static uint16_t secondBeacon;

static float Angle;
static float Xcoord;
static float Ycoord;

static void IRSelect(void);
/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     StartRunAlignBeaconSM
****************************************************************************/
void StartAlignBeaconSM ( ES_Event CurrentEvent )
{
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
		 //Normal mode
       CurrentState = Start;
       Init_IRCapture();
			 Init_LidarCapture();
		  // printf("\r\n Start AlignBeaconSM");
   }
   RunAlignBeaconSM(CurrentEvent);
}



/****************************************************************************
 Function
    RunAlignBeaconSM
****************************************************************************/
ES_Event RunAlignBeaconSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;
   AlignBeaconState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };
   ES_Event ReturnEvent = CurrentEvent; 
   
	 //Start - WaitForIR - AlignFirst - WaitForIR - AlignSecond*********************************************
	 
   switch ( CurrentState )
   {	 

			
       case Start :
				//	 printf("\r\n In AlignBeacon - Start state");

           if ( CurrentEvent.EventType == ES_ENTRY ) 
           {
						 		printf("\r\n align start");
               numBeaconAligned = 0;
						   oneAlign = false;
						 NECount = 0;
						 SECount = 0;
						 NWCount = 0;
						 SWCount = 0;
						 lastBeacon = 0;
						   ES_Event Event2Post;
						   Event2Post.EventType = ALIGNBEACON;
							 PostMasterSM(Event2Post);
//						   ES_Timer_InitTimer(IR_TIMER, 3);
						 
						   NextState = WaitForIR;
               MakeTransition = true;
               EntryEventKind.EventType = ES_ENTRY;
               ReturnEvent.EventType = ES_NO_EVENT;
               
           }
       break;
					 
			 case WaitForIR :
				
			  //printf("\r\n In AlignBeacon - WaitForIR state");
				if ( CurrentEvent.EventType == BEACON_ALIGNED ) 
				{				
					//switch (numBeaconAligned)
					if(oneAlign == false){	
							//case 0 :
							 NextState = AlignFirst;			
						   MakeTransition = true;
               EntryEventKind.EventType = ES_ENTRY;
               ReturnEvent.EventType = ES_NO_EVENT;
					//	printf("\r\n change to align first");
					}else{
							//case 1:
							  NextState = AlignSecond;			
						   MakeTransition = true;
               EntryEventKind.EventType = ES_ENTRY;
               ReturnEvent.EventType = ES_NO_EVENT;
					//	printf("\r\n change to align second");
			  }
			}
        break;
			 
       case AlignFirst:
				 
			  //printf("\r\n In AlignBeacon - AlignFirst state");
               if (CurrentEvent.EventType == BEACON_ALIGNED && numBeaconAligned < 1)
               {
                   numBeaconAligned++;
								   oneAlign = true;
                   //Stop();

									 firstBeacon = CurrentEvent.EventParam;

									 ES_Event Event2Post;
									 Event2Post.EventType = STOP;
									 printf("\r\n stop");
                   PostMasterSM(Event2Post);
									 ES_Timer_InitTimer(MOTOR_TIMER, 1100);
								 ReturnEvent.EventType = ES_NO_EVENT;

               }

							 if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == MOTOR_TIMER){
								 		ES_Event Event2Post;
									 Event2Post.EventType = GET_DISTANCE;
                   PostLidarService(Event2Post);
								// printf("\r\n send get distance request");
								 ReturnEvent.EventType = ES_NO_EVENT;
								 
							 }
               if(CurrentEvent.EventType == DISTANCE_FOUND)
               {
								 
								 if (DistanceCount == 0){
									 Distance1 = QueryDistance();
									 //printf("\r\n Distance get from Lidar %f", Distance1);
									 DistanceCount++;
									 ES_Event Event2Post;
									 Event2Post.EventType = GET_DISTANCE;
                   PostLidarService(Event2Post);
 								   ReturnEvent.EventType = ES_NO_EVENT;
								 } else if (DistanceCount == 1){
                   Distance2 = QueryDistance();
									// printf("\r\n Distance get from Lidar %f", Distance2);
									 DistanceCount++;
									 ES_Event Event2Post;
									 Event2Post.EventType = GET_DISTANCE;
                   PostLidarService(Event2Post);
									 ReturnEvent.EventType = ES_NO_EVENT;
								 } else { // DistanceCount == 2
									 Distance3 = QueryDistance();
									// printf("\r\n Distance get from Lidar %f", Distance3);
								 }
								 
								 if (Distance1 !=0 && Distance2 !=0 && Distance3 != 0)
								 {
									 float ratio1 = Distance1/Distance2;
									 float ratio2 = Distance2/Distance3;
									 if ( ratio1 > 0.8 || ratio1 < 1.2){
										 Distance_1 = (Distance1 + Distance2)/2;
									 } else if (ratio2 > 0.8 || ratio2 < 1.2){
										 Distance_1 = (Distance1 + Distance2)/2;
									 } else {
										 Distance_1 = (Distance1 + Distance3)/2;
									 }
									 Distance1 = 0; // reset for second align
									 Distance2 = 0;
									 Distance3 = 0;
									 DistanceCount = 0;
								//	 printf("\r\n Distance get from Lidar Local Data %f", Distance_1);
//								if(Distance_1 > 243.84){
//									printf("\r\n rotate a little and measure distance again");
//									MakeTransition = true;
//									NextState = Tuning;
//									EntryEventKind.EventType = ES_ENTRY;
//									ES_Event Event2Post;
//									Event2Post.EventType = ALIGNBEACON;
//									PostMasterSM(Event2Post);	
//									ES_Timer_InitTimer(MOTOR_TIMER, 50);
//									ReturnEvent.EventType = ES_NO_EVENT;
//									break;								 
//							 }
											 
						   NECount = 0;
						   SECount = 0;
						   NWCount = 0;
						   SWCount = 0;
						   ES_Event Event2Post;
						   Event2Post.EventType = ALIGNBEACON;
							 PostMasterSM(Event2Post);								 
//                 Angle_1 = HWREG(WTIMER0_BASE+TIMER_O_TAR);
								 //ES_Timer_InitTimer(LIDAR_TIMER, 500);
								NextState = WaitForIR;
								MakeTransition = true;
               EntryEventKind.EventType = ES_ENTRY;
								ReturnEvent.EventType = ES_NO_EVENT;
							 }
						 }

               break;
            
        case AlignSecond :
									
            if (CurrentEvent.EventType == BEACON_ALIGNED && numBeaconAligned < 2)
            {

							secondBeacon = CurrentEvent.EventParam;
							//	printf("\r\n secondBeacon %i firstBeacon %i", secondBeacon, firstBeacon);
								
							if(secondBeacon != firstBeacon){
					//				printf("\r\n align second IR");
									HWREG(WTIMER0_BASE+TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
									numBeaconAligned++;
									ES_Event Event2Post;
									Event2Post.EventType = STOP;
									PostMasterSM(Event2Post);
									ES_Timer_InitTimer(MOTOR_TIMER, 1100);
								}else{
									ES_Event Event2Post;
									Event2Post.EventType = ALIGNBEACON;
									PostMasterSM(Event2Post);
								}
//							else if(secondBeacon != firstBeacon){
//									printf("\r\n align third or fourth IR");
//									//set the firstBeacon as the secondBeacon and find the next beacon
//									firstBeacon = secondBeacon;	
//									ES_Event Event2Post;
//									Event2Post.EventType = ALIGNBEACON;
//									PostMasterSM(Event2Post);									
//								}
//								else{
//									ES_Event Event2Post;
//									Event2Post.EventType = ALIGNBEACON;
//									PostMasterSM(Event2Post);
//								}
								ReturnEvent.EventType = ES_NO_EVENT;
            }
						if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == MOTOR_TIMER){
									ES_Event Event2Post;
								 Event2Post.EventType = GET_DISTANCE;
                 PostLidarService(Event2Post);
							ReturnEvent.EventType = ES_NO_EVENT;
						}
            
					 if(CurrentEvent.EventType == DISTANCE_FOUND)
					 {
						 
						 if (DistanceCount == 0){
							 Distance1 = QueryDistance();
				//			 printf("\r\n Distance get from Lidar %f", Distance1);
							 DistanceCount++;
							 ES_Event Event2Post;
							 Event2Post.EventType = GET_DISTANCE;
							 PostLidarService(Event2Post);
							 ReturnEvent.EventType = ES_NO_EVENT;
						 } else if (DistanceCount == 1){
							 Distance2 = QueryDistance();
				//			 printf("\r\n Distance get from Lidar %f", Distance2);
							 DistanceCount++;
							 ES_Event Event2Post;
							 Event2Post.EventType = GET_DISTANCE;
							 PostLidarService(Event2Post);
							 ReturnEvent.EventType = ES_NO_EVENT;
						 } else { // DistanceCount == 2
							 Distance3 = QueryDistance();
					//		 printf("\r\n Distance get from Lidar %f", Distance3);
						 }
						 
						 if (Distance1 !=0 && Distance2 !=0 && Distance3 != 0)
						 {
							 float ratio1 = Distance1/Distance2;
							 float ratio2 = Distance2/Distance3;
							 if ( ratio1 > 0.8 || ratio1 < 1.2){
								 Distance_2 = (Distance1 + Distance2)/2;
							 } else if (ratio2 > 0.8 || ratio2 < 1.2){
								 Distance_2 = (Distance1 + Distance2)/2;
							 } else {
								 Distance_2 = (Distance1 + Distance3)/2;
							 }
							 Distance1 = 0; // reset for second align
							 Distance2 = 0;
							 Distance3 = 0;
							 DistanceCount = 0;
							 printf("\r\n Distance get from Lidar %f", Distance_2);
//			if((secondBeacon - firstBeacon != 1 && firstBeacon - secondBeacon != 3)||Distance_2 > 300){
//									printf("\r\n align third or fourth IR");
//									//set the firstBeacon as the secondBeacon and find the next beacon
//									firstBeacon = secondBeacon;
//									numBeaconAligned--;	
//									ES_Event Event2Post;
//									Event2Post.EventType = ALIGNBEACON;
//									PostMasterSM(Event2Post);	
//									Distance_1 = Distance_2;
//				break;
//}	
//							if(Distance_2 > 243.84){
//							  printf("\r\n rotate a little and measure distance again");
//								MakeTransition = true;
//								NextState = Tuning;
//								EntryEventKind.EventType = ES_ENTRY;
//								ES_Event Event2Post;
//								Event2Post.EventType = ALIGNBEACON;
//								PostMasterSM(Event2Post);	
//								ES_Timer_InitTimer(MOTOR_TIMER, 50);
//								 ReturnEvent.EventType = ES_NO_EVENT;
//								break;								 
//							 }
//                if(numBeaconAligned == 2)
                   float Xcoord_t;
                   float Ycoord_t;
                   //Xcoord_t = 121.92 + 121.92 * (Distance_2*Distance_2-Distance_1*Distance_1)/14864.48;
                   Xcoord_t = 243.84 - Distance_1 * ((Distance_1 * Distance_1)-(Distance_2 * Distance_2)+ 59457.95)/(487.68*Distance_1);
									 //Ycoord_t = sqrt(abs(Distance_1*Distance_1 -Xcoord_t*Xcoord_t));
									 Ycoord_t = 243.84 - sqrt(Distance_1*Distance_1 - (243.84-Xcoord_t)*(243.84-Xcoord_t));
									 printf("\r\n Distance_1 %f, Distance_2 %f, Xcoord_t %f, Ycoord_t %f",Distance_1, Distance_2, Xcoord_t, Ycoord_t);
									 if(Xcoord_t == NAN || Ycoord_t == NAN){
										 printf("\r\n not valid coord, recalibarte");
										 ES_Event Event2Post;
										 Event2Post.EventType = LOCATION_NOT_FOUND;
										 if(Distance_2 - Distance_1 > 60){
											 Event2Post.EventParam = 60;
										 }else if(Distance_1 - Distance_2 > 60){
											 Event2Post.EventParam = 120;
										 }else{
											 Event2Post.EventParam = 180;
										 }
//										 	MakeTransition = true;
//											NextState = Start;
//											EntryEventKind.EventType = ES_ENTRY;
										 PostMasterSM(Event2Post);
										 break;
									 }
                   switch (firstBeacon)
                   {
                       case 1:
                           Xcoord = Xcoord_t;
                           Ycoord = Ycoord_t;
                           Angle = (atan(Xcoord/(243.84-Ycoord)))*180.0/PI;
											     printf("\r\n case1 \r\n");
                           break;
                       case 2:
                           Xcoord = 243.84 - Ycoord_t;
                           Ycoord = Xcoord_t;
                           //Angle = atan(Xcoord/243.84-Ycoord)+90.0;//????
											     Angle = atan(Xcoord/Ycoord)*180.0/PI+90.0;
											     printf("\r\n case2 \r\n");
                           break;
                       case 3:
                           Xcoord = 243.84 - Xcoord_t;
                           Ycoord = 243.84 - Ycoord_t;
                           //Angle = atan(Xcoord/243.84-Ycoord)+180.0;///???
											     Angle = atan((243.84 - Xcoord)/Ycoord)*180.0/PI+180.0;
											     printf("\r\n case3 \r\n");
                           break;
                       case 4:
                           Xcoord = Ycoord_t;
                           Ycoord = 243.84 - Xcoord_t;
                           //Angle = atan(Xcoord/243.84-Ycoord)+270.0;
											     Angle = atan((243.84-Xcoord)/(243.84-Ycoord))*180.0/PI+270.0;
											     printf("\r\n case4 \r\n");
                           break;
                    }


									  
				  				  printf("\r\n Xcoord %f, Ycoord %f , Angle %f are", Xcoord, Ycoord, Angle);				
 

										ES_Event Event2Post;
						        Event2Post.EventType = OBTAIN_LOCATION;
							      PostMasterSM(Event2Post);
									 
										//MakeTransition = true;
										//NextState = Start;
										//EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent.EventType = ES_NO_EVENT;
									}
            }
            break;
						
					case Tuning:
							if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == MOTOR_TIMER){
								if(numBeaconAligned == 2){
								NextState = AlignSecond;
								}else{
									NextState = AlignFirst;
								}
									ES_Event Event2Post;
									Event2Post.EventType = STOP;
									PostMasterSM(Event2Post);
									Event2Post.EventType = GET_DISTANCE;
                 PostLidarService(Event2Post);
							   ReturnEvent.EventType = ES_NO_EVENT;
								 MakeTransition = true;
								EntryEventKind.EventType = ES_ENTRY;
							}
							break;
		}
	 
    if (MakeTransition == true)
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunAlignBeaconSM(CurrentEvent);
        
        CurrentState = NextState; //Modify state variable
               
               //   Execute entry function for new state
               // this defaults to ES_ENTRY
        RunAlignBeaconSM(EntryEventKind);
     }
    return(ReturnEvent);
}
/****************************************************************************
 Function
     QueryRunAlignBeaconSM
****************************************************************************/
//AlignBeaconState_t QueryAlignBeaconSM(void)
//{
//   return CurrentState;
//}

float QueryX(void)
{
    return Xcoord;
}
float QueryY(void)
{
    return Ycoord;
}
float QueryAngle(void)
{
    return Angle;

}
/***************************************************************************
 private functions
 ***************************************************************************/

//No during fcn, lowest level of the Hierarchy

/***************************************************************************
 Hardware Init/Response functions
 ***************************************************************************/

void Init_IRCapture(void){
    //Enable the clock to the Wide timer 0
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
    //Enable the cloeck to Port C
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
    //make sure Timer A is disabled before configuring
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
    //Set the timer to 32bit wide individual mode
    HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    //Initialize the timer A to full 32 bit count
    HWREG(WTIMER0_BASE+TIMER_O_TAILR) = 0xffffffff;
    //Set the timer A to capture mode (TAMR = 3, TAAMS = 0), edge time (TACMR = 1), up-counting (TACDIR = 1)
    HWREG(WTIMER0_BASE+TIMER_O_TAMR) = (HWREG(WTIMER0_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
    //Set the event to rising edge, modify the TAEVENT bits in GPTMCTL. Rising edge = 00
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
    //Set the port to capture, start by setting the alternate function for port C bit 4 (WT0CCP0)
    HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT4HI;
    //Map bit 4 alternate function to WT0CCP0
    HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xfff0ffff)+(7<<16);
    //Enable PC4 for digital input
    HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT4HI;
    HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= BIT4LO;
    //Enable a local capture interrupt
    HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
    //Enable the Timer A in WTIMER 0 interrupt in the NVIC
    HWREG(NVIC_EN2) |= BIT30HI;
    // make sure interrupts are enabled globally
    __enable_irq();
    //Start the timer and enable the timer to stall
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
		printf("\r\n finish ir init");
    
}

void Response_IRCapture(void){
    HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
    //Clear the source of the interrupt
    //Grab the capture value and calculate for the period
    static uint32_t  LastCapture;
    ThisCapture = HWREG(WTIMER0_BASE+TIMER_O_TAR);
    Period = ThisCapture - LastCapture;
	
	if((Period < NWPeriod + toler) && (Period > NWPeriod - toler)){
		NWCount++;
	}else if((Period < NEPeriod + toler) && (Period > NEPeriod - toler)){
		NECount++;
	}else if((Period < SWPeriod + toler) && (Period > SWPeriod - toler)){
		SWCount++;
	}else if((Period < SEPeriod + tolerSE) && (Period > SEPeriod - tolerSE)){
		SECount++;
	}
	
	ES_Event Event2Post;
	Event2Post.EventType = BEACON_ALIGNED;
	if(NECount > CountNumNE){
		NECount = 0;
		Event2Post.EventParam = NE;
	 // printf("\r\n NE");	
PostMasterSM(Event2Post);		
	}else if(SECount > CountNumSE){
		SECount = 0;
		Event2Post.EventParam = SE;
		//printf("\r\n SE");	
PostMasterSM(Event2Post);		
	}else if(NWCount > CountNumNW){
		NWCount = 0;
		Event2Post.EventParam = NW;
		//printf("\r\n NW");
PostMasterSM(Event2Post);		
	}else if(SWCount > CountNumSW){
		SWCount = 0;
		Event2Post.EventParam = SW;
		//printf("\r\n SW");
PostMasterSM(Event2Post);		
	}else{
		//printf("\r\n not match %i", Period);
	}
//	if(lastBeacon!=Event2Post.EventParam){
//				PostMasterSM(Event2Post);
//				lastBeacon = Event2Post.EventParam;
//	}
		

	
    // update LastCapture to prepare for the next edge
    LastCapture = ThisCapture;

}
