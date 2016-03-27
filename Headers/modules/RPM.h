/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef RPM_H
#define RPM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { IDLE, CONTROL } RPMState_t ;

typedef struct ControlState_t {
		uint8_t			LeftDirection;
		uint8_t			RightDirection;
		uint16_t		RequestedDutyL;
		uint16_t		RequestedDutyR;
}ControlState;

// Public Function Prototypes
ES_Event RunRPMSM( ES_Event CurrentEvent);
void StartRPMSM( ES_Event CurrentEvent );
void InitEncoderCapture( void );
void EncoderLeftResponse( void );
void EncoderRightResponse( void );
static void PIControl(void);
ControlState GetSpeedControlState(void);
void InitPIControl(void);
void Stop(void);

#endif /*SHMTemplate_H */

