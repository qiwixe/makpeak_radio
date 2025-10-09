flash unsigned char zagolovok[]={"Transmitter v0.1.11"};  //  ВЕРСИЯ прошивки
#include <mega8.h>
#include <delay.h>
#include <string.h>
#include <stdio.h>
#include <sleep.h>
#include "trs_function.h"

//Светодиод ридера
#define LED_Reader PORTD.7          
#define LED_Reader_DDR DDRD.7

//Питание ридера !!ИНВЕРТИРОВАНО!!
#define POWER_Reader PORTD.4        
#define POWER_Reader_DDR DDRD.4   
 
//Питание радио модуля !!ИНВЕРТИРОВАНО!!
#define POWER_Radio PORTB.2         
#define POWER_Radio_DDR DDRB.2 
   
//Геркон
#define Reed_switch PIND.3          
#define Reed_switch_DDR DDRD.3      
#define Reed_switch_PORT PORTD.3    

// Настройка прерывания INT1
#define INT1_ENABLE()        (GICR |= (1<<INT1))        // разрешить прерывание
#define INT1_DISABLE()       (GICR &= ~(1<<INT1))       // запретить прерывание

// Прерывание по спаду сигнала (лог.1 > лог.0)
// Срабатывает, когда сигнал на пине INT1 переходит из высокого уровня в низкий.
// Наиболее часто используется при работе с кнопками, датчиками, импульсами.
#define INT1_FALLING_EDGE()  (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | (1<<ISC11))  

// Прерывание по фронту сигнала (лог.0 > лог.1)
// Срабатывает при переходе входа INT1 из низкого уровня в высокий.
// Полезно, если нужно отлавливать начало импульса или событие "включения".
#define INT1_RISING_EDGE()   (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | ((1<<ISC11)|(1<<ISC10)))  

// Прерывание по уровню (лог.0)
// Срабатывает, пока на пине INT1 удерживается низкий уровень.
// Прерывание активное постоянно, пока вход "0" — используется редко.
#define INT1_LOW_LEVEL()     (MCUCR &= ~((1<<ISC11)|(1<<ISC10)))  

// Прерывание при любом изменении уровня
// Срабатывает и на фронте, и на спаде (любое изменение сигнала).
// Удобно при обработке импульсных датчиков или кодеров.
#define INT1_ANY_CHANGE()    (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | (1<<ISC10))  

// Режимы сна
#define SLEEP_ENABLE()     (MCUCR |= (1<<SE))       // разрешить переход в сон
#define SLEEP_DISABLE()    (MCUCR &= ~(1<<SE))      // запретить переход в сон

// Режим "Idle" — минимальный сон: останавливается только ЦПУ,
// но периферия (таймеры, UART, SPI, прерывания и т.п.) продолжают работать.
// Быстрое пробуждение.
#define SLEEP_MODE_IDLE()        (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | 0)

// Режим "ADC Noise Reduction" — снижает шум при измерениях АЦП.
// Работает только АЦП, остальная периферия и ЦПУ спят.
// Хорош для точных аналоговых измерений.
#define SLEEP_MODE_ADC_NOISE()   (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | (1<<SM0))

// Режим "Power-down" — глубокий сон.
// Останавливаются все тактируемые блоки, кроме внешних прерываний или watchdog.
// Минимальное энергопотребление, медленное пробуждение.
#define SLEEP_MODE_POWER_DOWN()  (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | (1<<SM1))

// Режим "Standby" — глубокий сон с сохранением генератора.
// Быстрее пробуждение, чем Power-down, но чуть выше потребление.
#define SLEEP_MODE_STANDBY()     (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | ((1<<SM1)|(1<<SM0)))

//Глобальные переменные
#define RFID_PACKET_LENGTH 14               //Длина приходящего пакета
#define DEBOUNCE_MS 100                    //Время обработки дребезга 
char RAM_RFID_buffer[RFID_PACKET_LENGTH];   //Буфер для запоминания метки
char RFID_buffer[RFID_PACKET_LENGTH];       //Буфер для приходящего сообщения
char MESSAGE_BUFFER[18];                    //Буфер для сообщения !СУММА=НОМЕР+МЕТКА+ЗАРЯД*
unsigned char RFID_index = 0;               //индекс для сообщения
unsigned int timer = 0;                     //Таймер времени
unsigned int ms_counter = 0;                //Таймер дребезга

//Обработчик прерывания от геркона
interrupt [EXT_INT1] void ext_int1_isr(void){
    ms_counter = 0;                      //Сброс таймера дребезга
}
//Обработчик прерывания по времени работы 
interrupt [TIM1_OVF] void timer1_ovf_isr(void){
    timer++;                            //счетчик времени
    //отключение после минуты без считывания карты
    if (timer >= 7) {                   //1 это примено 8.5 сек (7 это 60.2 сек)
        POWER_Reader = 1;               //Выключаем ридер !!ИНВЕРТИРОВАНО!!
        LED_Reader = 0;                 //Выключаем светодиод ридера   
        timer = 0;                      //Сброс счетчика 
        #asm("sleep")                   //Уход в сон
    }
}
//Обработчик включения ридера по геркону
interrupt [TIM2_COMP] void timer2_compare_isr(void){
    if (Reed_switch == 0){              //Если геркон включен   
        ms_counter++;                   //Таймер дребезга
    } 
    if (ms_counter > DEBOUNCE_MS){      //Если пин в одном положении по времени > времени дребезга
        POWER_Reader = 0;               //Включаем ридер !!ИНВЕРТИРОВАНО!!
        LED_Reader = 1;                 //Включаем светодиод ридера
        timer = 0;                      //Обнуляем счётчик
    }
}
void main(void){
char ch;                                //Переменная для символов
char msg[18];                           //Переменная для подготовки сообщения к отправке
char number_trs[] = "33";               //Номер передатчика
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
//Геркон
{    
    POWER_Radio_DDR = 1;                // Пин питания радиомодуля как выход
    POWER_Radio = 1;                    // изначально выключен !!ИНВЕРТИРОВАНО!!

    POWER_Reader_DDR = 1;               // Пин питания ридера как выход
    POWER_Reader = 1;                   // изначально выключен !!ИНВЕРТИРОВАНО!!
        
    LED_Reader_DDR = 1;                 // Пин питания светодиода ридера как выход
    LED_Reader = 0;                     // изначально выключен
    
    Reed_switch_DDR = 0;                // Пин геркона как вход
    Reed_switch_PORT = 1;               // подтяжка

    INT1_LOW_LEVEL();                   // Прерывание по уровню (лог.0)
    INT1_ENABLE();                      // Разрешаем прерывание INT1
}  
//Настройка ADC1
{
    ADMUX = (1<<REFS0) | (1<<MUX0);  // AVCC, ADC1
    ADCSRA = (1<<ADEN) | (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // делитель 128
}
//Настройка таймера
{
    TCCR1A = 0x00;
    TCCR1B = (1<<CS12) | (0<<CS11) | (1<<CS10);
    TIMSK |= (1<<TOIE1);
}
//Настройка таймера защиты от дребезга
{
    TCCR2 = (1<<WGM21) | (1<<CS01) | (1<<CS00);
    OCR2 = 124;          // при 8 МГц даёт 1 мс
    TIMSK |= (1<<OCIE2); // Разрешить прерывание по совпадению
}
//Настройка сна
{
    SLEEP_MODE_POWER_DOWN();            // выбрать режим "power-down"
    SLEEP_ENABLE();                     // разрешить переход в сон;
}
#asm("sei") // глобально разрешаем прерывания    
while (1) {
        ch = uart_receive();
        if (ch == 0x02) {                                                                   // Проверка старт байт
            RFID_index = 0;                                                                 // установки индекса на 1 символ
            RFID_buffer[RFID_index++] = ch;                                                 // записывание этого символа
            while (RFID_index < RFID_PACKET_LENGTH) {                                       // считывание 13 оставшихся байт, ограничение по длине сообщения
                RFID_buffer[RFID_index++] = uart_receive();                                 // записывание символов
            }
            if (RFID_buffer[13] == 0x03 && check_checksum(RFID_buffer)) {                   //Проверка стоп байта и если контрольная сумма верна
                if (memcmp(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH) != 0){         //Метка отличается от предыдущей
                memcpy(RAM_RFID_buffer, RFID_buffer, RFID_PACKET_LENGTH);                   //Запоминаем метку
                sprintf(msg, "=%s+%s+%d*", number_trs, get_tag(RFID_buffer), adc1_read());  //Формирование строки =НОМЕР+МЕТКА+ЗАРЯД*
                build_message(msg, MESSAGE_BUFFER);                                         //Расчет контрольной суммы и формирование строки !СУММА=НОМЕР+МЕТКА+ЗАРЯД*
                POWER_Radio = 0;                                                            //Включение радио модуля !!ИНВЕРТИРОВАНО!!
                uart_send_times(MESSAGE_BUFFER,5);                                          //Отправка сообщения 5 раз
                POWER_Radio = 1;                                                            //Выключение радио модуля !!ИНВЕРТИРОВАНО!!
                POWER_Reader = 1;                                                           //Выключаем ридер !!ИНВЕРТИРОВАНО!!
                LED_Reader = 0;                                                             //Выключаем светодиод ридера   
                #asm("sleep")                                                               //Уход в сон, проснемся по прерыванию от геркона
            }
            }
        }  
    }
}

