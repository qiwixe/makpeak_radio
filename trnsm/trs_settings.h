#ifndef trs_settings_h
#define trs_settings_h

#define DEBOUNCE_MS 100             //Время обработки дребезга
#define RFID_WAITING 60000          //Время ожидания карты в мс 
#define RFID_FORGET 70              //Время через которое карта будет забыта (7 = минуте) 
#define LED_Order_conf_enable 5000                    //Время горение светодиода "Заказ принят" 
#define RFID_PACKET_LENGTH 14               //Длина приходящего пакета

//Светодиод ридера
#define LED_Order_conf PORTD.7          
#define LED_Order_conf_DDR DDRD.7

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
#define ADC1_ENABLE()  (ADCSRA |= (1<<ADEN))
#define ADC1_DISABLE() (ADCSRA &= ~(1<<ADEN))

void uart_config();
void adc_config();
void timer_config();
void ms_timer_config();

#endif