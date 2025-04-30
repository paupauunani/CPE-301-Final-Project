// Written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies

#define FOSC 16000000
#define BAUD 9600

void USART_init(unsigned char ubrr)
{       UBRRH = (unsigned char)ubrr >> 8;
        UBBRL = (unsigned char)ubrr;
        UCSRB = (1 << RXEN) | (1 << TXEN);
        UCSRC = (1 << USBS) | (3 << UCSZ0);
}
void setup(void)
{       USART_init((FOSC / (16 * BAUD)) - 1);
}
void loop(void)
{
}