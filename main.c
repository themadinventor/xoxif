/*
 * xoxif
 * (c) 2015 Fredrik Ahlberg <fredrik@z80.se>
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/*
 * PD0: RX	Midi In
 *
 * PB1: LED	Status Blinkenlights
 *
 * PC4: "Control"
 * PC5: "Control"
 * PC1: Gate
 *
 * PB4:	Note latch clk
 * PD1: Note data 4
 * PD2: Note data 1
 * PD3: Note data 5
 * PD4: Note data 3
 * PD5: Note data 0
 * PD6: Note data 2
 *
 */

//volatile uint8_t buffer[256], head, tail;

uint8_t cmd, param1, param2, current_note;

enum {
	CMD,
	PARAM1,
	PARAM2
} state = CMD;

enum {
	NOTE_OFF	= 0x80,
	NOTE_ON		= 0x90,
	AFTERTOUCH	= 0xa0,
	CC		= 0xb0,
	PATCH		= 0xc0,
	PRESSURE	= 0xd0,
	PITCH		= 0xe0,
	SYSEX		= 0xf0,
};

static inline void set_note(uint8_t note)
{
	//uint8_t tmp = 0;
	PORTD &= 1;

	if (note & 0x01) {
		PORTD |= _BV(5);
	}
	if (note & 0x02) {
		PORTD |= _BV(2);
	}
	if (note & 0x04) {
		PORTD |= _BV(6);
	}
	if (note & 0x08) {
		PORTD |= _BV(4);
	}
	if (note & 0x10) {
		PORTD |= _BV(1);
	}
	if (note & 0x20) {
		PORTD |= _BV(3);
	}


	PORTB |= _BV(4);
	//_delay_us(1);
	PORTB &= ~_BV(4);
}

static inline void gate_on(void)
{
	PORTC |= _BV(1);
}

static inline void gate_off(void)
{
	PORTC &= ~_BV(1);
}

static inline void process_event(void)
{
	switch (cmd & 0xf0) {
	case NOTE_ON:
		set_note(param1);
		current_note = param1;
		gate_on();

		/* Toggle LED */
		PORTB |= _BV(1);
		break;

	case NOTE_OFF:
		if (param1 == current_note) {
			gate_off();

			/* Toggle LED */
			PORTB &= ~_BV(1);
		}
		break;
	}
}

/* MIDI ISR */
ISR(USART_RX_vect)
{
	uint8_t b = UDR0;

	if (b & 0x80) {
		cmd = b;
		state = PARAM1;

	
	} else if (state == PARAM1) {
		param1 = b;
		state = PARAM2;
	} else if (state == PARAM2) {
		param2 = b;
		state = CMD;
		process_event();
	}
}

void uart_init(void)
{
	UCSR0B = _BV(RXEN0) | _BV(RXCIE0);
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	uint16_t UBRR = F_CPU / 16 / (31250) - 1;
	UBRR0H = UBRR >> 8;
	UBRR0L = UBRR;
}

int main(void)
{
	uart_init();

	DDRB = _BV(1) | _BV(4);
	PORTD = _BV(0);
	DDRD = _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6);

	PORTC = 0;
	DDRC = _BV(1) | _BV(4) | _BV(5);

	sei();

	for (;;) ;
}

