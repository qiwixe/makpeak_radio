#ifndef trs_function_h
#define trs_function_h

char uart_receive(void);
void uart_transmit(char data);
void uart_send(char *str);
void uart_send_times(char *str, char count);
unsigned char hex2byte(char high, char low);
char check_checksum(char *buffer);
int adc1_read();
void build_message(char *src, char *dst);
void hex_id_to_decimal_string(char *hex_str, char *dec_str);
void my_ltoa(unsigned long value, char *str);
char* get_tag(char *buffer);
#endif