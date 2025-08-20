#ifndef trs_function_h
#define trs_function_h

char uart_receive(void);
void uart_transmit(char data);
void uart_send(char *str);
void uart_send_times(char *str, char count);
unsigned char hex2byte(char high, char low);
char check_checksum(char *buffer);
float adc1_read();
#endif