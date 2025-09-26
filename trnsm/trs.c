#include <mega8.h>
#include <delay.h>
#include <string.h>
#include <stdio.h>
#include "trs_function.h"
#define RFID_PACKET_LENGTH 14
#define DEBOUNCE_MS 1000        

//���� ����������
#define LED_Reader PORTD.6      //��������� ������ (PD6)
#define POWER_Reader PORTB.2    //������� ������ !�������������! (PB2)
#define POWER_Radio PORTB.0     //������� ����� ������ !�������������! (PB0)

//���������� ����������
char RAM_RFID_buffer[RFID_PACKET_LENGTH];
char RFID_buffer[RFID_PACKET_LENGTH];
char MESSAGE_BUFFER[18];
unsigned char RFID_index = 0;
unsigned int timer = 0;
unsigned int ms_counter = 0;

//���������� ���������� �� ������� (PD3)
interrupt [EXT_INT1] void ext_int1_isr(void)
{
    ms_counter = 0; //����� ������� ��������
}
//���������� ���������� �� Timer1
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
    timer++;
    if (timer >= 7) {           //1 ��� ������� 8.5 ���
        POWER_Reader = 1;       //��������� ����� !�������������! (PB2)
        LED_Reader = 0;         //��������� ��������� ������ (PD6)     
        timer = 0;              //����� �������� 
    }
}
//������2
interrupt [TIM2_COMP] void timer2_compare_isr(void)
{
    if (PIND.3 == 0)
    {   
        ms_counter++;          //������ ��������
    } 
    if (ms_counter == DEBOUNCE_MS)    //���� ��� � ���������� �� ������� > ������� ��������
    {
        POWER_Reader = 0;       //�������� ����� !�������������! (PB2)
        LED_Reader = 1;         //�������� ��������� ������ (PD6)
        timer = 0;              //�������� �������
    }
}
void main(void){
char ch;
char msg[18]; 
//����� �����������
char number_trs[] = "33";
//UART
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
//������
{    
    //PB2 ��� �����
    DDRB |= (1<<2);
    PORTB &= ~(1<<2);  // ���������� ��������
    
    //PD2 ��� �����
    DDRD |= (1<<6);
    PORTD &= ~(1<<6);  // ���������� ��������
    
    //PD3 (������) ��� ���� � ���������
    DDRD &= ~(1<<3);
    PORTD |= (1<<3);

    //��������� INT1
    MCUCR |= (1<<ISC11); // ���������� �� �����
    GICR  |= (1<<INT1);
}    
// ��������� ADC1
{
    ADMUX = (1<<REFS0) | (1<<MUX0);  // AVCC, ADC1
    ADCSRA = (1<<ADEN) | (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // �������� 128
}
// ��������� �������
{
    TCCR1A = 0x00;
    TCCR1B = (1<<CS12) | (0<<CS11) | (1<<CS10);
    TIMSK |= (1<<TOIE1);
}
// ��������� ������� ������ �� ��������
{
    TCCR2 = (1<<WGM21) | (1<<CS01) | (1<<CS00);
    OCR2 = 124;          // ��� 8 ��� ��� 1 ��
    TIMSK |= (1<<OCIE2); // ��������� ���������� �� ����������
}
//�����������
{
  //PB0 ��� �����
    DDRB |= (1<<0);
}

#asm("sei") // ��������� ��������� ����������    
while (1) {
        ch = uart_receive();
        // ��������� ����� ����
        if (ch == 0x02) {
            RFID_index = 0;
            RFID_buffer[RFID_index++] = ch;

            // ��������� ���������� 13 ����
            while (RFID_index < RFID_PACKET_LENGTH) {
                RFID_buffer[RFID_index++] = uart_receive();
            }

            // ��������� ���� ����
            if (RFID_buffer[13] == 0x03 && check_checksum(RFID_buffer)) {                   //���� ����������� ����� �����
                if (memcmp(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH) != 0){         //����� ���������� �� ����������
                memcpy(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH);                   //���������� �����
                sprintf(msg, "=%s+%s+%d*", number_trs, get_tag(RFID_buffer), adc1_read());  //������������ ������ =�����+�����+�����*
                build_message(msg, MESSAGE_BUFFER);                                         //������ ����������� ����� � ������������ ������ !�����=�����+�����+�����*
                POWER_Radio = 0;                                                            //��������� ����� ������ !�������������! (PB0)
                uart_send_times(MESSAGE_BUFFER,5);                                          //�������� ��������� 5 ���
                POWER_Radio = 1;                                                            //���������� ����� ������ !�������������! (PB0)
                POWER_Reader = 1;                                                           //��������� ����� !�������������! (PB2)
                LED_Reader = 0;                                                             //��������� ��������� ������ (PD6)
            }
            }
        }  
    }
}

