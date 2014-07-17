//#include <string.h>
//#include <arduino.h>
#include "DmxDue.h"


// Constructors 
DmxDue::DmxDue(Usart* pUsart, IRQn_Type dwIrq, uint32_t dwId, uint8_t *pRx_buffer, uint8_t *pTx_buffer)
{
  _rx_buffer = pRx_buffer;
  _pUsart = pUsart;
  _dwIrq = dwIrq;
  _dwId = dwId;
  
  _tx_buffer = pTx_buffer;
  _txReadyEvent = NULL;
  
  clrTxBuffer();
  setTxMaxChannels(DMX_TX_MAX);
  
  
}

/* --------- transmitter ---------- */

// clear transmit buffer
void DmxDue::clrTxBuffer()
{
  	for (uint16_t i = 0; i < DMX_TX_MAX; i++) {
  		_tx_buffer[i] = 0;
  	}
}	

// set max transmit channels
void DmxDue::setTxMaxChannels(uint16_t maxChannels)
{
	if (maxChannels > DMX_TX_MAX)
		_maxTxChannels = DMX_TX_MAX;
	else 
		_maxTxChannels = maxChannels;
}	

// write value to transmit buffer
void DmxDue::write(uint16_t channel, uint8_t value)
{
	// sanity check
	if (channel == 0 || channel > DMX_TX_MAX) return;
	
	_tx_buffer[channel - 1] = value;
}

/* --------- receiver ---------- */

// return dmx value from channel 
uint8_t DmxDue::read(uint16_t channel)
{
	// sanity check
	if (channel == 0 || channel > DMX_RX_MAX) return 0;
	
	return _rx_buffer[channel];
}

// initialise hardware
void DmxDue::begin()
{
  // Configure PMC
  pmc_enable_periph_clk( _dwId ) ;

  // Disable PDC channel
  _pUsart->US_PTCR = US_PTCR_RXTDIS | US_PTCR_TXTDIS ;

  // Reset and disable receiver and transmitter
  _pUsart->US_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS ;

  // Configure mode
  _pUsart->US_MR = US_MR_USART_MODE_NORMAL | US_MR_USCLKS_MCK | US_MR_CHRL_8_BIT | US_MR_PAR_NO |
                   US_MR_NBSTOP_2_BIT | US_MR_CHMODE_NORMAL;

  // Configure baudrate, asynchronous no oversampling
  _pUsart->US_BRGR = (SystemCoreClock / 250000) / 16 ;

  // Configure interrupts
  _pUsart->US_IDR = 0xFFFFFFFF;
 // _pUsart->US_IER = US_IER_RXRDY | US_IER_OVRE| US_IER_FRAME | US_IER_RXBRK | US_IER_TXRDY | US_IER_TXEMPTY;
   _pUsart->US_IER = US_IER_RXRDY | US_IER_RXBRK | US_IER_TXRDY;

  // Enable UART interrupt in NVIC
  NVIC_EnableIRQ( _dwIrq ) ;

  // first thing to do is generate break	
  _txState = TX_BREAK;
  
  // Enable receiver and transmitter
  _pUsart->US_CR = US_CR_RXEN | US_CR_TXEN;
}


void DmxDue::setBreakEvent(Fptr f)
{
	_txReadyEvent = f;
}

void DmxDue::markAfterBreak()
{
	//  stop brake
  	_pUsart->US_CR |= US_CR_STPBRK;
}

// Send start code
void DmxDue::beginWrite()
{
  	_pUsart->US_THR = 0;
  	_txCnt = 0;
  	_txState = TX_DATA;  // next state
}

// all usartX interupts together
void DmxDue::IrqHandler(void)
{
	uint32_t status = _pUsart->US_CSR;   // get state before data!
   
	// Receiver Break = frame error
	if (status & US_CSR_RXBRK) {  	//check for break
		_rxState = BREAK; // break condition detected.
		_rxLength = _rxCnt; // store dmx length 
		_pUsart->US_CR |= US_CR_RSTSTA;
	} 

	// Did we receive data ?
	if (status & US_CSR_RXRDY) {
		uint8_t dmxByte =_pUsart->US_RHR;	// get data
		
		switch (_rxState) {
			case BREAK:
				// first byte is the start code
				_rx_buffer[0] = dmxByte;
				if (dmxByte == 0) {
					_rxState = DATA;  // normal DMX start code detected
					_rxCnt = 1;       // start with channel # 1
				} else {
					_rxState = IDLE; //  wait for next BREAK !
				} 
				break;	
			case DATA:
				_rx_buffer[_rxCnt] = dmxByte;	// store received data into dmx data buffer
				if (_rxCnt < DMX_RX_MAX) { 
					_rxCnt++; // next channel
				} else {
					_rxState = IDLE;	// wait for next break
				}
				break;
			case IDLE:
			default:
				break;	
		}
	}

	// tx ready
	if (status & US_CSR_TXRDY) {  	
  		switch (_txState) {
  		case TX_DATA:
  			// Send character
  			_pUsart->US_THR = _tx_buffer[_txCnt];
  			if (_txCnt < _maxTxChannels - 1) {
  				_txCnt++;
  			} else {
  				_txState = TX_BREAK;// next state
  			}	
  			break;
  		case TX_BREAK:
  			// send first break byte
  			_pUsart->US_CR |= US_CR_STTBRK;
  			_txState = TX_BREAK2;
  			break;
  		case TX_BREAK2:
  			// break must be 88 uSec so , 44 uSec to go
  			if (_txReadyEvent)	_txReadyEvent();
  			break;	
  		default:	
  		  break;
  		}
   	} 
 
  /* 
  // Acknowledge errors
  if (status & US_CSR_OVRE)
  {
	// TODO: error reporting outside ISR
    _pUsart->US_CR |= US_CR_RSTSTA;
  }
  */
  
}


// ----------------------------------------------------------------------------
/*
 * USART objects
 */

// Array of DMX values (raw).
// Entry 0 will never be used for DMX data but will store the startbyte (0 for DMX mode).
uint8_t dmx_rx_buffer1[DMX_RX_MAX + 1];
uint8_t dmx_rx_buffer2[DMX_RX_MAX + 1];
uint8_t dmx_rx_buffer3[DMX_RX_MAX + 1];


uint8_t dmx_tx_buffer1[DMX_TX_MAX];
uint8_t dmx_tx_buffer2[DMX_TX_MAX];
uint8_t dmx_tx_buffer3[DMX_TX_MAX];

// Create class instances
DmxDue DmxDue1(USART0, USART0_IRQn, ID_USART0, dmx_rx_buffer1, dmx_tx_buffer1);
DmxDue DmxDue2(USART1, USART1_IRQn, ID_USART1, dmx_rx_buffer2, dmx_tx_buffer2);
DmxDue DmxDue3(USART3, USART3_IRQn, ID_USART3, dmx_rx_buffer3, dmx_tx_buffer3);

// IT handlers
void USART0_Handler( void )
{
  DmxDue1.IrqHandler() ;
}

void USART1_Handler( void )
{
  DmxDue2.IrqHandler() ;
}

void USART3_Handler( void )
{
  DmxDue3.IrqHandler() ;
}
