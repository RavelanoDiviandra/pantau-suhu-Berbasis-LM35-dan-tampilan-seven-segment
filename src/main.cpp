#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Pola Seven Segment Common Anode (0-9)
const uint8_t pola_CA[] = {
  0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 
  0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000  
};

volatile int suhuSekarang = 0;
volatile uint8_t hitungTimer = 0;

void setup() {
  // Inisialisasi I/O
  DDRD = 0xFF; // Port D sebagai Output (Data Display & Buzzer)
  DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB3) | (1 << DDB4) | (1 << DDB5);

  // Konfigurasi ADC (AVCC Ref, Prescaler 128)
  ADMUX = (1 << REFS0); 
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

  // Konfigurasi Timer2 CTC Mode (~10ms Interrupt)
  TCCR2A = (1 << WGM21);
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
  OCR2A = 156;
  TIMSK2 |= (1 << OCIE2A);
  
  sei(); // Enable Global Interrupt
}

ISR(TIMER2_COMPA_vect) {
  hitungTimer++;
  
  // Eksekusi sampling ADC setiap 500ms (50 x 10ms)
  if (hitungTimer >= 50) {             
    ADCSRA |= (1 << ADSC); // Start Conversion
    while (ADCSRA & (1 << ADSC));

    // Konversi nilai mentah ADC ke format Celcius
    float celcius = (ADC * 5.0 / 1023.0) * 100.0;
    suhuSekarang = (int)(celcius + 0.5);

    // Reset status pin sebelum pembaruan logika
    PORTB &= ~((1 << PORTB3) | (1 << PORTB4) | (1 << PORTB5));
    PORTD &= ~(1 << PORTD7);

    // Evaluasi aktuator berdasarkan ambang batas suhu
    if (suhuSekarang < 30) {
      PORTB |= (1 << PORTB3); // Aman
    } 
    else if (suhuSekarang >= 30 && suhuSekarang <= 35) {
      PORTB |= (1 << PORTB4); // Waspada
    } 
    else {
      PORTB |= (1 << PORTB5); 
      PORTD |= (1 << PORTD7); // Bahaya (LED + Buzzer)
    }
    
    hitungTimer = 0;
  }
}

void loop() {
  updateSevenSegment(suhuSekarang);
}

void updateSevenSegment(int angka) {
  if (angka > 99) angka = 99; // Proteksi overflow digit
  
  int puluhan = angka / 10;
  int satuan = angka % 10;
  
  // Amankan state pin 7 agar interupsi buzzer tidak rusak
  uint8_t statusBuzzer = (PORTD & 0x80); 

  // Multiplexing Digit Puluhan
  PORTB &= ~((1 << PORTB0) | (1 << PORTB1));        // Matikan transistor (cegah ghosting)
  PORTD = (pola_CA[puluhan] & 0x7F) | statusBuzzer; 
  PORTB |= (1 << PORTB0);                           
  _delay_ms(5);                                     

  // Multiplexing Digit Satuan
  PORTB &= ~((1 << PORTB0) | (1 << PORTB1));        
  PORTD = (pola_CA[satuan] & 0x7F) | statusBuzzer;  
  PORTB |= (1 << PORTB1);                           
  _delay_ms(5);
}