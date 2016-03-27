/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef AlignBeaconService_H
#define AlignBeaconService_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Start, WaitForIR, AlignFirst, AlignSecond, Tuning, Shoot} AlignBeaconState_t ;


// Public Function Prototypes

ES_Event RunAlignBeaconSM( ES_Event CurrentEvent );
void StartAlignBeaconSM ( ES_Event CurrentEvent );
AlignBeaconState_t QueryAlignBeaconSM ( void );
float QueryX(void);
float QueryY(void);
float QueryAngle(void);

#endif /*AlignBeaconService_H */

