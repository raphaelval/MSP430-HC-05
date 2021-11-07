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
void configureADC();
void getanalogvalues();
int getADCValue(int port);
int getEngineTemp();

static unsigned char data;
int flag = 0;
int i = 0;
int temp = 0;
int temproom = 0;
int ADCReading [1];

void main(void)

{
    WDTCTL = WDTPW + WDTHOLD;                   // Stop WDT

    P1DIR |= BIT6;                          // Set the LEDs on P1.0, P1.6 as outputs
    P1OUT |= BIT6;                          // Set the P1.6 LED

    BCSCTL1 = CALBC1_1MHZ;                      // Set DCO to 1MHz
    DCOCTL = CALDCO_1MHZ;                       // Set DCO to 1MHz

    /* Configure hardware UART */
    P1SEL = BIT1 + BIT2;                       // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2;                      // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                       // Use SMCLK
    UCA0BR0 = 104;                              // Set baud rate to 9600 with 1MHz clock (Data Sheet 15.3.13)
    UCA0BR1 = 0;                                // Set baud rate to 9600 with 1MHz clock
    UCA0MCTL = UCBRS0;                          // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                       // Initialize USCI state machine
    IE2 |= UCA0RXIE;                            // Enable USCI_A0 RX interrupt

    __delay_cycles(2000000);

    configureADC();

    __delay_cycles(250);
    getanalogvalues();
    temproom = temp;
    __delay_cycles(250);

    //__bis_SR_register(LPM0_bits | GIE);         // enter LPM0 with interrupt enable

    for(;;){

        getanalogvalues();

        if ( temp > temproom * 1.02 ) {
            P1OUT |=  BIT6;
            __delay_cycles(200);
        }    // LED on
        else if(temp < temproom * 1.01 ){
            P1OUT &= ~BIT6;
            __delay_cycles(200);
        }    // LED off

    }

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
        ADC10SA = (unsigned)&ADCReading[0];           //RAM Address of ADC Data, must be reset every conversion
        ADC10CTL0 |= (ENC | ADC10SC);                 //Start ADC Conversion
        while (ADC10CTL1 & BUSY);                     //Wait while ADC is busy
        temp += ADCReading[0];
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

/****************************************************************************************
 * This function takes in the port number and then returns the ADC value of the
 * requested port. Note that the MSP430G2553 only reads ADC values from P1.x. To assign
 * a port, simply use a 4-digit hex number, with the desired port number as the first
 * digit. Example: I want to use port 1.1, so I pass in the value 0x1000 as the port
 * value.
 ***************************************************************************************/
/*int getADCValue(int port)
{
    int ADCValue = 0;

    ADC10CTL0 = CONSEQ_1; //Various analog reading selections
    ADC10CTL1 = port; //Get data from the selected port
    ADC10AE0 = port; // Signal that we are ready to start
    ADC10CTL0 |= ADC10SHT_2 | MSC | ADC10ON; // Sampling and conversion start

    while (ADC10CTL1 & 0x0001); // Wait for conversion to complete - ADC10BUSY?
    ADCValue = ADC10MEM; // Read ADC value

    return ADCValue;
}*/
/****************************************************************************************
 * This function returns the current engine temperature by checking the current voltage
 * across the resistor bridge. The voltage is sensed via an ADC port. The port can
 * be redefined by changing the engineTempPort value.
 ***************************************************************************************/
/*int getEngineTemp()
{
    const int port1_3 = BIT3; //This is Temporary! Change this back to port 1.6 when we are ready for final implementation
    //temp = 0;

    ADCReading = getADCValue(port1_3);

    //currentTemp /= 6.8; //Until I calibrate the sensor, this is just a wild guess.

    return ADCReading; //Value is returned as a Fahrenheit temperature.
}*/

/****************************************************************************************
 * As above, this function fetches the boost pressure from an ADC port, and then converts
 * the value to a value in PSI. boostPort indicates which port is being read from.
 ***************************************************************************************/
/*int getBoostPressure()
{
    const int boostPort = 0x7000;
    int currentBoost = 0;

    currentBoost = getADCValue(boostPort);

    currentBoost /= 399;

    return currentBoost;
}*/
