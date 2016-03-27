/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef HallSensor_H
#define HallSensor_H


// typedefs for the states
// State definitions for use with the query function


// Public Function Prototypes

ES_Event RunHallSensor( ES_Event CurrentEvent );
void StartHallSensor ( ES_Event CurrentEvent );

void InitHallCapture(void);
void HallCaptureLeftResponse(void);
void HallCaptureRightResponse(void);
void HallCaptureMidResponse(void);

#endif /*SHMTemplate_H */

