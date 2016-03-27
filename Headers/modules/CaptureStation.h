/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef CaptureStation_H
#define CaptureStation_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { ReadSignal1, ReadSignal2, WaitForResponse1, SensorRead} CaptureStation_t;


// Public Function Prototypes

ES_Event RunCaptureStationSM( ES_Event CurrentEvent );
void StartCaptureStationSM ( ES_Event CurrentEvent );
//TemplateState_t QueryTemplateSM ( void );

#endif /*SHMTemplate_H */

