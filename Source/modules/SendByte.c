/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
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


#define ENTRY_STATE WAIT2SEND
#define BitsPerNibble 4
#define TicksPerMS    40000
#define ALL_BITS     (0xff<<2)
#define CPSDVSR 0x0014 //(20)
#define SCR 0x0009
#define BYTE_TIMER_LENGTH 2
#define SENDCOMMAND_TIMER_LENGTH 1
#define BYTETIMEOUT_TIMER_LENGTH 2

static SendByteState_t CurrentState;
static uint8_t NewCommand;
static uint8_t Byte4Return[5] = {0,0,1,0,0};

static ES_Event DuringWait4Timeout( ES_Event Event);
static ES_Event DuringWait2Send( ES_Event Event);
static ES_Event DuringWait4EOT( ES_Event Event);
ES_Event RunSendByte( ES_Event CurrentEvent );
void SPIInit(void);
uint8_t GetSPIByte( uint8_t );



ES_Event RunSendByte( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendByteState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch (CurrentState)
   {
       case WAIT2SEND :       
			  CurrentEvent = DuringWait2Send(CurrentEvent);
			 if ( CurrentEvent.EventType != ES_NO_EVENT ) 
			 {
				   //printf("\r\nin wait2send\r\n");
					if(CurrentEvent.EventType == ES_SENDCOMMAND)
						{						  
					    if( (HWREG(SSI0_BASE + SSI_O_SR) & SSI_SR_TFE) == SSI_SR_TFE )
              {
							//printf("\r\nin wait2send_sending command\r\n");
							 NewCommand = CurrentEvent.EventParam;
							// printf("Newcommand:%x\r\n",NewCommand);

               // Write query byte to the data output register
               HWREG(SSI0_BASE + SSI_O_DR) = NewCommand;

							// write 8 0s to the data output register
                	for (int i=0;i<4;i++){
		              HWREG(SSI0_BASE+SSI_O_DR) = 0x00;
	               }
				      }	
							// make sure that it has started shifting by waiting for the BSY bit in the SSI Status Register to go to 1
							while((HWREG(SSI0_BASE+SSI_O_SR) & SSI_SR_BSY) != SSI_SR_BSY);
							               HWREG(NVIC_EN0) |= BIT7HI;
							//printf("\n\r NewCommand = %x\n\r", NewCommand); 
            // Set the SSI transmit interrupt as unmasked to enable EOT interrupt (SSI_IM_TXIM)					
							 NextState = WAIT4EOT;//Decide what the next state will be
							 MakeTransition = true; //mark that we are taking a transition
							 //ES_Timer_InitTimer(BYTE_TIMER, BYTE_TIMER_LENGTH);
							// not sure whether I should consume the event here
							ReturnEvent.EventType = ES_NO_EVENT;
						//break;
						}							
			 }
			 break;
			 			 
			 
			 case WAIT4EOT:
				 CurrentEvent = DuringWait4EOT(CurrentEvent);
			  
			 if (CurrentEvent.EventType != ES_NO_EVENT)
			 {
				 if(ES_EOT == CurrentEvent.EventType)
				 {
				    //printf("\n\r EOT_DETECTED \n\r");
						ES_Timer_InitTimer(BYTETIMEOUT_TIMER, BYTETIMEOUT_TIMER_LENGTH);
						NextState = WAIT4TIMEOUT;
					  MakeTransition = true;
					 ReturnEvent.EventType = ES_NO_EVENT; // consume the event here		
			    }
         
				 //HWREG(SSI1_BASE+SSI_O_CR1) |= SSI_CR1_SSE;		
		
		    }
			break;
				 
      // repeat state pattern as required for other states
			 case WAIT4TIMEOUT:
				 CurrentEvent = DuringWait4Timeout(CurrentEvent);
			   
			 if ( CurrentEvent.EventType != ES_NO_EVENT ) 
			 {
				 if(ES_TIMEOUT == CurrentEvent.EventType && BYTETIMEOUT_TIMER == CurrentEvent.EventParam ) 
				 {
					 //printf("\n\r in WAIT4TIMEOUT \n\r");
           NextState = WAIT2SEND;
					 //printf("\n\r go back to WAIT2SEND \n\r");
					 MakeTransition = true;
					 ES_Event Event2Post;
					 Event2Post.EventType = RETURN_RESPONSE_SENDBYTE;
					 PostMasterSM (Event2Post);		
           //ReturnEvent.EventType = ES_NO_EVENT; // consume the event					 
				}
			 }		 
			 break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunSendByte(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunSendByte(EntryEventKind);
     }
     return(ReturnEvent);
}

void StartSendByte ( ES_Event CurrentEvent)
{

   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunSendByte(CurrentEvent);
	 //printf("\r\nStartSendByte.");
}


void SPIInit(void){

		// Enable clock to the GPIO port (A)
		HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
		// Enable clock to the SSI Module 0
		HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0;
		// Wait for GPIO port A to be ready
		while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0 ) != SYSCTL_PRSSI_R0 ) ;
		// Program pins A2-A5 as alternate functions on the GPIO to use SSI (Set High)
		HWREG(GPIO_PORTA_BASE + GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
		// Changes: GPIO_O_ODR
		// SS (Pin A3) now configured as open drain, see TIVA Data Sheet page 676 
		//HWREG(GPIO_PORTA_BASE + GPIO_O_ODR) |= (BIT3HI);
		// Select the SSI alternate functions on pins A2-A5
		// Set mux position on GPIOPCTL to select the use of the pins (2 for SSI) 
		HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL) |=(HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL) & ~0x00ffff00) + (2 << ( 2 * BitsPerNibble)) +
		(2 << ( 3 * BitsPerNibble)) + (2 << ( 4 * BitsPerNibble)) + (2 << ( 5 * BitsPerNibble));
		// Program port lines for pins A2-A5 as digital I/O
		HWREG(GPIO_PORTA_BASE + GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
		// Program required data directions on the port lines (2/3/5 Output, 4 Input)
		HWREG(GPIO_PORTA_BASE + GPIO_O_DIR) |= (BIT2HI | BIT3HI | BIT5HI);
	  // Set PA6 High
//		HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= BIT6HI;
//		HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= BIT6HI;
//		HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT6HI;
		HWREG(GPIO_PORTA_BASE + GPIO_O_DIR) &= BIT4LO;
		// Program pullup resistor on clock line (A2), (changes) pull down resistor on receive line (A4)
		HWREG(GPIO_PORTA_BASE + GPIO_O_PUR) |= (BIT2HI); 
		//HWREG(GPIO_PORTA_BASE + GPIO_O_PDR) |= (BIT4HI);
		// Wait for the SSI0 Module 0 to be ready
		while((HWREG(SYSCTL_RCGCSSI) & SYSCTL_RCGCSSI_R0) != SYSCTL_RCGCSSI_R0);
		// Make sure SSI Module 0 is disabled before programming mode bits
		HWREG(SSI0_BASE + SSI_O_CR1) &= ~SSI_CR1_SSE;
		// Select master mode (MS) & TXRIS interrupt indicating end of transmission (EOT)
		HWREG(SSI0_BASE + SSI_O_CR1) &= ~SSI_CR1_MS; 
		HWREG(SSI0_BASE + SSI_O_CR1) |= SSI_CR1_EOT;
		// Configure the SSI clock source to the system clock
		HWREG(SSI0_BASE + SSI_O_CC) |= SSI_CC_CS_SYSPLL; 
		//HWREG(SSI0_BASE + SSI_O_CC) &= ~(BIT0HI | BIT1HI | BIT2HI | BIT3HI);
		// Configure the clock pre-scaler  = 20
//			HWREG(SSI0_BASE + SSI_O_CPSR) |= 0xff;
//   	HWREG(SSI0_BASE + SSI_O_CPSR) &= ~(0x05);
		HWREG(SSI0_BASE + SSI_O_CPSR) |= CPSDVSR;
		// Configure the clock rate (SCR), phase (SPH) & polarity (SPO), mode (FRF), and data size (DSS)
      HWREG(SSI0_BASE + SSI_O_CR0) |= SCR<<8;
		//	HWREG(SSI0_BASE + SSI_O_CR0) |= 0x0A00; 
		// Set SCR to 8
		HWREG(SSI0_BASE + SSI_O_CR0) |= (SSI_CR0_SPH | SSI_CR0_SPO); // Set SPH and SPO to 1
		HWREG(SSI0_BASE + SSI_O_CR0) |= SSI_CR0_FRF_MOTO; // freescale SPI Frame Format 
		// Set frame mode (pg 969)
		HWREG(SSI0_BASE + SSI_O_CR0) |= SSI_CR0_DSS_8; // Set data size to 8-bit
		
		// Locally enable interrupts on TXRIS
			HWREG(SSI0_BASE + SSI_O_IM) |= SSI_IM_TXIM;
		// Set NVIC enable
		HWREG(NVIC_EN0) |= BIT7HI;
		// Make sure interrupts are enables globally
		__enable_irq( );
		// Make sure that the SSI is enabled for operation
		 HWREG(SSI0_BASE + SSI_O_CR1) |= SSI_CR1_SSE;
printf("\r\nInit SPI successfully.");
}


void EOTResponse( void )
{
//uint16_t ThisRead;
ES_Event PostEvent;
ES_Event InvalidEvent;
//printf("In the EOTResponse");
//LastSlaveRead = NewSlaveRead;
// Make sure the SPI it not transmitting/receiving before reading receive register
//if( (HWREG(SSI0_BASE + SSI_O_SR) & SSI_SR_BSY) != SSI_SR_BSY ) {
            // Check if the data input FIFO queue is full
//if( (HWREG(SSI0_BASE + SSI_O_SR) & SSI_SR_RFF) == SSI_SR_RFF ) {
// Read 2 bytes received from the RS into NewSlaveRead
	      //disable the NVIC 
         HWREG(NVIC_EN0) &= ~BIT7HI;
	       for (int i=0; i<5; i++)
					{
          // Save each read as 16bit
          //ThisRead = HWREG(SSI0_BASE + SSI_O_DR);
				//printf("ThisRead:%x\r\n",ThisRead);
	
         // Store thisRead as Byte4Return
          //Byte4Return[i] = ThisRead;
						Byte4Return[i] = HWREG(SSI0_BASE + SSI_O_DR);
					printf("Bit received [%d]:%x\r\n", i,Byte4Return[i]);
					}
				//printf("Receive %x",Byte4Return);
					PostEvent.EventType = ES_EOT;
					
					PostMasterSM(PostEvent);
					
					if (Byte4Return[2] == 0xFF && Byte4Return[3] == 0xFF && Byte4Return[4] == 0xFF)
					{
						InvalidEvent.EventType = INVALID_RESPONSE_RECEIVED;
						PostMasterSM(InvalidEvent);
					}
          
 //       }


   // }
}

SendByteState_t QuerySendByte (void)
{
   return(CurrentState);
}

// later on should use a pointer
uint8_t GetSPIByte(uint8_t i)
{
	return Byte4Return[i];
}


/***************************************************************************
 private functions
 ***************************************************************************/


static ES_Event DuringWait4Timeout( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        // implement any entry actions required for this state machine
    } else if ( Event.EventType == ES_EXIT ) {
        // on exit, give the lower levels a chance to clean up first
    } else {
        // do any activity that is repeated as long as we are in this state

    }
    return(ReturnEvent);
}
static ES_Event DuringWait4EOT( ES_Event Event)
{
	    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        // implement any entry actions required for this state machine
    } else if ( Event.EventType == ES_EXIT ) {
        // on exit, give the lower levels a chance to clean up first
    } else {
        // do any activity that is repeated as long as we are in this state
    }
		
		return(ReturnEvent);
}
static ES_Event DuringWait2Send( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        // implement any entry actions required for this state machine
    } else if ( Event.EventType == ES_EXIT ) {
        // on exit, give the lower levels a chance to clean up first
    } else {
        // do any activity that is repeated as long as we are in this state
    }
    return(ReturnEvent);
}
