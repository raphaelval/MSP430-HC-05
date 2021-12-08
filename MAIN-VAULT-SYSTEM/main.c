/**
 * main.c
 * Sample Code showing the use of: UART, Rx Bluetooth Input, Tx Bluetooth Output
 * Raphael Valente
 */

#include <stdio.h>
#include <msp430.h> 
#include <string.h>

/*  UART  */
#define UARTReceive UCA0RXBUF;                  // Set RXD as UARTReceive

// Prototypes
void servoUnlock();
void servoLock();
void int_char(int tempCopy);
void UARTSendTemp(char *tempArray);
void UARTSendArray(unsigned char *TxArray);
void UARTSendChar(unsigned char *c);
void configureADC();
void getanalogvalues();
void ADC_Reading_temp();
void ADC_Reading_X();
void ADC_Reading_Y();
void ADC_Reading_Z();
void ADC_Reading_touch();

// RXD Data variable
static unsigned char data;

// Temperature String
char tempStr[3];

// Flags
int alarmFlag = 0;          // 0: Alarm Off               | 1: Motion is Detected/Alarm On
int lockFlag = 0;           // 0: System is Unlocked      | 1: System is Fully Locked
int unlockFlag = 0;         // 0: Vault Physically Locked | 1: Vault Physically Unlocked
int tempFlag = 0;           // 0: Temp Alarm Off          | 1: Temp Alarm On
int touchFlag = 0;          // 0: No Touch is Detected    | 1: Touch is Detected

// Iterator
int i = 0;

// Sensor Variables
int temp = 0;
int touch = 0;
int temproom = 0;
int touchroom = 0;
int x_axis = 0, y_axis = 0, z_axis = 0;
int x_axis_init = 0, y_axis_init = 0, z_axis_init = 0;

/*  NOTES  */
// P1.0 IN: Temp Sensor
// P1.1 IN: RXD
// P1.2 OUT: TXD
// P1.3 IN: Accelerometer X-Axis
// P1.4 IN: Accelerometer Y-Axis
// P1.5 IN: Accelerometer Z-Axis
// P1.6 IN: Touch Sensor
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
    BCSCTL1= CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    /* Configure PWM */
    P2DIR |= BIT2;
    P2SEL |= BIT2;                             //selection for timer setting
    TA1CCR0 = 20000;                           //PWM period

    // Starts program in Locked Mode
    servoLock();
    lockFlag = 1;

    __delay_cycles(2000000);

    configureADC();

    __delay_cycles(250);
    getanalogvalues();

    // Initializes the starting readings
    temproom = temp;
    touchroom = touch;
    x_axis_init = x_axis;
    y_axis_init = y_axis;
    z_axis_init = z_axis;

    __delay_cycles(250);

    __enable_interrupt();

    for(;;){

        getanalogvalues();

        // toggles servo mechanism with touch sensor
        if (touch <= touchroom*0.70) {
            // unlocks door when unlocked through app
            if (touchFlag == 0 && lockFlag == 0 && unlockFlag == 0) {
                touchFlag = 1;
                servoUnlock();
                unlockFlag = 1;
                UARTSendArray("Door is unlocked");
                UARTSendArray("\n");
                __delay_cycles(2000000);    // delay touch 2 seconds after unlocking
            }
            // locks door after servo physically unlocks door
            if (touchFlag == 0 && unlockFlag == 1) {
                touchFlag = 1;
                servoLock();
                lockFlag = 1;
                unlockFlag = 0;
                UARTSendArray("System locked");
                UARTSendArray("\n");
                __delay_cycles(2000000);    // delay touch 2 seconds after locking
            }
        } // toggles flag to 0 when not touching
        else if (touch >= touchroom*0.9) {
            touchFlag = 0;
        }

        // sets alarmflag to 1 when movement is detected
        if(    (x_axis > x_axis_init*1.04 || x_axis < x_axis_init*0.96 ||
                y_axis > y_axis_init*1.04 || y_axis < y_axis_init*0.96 ||
                z_axis > z_axis_init*1.04 || z_axis < z_axis_init*0.96) &&
                lockFlag == 1) {
            if (alarmFlag == 0) {
                UARTSendArray("alert: Vault was displaced");
                UARTSendArray("\n");
                alarmFlag = 1;
            }
        }

        // toggles temp alarm flags
        if(temp > temproom*1.05) {
            // sets tempflag to 1
            if(tempFlag == 0) {
                UARTSendArray("alert: temperature is high");
                UARTSendArray("\n");
                tempFlag = 1;
            }
        } else if (temp < temproom*1.03 && alarmFlag == 0) {
            // sets tempflag to 0
            // turns off alarm if flag is 0
            tempFlag = 0;
            P1OUT &= ~BIT7;
        } else if (temp < temproom*1.03) {
            // set tempflag to 0 when temp is normal
            tempFlag = 0;
        }

        // Different alarm sounds for each condition
        // Both Temp and Movement
        if (alarmFlag == 1 && tempFlag == 1) {
            P1OUT ^=  BIT7;                       // Toggle Buzzer
            __delay_cycles(250000);               // Every 0.25 Second
        } // Movement
        else if (alarmFlag == 1 && tempFlag == 0) {
            P1OUT ^=  BIT7;                       // Toggle Buzzer
            __delay_cycles(500000);               // Every 0.5 Second
        } // Temp
        else if (alarmFlag == 0 && tempFlag == 1) {
            P1OUT |=  BIT7;                       // Toggle Buzzer
            __delay_cycles(100000);
            P1OUT &= ~BIT7;
            __delay_cycles(100000);
            P1OUT |=  BIT7;                       // Toggle Buzzer
            __delay_cycles(100000);
            P1OUT &= ~BIT7;
            __delay_cycles(500000);               // Every 0.25 Second
        }

    }

}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    data = UARTReceive;                                     // Receives data from UART RXD
    unsigned int bytesToSend = strlen((const char*) data);  // Number of bytes in data;

    switch(data){
        case 'u':   // Unlock
        {
            UARTSendArray("\n");
            // UNLOCK
            lockFlag = 0;
            alarmFlag = 0;
            P1OUT &= ~BIT7;
            UARTSendArray("System unlocked");
            UARTSendArray("\n");
        }
        break;
        case 'l':   // Lock
        {
            UARTSendArray("\n");
            // LOCK
            lockFlag = 1;
            unlockFlag = 0;
            alarmFlag = 0;
            P1OUT &= ~BIT7;
            servoLock();
            UARTSendArray("System locked");
            UARTSendArray("\n");
            __delay_cycles(2000000);                        // for 2 seconds
        }
        break;
        case 'A':   // Test Alarm
        {
            UARTSendArray("testing alarm");
            UARTSendArray("\n");
            P1OUT |=  BIT7;                                 // Toggle Buzzer
            __delay_cycles(1000000);                        // For 1 Second
            P1OUT &= ~BIT7;
        }
        break;
        case 'a':   // Shut off Alarm
        {
            UARTSendArray("Alarm is off");
            UARTSendArray("\n");
            alarmFlag = 0;
            P1OUT &= ~BIT7;                                 // Turn off LED P1.7
        }
        break;
        case 't':   // Send Converted Temperature
        {
            UARTSendArray("*");
            int tempCopy = temp/3.37;
            int_char(tempCopy);
            UARTSendTemp(tempStr);
            UARTSendArray("\n");
        }
        break;

    }

    IFG2 &= ~UCA0RXIFG;                                     // Turn off interrupt flag

}

#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
}

void servoUnlock(){
    TA1CCR1 = 2700;                                 //CCR1 PWM Duty Cycle
    TA1CCTL1 = OUTMOD_7;                            //CCR1 selection reset-set
    TA1CTL = TASSEL_2|MC_1;                         //SMCLK submain clock,upmode
}

void servoLock(){
    TA1CCR1 = 1600;
    TA1CCTL1 = OUTMOD_7;                            //CCR1 selection reset-set
    TA1CTL = TASSEL_2|MC_1;
}

/* Convert the int value to char string */
void int_char(int tempCopy)
{
    int s = 0;
    s = tempCopy%10;
    tempStr[1] = (char)(s+48);
    tempCopy = tempCopy/10;
    s = tempCopy%10;
    tempStr[0] = (char)(s+48);
    tempCopy = tempCopy/10;
    tempStr[2] = '\0';
}

void configureADC(){
    ADC10CTL0 = SREF_0 + ADC10ON + ADC10IE;
}

void getanalogvalues()
{
    ADC_Reading_temp();
    ADC_Reading_X();
    ADC_Reading_Y();
    ADC_Reading_Z();
    ADC_Reading_touch();
}

// Read Temp Sensor
void ADC_Reading_temp(){
    i = 0, temp = 0;
    for (i = 0; i<5; i++) {
        ADC10CTL0 &= ~ENC;
        ADC10CTL1 = INCH_0 + CONSEQ_0;
        ADC10AE0 |= BIT0;
        ADC10CTL0 |= ENC + ADC10SC;
        while(ADC10CTL1 & BUSY);
        temp += ADC10MEM;
    }

    temp = temp/5;
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

// Read Touch Sensor
void ADC_Reading_touch() {
    i = 0, touch=0;
    for(i=0; i<5; i++) {
        ADC10CTL0 &= ~ENC;
        ADC10CTL1 = INCH_6 + CONSEQ_0;
        ADC10AE0 |= BIT6;
        ADC10CTL0 |= ENC + ADC10SC;
        while(ADC10CTL1 & BUSY);
        touch = ADC10MEM;
    }
    touch = touch/5;
}

void UARTSendTemp(char *tempArray) {
    while(*tempArray){                        // Loop until StringLength == 0 and post decrement
        while(!(IFG2 & UCA0TXIFG));         // Wait for TX buffer to be ready for new data
        UCA0TXBUF = *tempArray;
        tempArray++;
    }
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
