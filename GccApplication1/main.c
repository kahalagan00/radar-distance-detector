//Joshmar Morales 5005383833

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define UBRR_9600 103 // for 16Mhz with .2% error
#define ECHO_PIN PORTB0
#define TRIGGER_PIN PORTB1

void USART_init( unsigned int ubrr );
void USART_tx_string(const char *data );
void ULTRA_SONIC_init(void);
void sensor_scan(void);
void servo_rotate(void);

uint16_t overflowCT = 0;
double ticks = 0;
double distance = 0;
double degrees = 0;
int rotationDirection;
char outs[20];
char outs2[20];

int main()
{
	
	USART_init(UBRR_9600);
	
	//Initialize Servo Motor
	//Use prescaler 64 and Non-inverted PWM and mode 14
	TCCR3A =(1<<COM3A1)|(1<<COM3B1) | (1<<WGM31)|(0 << WGM30);  
	TCCR3B =(1<<WGM33)|(1<<WGM32) | (0<<CS32)|(1<<CS31)|(1<<CS30); 
	ICR3=4999;  //fPWM=50Hz (Period = 20ms Standard).
	DDRD |= (1 << PORTD2); 
	PORTD |= (1 << PORTD2);
	
	//Initialize Sensor
	DDRB |= (1 << TRIGGER_PIN);//Set Trigger pins as output
	DDRB &= ~(1 << ECHO_PIN); //Set Echo pin as input.
	TCCR1A = 0; //Set timer to Normal mode
	TIMSK1 = (1 << TOIE1);
	
	OCR3B = 100;
	
	
	while(1){
		//Enables timer overflow interrupt for sensor values
		sei();
		
		//Activate ultrasonic sensor
		sensor_scan();
		
		//Clear interrupt b/c motor doesn't need it
		cli();
		
		//Move servo motor by 2 degrees by increasing or decreasing PWM value.
		servo_rotate();

		//Proper output for processing software:  "degrees,distance."
		uint16_t intDegrees =  (uint16_t)degrees;
		uint16_t intDistance = (uint16_t)distance;
		sprintf(outs, "%d,", intDegrees);
		USART_tx_string(outs);
		sprintf(outs2, "%d.", intDistance);
		USART_tx_string(outs2);
		
		//Small delay make rotation smoother and avoid problems
		_delay_ms(1);
		
	}
}

void sensor_scan(void){
	//Use timer1 to capture distance in centimeters
	PORTB |= (1 << TRIGGER_PIN);
	_delay_us(10);
	PORTB &= (~(1 << TRIGGER_PIN));
	
	//Rising edge capture first
	TCNT1 = 0;
	TCCR1B = (1 << ICES1) | (0 << CS12) | (0 << CS11) | (1 << CS10);
	TIFR1 = 1 << ICF1;
	
	
	while( (TIFR1 & (1 << ICF1)) == 0);
	//Falling edge capture second
	TCNT1 = 0;
	TCCR1B = (0 << ICES1) | (0 << CS12) | (0 << CS11) | (1 << CS10);
	TIFR1 = 1 << ICF1;
	overflowCT = 0;
	
	while((TIFR1 & (1 << ICF1)) == 0);
	ticks = ICR1 + (65535*overflowCT);
	distance = (double)ticks/933;
}

//ISR for timer overflow
ISR (TIMER1_OVF_vect)
{
	++overflowCT;
}

void USART_init( unsigned int ubrr ) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << TXEN0); // Enable receiver, transmitter & RX interrupt
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); //asynchronous 8 N 1
}

void USART_tx_string(const char *data ) {
	while ((*data != '\0')) {
		while (!(UCSR0A & (1 <<UDRE0)));
		UDR0 = *data;
		data++;
	}
}

void servo_rotate(void){
	if(OCR3B == 565)
		rotationDirection = 0;
	if(OCR3B == 100)
		rotationDirection = 1;
	
	
	if(rotationDirection == 1){
		OCR3B += 5;
		degrees += 2.0;
	}
	else{
		OCR3B -= 5;
		degrees -= 2.0;
	}
}
