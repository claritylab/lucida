#ifndef __UART_H_
#define __UART_H_

#include <services\services.h>				// system services

void UARTinit( u32 baudrate );
void UARTwrite(u8* data, u32 len);
u8 UARThit();
u8 UARTsending();
u8 UARTread();

#endif
