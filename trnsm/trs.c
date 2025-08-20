#include <mega8.h>
#include <delay.h>
#include <string.h>
#include "trs_function.h"

#define RFID_PACKET_LENGTH 14
char RAM_RFID_buffer[RFID_PACKET_LENGTH];
char RFID_buffer[RFID_PACKET_LENGTH];
unsigned char RFID_index = 0;

// Обработчик прерывания от INT1 (PD3)
interrupt [EXT_INT1] void ext_int1_isr(void)
{
    if ((PIND & (1<<3)) == 0)  // проверяем PIND.3
    {
        PORTB |= (1<<2);       // включаем PB2
    }
    else
    {
        PORTB &= ~(1<<2);      // выключаем PB2
    }
}

void main(void)
{
// Declare your local variables here
char ch;
// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud Rate: 9600
UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
UCSRB=(0<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (1<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
UBRRH=0x00;
UBRRL=0x33;

    // Настройка PB2 как выход
    DDRB |= (1<<2);
    PORTB &= ~(1<<2);       // по умолчанию выключен

    // Настраиваем PD3 как вход с подтяжкой
    DDRD &= ~(1<<3);
    PORTD |= (1<<3);           // включаем pull-up

    // Настройка внешнего прерывания INT1
    MCUCR |= (1<<ISC10);    // прерывание по любому изменению уровня
    GICR  |= (1<<INT1);     // разрешаем INT1
    #asm("sei")             // глобально разрешаем прерывания
    
while (1) { 
    ch = uart_receive();
        if (ch == 0x02) {
            RFID_index = 0;
            RFID_buffer[RFID_index++] = ch;

            // Считываем оставшиеся 13 байт
            while (RFID_index < RFID_PACKET_LENGTH) {
                RFID_buffer[RFID_index++] = uart_receive();
            }

            // Проверяем стоп байт
            if (RFID_buffer[13] == 0x03 && check_checksum(RFID_buffer) && memcmp(RFID_buffer, RAM_RFID_buffer, RFID_PACKET_LENGTH) != 0) {
            // Если контрольная сумма верна,метка отличается от предыдущей, запоминаем метку и отсылаем
                memcpy(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH);
                uart_send_times(RFID_buffer,5);
            }
        }  
    }
}

