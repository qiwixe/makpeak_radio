/*******************************************************
This program was created by the CodeWizardAVR V3.40 
Automatic Program Generator
© Copyright 1998-2020 Pavel Haiduc, HP InfoTech S.R.L.
http://www.hpinfotech.ro

Project : 
Version : 
Date    : 03.08.2025
Author  : 
Company : 
Comments: 


Chip type               : ATmega8
Program type            : Application
AVR Core Clock frequency: 8,000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*******************************************************/

#include <mega8.h>
#include <delay.h>
// Declare your global variables here

// Standard Input/Output functions
#include <stdio.h>

// === Отправка символа ===
void uart_putchar(char c) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
}

// === Отправка строки ===
void uart_puts(char *str) {
    while (*str) uart_putchar(*str++);
}

// === Преобразование HEX символа (ASCII) в байт ===
unsigned char hex_to_byte(char high, char low) {
    unsigned char h = (high >= 'A') ? (high - 'A' + 10) : (high - '0');
    unsigned char l = (low >= 'A') ? (low - 'A' + 10) : (low - '0');
    return (h << 4) | l;
}

// === Преобразование байта в 2 символа ASCII-HEX ===
void byte_to_ascii_hex(unsigned char byte, char *high, char *low) {
    const char hex[] = "0123456789ABCDEF";
    *high = hex[(byte >> 4) & 0x0F];
    *low = hex[byte & 0x0F];
}

// === Отправка одного пакета метки ===
void send_rfid_packet(const char *id10chars) {
    char packet[14];
    unsigned char checksum = 0;
    char cs_high, cs_low;
    unsigned char i = 0;

    packet[0] = 0x02; // Старт

    // Копируем ID и вычисляем контрольную сумму
    for (i = 0; i < 10; i++) {
        packet[i + 1] = id10chars[i];
    }

    // Контрольная сумма = XOR из 5 байт ID (по 2 ASCII символа на байт)
    for (i = 0; i < 10; i += 2) {
        unsigned char byte = hex_to_byte(id10chars[i], id10chars[i + 1]);
        checksum ^= byte;
    }

    // Преобразуем чек-сумму в ASCII-HEX
    byte_to_ascii_hex(checksum, &cs_high, &cs_low);
    packet[11] = cs_high;
    packet[12] = cs_low;

    packet[13] = 0x03; // Стоп

    // Отправка
    for (i = 0; i < 14; i++) {
        uart_putchar(packet[i]);
    }
}

void main(void)
{
// Declare your local variables here
const char tagID[] = "2F0039E41F";
const char tagID1[] = "2F0039E41F";
const char tagID2[] = "2F0039E41F";
const char tagID3[] = "2F0139E41F";
unsigned char i;
// Input/Output Ports initialization

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

while (1)
      {
      /*
      unsigned char data[3] = {0x02,0x55,0x03};  
      
      for (i = 0; i < 3; i++) {
        while (!(UCSRA & (1 << UDRE)));  // Ждём, пока регистр передачи пуст
        UDR = data[i];                   // Отправляем байт
        }
        delay_ms(2000);
        */
        send_rfid_packet(tagID);
        delay_ms(2000);
        send_rfid_packet(tagID1);
        delay_ms(2000);
        send_rfid_packet(tagID2);
        delay_ms(2000);
        send_rfid_packet(tagID3);
        delay_ms(2000);
        //while (!(UCSRA & (1 << UDRE)));
        //UDR = 0x55;
        //delay_ms(500);
        //UDR = 0x02;
        //delay_ms(2000); // Отправлять каждые 2 сек
        //UDR = 0x55;
        //delay_ms(50);
        //UDR = 0x03;
        //delay_ms(50);
      }
}
