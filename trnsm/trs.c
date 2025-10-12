flash unsigned char zagolovok[]={"Transmitter v0.1.12"};  //  ВЕРСИЯ прошивки
#include <mega8.h>
#include <delay.h>
#include <string.h>
#include <stdio.h>
#include <sleep.h>
#include "trs_function.h"

//Светодиод ридера
#define LED_Order_conf PORTD.7          
#define LED_Order_conf_DDR DDRD.7
#define LED_Order_conf_enable 5000                    //Время горение светодиода "Заказ принят" 

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

//Геркон (Дублер)
#define Reed_switch_copy PIND.2          
#define Reed_switch_copy_DDR DDRD.2      
#define Reed_switch_copy_PORT PORTD.2 

// Прерывание по спаду (лог.1 > лог.0)
#define INT1_FALLING_EDGE()  (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | (1<<ISC11))
#define INT0_FALLING_EDGE()  (MCUCR = (MCUCR & ~((1<<ISC01)|(1<<ISC00))) | (1<<ISC01))

// Прерывание по фронту (лог.0 > лог.1)
#define INT1_RISING_EDGE()   (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | (1<<ISC11)|(1<<ISC10))
#define INT0_RISING_EDGE()   (MCUCR = (MCUCR & ~((1<<ISC01)|(1<<ISC00))) | (1<<ISC01)|(1<<ISC00))

// Прерывание по уровню (лог.0)
#define INT1_LOW_LEVEL()     (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))))
#define INT0_LOW_LEVEL()     (MCUCR = (MCUCR & ~((1<<ISC01)|(1<<ISC00))))

// Прерывание при любом изменении уровня
#define INT1_ANY_CHANGE()    (MCUCR = (MCUCR & ~((1<<ISC11)|(1<<ISC10))) | (1<<ISC10))
#define INT0_ANY_CHANGE()    (MCUCR = (MCUCR & ~((1<<ISC01)|(1<<ISC00))) | (1<<ISC00))

// Настройка прерывания INT1
#define INT1_ENABLE()        (GICR |= (1<<INT1))        // разрешить прерывание INT1
#define INT1_DISABLE()       (GICR &= ~(1<<INT1))       // запретить прерывание INT1

// Настройка прерывания INT0
#define INT0_ENABLE()        (GICR |= (1<<INT0))        // разрешить прерывание INT0
#define INT0_DISABLE()       (GICR &= ~(1<<INT0))       // запретить прерывание INT0

// Режимы сна
#define SLEEP_ENABLE()     (MCUCR |= (1<<SE))       // разрешить переход в сон
#define SLEEP_DISABLE()    (MCUCR &= ~(1<<SE))      // запретить переход в сон

// Режим "Idle" — минимальный сон: останавливается только ЦПУ,
// но периферия (таймеры, UART, SPI, прерывания и т.п.) продолжают работать.
#define SLEEP_MODE_IDLE()        (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))))

// Режим "ADC Noise Reduction" — снижает шум при измерениях АЦП.
// Работает только АЦП, остальная периферия и ЦПУ спят.
#define SLEEP_MODE_ADC_NOISE()   (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | (1<<SM0))

// Режим "Power-down" — глубокий сон.
// Останавливаются все тактируемые блоки, кроме внешних прерываний или watchdog.
#define SLEEP_MODE_POWER_DOWN()  (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | (1<<SM1))

// Режим "Standby" — глубокий сон с сохранением генератора.
#define SLEEP_MODE_STANDBY()     (MCUCR = (MCUCR & ~((1<<SM1)|(1<<SM0))) | ((1<<SM1)|(1<<SM0)))

// Включение/отключение ms_timer
#define MS_TIMER_ENABLE()         (TIMSK |= (1<<OCIE2)) 
#define MS_TIMER_DISABLE()        (TIMSK &= ~(1<<OCIE2))

// Включение/отключение timer1
#define TIMER_ENABLE()         (TIMSK |= (1<<TOIE1)) 
#define TIMER_DISABLE()        (TIMSK &= ~(1<<TOIE1))

// Включение/отключение UART
#define UART_ENABLE()  (UCSRB |= (1<<RXEN) | (1<<TXEN))
#define UART_DISABLE() (UCSRB &= ~((1<<RXEN) | (1<<TXEN)))

// Включение/отключение ADC1
#define ADC1_ENABLE()  ADCSRA |= (1<<ADEN)
#define ADC1_DISABLE() ADCSRA &= ~(1<<ADEN)

//Глобальные переменные
#define RFID_PACKET_LENGTH 14               //Длина приходящего пакета
unsigned int DEBOUNCE_MS = 100;             //Время обработки дребезга
unsigned int RFID_WAITING = 60000;          //Время ожидания карты в мс 
unsigned int RFID_FORGET = 70;              //Время через которое карта будет забыта (7 = минуте) 
char RAM_RFID_buffer[RFID_PACKET_LENGTH];   //Буфер для запоминания метки
char RFID_buffer[RFID_PACKET_LENGTH];       //Буфер для приходящего сообщения
char MESSAGE_BUFFER[18];                    //Буфер для сообщения !СУММА=НОМЕР+МЕТКА+ЗАРЯД*
unsigned char RFID_index = 0;               //индекс для сообщения
unsigned int timer = 0;                     //Таймер времени
unsigned int ms_counter_debounce = 0;       //Таймер дребезга
unsigned char first_init_reader_flag = 0;

unsigned char order_conf_flag = 0;
unsigned int ms_counter_LED = 0;

unsigned int ms_counter = 0;

void uart_config(){
    // USART initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART Receiver: On
    // USART Transmitter: On
    // USART Mode: Asynchronous
    // USART Baud Rate: 9600
    UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
    UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
    UBRRH=0x00;
    UBRRL=0x33;
}

void adc_config(){
    // Выбираем источник опорного напряжения и канал ADC1
    ADMUX = (1 << REFS0) | (1 << MUX0);  // AVCC, вход ADC1

    // Настраиваем делитель тактовой частоты
    ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // делитель 128
}

//Режимы работы платы
void power_mode(char mode) {
    switch (mode)
    {
        case 1:
            UART_ENABLE();                  // uart работает  
            TIMER_ENABLE();                 // timer1 работает  
            MS_TIMER_ENABLE();              // timer2 работает  
            ADC1_ENABLE();                  // ADC работает
            SLEEP_DISABLE();                // Сон отключен
            break;

        case 2:
            //UART_DISABLE();                 // uart отключен  
            TIMER_ENABLE();                 // timer1 работает 
            MS_TIMER_DISABLE();             // timer2 отключен  
            ADC1_DISABLE();                 // ADC отключен
            SLEEP_MODE_IDLE();              // Сон idle
            SLEEP_ENABLE();                 // Сон включен
            break; 
        
        case 3:
            //UART_DISABLE();                 // uart отключен  
            TIMER_DISABLE();                // timer1 отключен 
            MS_TIMER_DISABLE();             // timer2 отключен  
            ADC1_DISABLE();                 // ADC отключен
            SLEEP_MODE_POWER_DOWN();        // Сон power-down
            SLEEP_ENABLE();                 // Сон включен
            break;
    }
}

//Режим ожидания
void idle_mode () {
    //power_mode(2);
}
//Переключение светодиода "Заказ принят"
void order_status(char status) {
    switch (status){
        case 0:
            order_conf_flag = 0;      //Флаг для таймера
            LED_Order_conf = 0;       //Выключение светодиода
            idle_mode ();
            break;
        case 1:
            POWER_Radio = 1;          //Выключение радио модуля !!ИНВЕРТИРОВАНО!!
            POWER_Reader = 1;         //Выключаем ридер !!ИНВЕРТИРОВАНО!!
            order_conf_flag = 1;      //Флаг для таймера
            LED_Order_conf = 1;       //Включение светодиода
            ms_counter_LED = 0;       //Сброс таймера светодиода
            break;
    }
}


//Обработчик прерывания от геркона
interrupt [EXT_INT1] void ext_int1_isr(void){
    power_mode(1);
    ms_counter_debounce = 0;                      //Сброс таймера дребезга
    ms_counter = 0;                               //Сброс таймера ожидания карты 
    timer = 0;
    first_init_reader_flag =0;
}
//Обработчик прерывания от геркона
interrupt [EXT_INT0] void ext_int0_isr(void){
}
//Обработчик прерывания по времени работы 
interrupt [TIM1_OVF] void timer1_ovf_isr(void){
    timer++;                            //счетчик времени (7 = 60 сек)
    if (timer >= RFID_FORGET && Reed_switch == 1){          //Раз в 8 сек проверяет прошло ли 10 минут и Разомкнулся ли геркон
        memset(RAM_RFID_buffer, 0, RFID_PACKET_LENGTH);     //Очистка памяти
            SLEEP_MODE_POWER_DOWN();        // Сон power-down
            SLEEP_ENABLE();                 // Сон включен
    #asm("sleep");
    }
}
//ms_timer
interrupt [TIM2_COMP] void timer2_compare_isr(void){
    ms_counter++;
    //Обработчик антидребезга геркона
    if (Reed_switch == 0){                          //Если геркон замкнут   
        ms_counter_debounce++;                      //Таймер дребезга
    } 
    if (ms_counter_debounce > DEBOUNCE_MS && first_init_reader_flag == 0){         //Если пин в одном положении по времени > времени дребезга и ридер не вызывался
        first_init_reader_flag = 1;
        POWER_Reader = 0;                //Включаем ридер !!ИНВЕРТИРОВАНО!!                                                 
    }
    //Обработчик включения светодиода
    if (order_conf_flag == 1){                      //Если флаг активен 
        ms_counter_LED++;                           //Считаем время горения светодиода 
    }
    if (ms_counter_LED >= LED_Order_conf_enable){   //Если время вышло 
        order_status(0);                            //Переключаем режим "Заказ принят"
    }
    if (ms_counter >= RFID_WAITING) {   //отключение после RFID_WAITING мс без считывания карты      
        POWER_Reader = 1;               //Выключаем ридер !!ИНВЕРТИРОВАНО!!
        ms_counter = 0;                 //Сброс счетчика
        idle_mode();                    //Переход в режим ожидания
    }
}
void main(void){
char ch;                                //Переменная для символов
char msg[18];                           //Переменная для подготовки сообщения к отправке
char number_trs[] = "33";               //Номер передатчика
//Инициализация
{
    adc_config();                       //Настройка ADC1
    uart_config();                      //Настройка UART  
    
    POWER_Radio_DDR = 1;                // Пин питания радиомодуля как выход
    POWER_Radio = 1;                    // изначально выключен !!ИНВЕРТИРОВАНО!!

    POWER_Reader_DDR = 1;               // Пин питания ридера как выход
    POWER_Reader = 1;                   // изначально выключен !!ИНВЕРТИРОВАНО!!
        
    LED_Order_conf_DDR = 1;             // Пин питания светодиода ридера как выход
    LED_Order_conf = 0;                 // изначально выключен
    
    Reed_switch_DDR = 0;                // Пин геркона как вход
    Reed_switch_PORT = 1;               // подтяжка 
    
    Reed_switch_copy_DDR = 0;           // Пин геркона как вход
    Reed_switch_copy_PORT = 1;          // подтяжка

    INT1_LOW_LEVEL();                   // Прерывание по уровню (лог.0)
    INT1_ENABLE();                      // Разрешаем прерывание INT1
      
    INT0_LOW_LEVEL();                   // Прерывание по уровню (лог.0)
    INT0_ENABLE();                      // Разрешаем прерывание INT0
}  
//Настройка таймера1
{
    TCCR1A = 0x00;
    TCCR1B = (1<<CS12) | (0<<CS11) | (1<<CS10);
}
//Настройка таймера мс
{
    TCCR2 = (1<<WGM21) | (1<<CS01) | (1<<CS00);
    OCR2 = 124;          // при 8 МГц даёт 1 мс
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
                order_status(1);                                                            //Обработка статуса "Заказ принят"   
            }
            }
        }  
    }
}

