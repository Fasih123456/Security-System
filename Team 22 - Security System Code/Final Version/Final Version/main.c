/*
 * Functions.h
 *
 * Created: 2020-03-28 2:49:00 PM
 *  Author: Admin
 */ 

/********************************************* Declaration*****************************************/

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* UART Code */
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
void init_uart(void);
/******************************************************************************
******************************************************************************/

void init_hardware(void);
void set_row_low(unsigned int row);
int col_pushed(void);
char get_button(void);
char get_new_button(void);
void F(int x);


int ARMED(int s);
void DISARM(void);
void Blink(void);

void ADMIN (void);
void PIN(void);
void Keypad(void);

void ONMotion(void);
void SignAlarm(void);

void Audio_Alarm(void);
void Audio_Chirp(void);

void halleffect(void);

void Trigger(void);
int InitEcho(void);


/********************************************* Application*****************************************/

int main(void){

	init_hardware();
	init_uart(); // initialization
	printf("System Booted, built %s on %s \n", __TIME__, __DATE__);
	
	halleffect();
	DISARM(); //Start in disarmed state

	return 0;
}

/********************************************* Implemention*****************************************/

/*
 * Fuctions.c
 *
 * Created: 2020-03-28 2:43:27 PM
 *  Author: Admin
 */ 

volatile unsigned int ex = 0 ;  // ex is the counter for the time passed
volatile unsigned int overflow;  // overflow is the counter for the time passed when counting down alarm
volatile unsigned int detected = 0; //Used to say that someone has been detected (use as bool)
#define N 4
volatile unsigned char d[] = {'3','9','5','2'};  // d contains the correct passcode
volatile unsigned char e[] = {'4','4','7','0'};  // b contains the correct panic code	
volatile unsigned char a[] = {'0','6','4','9', '7', '9'}; //a contains the admin code
volatile unsigned int AvgEcho; //The average distance the door is originally at
volatile unsigned char MIP;
volatile unsigned int ECHOHigh, ECHOLow, ECHOHighPipe;
volatile unsigned int TimeOutCnt,Tick;
	
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

int uart_putchar(char c, FILE *stream)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}
/******************************************************************************
******************************************************************************/
int  uart_getchar(FILE *stream)
{
	/* Wait until data exists. */
	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}
/******************************************************************************
******************************************************************************/
void init_uart(void)
{
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UBRR0 = 103;  //With 16 MHz Crystal, 9600 baud = 103
	stdout = &mystdout;
	stdin = &mystdin;
}
void init_hardware(void)
{
  
	PORTC |= (1<<PORTC3) | (1<<PORTC4) | (1<<PORTC5)  ;   // Read C3-5
	DDRD = (1<<PORTD2) | (1<<PORTD3) | (1<<PORTD4) | (1<<PORTD5)  ;     // Write D2-5
  
}

void set_row_low(unsigned int row)
{
   
	
		if ( row == 0){
			PORTD &= ~(1<<2);
			PORTD |= (1<<3) | (1<<4) | (1<<5);
		}else if( row == 1){
			PORTD &= ~(1<<3);
			PORTD |= (1<<2) | (1<<4) | (1<<5);
		}else if( row == 2){
			PORTD &= ~(1<<4);
			PORTD |= (1<<2) | (1<<3) | (1<<5);
		}else if( row == 3){
			PORTD &= ~(1<<5);
			PORTD |= (1<<2) | (1<<3) | (1<<4);
		}
    
    //You could use for example an if statement, case statement, etc.
}

int col_pushed(void)
{
   
	
	if( ( PINC &  (1<<PINC3) ) == 0 ){
		return 1;
	}if( ( PINC & (1<<PINC4) ) == 0 ){
		return 2;
	}if( ( PINC & (1<<PINC5) ) == 0 ){
		return 3;
	}else {
		return 0;
	}

	
}


char buttons[4][3] = {{'1', '2', '3'},
                      {'4', '5', '6'},
                      {'7', '8', '9'},
                      {'*', '0', '#'}};

char get_button(void)
{
    //Use the scanning example from main_test_1()
    //to build a simple code that scans and waits for
    //a button press. Return the character pressed.
	
	init_hardware();
	
	init_uart(); // initialization
	
		
	for(int row = 0; row < 4; row++){
		set_row_low(row);
		_delay_ms(20);

		int col = col_pushed();

		if(col != 0){
			char buttonpress = buttons[row][col-1];
			return buttonpress;
		}
	}
}
char get_new_button(void)
{
    static char last_button;
    char b = get_button();
	
    //Check if we held button down
    if(b == last_button) return 0;
    
    last_button = b;
    
    return b;
}
/* Function F controls the time measurements associated with the keypad*/
void F(int x){
	
	TCCR0A = (1<<WGM01);  // Enables CTC mode (compare on match)
	OCR0A = 244;   // number of counts needed for 1sec
	TIMSK0 |= (1 << OCIE0A );  // compares values with A
	TCCR0B |= (1<<CS02);  // determines the speed (pre-scale /256)
	
	sei();
	
	while(1){
		 //printf("ex = %d, x = %d \n", ex, x);
		 if(ex >= x){
			 ex = 0;
			 break;
			 
		 }	
	}
	
	return;
}
/*Activates the armed state, where RED LED lights up*/
int ARMED(int s){
	
	if( s == 1){
		PORTC |= (1<<PC1);
		AvgEcho = InitEcho();
		ONMotion();
		return 1;
	}else return 0;

}
/*Activates the disarmed state, where GREEN LED lights up*/
void DISARM(void){
	
	printf("System is Disarmed \n\r");
	
	PORTC |= (1<<PC0);
	PORTC &= ~ (1<<PC1);
	
	DDRD |= (1<<PORTD7); 
	PORTD &= ~(1<<PORTD7); //In case it was on
	
	Keypad();
	
}
/* blinks the RED LED during the arm countdown phase, to indicate transition */
void Blink(void){
	
	for(int i = 0; i < 15; i++){
		DDRC |= (1<<PC1); 
		PINC |= (1<<PC1); 
		printf("\n%d seconds has passed", (i + 1));
		F(244);
	}
	return;
}
/*Receives the PIN, and makes a decision accordingly*/
void ADMIN (void){
	
	init_hardware();
	init_uart(); // initialization
	
	printf("Admin Code launched \n\n");
	printf("Enter '*' to delete a pin and then add a new one \n");
	
	while(1){
		
		char b = get_new_button();
		int c = 0;
		
		if( b == '*'){  // delete the code
			
			for (c = 3; c >= 0; c--){
				d[c] = 0;
			}
			
			printf("Pin has been deleted \n \r");
			printf("Enter new pin \n\n");
			c = 0;
			
			while(1) {
				char b = get_new_button();
				if(b){
					d[c++] = b;
				}
				if (c == 4) {
					printf("Entered PIN: %s \n  \r", d);
					break;
				} 
			}
			break;
		} 
	}
		
	printf("Restarting System \n  \n");

	Audio_Chirp();
	DISARM();
	return;
}

void PIN(void){
	
	init_hardware();
	init_uart(); // initialization

	TCCR0A &= ~(1<<WGM01);  // Disables CTC mode (compare on match)
	
	TIMSK0 = (1<<TOIE0); //Enables timer0 overflow interrupt
	TCNT0 = 0x00; //Starts at 0 ticks
	overflow = 0; //reset overflow
	TCCR0B = (1<<CS02); //determines the speed (pre-scale /256)

	sei();
	
	char pin[N];
	int i = 0;
	int t = 0;  // t is the number of tries
	
	while(1){
		
		char b = get_new_button();
		
		if(b){
			pin[i++] = b;
		}

		if (overflow < 7353) {
			if(i == 4){
				pin[4] = 0;
				printf("Entered PIN: %s \n \r", pin);
				
				if((pin[0] == e[0]) &&  (pin[1] == e[1]) && (pin[2] == e[2]) && (pin[3] == e[3])){  // Panic Code
					printf("SILENT ALARM RAISED \n");
					DDRB |= (1<<PORTB2);
					PORTB |= (1<<PORTB2);
					DISARM();
				}
				else if( (pin[0] == d[0]) &&  (pin[1] == d[1]) && (pin[2] == d[2]) && (pin[3] == d[3]) ){
					t = 0;
					printf("correct \n \r");
					DISARM();
				}else{
					t++;
					if(t >= 3) {
						printf("Attempt limit reached \n\r");
						TIMSK0 &= ~(1<<TOIE0); //Disables timer0 overflow interrupt
						SignAlarm();
						break;
					}
				}
				i = 0;
			}
		} else {
			//Should never run
			printf("Time limit has run up \n\r");
			SignAlarm();
			break;
		}
	}
		return;
		
}

void Keypad(void){
	
	init_hardware();
	init_uart(); // initialization
	
	int state = 0;  // state expresses the condition of the system
	char k[6];
	int j = 0;
	
	while(1){
		
		char b = get_new_button();
		
		if (b){
			
			//resets the array every time a 0 is inputted
			if (b == '0'){
				j = 0;
			}
			
			//This is creating an array to check if the code is the admin code
			if (j < 6){
				printf("added %c is %d in the array \n", b, j);
				k[j] = b;
				j++;
			} else {
				j = 0;
			}
			
			//Checks if the array is equal to the admin code
			if( (k[0] == a[0]) &&  (k[1] == a[1]) && (k[2] == a[2]) && (k[3] == a[3]) && (k[4] == a[4]) && (k[5] == a[5]) ){
				ADMIN();
			}
			
		
			if( b == '#'){
				PORTC &=~ (1<<PC0);  // turn Green LED off
				Blink();   // Activate blinking of RED LED
				Audio_Chirp();
				state = ARMED(1);
				printf("\n\rSystem is Armed \n \r");
			}
		
			if( state == 1){   // if system is armed, prepare to receive PIN
				printf("Waiting for detection \n");
				while (state == 1) {
					//halleffect();                                                                              ////////////////////////////////////////////// FIX MAKE IT RUN AT ALL TIMES (INTERRUPT)
					Trigger(); //Triggers to check change
					while (MIP == 1) {}
					if (PINC & (1<<PINC2)){ //If PIR goes off
						if (AvgEcho < (ECHOLow - 3) || AvgEcho > (ECHOLow + 3)) { //Saying that a person came through the door
							detected = 1; //Slightly unnecessary only for repeats
							_delay_ms(2500);
							PORTC &= ~(1<<PORTC2);
							printf("Requesting Pin: \n \r");
							PIN();
						} else {
							printf("Unauthorized entry \n");
							_delay_ms(1000);
							PORTC &= ~(1<<PORTC2);
							SignAlarm();
							break;
						}
					}
				}
			} else {
				printf("System is unarmed \n");
				detected = 0;
			}
		
		}
		
	}
			
	return;	
}

void ONMotion(void) {
	
	DDRD &= ~(1<<PORTD6); //Sets D6 to read
	PORTD |= (1<<PORTD6); //Creates pullup resistor //meaning when its low its high
	
	DDRC |= (1<<PORTC2); //Sets C2 to write
	PORTC &= ~(1<<PORTC2); //turns off C2
	
	PCMSK2 |= (1<<PCINT22); //Enables on pin D6 to detect change
	PCICR |= (1<<PCIE2); //Enables interrupt
	
	sei();

	return;
}


void SignAlarm(void) {
	
	DDRD |= (1<<PORTD7); //Sets D7 to write
	PORTD |= (1<<PORTD7); //turns on D7
	
	printf("THE ALARM HAS BEEN SET OFF \n");
	
	Audio_Alarm(); //ALARM AUDIO TRIGGERED
	
	PIN(); //Sends back to pin to allow for disarm
	
}

void Audio_Alarm(void){
	DDRD |= (1<<PORTD3); //PD3 is audio output
	OCR2A = 32;   //set freq
	OCR2B = 16;  // sets PWM %
	
	TCCR2A |= (1<< COM2B1);// set non-inverting mode
	
	TCCR2B |= (1<<WGM22); //fast PWM mode, OCR2A as top
	TCCR2A |= (1<<WGM21) | (1<<WGM20);
	
	TCCR2B |= (1<<CS22) | (1<<CS20); //set pre-scaler to 128 and starts PWM
	
	while (1) {
		for (int i=0; i<4;i++) //set number of sound cycles here
		{                           //paste desired sound effect here
			OCR2A = 379;
			_delay_ms(300);
			DDRD=0;
			_delay_ms(25);
			DDRD |= (1<<PORTD3);
			OCR2A = 379;
			_delay_ms(150);
			DDRD=0;
			_delay_ms(25);
			DDRD |= (1<<PORTD3);
			OCR2A = 379;
			_delay_ms(150);
			OCR2A = 350;
			_delay_ms(150);
			OCR2A = 379;
			_delay_ms(150);
			DDRD=0;
			_delay_ms(25);
			DDRD |= (1<<PORTD3);
			
		}
		DDRD &= ~(1<<PORTD3); //turn of  audio output
		TCCR2A &= ~(1<< COM2B1);// Disables the alarm ...
		break;
	}
	return;
	
}

void Audio_Chirp(void){
	DDRD |= (1<<PORTD3); //PD3 is output
	OCR2A = 32;   //set freq
	OCR2B = 16;  // sets PWM %
	
	TCCR2A |= (1<< COM2B1);// set non-inverting mode
	
	TCCR2B |= (1<<WGM22); //fast PWM mode, OCR2A as top
	TCCR2A |= (1<<WGM21) | (1<<WGM20);
	
	TCCR2B |= (1<<CS22) | (1<<CS20); //set prescaler to 128 and starts PWM
	
	while (1) {
		for (int i=0; i<2;i++) //set number of sound cycles here one have 1 for testing
		{
			OCR2A = 380;
			_delay_ms(50);
			OCR2A = 319;
			_delay_ms(50);
			//paste desired sound effect here
			
			
		}
		DDRD &= ~(1<<PORTD3); //turn of  audio output
		TCCR2A &= ~(1<< COM2B1);// Disables the alarm ...
		break;
	}
	
	
	
	
	return;
	
}

void halleffect(void)
{
	PORTB &= ~(1<<PORTB4); //LED off
	while(1)
	{
		
		if(PINC &(1<<PINC0))            // check for sensor pin PC.0 using bit
		{
			PORTB |= (1<<PORTB4); // LED on
			_delay_ms(5000);	//how long the LED should be on
			PORTB &= ~(1<<PORTB4);          //LED off
		}
		else{
			PORTB &= ~(1<<PORTB4); //LED off
		}


	}
	sei();

	return;
}

int InitEcho(void){
	int x = 0;
	int avg = 0;
	int y[15];
	
	sei();
	
	while (x < 15){
		Trigger();
		while(MIP == 1) {}
		
		y[x] = ECHOLow;
		printf("\n Echo; %d - %dcm\n\n",	x, y[x]);
		
		x++;
	}
	
	for (int i = 0; i < 15; i++){
		avg = avg + y[i];
	}
	
	return avg/15;
	
}

void Trigger(void) {		// Config Timer 1 for 10 to 15uS pulse.
	if(MIP == 0) {	// Don't allow re-trigger.
		MIP = 1;				// Set Measurement in progress FLAG
		DDRB |= (1<<PB1);		// PB1 as Output for Trigger pulse.
		DDRD &= ~(1<<PB0);		// PB0 as Input for Input Capture (ECHO).
		
		TCNT1 = 0;				// Clear last Echo times.
		ECHOHighPipe = 0;
		
		OCR1B = 10100;			// 10 mS Post echo Delay
		OCR1A = 12;				// 10 us Trigger length.

		PORTB |= (1<<PB1);		// Start Pulse.

		TIFR1 = 0xFF;			//  Clear all timer interrupt flags
		TCCR1A = 0;   // Timer mode with Clear Output on Match
		TCCR1B = (1<<WGM12) | (1<<CS11);  // Counting with CKio/8 CTC Mode enabled
		TIMSK1 = (1<<OCIE1A);	// enables the T/C1 Overflow, Compare A, and Capture interrupt;
	}
	
}

ISR(TIMER0_COMPA_vect){
	
	ex++;
	
}

ISR (PCINT2_vect) { //This interrupt detects a change.
	printf("Detected \n"); //No printf's should be in interrupt but it works fine
	PORTC |= (1<<PORTC2); // LED on
	PCICR &= ~(1<<PCIE2); //Disables interrupt
}

ISR (PCINT0_vect) { //This interrupt detects a change.
	printf("Detected Magnet has Moved\n"); 
	SignAlarm();
}

ISR (TIMER0_OVF_vect) {
	if (detected == 1) {
		overflow++;
		if (overflow >= 7353) {
			overflow = 0;
			printf("Time limit has run up \n");
			TIMSK0 &= ~(1<<TOIE0); //Disables timer0 overflow interrupt
			SignAlarm();
		}
	}
}

ISR (TIMER1_OVF_vect) {	// For long ECHO's
	if(ECHOHighPipe >= 2) {
		TIMSK1 = 0;	// No further interrupts.
		TCCR1B = 0; // Stop Clock
		MIP = 0xFF;	// End Measurement
	}
	
	ECHOHighPipe++;	// Add 1 to High byte.
}

ISR (TIMER1_CAPT_vect) {	// Start and Stop ECHO measurement;
	if((TCCR1B & (1<<ICES1)) != 0) { // a rising edge has been detected
		TCCR1B |= (1<<CS11);	// Start counting with ck/8;
		TCCR1B &= ~(1<<ICES1);  // Configure Negative Edge Capture for end of echo pulse.
	}
	
	else {						// a falling edge has been detected
		ECHOLow = TCNT1;
		ECHOLow = ((ECHOLow*0.0000005)*1000000)/58.0;
		ECHOHigh = ECHOHighPipe;
		TIMSK1 = (1<<OCIE1B);	// Enables the Compare B interrupt for POST Trigger Delay: Approx 10mS
		TCNT1 = 0;
	}
}

ISR (TIMER1_COMPB_vect) {	// Compare B: Post ECHO delay 10mS
	MIP = 0;	// End Measurement
}

ISR (TIMER1_COMPA_vect) {	// Compare A : End of Trigger Pulse
	PORTB &= ~(1<<PORTB1);
	TIMSK1 = (1<<ICIE1)|(1<<TOIE1); // enables the T/C1 Overflow and Capture interrupt;
	TCCR1B = (1<<ICES1);			// Set Positive edge for capture but Don't count yet
}