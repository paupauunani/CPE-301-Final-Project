/* written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies */

/* must manually configure include path if compiling outside Arduino IDE */
// #include <LiquidCrystal.h>
/* must manually configure include path if compiling outside Arduino IDE */
// #include <LiquidCrystal.h>

/* registers required for adc functionality */
volatile unsigned int* myADMUX = (unsigned char*) 0x7C;
volatile unsigned int* myADCSRB = (unsigned char*) 0x7B;
volatile unsigned int* myADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* myADCH = (unsigned char*) 0x79;
volatile unsigned char* myADCL = (unsigned char*) 0x78;

/* registers required for usart functionality */
volatile unsigned char* myUDRO = (unsigned char*) 0xC6;
volatile unsigned char* myUBRR0H = (unsigned char*) 0xC5;
volatile unsigned char* myUBRR0L = (unsigned char*) 0xC4;
volatile unsigned char* myUCSR0C = (unsigned char*) 0xC2;
volatile unsigned char* myUCSR0B = (unsigned char*) 0xC1;
volatile unsigned char* myUSCR0A = (unsigned char*) 0xC0;

#define fosc 16000000 
#define baud 9600
#define RDA 0x80
#define TBE 0x20

void adc_init()
{       /* clear adc multiple selection register */
        *myADMUX = 0b00000000;
        *myADMUX = 0b00000000;
        /* set voltage reference */
        *myADMUX |= 0b01000000;
        *myADMUX |= 0b01000000;
        /* clear adc control and status register b */
        *myADCSRB = 0b00000000;
        *myADCSRB = 0b00000000;
        /* clear adc control and status register a */
        *myADCSRA = 0b00000000;
        *myADCSRA = 0b00000000;
        /* enable adc */
        *myADCSRA |= 0b10000000;
        *myADCSRA |= 0b10000000;
        /* set pre-scaler */
        *myADCSRA |= 0b00000111;
        *myADCSRA |= 0b00000111;
} /* adc_init */
unsigned int adc_read(unsigned char adc_channel)
{       /* clear input channel */
        *myADMUX &= 0b11100000;
        *myADMUX &= 0b11100000;
        /* set input channel */
        *myADMUX |= adc_channel & 0b00000111;
        /* start adc conversion */
        *myADCSRA |= 0b01000000;
        *myADCSRA |= 0b01000000;
        /* wait until conversion is completed */
        while(*myADCSRA & 0b01000000);
        while(*myADCSRA & 0b01000000);
        /* return conversion result */
        return *myADCL | (*myADCH << 8);
        return *myADCL | (*myADCH << 8);
} /* adc_read */
void usart_init(unsigned int ubbr)
{       /* set baud rate */
        *myUBRR0H = (unsigned char)ubbr >> 8;
        *myUBRR0L = (unsigned char)ubbr;
        *myUBRR0H = (unsigned char)ubbr >> 8;
        *myUBRR0L = (unsigned char)ubbr;
        /* enable RXEN0 and TXEN0 */
        *myUCSR0B = 0b00011000;
        *myUCSR0B = 0b00011000;
        /* set frame format: 8 data bits, 2 stop bits */
        *myUCSR0C = 0b00001110;
        *myUCSR0C = 0b00001110;
} /* usart_init */
void setup(void)
{       usart_init((fosc / (16 * baud)) - 1);
        adc_init();
        u0_init(9600);
} /* setup */
void loop(void)
{       (void)adc_read(0x00);
    unsigned char c;
    while(u0_kbhit() == 0){};
    c = u0_getchar();
    u0_putChar(c);
} /* loop */

void u0_init(unsigned long u0baud){
    unsigned int tbaud;
    tbaud = (fosc / 16 / u0baud - 1);
    *myUSCR0A = 0x20;
    *myUCSR0B = 0x18;
    *myUCSR0C = 0x06;
    *myUBRR0L = tbaud;
}

//Reads RDA status bit from USART0 & return true if set
unsigned char u0_kbhit(){
    if(*myUSCR0A & RDA) return 1;
    else return 0;
}

//read input from USART0 input buffer
unsigned char u0_getchar(){
    return *myUDRO;
}

//Waits for TBE to be set in UCSROA and write character to transmit buffer
void u0_putChar(unsigned char u0pdata){
    while(!(*myUSCR0A & TBE));
    *myUDRO = u0pdata;
}

// void putstr(unsigned char* str)
// {       while(*str)
//         {       putchar(*str++);
//         }
//         putchar('\n');
// }