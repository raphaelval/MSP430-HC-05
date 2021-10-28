#include <msp430.h> 
#include <string.h>

#define UARTReceive UCA0RXBUF;

/**
 * main.c
 */

void UARTSendArray(unsigned char *TxArray);
//unsigned char UARTReceive(void);

static unsigned char data;

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

    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled


}

// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{

    data = UARTReceive;
    //*data = UCA0RXBUF;
    UARTSendArray("Received command: ");
    UARTSendArray(&data);
    UARTSendArray("\n");

    //__bic_SR_register_on_exit(LPM0_bits);

    switch(data){
        case 'G':
        {
            P1OUT |= BIT0;
        }
        break;
        case 'g':
        {
            P1OUT &= ~BIT0;
        }
        break;
        case 'R':
        {
            P1OUT |= BIT6;
        }
        break;
        case 'r':
        {
            P1OUT &= ~BIT6;
        }
        break;
        default:
        {
            //UARTSendArray("Unknown Command: ");
            UARTSendArray(&data);
            //UARTSendArray("\n");
        }
        break;

    }
    //__bic_SR_register_on_exit(LPM0_bits);

}

void UARTSendArray(unsigned char *TxArray){
 // Send number of bytes Specified in ArrayLength in the array at using the hardware UART 0
 // Example usage: UARTSendArray("Hello", 5);
 // int data[2]={1023, 235};
 // UARTSendArray(data, 4); // Note because the UART transmits bytes it is necessary to send two bytes for each integer hence the
 // data length is twice the array length

    while(*TxArray != '\0'){                        // Loop until StringLength == 0 and post decrement
        while(!(IFG2 & UCA0TXIFG));         // Wait for TX buffer to be ready for new data
        UCA0TXBUF = *TxArray;               //Write the character at the location specified by the pointer
        TxArray++;                          //Increment the TxString pointer to point to the next character
    }
}

void uart_putc(unsigned char c)             // print to PC over TX one character at a time.
{
    while (!(IFG2 & UCA0TXIFG));            // USCI_A0 TX buffer ready?
    UCA0TXBUF = c;                          // TX
}

void uart_puts(const char *str)             // print any string message to PC with this function.
{
     while(*str) uart_putc(*str++);
}
