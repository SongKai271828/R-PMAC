#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#define PHYPayloadLength 100 // define max phy length
#define PHYMAXDepth 16 // define max route depth
#define UARTBufferLength 128 // 

#define UARTTimeout 300 // larger than 1/Baudrate * UARTBufferLength * 8
#define FlashAccessTimeout 100

#endif
