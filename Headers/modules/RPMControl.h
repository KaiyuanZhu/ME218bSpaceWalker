/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef RPMControl_H
#define RPMControl_H

typedef enum { IDLE, CONTROL } RPMControlState_t ;
// typedefs for the states
// State definitions for use with the query function
typedef struct ControlState_t {
		uint8_t    LeftDirection;
		uint8_t    RightDirection;
		uint16_t   RequestedDutyL;
		uint16_t   RequestedDutyR;
}ControlState;

// Public Function Prototypes

ES_Event RunRPMControl( ES_Event CurrentEvent );
void StartRPMControl ( ES_Event CurrentEvent );
void InitEncoderCapture( void );
void EncoderLeftResponse( void );
void EncoderRightResponse( void );
ControlState GetSpeedControlState(void);
void InitPIControl(void);
void Stop(void);
float GetDistanceToComplete(void);
float GetAngleToComplete(void);
#endif /*RPMControl_H */

