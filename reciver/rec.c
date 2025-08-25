#include <mega8.h>
#include <delay.h>
#include <string.h>
#include <stdio.h>
#include "rec_function.h"
// Declare your global variables here
#define RFID_PACKET_LENGTH 18
char RFID_buffer[RFID_PACKET_LENGTH];
char RAM_RFID_buffer[RFID_PACKET_LENGTH];
unsigned char RFID_index = 0;


// ������������� Timer1
void timer1_init(void) {
    // ����� CTC: WGM12 = 1
    TCCR1B |= (1 << WGM12);

    // ������������: 1024
    TCCR1B |= (1 << CS12) | (1 << CS10);

    // ������� �� OCR1A = 15624 -> 2 ������� ��� 8 ��� � �������� 1024
    OCR1A = 3906;

    // ��������� ���������� �� ����������
    TIMSK |= (1 << OCIE1A);
    
    // ��������� ���������� ����������
    #asm("sei")
}
// ���������� �� ���������� � OCR1A
interrupt [TIM1_COMPA] void timer1_compa_isr(void) {
    memset(RAM_RFID_buffer, 0, RFID_PACKET_LENGTH);
}

void main(void)
{
char ch;
char tag[16];
char dec_str[15];
// UART
{
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
}
timer1_init();
while (1) {
        ch = uart_receive();
        //�������� ����� ����� (!)
        if (ch == '!') {
            RFID_index = 0;
            RFID_buffer[RFID_index++] = ch;

            // ��������� ���������� 17 ����
            while (RFID_index < RFID_PACKET_LENGTH) {
                RFID_buffer[RFID_index++] = uart_receive();
            }

            // ��������� ���� ���� (*)
            if (RFID_buffer[17] == '*' && check_checksum(RFID_buffer)) {                    //���� ����������� ����� �����
                if (memcmp(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH) != 0){         //����� ���������� �� ����������
                memcpy(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH);                   //���������� �����
                strncpy(tag, &RFID_buffer[3], 15);                                          //��������� ����������� �����
                tag[16] = '\0';                                                             //���������� ����� ����� ������
                uart_send(tag);                                                             //����������� ��������� ��� ����������� �����
            }
        }
    }
}
}
