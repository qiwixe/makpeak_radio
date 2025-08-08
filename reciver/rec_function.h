#ifndef rec_function_h
#define rec_function_h

char uart_receive(void);
void uart_transmit(char data);
void uart_send(char *str);
void uart_send_times(char *str, char count);
unsigned char hex2byte(char high, char low);
char check_checksum(char *buffer);
void my_ltoa(unsigned long value, char *str);
void hex_id_to_decimal_string(char *hex_str, char *dec_str);

#endif