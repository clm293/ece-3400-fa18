/*
fft_adc_serial.pde
guest openmusiclabs.com 7.7.14
example sketch for testing the fft library.
it takes in data on ADC0 (Analog0) and processes them
with the fft. the data is sent out over the serial
port at 115.2kb.
*/

#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft

#include <FFT.h> // include the library

int i = 0;
uint32_t target_freq = 6080;
uint32_t smp_freq = 19235;

void turn_led(int i){
  int pin = LED_BUILTIN;
  pinMode(pin,OUTPUT);
  i ? digitalWrite(pin,HIGH) : digitalWrite(pin, LOW); 
}


void setup() {
  Serial.begin(115200); // use the serial port
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
  //while (!Serial.available());
  //pinMode(3,OUTPUT);
}

void loop() {
  // long start = target_freq*FFT_N/smp_freq - 10;
  // long endl = target_freq*FFT_N/smp_freq + 10; bin = *Fs/N
  uint32_t freq_loc = target_freq*(FFT_N/2)/smp_freq;
  Serial.println(freq_loc);
  while(i<1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i+1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();   
    Serial.println(fft_log_out[freq_loc]);
    if(fft_log_out[freq_loc]>45){
      turn_led(1);
    }else{
      turn_led(0);
    }
    /*
    for (byte i = 0 ; i < FFT_N/2 ; i++) { 
      Serial.println(fft_log_out[i]); // send out the data
    }
    i++;*/
  }
}
