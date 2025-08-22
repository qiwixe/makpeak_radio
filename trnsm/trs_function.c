#include <mega8.h>
#include <string.h>
#include <delay.h>
#include <stdio.h>
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
    uart_send(str);
    delay_ms(50);
    } 
}
int adc1_read(){
    char r1 = 10000;
    char r2 = 1000;
    unsigned int adc_value;
    // Запуск преобразования
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC)); // ждём окончания

    adc_value = ADCW; // 10-битное значение ADC 0–1023
    // Учитываем делитель
    //adc_value *= r2 / r1;
    if (adc_value < 600) return 0;   // защита от выхода за границы
    if (adc_value > 900) return 100; // защита от выхода за границы
    return (adc_value - 600) * 100 / (900 - 600);
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
// Преобразуем число в строку десятичного формата
void my_ltoa(unsigned long value, char *str) {
    char buffer[20]; // Временный буфер для цифр
    char i = 0;
    char j = 0;
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    // Разбираем число на цифры с конца
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    // Переворачиваем строку
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}
void hex_id_to_decimal_string(char *hex_str, char *dec_str) {
    unsigned long id = 0;
    unsigned char i;
    // Преобразуем 8 HEX-символов в 4 байт и собираем как одно число
    for (i = 0; i < 8; i += 2) {
        id = (id << 8) | hex2byte(hex_str[i], hex_str[i + 1]);
    }
    // Преобразуем число в строку десятичного формата
    my_ltoa(id, dec_str);
}
void build_message(char *src, char *dst) {
    unsigned int sum = 0;
    char *p = src;
    while (*p) {
        sum += *p;   // прибавляем ASCII-код
        p++;
    }
    sprintf(dst, "!%02X%s", sum & 0xFF, src);     // формируем "!XXстрока"
}

char* get_tag(char *buffer){
static char dec_str[16];
char tag[10];
 strncpy(tag, buffer + 3, 8);
 tag[8] = '\0';
 hex_id_to_decimal_string(tag, dec_str);
 return dec_str; 
}