/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GoToShootingStationSM_H
#define GoToShootingStationSM_H

#include "ES_Configure.h"
#include "ES_Framework.h"
// typedefs for the states
// State definitions for use with the query function
typedef enum { GetToBucket, DrivingToBucket} GoToShootingStationState_t ;


// Public Function Prototypes

ES_Event RunGoToShootingStationSM( ES_Event CurrentEvent );
void StartGoToShootingStationSM ( ES_Event CurrentEvent );
GoToShootingStationState_t QueryGoToShootingStationSM ( void );
uint8_t GetTargetBucket(void);
//float GetBucketDistance (void);
//float GetBucketAngle (void);


#endif /*GoToShootingStationSM_H */

