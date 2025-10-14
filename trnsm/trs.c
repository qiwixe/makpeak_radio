flash unsigned char zagolovok[]={"Transmitter v0.1.13"};  //  ВЕРСИЯ прошивки
#include <mega8.h>
#include <delay.h>
#include <string.h>
#include <stdio.h>
#include <sleep.h>
#include "trs_function.h"
#include "trs_settings.h"

//Глобальные переменные
char RAM_RFID_buffer[RFID_PACKET_LENGTH];   //Буфер для запоминания метки
char RFID_buffer[RFID_PACKET_LENGTH];       //Буфер для приходящего сообщения
char MESSAGE_BUFFER[RFID_PACKET_LENGTH+4];  //Буфер для сообщения !СУММА=НОМЕР+МЕТКА+ЗАРЯД*

bit first_init_reader_flag = 0;             //Флаг первого включения ридера
bit RFID_WAITING_fail_flag = 0;             //Флаг ожидания карты
bit Order_conf_LED_flag = 0;                //Флаг работы светодиода

unsigned int timer = 0;                     //Таймер времени

unsigned int ms_counter = 0;                //Таймер ожидания карты
unsigned int ms_counter_LED = 0;            //Таймер работы светодиода
unsigned int ms_counter_debounce = 0;       //Таймер дребезга

//Режимы работы платы
void power_mode(char mode) {
    switch (mode)
    {
        case 1:
            UART_ENABLE();                  // uart работает  
            TIMER_ENABLE();                 // timer1 работает  
            MS_TIMER_ENABLE();              // timer2 работает  
            SLEEP_DISABLE();                // Сон отключен
            break;

        case 2:
            UART_DISABLE();                 // uart отключен  
            TIMER_ENABLE();                 // timer1 работает 
            MS_TIMER_DISABLE();             // timer2 отключен  
            SLEEP_MODE_IDLE();              // Сон idle
            SLEEP_ENABLE();                 // Сон включен
            break; 
        
        case 3:
            UART_DISABLE();                 // uart отключен  
            TIMER_DISABLE();                // timer1 отключен 
            MS_TIMER_DISABLE();             // timer2 отключен  
            ADC1_DISABLE();                 // ADC отключен
            SLEEP_MODE_POWER_DOWN();        // Сон power-down
            SLEEP_ENABLE();                 // Сон включен
            //#asm("sleep");
            break;
    }
}

//Переключение светодиода "Заказ принят"
void order_status(char status) {
    switch (status){
        case 0:
            Order_conf_LED_flag = 0;        //Флаг работы светодиода
            LED_Order_conf = 0;             //Выключение светодиода
            if(first_init_reader_flag == 1){
            power_mode(2);
            }
            break;
        case 1:
            Order_conf_LED_flag = 1;        //Флаг работы светодиода
            LED_Order_conf = 1;             //Включение светодиода
            break;
    }
}

//Обработчик прерывания от геркона
interrupt [EXT_INT1] void ext_int1_isr(void){
    power_mode(1);
    ms_counter = 0;                               //Сброс таймера ожидания карты 
    timer = 0;
    RFID_WAITING_fail_flag = 0;
    first_init_reader_flag =0;
}
//Обработчик прерывания от геркона
interrupt [EXT_INT0] void ext_int0_isr(void){
    power_mode(1);
    ms_counter = 0;                               //Сброс таймера ожидания карты 
    timer = 0;
    RFID_WAITING_fail_flag = 0;
    first_init_reader_flag =0;
}
//Обработчик прерывания по времени работы 
interrupt [TIM1_OVF] void timer1_ovf_isr(void){
    //timer++;                            //счетчик времени (7 = 60 сек)
    //if (timer >= RFID_FORGET && Reed_switch == 1){          //Раз в 8 сек проверяет прошло ли 10 минут и Разомкнулся ли геркон
        //memset(RAM_RFID_buffer, 0, RFID_PACKET_LENGTH);     //Очистка памяти
        //timer = 0;
        if (Reed_switch == 1){
            if (first_init_reader_flag == 1){
                if (Order_conf_LED_flag == 0){
                    //power_mode(3);
                }
            }
    }
}
//ms_timer
interrupt [TIM2_COMP] void timer2_compare_isr(void){
    //Обработчик антидребезга геркона
    if (first_init_reader_flag == 0 && Reed_switch == 0){   //Если геркон замкнут и ридер не включался   
        ms_counter_debounce++;                              //Таймер антидребезга
        if (ms_counter_debounce > DEBOUNCE_MS ){            //Если пин в одном положении по времени > времени дребезга 
            first_init_reader_flag = 1;                     //Переключаем флаг включения ридера
            POWER_Reader = 0;                               //Включаем ридер !!ИНВЕРТИРОВАНО!!
            ms_counter_debounce = 0;                        //Сброс таймера дребезга                                                 
        }
    } 

    //Обработчик ложного срабатывания
    if (RFID_WAITING_fail_flag == 0){                       //Флаг ожидания карты
        ms_counter++;                                       //Таймера ожидания карты
        if (ms_counter >= RFID_WAITING) {                   //Отключение после RFID_WAITING мс без считывания карты      
            RFID_WAITING_fail_flag == 1;                    //Флаг что карта не считана
            POWER_Reader = 1;                               //Выключаем ридер !!ИНВЕРТИРОВАНО!!
            ms_counter = 0;                                 //Сброс таймера ожидания карты
        }
    } 
    
    //Обработчик включения светодиода
    if (Order_conf_LED_flag == 1){                          //Если флаг светодиода активен 
        ms_counter_LED++;                                   //Считаем время горения светодиода 
        if (ms_counter_LED >= LED_Order_conf_enable){       //Если время вышло 
            order_status(0);                                //Переключаем режим "Заказ принят" 
            ms_counter_LED = 0;                             //Сброс таймера светодиода
        }
    } 
    
}
void main(void){
unsigned char RFID_index = 0;           //индекс для сообщения
char ch;                                //Переменная для символов
char msg[18];                           //Переменная для подготовки сообщения к отправке
char number_trs[] = "33";               //Номер передатчика
//Инициализация
{
    adc_config();                       //Настройка ADC1
    uart_config();                      //Настройка UART
    timer_config();                     //Настройка таймера1
    ms_timer_config();                  //Настройка мс_таймера
    
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
    
    #asm("sei")                         // глобально разрешаем прерывания    
}  
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
                
                ADC1_ENABLE();
                sprintf(msg, "=%s+%s+%d*", number_trs, get_tag(RFID_buffer), adc1_read());  //Формирование строки =НОМЕР+МЕТКА+ЗАРЯД*
                ADC1_DISABLE();
                
                build_message(msg, MESSAGE_BUFFER);                                         //Расчет контрольной суммы и формирование строки !СУММА=НОМЕР+МЕТКА+ЗАРЯД*
                
                POWER_Radio = 0;                                                            //Включение радио модуля !!ИНВЕРТИРОВАНО!!
                uart_send_times(MESSAGE_BUFFER,5);                                          //Отправка сообщения 5 раз
                POWER_Radio = 1;                                                            //Выключение радио модуля !!ИНВЕРТИРОВАНО!!
                
                POWER_Reader = 1;                                                           //Выключаем ридер !!ИНВЕРТИРОВАНО!!
                
                order_status(1);                                                            //Обработка статуса "Заказ принят"   
            }
            }
        }  
    }
}

