

#ifndef SENDBYTE_H
#define SENDBYTE_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { WAIT4TIMEOUT, WAIT4EOT, WAIT2SEND } SendByteState_t ;


// Public Function Prototypes

ES_Event RunSendByte( ES_Event CurrentEvent );
void StartSendByte ( ES_Event CurrentEvent );
SendByteState_t QuerySendByte ( void );
uint8_t GetSPIByte( uint8_t );
void SPIInit(void);


#endif /*SENDBYTE_H */
