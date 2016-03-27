#ifndef Lidar_H
#define Lidar_H

#include "ES_Types.h"
#include "ES_Configure.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitLidar,LidarStart, LidarMeasure} LidarState_t;

bool InitLidarService (uint8_t Priority);
bool PostLidarService (ES_Event ThisEvent);
ES_Event RunLidarService(ES_Event ThisEvent);
void Init_LidarCapture(void);
void Response_LidarCapture(void);
float QueryDistance(void);

#endif /* LIDAR_H */

