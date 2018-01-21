/*
  Copyright (c) 2014 Michel Vergeest.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define DMX1
//#define DMX2
//#define DMX3
//#define DMX4

#ifndef _DmxDue_
#define _DmxDue_

// Includes Atmel CMSIS
//#include <chip.h>
// Include Atmel CMSIS driver
#include <include/usart.h>


#include "HardwareSerial.h"

#define DMX_RX_MAX 512
#define	DMX_TX_MAX 512

// State of receiving DMX Bytes
typedef enum {
  IDLE, BREAK, DATA
} dmxRxState_t;


typedef enum {
  TX_DATA, TX_BREAK, TX_BREAK1, TX_BREAK2, TX_START, TX_DISABLE 
} dmxTxState_t;

//typedef void (*Fptr)();

extern "C" {
  typedef void (*Fptr)();
}

class DmxDue {
protected:
	uint8_t 	*_rx_buffer;
	Usart		*_pUsart;
    IRQn_Type 	_dwIrq; 
    uint32_t 	_dwId;
    
private:
	dmxRxState_t _rxState; // break condition detected.
	uint16_t	_rxCnt;       // The next data byte is the start byte
	uint16_t	_rxLength;
	
	// transmitter
	dmxTxState_t	       _txState;
	uint16_t		_txCnt;	
	uint8_t 		*_tx_buffer;
	Fptr 			_txReadyEvent; 
	Fptr 			_rxReadyEvent; 

	uint16_t		_maxTxChannels;
	void markAfterBreak();
	
public:
    DmxDue(Usart* pUsart, IRQn_Type dwIrq, uint32_t dwId, uint8_t* pRx_buffer, uint8_t* pTx_buffer);
    void begin();
    void setInterruptPriority(uint32_t priority);
    void IrqHandler(void);

    uint8_t read(uint16_t channel);
    uint16_t getRxLength();	
	
	//transmitter
	void beginWrite();
	
	void setBreakEvent(Fptr f);
	// attach function that will be called when new data was received.
        void setRxEvent(Fptr f);
	
	
	// clear transmit buffer 
	void clrTxBuffer();
	
	// write value to transmit buffer
	void write(uint16_t channel , uint8_t value);
	
	// set max transmit channels
	void setTxMaxChannels(uint16_t maxChannels);
};

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus

#ifdef DMX1
extern DmxDue DmxDue1;
#endif
#ifdef DMX2
extern DmxDue DmxDue2;
#endif
#ifdef DMX3
extern DmxDue DmxDue3;
#endif
#ifdef DMX4
extern DmxDue DmxDue4;
#endif

#endif

#endif // _DmxDue_
