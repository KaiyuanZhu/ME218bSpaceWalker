/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef DrivingSM_H
#define DrivingSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { FindCurrentLocation, GoToPollingStation, FindPollingStation, CapturePollingStation, Idle} DrivingState_t ;


// Public Function Prototypes

ES_Event RunDrivingSM( ES_Event CurrentEvent );
void StartDrivingSM ( ES_Event CurrentEvent );
DrivingState_t QueryDrivingSM ( void );

#endif /*DrivingSM_H */

