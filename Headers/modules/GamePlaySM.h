/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GamePlaySM_H
#define GamePlaySM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {Driving, Shooting } GamePlayState_t ;


// Public Function Prototypes

ES_Event RunGamePlaySM( ES_Event CurrentEvent );
void StartGamePlaySM ( ES_Event CurrentEvent );
GamePlayState_t QueryGamePlaySM ( void );
uint8_t GetLEDColor(void);
uint8_t GetFreezeTimerFlag(void);

#endif /*GamePlaySM_H */

