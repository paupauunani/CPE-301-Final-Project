/* written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies */

/* must manually configure include path if compiling outside Arduino IDE */
// #include <LiquidCrystal.h>

/* registers required for adc functionality */
volatile unsigned int* myADMUX = (unsigned char*) 0x7C;
volatile unsigned int* myADCSRB = (unsigned char*) 0x7B;
volatile unsigned int* myADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* myADCH = (unsigned char*) 0x79;
volatile unsigned char* myADCL = (unsigned char*) 0x78;

/* registers required for usart functionality */
volatile unsigned char* myUBRR0H = (unsigned char*) 0xC5;
volatile unsigned char* myUBRR0L = (unsigned char*) 0xC4;
volatile unsigned char* myUCSR0C = (unsigned char*) 0xC2;
volatile unsigned char* myUCSR0B = (unsigned char*) 0xC1;

#define fosc 16000000 
#define baud 9600

void adc_init(void)
{       /* clear adc multiple selection register */
        *myADMUX = 0b00000000;
        /* set voltage reference */
        *myADMUX |= 0b01000000;
        /* clear adc control and status register b */
        *myADCSRB = 0b00000000;
        /* clear adc control and status register a */
        *myADCSRA = 0b00000000;
        /* enable adc */
        *myADCSRA |= 0b10000000;
        /* set pre-scaler */
        *myADCSRA |= 0b00000111;
} /* adc_init */
unsigned int adc_read(unsigned char adc_channel)
{       /* clear input channel */
        *myADMUX &= 0b11100000;
        /* set input channel */
        *myADMUX |= adc_channel & 0b00000111;
        /* start adc conversion */
        *myADCSRA |= 0b01000000;
        /* wait until conversion is completed */
        while(*myADCSRA & 0b01000000);
        /* return conversion result */
        return *myADCL | (*myADCH << 8);
} /* adc_read */
void usart_init(unsigned int ubbr)
{       /* set baud rate */
        *myUBRR0H = (unsigned char)ubbr >> 8;
        *myUBRR0L = (unsigned char)ubbr;
        /* enable RXEN0 and TXEN0 */
        *myUCSR0B = 0b00011000;
        /* set frame format: 8 data bits, 2 stop bits */
        *myUCSR0C = 0b00001110;
} /* usart_init */
void setup(void)
{       usart_init((fosc / (16 * baud)) - 1);
        adc_init();
} /* setup */
void loop(void)
{       (void)adc_read(0x00);
} /* loop */

// void putstr(unsigned char* str)
// {       while(*str)
//         {       putchar(*str++);
//         }
//         putchar('\n');
// }