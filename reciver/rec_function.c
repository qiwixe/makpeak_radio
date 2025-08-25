#include <mega8.h>
#include <string.h>
#include <delay.h>
#include <stdio.h>
#include "rec_function.h"

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
    unsigned int sum = 0;
    unsigned int sum_from_str = -1;
    int i = 0;
    int RFID_PACKET_LENGTH = 18;
    
    // считаем сумму оставшейся части строки (после "!XX")
    // суммируем ASCII-коды с позиции 3 до последнего символа перед '*'
    for (i = 3; i < RFID_PACKET_LENGTH; i++) {
        sum += buffer[i];
    }
    // читаем контрольную сумму (первые 2 символа после "!")
    sum_from_str = (buffer[1] - '0') * 10 + (buffer[2] - '0');
    // сравниваем
    //printf("%d\r\n",sum & 0xFF);
    //printf("%d\r\n",sum_from_str);
    return ((sum & 0xFF) == sum_from_str);
}
