/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef FindLocationSM_H
#define FindLocationSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Align, GetLocation } FindLocationState_t ;


// Public Function Prototypes

ES_Event RunFindLocationSM( ES_Event CurrentEvent );
void StartFindLocationSM ( ES_Event CurrentEvent );
FindLocationState_t QueryFindLocationSM ( void );
uint16_t QueryCurrentPosition(void);
#endif /*FindLocationSM_H */

