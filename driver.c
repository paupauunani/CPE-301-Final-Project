/* written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies */

//What libraries can be used
//If using on VSC, may need to configure file path for library; Already in Arduino IDE
//#include <LiquidCrystal.h> - had to comment out

//Register values for GPIO
//Register B
//Pin 12 (Output) - Multicolor LED (PB6)
//Pin 10 (Input) - Humidity Sensor (PB4)
unsigned char* port_b = (unsigned char *) 0x25;
unsigned char* ddr_b = (unsigned char *) 0x24;
//Only use pin_b when dealing with input
volatile unsigned char* pin_b = (unsigned char*) 0x23;

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

/* registers required for adc functionality */
volatile unsigned int* ADMUX = (unsigned char*) 0x7C;
volatile unsigned int* ADCSRB = (unsigned char*) 0x7B;
volatile unsigned int* ADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* ADCH = (unsigned char*) 0x79;
volatile unsigned char* ADCL = (unsigned char*) 0x78;

/* registers required for usart functionality */
volatile unsigned char* UBRR0H = (unsigned char*) 0xC5;
volatile unsigned char* UBRR0L = (unsigned char*) 0xC4;
volatile unsigned char* UCSR0C = (unsigned char*) 0xC2;
volatile unsigned char* UCSR0B = (unsigned char*) 0xC1;

#define fosc 16000000 
#define baud 9600

void adc_init(void)
{       /* clear adc multiple selection register */
        *ADMUX = 0b00000000;
        /* set voltage reference */
        *ADMUX |= 0b01000000;
        /* clear adc control and status register b */
        *ADCSRB = 0b00000000;
        /* clear adc control and status register a */
        *ADCSRA = 0b00000000;
        /* enable adc */
        *ADCSRA |= 0b10000000;
        /* set pre-scaler */
        *ADCSRA |= 0b00000111;
} /* adc_init */
unsigned int adc_read(unsigned char channel)
{       /* clear input channel */
        *ADMUX &= 0b11100000;
        /* set input channel */
        *ADMUX |= channel & 0b00000111;
        /* start adc conversion */
        *ADCSRA |= 0b01000000;
        /* wait until conversion is completed */
        while(*ADCSRA & 0b01000000);
        /* return conversion result */
        return *ADCL | (*ADCH << 8);
} /* adc_read */
void usart_init(unsigned int ubbr)
{       /* set baud rate */
        *UBRR0H = (unsigned char)ubbr >> 8;
        *UBRR0L = (unsigned char)ubbr;
        /* enable RXEN0 and TXEN0 */
        *UCSR0B = 0b00011000;
        /* set frame format: 8 data bits, 2 stop bits */
        *UCSR0C = 0b00001110;
} /* usart_init */

void setup(void)
{
    USART_Init((fosc / (16 * baud)) - 1);
    art_init();
}

void loop(void)
{

}
{       (void)adc_read(0x00);
        
} /* loop */