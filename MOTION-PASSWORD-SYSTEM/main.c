/**
 * main.c
 * Sample Code showing the use of: UART, Rx Bluetooth Input, Tx Bluetooth Output
 *
 * Password authentication using motion from accelrometer.
 *
 * Raphael Valente
 */

#include <msp430.h> 
#include <string.h>

/*  UART  */
#define UARTReceive UCA0RXBUF;                  // Set RXD as UARTReceive

// Prototypes
void UARTSendArray(unsigned char *TxArray);
void UARTSendChar(unsigned char *c);
void configureADC();
void getanalogvalues();
void ADC_Reading_X();
void ADC_Reading_Y();
void ADC_Reading_Z();

// RXD Data variable
static unsigned char data;

int i = 0;

int pin1 = 0, pin2 = 0, pin3 = 0, pin4 = 0;
int x_axis = 0, y_axis = 0, z_axis = 0;
int x_axis_init = 0, y_axis_init = 0, z_axis_init = 0;

/*  NOTES  */
// P1.0 OUT: Green LED
// P1.1 IN: RXD
// P1.2 OUT: TXD
// P1.3 IN: Accelerometer X-Axis
// P1.4 IN: Accelerometer Y-Axis
// P1.5 IN: Accelerometer Z-Axis
// P1.6 OUT: Red LED
// P1.7 OUT: Buzzer

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                  // Stop WDT

    P1DIR |= BIT0 + BIT6 + BIT7;               // Set the LEDs on P1.0, P1.6, P1.7 as outputs
    P1OUT &= ~BIT0;                            // Set the P1.0 LED
    P1OUT &= ~BIT6;
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
    BCSCTL1= CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    __delay_cycles(2000000);
    P1OUT |= BIT0;
    P1OUT |= BIT7;
    __delay_cycles(100000);
    P1OUT &= ~BIT0;
    P1OUT &= ~BIT7;

    configureADC();

    __delay_cycles(250);
    getanalogvalues();

    // Initializes the starting readings
    x_axis_init = x_axis;
    y_axis_init = y_axis;
    z_axis_init = z_axis;

    __delay_cycles(250);

    __enable_interrupt();

    for(;;){

        getanalogvalues();

        /* PASSWORD VERFICATION */
        if (x_axis > x_axis_init*1.02 || pin1 == 1) {
            if (pin1 == 0) {
                P1OUT |= BIT0;
                P1OUT |= BIT7;
                UARTSendArray("1");                     // Send Correct PIN1 to App
                UARTSendArray("\n");
                __delay_cycles(500000);
                getanalogvalues();
                P1OUT &= ~BIT0;
                P1OUT &= ~BIT7;
            }

            pin1 = 1;
            if (z_axis > z_axis_init*1.2 || pin2 == 1) {
                if(pin2 == 0) {
                    P1OUT |= BIT0;
                    P1OUT |= BIT7;
                    UARTSendArray("2");                 // Send Correct PIN2 to App
                    UARTSendArray("\n");
                    __delay_cycles(500000);
                    getanalogvalues();
                    P1OUT &= ~BIT0;
                    P1OUT &= ~BIT7;
                }

                pin2 = 1;
                if (x_axis < x_axis_init*0.98 || pin3 == 1) {
                    if(pin3 == 0) {
                        P1OUT |= BIT0;
                        P1OUT |= BIT7;
                        UARTSendArray("3");             // Send Correct PIN3 to App
                        UARTSendArray("\n");
                        __delay_cycles(500000);
                        getanalogvalues();
                        P1OUT &= ~BIT0;
                        P1OUT &= ~BIT7;
                    }

                    pin3 = 1;
                    if (z_axis < z_axis_init*0.8) {
                        P1OUT |= BIT0;
                        P1OUT |= BIT7;
                        UARTSendArray("T");             // Send Correct Password to App
                        UARTSendArray("\n");
                        __delay_cycles(500000);
                        P1OUT &= ~BIT7;


                        pin4 = 1;
                        __delay_cycles(2000000);
                        P1OUT &= ~BIT0;
                        pin1 = 0;
                        pin2 = 0;
                        pin3 = 0;
                        pin4 = 0;
                    }
                } else if (x_axis > x_axis_init*1.02 || z_axis < z_axis_init*0.80 ||
                           z_axis > z_axis_init*1.20) {
                    pin1 = 0;
                    pin2 = 0;
                    pin3 = 0;
                    P1OUT |= BIT6;
                    UARTSendArray("X");                    // Send incorrect password to App
                    UARTSendArray("\n");
                    __delay_cycles(2000000);
                    P1OUT &= ~BIT6;
                    P1OUT |= BIT0;
                    P1OUT |= BIT7;
                    __delay_cycles(100000);
                    P1OUT &= ~BIT0;
                    P1OUT &= ~BIT7;
                }
            } else if (x_axis < x_axis_init*0.98 || z_axis < z_axis_init*0.80 ||
                       x_axis > x_axis_init*1.05) {
                pin1 = 0;
                pin2 = 0;
                P1OUT |= BIT6;
                UARTSendArray("X");                         // Send incorrect password to App
                UARTSendArray("\n");
                __delay_cycles(2000000);
                P1OUT &= ~BIT6;
                P1OUT |= BIT0;
                P1OUT |= BIT7;
                __delay_cycles(100000);
                P1OUT &= ~BIT0;
                P1OUT &= ~BIT7;
            }
        } else if (x_axis < x_axis_init*0.98 || z_axis < z_axis_init*0.80 ||
                   z_axis > z_axis_init*1.20) {
            pin1 = 0;
            P1OUT |= BIT6;
            UARTSendArray("X");                             // Send incorrect password to App
            UARTSendArray("\n");
            __delay_cycles(2000000);
            P1OUT &= ~BIT6;
            P1OUT |= BIT0;
            P1OUT |= BIT7;
            __delay_cycles(100000);
            P1OUT &= ~BIT0;
            P1OUT &= ~BIT7;
        }
    }
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    data = UARTReceive;                                     // Receives data from UART RXD
    unsigned int bytesToSend = strlen((const char*) data);  // Number of bytes in data;

    switch(data){                                           // You chose "data"
        case 'E':   // Example Case for debugging Serial Terminal
        {
            UARTSendArray("Example Text");
            UARTSendArray("\n");
        }
        break;

    }

    IFG2 &= ~UCA0RXIFG;             // Turn off interrupt flag

}

#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
}

void configureADC(){
    ADC10CTL0 = SREF_0 + ADC10ON + ADC10IE;
}

void getanalogvalues()
{
    ADC_Reading_X();
    ADC_Reading_Y();
    ADC_Reading_Z();
}

// Read X axis Accelerometer
void ADC_Reading_X(){
    i = 0, x_axis = 0;
    for(i=0;i<5;i++){
        ADC10CTL0 &= ~ENC;
        ADC10CTL1 = INCH_3 + CONSEQ_0;
        ADC10AE0 |= BIT3;
        ADC10CTL0 |= ENC + ADC10SC;
        while(ADC10CTL1 & BUSY);
        x_axis = ADC10MEM;
    }
    x_axis = x_axis/5;
}

// Read Y axis Accelerometer
void ADC_Reading_Y(){
    i=0, y_axis = 0;
    for (i=0;i<5;i++) {
        ADC10CTL0 &= ~ENC;
        ADC10CTL1 = INCH_4 + CONSEQ_0;
        ADC10AE0 |= BIT4;
        ADC10CTL0 |= ENC + ADC10SC;
        while(ADC10CTL1 & BUSY);
        y_axis = ADC10MEM;
    }
    y_axis = y_axis/5;
}

// Read Z axis Accelerometer
void ADC_Reading_Z(){
    i = 0, z_axis=0;
    for(i=0; i<5; i++) {
        ADC10CTL0 &= ~ENC;
        ADC10CTL1 = INCH_5 + CONSEQ_0;
        ADC10AE0 |= BIT5;
        ADC10CTL0 |= ENC + ADC10SC;
        while(ADC10CTL1 & BUSY);
        z_axis = ADC10MEM;
    }
    z_axis = z_axis/5;
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
