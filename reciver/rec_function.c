#include <mega8.h>
#include <string.h>
#include <delay.h>
#include <stdio.h>
#include "rec_function.h"

// ��������� ������
char uart_receive(void) {
    while (!(UCSRA & (1 << RXC)));
    return UDR;
}
// �������� �����
void uart_transmit(char data) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}
// �������� ������
void uart_send(char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}
// �������� ������ ��������� ���
void uart_send_times(char *str, char count) {
    unsigned char i = 0;
    for (i = 0; i < count; i++) {
    while (*str) {
    uart_transmit(*str++);
    }
    delay_ms(50);
    } 

}
// �������������� 2 �������� HEX � ����
unsigned char hex2byte(char high, char low) {
    unsigned char value = 0;

    if (high >= '0' && high <= '9') value = (high - '0') << 4;
    else if (high >= 'A' && high <= 'F') value = (high - 'A' + 10) << 4;

    if (low >= '0' && low <= '9') value |= (low - '0');
    else if (low >= 'A' && low <= 'F') value |= (low - 'A' + 10);

    return value;
}
// �������� ����������� �����
char check_checksum(char *buffer) {
    unsigned int sum = 0;
    unsigned int sum_from_str = -1;
    int i = 0;
    int RFID_PACKET_LENGTH = 18;
    
    // ������� ����� ���������� ����� ������ (����� "!XX")
    // ��������� ASCII-���� � ������� 3 �� ���������� ������� ����� '*'
    for (i = 3; i < RFID_PACKET_LENGTH; i++) {
        sum += buffer[i];
    }
    // ������ ����������� ����� (������ 2 ������� ����� "!")
    sum_from_str = (buffer[1] - '0') * 10 + (buffer[2] - '0');
    // ����������
    //printf("%d\r\n",sum & 0xFF);
    //printf("%d\r\n",sum_from_str);
    return ((sum & 0xFF) == sum_from_str);
}
