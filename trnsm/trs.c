#include <mega8.h>
#include <delay.h>
#include <string.h>
#include "trs_function.h"

#define RFID_PACKET_LENGTH 14
char RAM_RFID_buffer[RFID_PACKET_LENGTH];
char RFID_buffer[RFID_PACKET_LENGTH];
unsigned char RFID_index = 0;

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
                uart_send_times();
            }
        }  
    }
}

