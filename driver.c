// written by Alysia Carr, William Chuter-Davies, Shira Rotem, Paulane Tulop

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