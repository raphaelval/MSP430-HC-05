/**
 * main.c
 * Sample Code showing the use of: UART, Rx Bluetooth Input, Tx Bluetooth Output
 * Raphael Valente
 */

#include <msp430.h> 
#include <string.h>

/*  UART  */
#define UARTReceive UCA0RXBUF;                  // Set RXD as UARTReceive

void UARTSendArray(unsigned char *TxArray);
void UARTSendChar(unsigned char *c);
void configureADC();
void getanalogvalues();
int getADCValue(int port);

static unsigned char data;
int flag = 0;
int i = 0;
int temp = 0;
int temproom = 0;
int tempReading;

/*  NOTES  */
// P1.0 IN: Temp Sensor
// P1.7 OUT: Buzzer
// P2.2 OUT: Servo

void main(void)

{
    WDTCTL = WDTPW + WDTHOLD;                  // Stop WDT

    P1DIR |= BIT6 + BIT7;                      // Set the LEDs on P1.6, P1.7 as outputs
    P1OUT |= BIT6;                             // Set the P1.6 LED
    P1OUT &= ~BIT7;                            // Set P1.7 Buzzer OFF

    /* Configure hardware UART */
    P1SEL = BIT1 + BIT2;                       // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2;                      // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                      // Use SMCLK
    UCA0BR0 = 104;                             // Set baud rate to 9600 with 1MHz clock (Data Sheet 15.3.13)
    UCA0BR1 = 0;                               // Set baud rate to 9600 with 1MHz clock
    UCA0MCTL = UCBRS0;                         // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                      // Initialize USCI state machine
    IE2 |= UCA0RXIE;                           // Enable USCI_A0 RX interrupt

    /* Configure PWM */
    BCSCTL1= CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    P2DIR |= BIT2;
    P2SEL |= BIT2;  //selection for timer setting

    __delay_cycles(2000000);

    configureADC();

    __delay_cycles(250);
    getanalogvalues();
    temproom = temp;
    __delay_cycles(250);

    //__bis_SR_register(LPM0_bits | GIE);         // enter LPM0 with interrupt enable
    __enable_interrupt();

    for(;;){

        TA1CCR0 = 20000;  //PWM period

        getanalogvalues();

        if ( temp > temproom * 1.05 ) {
            P1OUT ^=  BIT7;                       // Toggle Buzzer
            __delay_cycles(500000);               // Every 0.5 Second
        }
        else if(temp < temproom * 1.03 ){
            //P1OUT &= ~BIT6;
            P1OUT &= ~BIT7;
            __delay_cycles(200);
        }

    }

}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    data = UARTReceive;                                     // Receives data from UART RXD
    unsigned int bytesToSend = strlen((const char*) data);  // Number of bytes in data;

    switch(data){                                           // You chose "data"
        case 'G':   //
        {
            UARTSendArray("Green LED is on");
            UARTSendArray("\n");
            //P1OUT |= BIT0;                                // Turn on LED P1.0
        }
        break;
        case 'g':   //
        {
            UARTSendArray("Green LED is off");
            UARTSendArray("\n");
            //P1OUT &= ~BIT0;                               // Turn off LED P1.0
        }
        break;
        case 'R':   //
        {
            UARTSendArray("Red LED is on");
            UARTSendArray("\n");
            P1OUT |= BIT6;                                  // Turn on LED P1.6
        }
        break;
        case 'r':   //
        {
            UARTSendArray("Red LED is off");
            UARTSendArray("\n");
            P1OUT &= ~BIT6;                                 // Turn off LED P1.6
        }
        break;
        case 'u':   // Unlock
        {
            UARTSendArray("Unlocked");
            UARTSendArray("\n");
            // UNLOCK
            TA1CCR1 = 700;  //CCR1 PWM Duty Cycle
            TA1CCTL1 = OUTMOD_7;  //CCR1 selection reset-set
            TA1CTL = TASSEL_2|MC_1;   //SMCLK submain clock,upmode
        }
        break;
        case 'l':   // Lock
        {
            UARTSendArray("Locked");
            UARTSendArray("\n");
            // LOCK
            TA1CCR1 = 1700;
            TA1CCTL1 = OUTMOD_7;  //CCR1 selection reset-set
            TA1CTL = TASSEL_2|MC_1;
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

void configureADC(){

    ADC10CTL1 = INCH_0 | CONSEQ_1;                // A0, single sequence
    ADC10CTL0 = ADC10SHT_2 | MSC | ADC10ON;
    while (ADC10CTL1 & BUSY);
    ADC10DTC1 = 0x01;                             // 1 conversions
    ADC10AE0 |= (BIT0);                           // ADC10 option select
}

void getanalogvalues()
{
    i = 0; temp = 0;            // set all analog values to zero
    for(i=1; i<=5 ; i++)                          // read all three analog values 5 times each and average
    {
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & BUSY);                     //Wait while ADC is busy
        ADC10SA = (unsigned)&tempReading;           //RAM Address of ADC Data, must be reset every conversion
        ADC10CTL0 |= (ENC | ADC10SC);                 //Start ADC Conversion
        while (ADC10CTL1 & BUSY);                     //Wait while ADC is busy
        temp += tempReading;
    }
    temp = temp/5; // Average the 5 reading for the three variables
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
