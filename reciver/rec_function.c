#include <mega8.h>
#include <string.h>
#include <delay.h>
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
    unsigned char checksum = 0;
    unsigned char id_byte;
    unsigned char i = 0;
    unsigned char received_checksum;

    // XOR ������ 5 ���� ID (10 �������� -> 5 ����)
    for (i = 1; i < 11; i += 2) {
        id_byte = hex2byte(buffer[i], buffer[i+1]);
        checksum ^= id_byte;
    }

    // ��������� ����� ����������� ����� �� ASCII
    received_checksum = hex2byte(buffer[11], buffer[12]);

    return (checksum == received_checksum);
}
// ����������� ����� � ������ ����������� �������
void my_ltoa(unsigned long value, char *str) {
    char buffer[20]; // ��������� ����� ��� ����
    char i = 0;
    char j = 0;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    // ��������� ����� �� ����� � �����
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    // �������������� ������
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}
void hex_id_to_decimal_string(char *hex_str, char *dec_str) {
    unsigned long id = 0;
    unsigned char i;
    // ����������� 8 HEX-�������� � 4 ���� � �������� ��� ���� �����
    for (i = 0; i < 8; i += 2) {
        id = (id << 8) | hex2byte(hex_str[i], hex_str[i + 1]);
    }

    // ����������� ����� � ������ ����������� �������
    my_ltoa(id, dec_str);
}