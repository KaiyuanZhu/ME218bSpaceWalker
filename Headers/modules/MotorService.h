/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef MotorService_H
#define MotorService_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { forward, backward, clockwise, counterclockwise, alignbeacon, no_event} LastEvent_t ;


// Public Function Prototypes

ES_Event RunMotorService( ES_Event CurrentEvent );
void StartMotorService ( ES_Event CurrentEvent );
void InitEncoderCapturePeriod( void );
void InputEncoderCaptureResponse( void );
int16_t GetTargetRPMLeft(void);
int16_t GetTargetRPMRight(void);
LastEvent_t GetLastEvent(void);
void Stop(void);
float Getdistance (void);
float Getangle (void);
#endif /*MotorService_H */

