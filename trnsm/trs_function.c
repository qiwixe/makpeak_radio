#include <mega8.h>
#include <string.h>
#include <delay.h>
#include "trs_function.h"

// получение байтов
char uart_receive(void) {
    while (!(UCSRA & (1 << RXC)));
    return UDR;
}
// отправка байта
void uart_transmit(char data) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}
// отправка строки
void uart_send(char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}
// отправка строки несколько раз
void uart_send_times(char *str, char count) {
    unsigned char i = 0;
    for (i = 0; i < count; i++) {
    while (*str) {
    uart_transmit(*str++);
    }
    delay_ms(50);
    } 

}
// Преобразование 2 символов HEX в байт
unsigned char hex2byte(char high, char low) {
    unsigned char value = 0;

    if (high >= '0' && high <= '9') value = (high - '0') << 4;
    else if (high >= 'A' && high <= 'F') value = (high - 'A' + 10) << 4;

    if (low >= '0' && low <= '9') value |= (low - '0');
    else if (low >= 'A' && low <= 'F') value |= (low - 'A' + 10);

    return value;
}
// Проверка контрольной суммы
char check_checksum(char *buffer) {
    unsigned char checksum = 0;
    unsigned char id_byte;
    unsigned char i = 0;
    unsigned char received_checksum;

    // XOR первых 5 байт ID (10 символов -> 5 байт)
    for (i = 1; i < 11; i += 2) {
        id_byte = hex2byte(buffer[i], buffer[i+1]);
        checksum ^= id_byte;
    }

    // Получение байта контрольной суммы из ASCII
    received_checksum = hex2byte(buffer[11], buffer[12]);

    return (checksum == received_checksum);
}

