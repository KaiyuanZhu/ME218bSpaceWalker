/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GoToPollingStationSM_H
#define GoToPollingStationSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { GetToTarget, DrivingToStation, ARRIVE} GoToPollingStationState_t ;


// Public Function Prototypes

ES_Event RunGoToPollingStationSM( ES_Event CurrentEvent );
void StartGoToPollingStationSM ( ES_Event CurrentEvent );
GoToPollingStationState_t QueryGoToPollingStationSM ( void );
uint8_t GetForwardFlag(void);
//float Getdistance (void);
//float Getangle (void);

#endif /*GoToPollingStationSM_H */

