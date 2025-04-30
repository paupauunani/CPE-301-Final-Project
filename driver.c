// written by Alysia Carr, William Chuter-Davies, Shira Rotem, Paulane Tulop

#define FOSC 16000000
#define BAUD 9600

void USART_init(unsigned char ubrr)
{       
}
void setup(void)
{       USART_init((FOSC / (16 * BAUD)) - 1);
}
void loop(void)
{
}