/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef ShootingSM_H
#define ShootingSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {Wait2Shoot, FindCurrentShootingLocation  ,DriveToShootingPosition, ReadyToShoot, BallShot} ShootingState_t ;


// Public Function Prototypes

ES_Event RunShootingSM( ES_Event CurrentEvent );
void StartShootingSM ( ES_Event CurrentEvent );
ShootingState_t QueryShootingSM ( void );
uint8_t GetLEDColor(void);
#endif /*ShootingSM_H */

