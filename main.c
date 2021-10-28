#include <msp430.h> 
#include <string.h>

#define UARTReceive UCA0RXBUF;                  // Set RXD as UARTReceive

/**
 * main.c
 * Sample Code showing the use of: UART, Rx Bluetooth Input, Tx Bluetooth Output
 * Raphael Valente
 */

void UARTSendArray(unsigned char *TxArray);
void UARTSendChar(unsigned char *c);

static unsigned char data;
int flag = 0;

void main(void)

{
    WDTCTL = WDTPW + WDTHOLD;                   // Stop WDT

    P1DIR |= BIT0 + BIT6;                       // Set the LEDs on P1.0, P1.6 as outputs
    P1OUT = BIT0;                               // Set P1.0

    BCSCTL1 = CALBC1_1MHZ;                      // Set DCO to 1MHz
    DCOCTL = CALDCO_1MHZ;                       // Set DCO to 1MHz

    /* Configure hardware UART */
    P1SEL = BIT1 + BIT2 ;                       // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2 ;                      // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                       // Use SMCLK
    UCA0BR0 = 104;                              // Set baud rate to 9600 with 1MHz clock (Data Sheet 15.3.13)
    UCA0BR1 = 0;                                // Set baud rate to 9600 with 1MHz clock
    UCA0MCTL = UCBRS0;                          // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                       // Initialize USCI state machine
    IE2 |= UCA0RXIE;                            // Enable USCI_A0 RX interrupt

    __bis_SR_register(LPM0_bits | GIE);         // enter LPM0 with interrupt enable
}

// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    data = UARTReceive;                                     // Receives data from UART RXD
    unsigned int bytesToSend = strlen((const char*) data);  // Number of bytes in data;

    switch(data){
        case 'G':                                           // You chose "data"
        {
            UARTSendArray("Green LED is on");
            //UARTSendChar(&data);                          // Send data to UART TXD
            UARTSendArray("\n");
            P1OUT |= BIT0;                                  // Turn on LED P1.0
        }
        break;
        case 'g':
        {
            UARTSendArray("Green LED is off");
            //UARTSendChar(&data);                          // Send data to UART TXD
            UARTSendArray("\n");
            P1OUT &= ~BIT0;                                 // Turn off LED P1.0
        }
        break;
        case 'R':
        {
            UARTSendArray("Red LED is on");
            //UARTSendChar(&data);                          // Send data to UART TXD
            UARTSendArray("\n");
            P1OUT |= BIT6;                                  // Turn on LED P1.6
        }
        break;
        case 'r':
        {
            UARTSendArray("Red LED is off");
            //UARTSendChar(&data);                          // Send data to UART TXD
            UARTSendArray("\n");
            P1OUT &= ~BIT6;                                 // Turn off LED P1.6
        }
        break;
        default:                                            // Command "data" invalid
        {
            UARTSendArray("Command ");
            UARTSendChar(&data);
            UARTSendArray(" invalid.\n");
        }
        break;

    }

    IFG2 &= ~UCA0RXIFG;             // Turn off interrupt flag

}

void UARTSendChar(unsigned char *c){

     while(!(IFG2 & UCA0TXIFG));            // Wait for TX buffer to be ready for new data
     UCA0TXBUF = *c;                        // Send Character that is at location of pointer
     c++;

}

void UARTSendArray(unsigned char *TxArray){

    while(*TxArray){                        // Loop until StringLength == 0 and post decrement
        while(!(IFG2 & UCA0TXIFG));         // Wait for TX buffer to be ready for new data
        UCA0TXBUF = *TxArray;               // Send Character that is at location of pointer
        TxArray++;                          // Increment the TxString pointer to point to the next character
    }

}
