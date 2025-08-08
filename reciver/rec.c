#include <mega8.h>
#include <delay.h>
#include <string.h>
#include "rec_function.h"
// Declare your global variables here
#define RFID_PACKET_LENGTH 14
char RFID_buffer[RFID_PACKET_LENGTH];
char RAM_RFID_buffer[RFID_PACKET_LENGTH];
unsigned char RFID_index = 0;


// Инициализация Timer1
void timer1_init(void) {
    // Режим CTC: WGM12 = 1
    TCCR1B |= (1 << WGM12);

    // Предделитель: 1024
    TCCR1B |= (1 << CS12) | (1 << CS10);

    // Считаем до OCR1A = 15624 -> 2 секунды при 8 МГц и делителе 1024
    OCR1A = 3906;

    // Разрешить прерывание по совпадению
    TIMSK |= (1 << OCIE1A);
    
    // Разрешить глобальные прерывания
    #asm("sei")
}
// Прерывание по совпадению с OCR1A
interrupt [TIM1_COMPA] void timer1_compa_isr(void) {
    memset(RFID_buffer, 0, RFID_PACKET_LENGTH);
}

void main(void)
{
// Declare your local variables here
char ch;
char tag[10];
char dec_str[16];
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
            // Если контрольная сумма верна отправляет и запоминаем на 0.5 секунды, после сбрасываем память
                memcpy(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH);
                
                strncpy(tag, &RFID_buffer[3], 8);  // Сохраняем 8 HEX символов без версии
                tag[8] = '\0';
                hex_id_to_decimal_string(tag, dec_str);
                uart_send(dec_str);
            }
        }
        
        
    }
}

