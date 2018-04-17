# DmxDue
======

Dmx Library for Arduino Due
# Features:
* Hardware USART and interrupts using. No wasting CPU time
* Simultaneously RX and TX using single USART

# Due compilation issue "USART<X>_Handler redefinition"
Please, open  /variants/arduino_due_x/variant.cpp file, then add USART0_Handler method definition like this
void USART0_Handler(void) __attribute__((weak));

The normal path to find this file in platformio is:
.platformio/packages/framework-arduinosam/variants/arduino_due_x
