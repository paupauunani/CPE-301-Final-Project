//Written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies

//What libraries can be used
//If using on VSC, may need to configure file path for library; Already in Arduino IDE
//#include <LiquidCrystal.h> - had to comment out

//Register values for GPIO need to decide what pins we are using for input/output

//Register values for Timer Theory
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

//Register values for USART - used in output on Serial Monitor
volatile unsigned char *myUCSR0A = (unsigned char *) 0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *) 0x00C6;

//Register values for ADC (analog to digital conversion)
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char* UBRR0H = (unsigned char*) 0xC5;
volatile unsigned char* UBRR0L = (unsigned char*) 0xC4;
volatile unsigned char* UCSR0C = (unsigned char*) 0xC2;
volatile unsigned char* UCSR0B = (unsigned char*) 0xC1;

#define fosc 16000000 
#define baud 9600

void usart_init(unsigned int ubbr)
{       /* set baud rate */
        UBRR0H = (unsigned char)ubbr >> 8;
        UBRR0L = (unsigned char)ubbr;
        /* enable RXEN0 and TXEN0 */
        UCSR0B = 0b00011000;
        /* set frame format: 8 data bits, 2 stop bits */
        UCSR0C = 0b00001110;
} /* usart_init */
void setup(void)
{       USART_Init((fosc / (16 * baud)) - 1);
} /* setup */
void loop(void)
{
} /* loop */

void ADC_init()
{
    //setup A register
    *my_ADCSRA |= 0x80;
    *my_ADCSRA &= 0xDF;
    *my_ADCSRA &= 0xF7;
    *my_ADCSRA &= 0xF8;

    //setup B register
    *my_ADCSRB &= 0xF7;
    *my_ADCSRB &= 0xF8;

    //setup MUX
    *my_ADMUX &= 0x7F;
    *my_ADMUX |= 0x40;
    *my_ADMUX &= 0xDF;
    *my_ADMUX &= 0xE0;
}

unsigned int ADC_read(unsigned char adc_channel_num)
{
    *my_ADMUX &= 0xE0;
    *my_ADCSRB &= 0xF7;
    *my_ADMUX += adc_channel_num;
    *my_ADCSRA |= 0x40;
    while((*my_ADCSRA & 0x40) != 0);
        unsigned int val = (*my_ADC_DATA & 0x3FF);
    return val;
}