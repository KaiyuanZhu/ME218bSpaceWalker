/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef MasterSM_H
#define MasterSM_H

// State definitions for use with the query function
typedef enum { WaitForStart, GamePlay, Idle2 } MasterState_t ;
// Public Function Prototypes

ES_Event RunMasterSM( ES_Event CurrentEvent );
void StartMasterSM ( ES_Event CurrentEvent );
bool PostMasterSM( ES_Event ThisEvent );
bool InitMasterSM ( uint8_t Priority );

#endif /*MasterSM_H */

