
# Lab 2: Analog Circuitry and FFTs

## Objectives
* Understand and Utilize the FFT Library
* Implement 660Hz Tone Detection
* Implement 6.08kHz IR signal detection, ignoring decoys

## Introduction
In lab 2, we added hardware sensors and signal processing capabilities to the robot.  We split into two subteams, with Tara and Chrissy working on acoustics and Xiaoyu and Patrick on optical sensing. The start of our final maze will commence with a 660 Hz whistle blow, so the acoustic team used an Electret microphone and an amplifying circuit to detect the tone and distinguish it from background noise.  The optical group used an IR transistor to detect other robots emitting IR at 6.08kHz, and ignore decoys (18kHz).

## FFT Analysis
The “Fast Fourier Transform” is an operation that uses the Discrete Time Fourier Transform in a time-efficient method to sample a signal over time and return its frequency components. Because both sub teams had to implement algorithms to detect specific frequencies in a noisy environment, we began the lab by familiarizing ourselves with the Arduino Open Music Labs FFT library in order to allow us to use digital filters to process these signals. We each installed the library in our Arduino IDE and studied the example script fft_adc_serial, with the goal of understanding how to use the FFT library and identifying the frequency bin where we should look for our signal. First, we looked at the sampling frequency of the ADC. There are two ways to identify this rate -- the first is to use the oscilloscope and the digitalWrite() function to change the state of the digital pin whenever the ADC finishes one conversion. The scope will then measure the frequency of the output wave. Based on our implementation, we know that the ADC converts two values per period, so the frequency is about 38 kHz.  

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/frequency%20of%20ADC.PNG" width="800"/>
    <font size="2">
    <figcaption> <b> Frequency of the ADC </b>
    </figcaption>
    </font>
</figure>

To confirm this result, we referenced Section 28 of the ATmega328 datasheet, which provides information about the ADC. It indicates that the last 3 bits of the ADC Control and Status Register A determine a prescalar by which the Arduino clock frequency is divided to determine the ADC clock frequency. The example script fft_adc_serial sets this division factor to 32 in the second line of the code snippet below.  Given the 16MHz Arduino system clock frequency and the 13 clock cycles it takes the ADC to convert, we used the formula `Arduino clock cycle / total conversion clock cycles / prescalar` to find the 38 kHz sampling frequency. 

Calling the FFT function is simple using the Music Labs' library where we first must setup the ADC settings:
```cpp
    ADMUX = 0x40; // use adc0
    ADCSRA = 0xe5; // adc prescalar
```
The FFT libraries takes care of the actual calculations of frequencies from the analog input to FFT outputs. We had to take the necessary samples for the library to calculate:
```cpp
for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
    while(!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf7; // restart adc
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
```
Once this is down we can grab our data using 
```cpp
    fft_log_out
```
which hosts the stored data in bins with a specific frequency range per bin.

## Acoustic Team

From our analysis of the FFT and our determination of the frequency bin width, we determined that our 660Hz audio signal should fall in the fifth bin. We confirmed this by inputting a 660Hz sine wave from a function generator through a 330-ohm resistor into an analog Arduino pin and running the example fft_adc_serial code. We graphed the FFT output, as shown below. From this graph we saw our expected peak in the fifth bin.

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/fft_660.PNG" width="500"/>
    <font size="2">
    <figcaption> <b>FFT output from fft_adc_serial example code with 660Hz Signal from Function Generator </b>
    </figcaption>
    </font>
</figure>

Next we created the simple microphone circuit from the lab document:

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/simple_microphone.PNG" width="300"/>
    <font size="2">
    <figcaption><b> Basic Microphone Circuit from Lab Document</b>
    </figcaption>
    </font>
</figure>

Although we had trouble getting any signal out of the microphone at first, we eventually were able to see a response on the oscilloscope when we played a 660Hz tone near the microphone from a tone generator on our phones. We observed that the amplitude of the signal was around 40 mV, and the output was unsteady and weak. 

To amplify this signal to a more detectable and readable value, we created a simple non-inverting amplifier, starting with a modest gain of around 5 to be safe. We first tested the amplifier with an input signal from the function generator, reading the output on the oscilloscope. For a while we were not able to obtain any output signal at all. After switching our op amp from an LF353 to LM358AN, the amplifier worked as expected; we saw the desired output and amplification.

<img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/amp_input.jpg" width = "340"/>        <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/amp_output3.jpg" width = "300"/>

We then put the microphone output through the amplifier. We again were not able to obtain any output signal, so we re-examined our connection between the microphone and amplifier and added a DC bias. Rather than send the simple microphone circuit output straight into the amplifier, we decided to keep the capacitor from the example circuit to remove its DC offset, and use a voltage divider to create a small, 50mV bias at the input to the amplifier. This allowed us to control our DC offset and left room for a large amplification. After solving this problem, we successfully increased our amplification to around 52. We played the 660Hz tone near the microphone and read the output to the scope to confirm that the signal was what we expected it to be. Our final design was the following amplifier circuit:

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/Acoustic.png" width="600"/>
    <font size="2">
    <figcaption> <b>Acoustic Amplifier Circuit Design</b>
    </figcaption>
    </font>
</figure>

Finally, we connected the output from the amplified microphone signal to an analog Arduino pin and ran the FFT code. For clarity, we plotted only the bin 5 output on the serial plotter, and watched it spike when we brought the 660Hz tone near the microphone. In an effort to filter out more ambient noise, we narrowed the bin frequency width by changing the prescalar for the ADC clock input to 128. This was done by changing the ADC Control and Status Register A:

```cpp
    ADCSRA = 0xe5;
```

Re-calculating the frequency bin width we determined we should find our 660Hz frequency in bin 19, and we confirmed this by running the FFT and plotting the results:

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/fft_660_128.PNG" width="600"/>
    <font size="2">
    <figcaption><b> Acoustic Amplifier Circuit Design</b>
    </figcaption>
    </font>
</figure>

As a finishing touch, we created a placeholder “robot start” function by lighting an LED whenever the 660Hz tone was detected, which we did by setting a threshold intensity value for bin 19 as shown in the following code:

```cpp
  if(fft_log_out[18] > 90){
      digitalWrite(LED,HIGH);
    } else {
      digitalWrite(LED,LOW);
    }
```

A demonstration of this detection is shown in the following video:

<iframe width="560" height="315" src="https://www.youtube.com/embed/JvM9OUa2xY0" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>


## Optical

### Initial design
We used the OP598A phototransistor to detect IR signals. The phototransistor was built exactly like the schemamtic from the lab, with a 1.8k reistor connected to 5V power supply and the photoresistor connected to ground. We first put that output into the oscilioscope and got the following reading for FFT: 

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/IR_FFT_without%20opamp.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>Oscilloscope FFT of IR Sensor without augmentations</b>
    </figcaption>
    </font>
</figure>

The result is from turning the IR hat near the sensor. The signal strength appears to be pretty strong as there are clearly readings at the 6kHz mark and all its harmonics. For the Arduino, we used FFT library from Music Labs' FFT. With the library, we can check the intensity of a desired frequency range to see if the it recieved any signals. We chose a 256 point FFT with a known ADC sampling frequency of 38.5kHz. Therefore, each bin of the FFT has a frequency range of Fs/N or around 150Hz per bin. Thus the 6.08kHz desired frequency is located at bin 40. Thus, all we have to call is 
```cpp
    fft_log_out[40]; //fft_log_out is the strength of the signal converted to dB.
```
to get our readings.

### Upgraded Design
The signal strength of the FFT at our desired bin was already strong but we wanted to implement noise filtering which means we need a filter. We also want to amplify the values in order to utilize all 10 bits of the ADC for a higher resolution reading. The Arduino Analog input can only take in voltage values from 0 to 5 volts which means that any the input voltage can’t be negative or higher than that which will result in cut off and possibly damaging the circuit. Thus, this is the resulting schematic:

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/schematic-Phototransistor%20with%20OpAmp.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>Schematic of IR with OpAmp and bandpass filter</b>
    </figcaption>
    </font>
</figure>

We opted with using a high pass filter to remove any DC bias inherent in the output of the sensor because the DC bias is already high at around 4v. We then amplified the filtered signal by a factor of 20 which can be adjusted as needed. The amplification seems like a good amount based on the detection strength of the IR sensor. After we amplified the signal, we ran the output through a low pass filter that removes any high frequency noise and harmonics. We chose the values of the capacitor and resistor such that the bandpass filter contains a lower cutoff frequency of 5.5kHz and higher cuttoff frequency of 6.5kHz. The formula to calculate the cutoff frequency is 1/(2*pi*RC). This completes our bandpass filter.


### Testing
For testing we started with unit tests by turning on the hat and holding it a certain distance from the phototransistor and check the output of the FFT printing to serial. We also implemented a blinking LED that would increase blinking rate as the IR gets closer to the phototransistor. The frequency of the blink rates tell us how close the hat is to the IR sensor. This tells us that the sensor is working as intended. In the video, the blinking is from the Arduino's internal LED although we should have used an external instead for demostration purposes. 

<iframe width="560" height="315" src="https://www.youtube.com/embed/_hD_c_GUQas?rel=0&amp;controls=0" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>

We also reedited the FFT library's codes to record FFT values in a single FFT cycle for better side by side comparison. Here are the results:

We divided the tests as such:
- off: IR hat turned off
- far: IR hat 1.5 intersections away from sensor
- mid: IR hat 0.5 intersections away from sensor
- close: IR hat right next to sensor

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/IRnoOpAmp.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>FFT of IR without Op Amp</b>
    </figcaption>
    </font>
</figure>

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/IROpAmp.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>FFT of IR with Op Amp</b>
    </figcaption>
    </font>
</figure>

From these two comparisons, we can see that the op amp increases mid range performance of the IR sensor by detecting more of the IR hat's correct frequency signal whereas the harmonics appears to be more filtered out as a result of the installed bandpass filters. The long range performance appears to be unaffected by the augmentation and the close range performance clearly increased slightly.


After testing that the sensor could detect the desired signal, we then tested the robustness of our filtering software and hardware by giving it decoy signals. We used a decoy IR signal at around 12kHz placed next to the sensor and read its FFT's. We also used the decoy to test our sensor for detecting different frequencies. 
<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/DecoyOpAmp.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>FFT of Decoy </b>
    </figcaption>
    </font>
</figure>

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/Decoy.PNG" width="800"/>
    <font size="2">
    <figcaption> <b>FFT of Decoy with Op Amp </b>
    </figcaption>
    </font>
</figure>
We reached two conclusions with this test. The augmented sensor worked with different frequencies because the signal strengths were clearly amplified. Although for this test, it appears some of the noise were amplified as well but it was not significant. We also see that the 40th bin of the FFT is contains only background signals. This means that the signal strength will not be enough for the threshhold to detect a false positive. With these two tests, we proved that the IR sensor's hardware and software can detect the desired frequency amount and augmented it so that it detect farther away signals.  


## Integration

<figure>
    <img src="https://raw.githubusercontent.com/PBC48/ECE-3400-Fall-2018/master/docs/images/lab02/20181003_164149.jpg"/>
    <font size="2">
    <figcaption> <b>The IR sensor and microphone together!</b>
    </figcaption>
    </font>
</figure>


To integrate both the optical and the acoustic sensors, we first read input from the acoustic sensor through pin A0. Once we get a hit from the acoustic sensor, we switch to reading input from the IR sensor at pin A1. Both of these inputs rely on the same FFT function. 

<iframe width="560" height="315" src="https://www.youtube.com/embed/v4Z3QcfFZ4k" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>

In order to properly incorporate this, we have put in a 5-part finite state machine. The state starts. It then moves on to recording and running the FFT on the audio. Once we record the audio, we go to process it. If the bin containing 660Hz passes its intensity threshold (meaning we've detected the tone and we need to start), we start recording IR. If not, then we go back to recording audio. From here, we continue to recording and running the FFT on the IR. If the IR surpasses its threshold (meaning a robot is detected), it writes to the serial monitor and goes back to the start. Else, it keeps recording IR. 

``` cpp

enum states{
    START,
    AUDIO_FFT,
    AUDIO_PROC,
    IR_FFT,
    IR_PROC
};
uint8_t state;
void loop() {
    
    switch (state){   
        case START:
            //check point
            state = AUDIO_FFT; //next state
            break;
        case AUDIO_FFT:
            ADMUX = 0x40; // use adc0
            /*
                Some FFT array from ADC generating code....
            */
            state = AUDIO_PROC;
            break;
        
        case AUDIO_PROC:
            /*
                Processing FFT result code and check threshhold...
            */
            if(past_some_threshhold){
                state = IR_PROC; //past threshhold so we move on
            }else{
                state = AUDIO_FFT; //recalculate FFT with new samples
            }
            break;

        case IR_FFT:
            ADMUX = 0x41; // use adc1
            /*
                Some FFT array from ADC generating code....
            */
            state = IR_PROC;
            break;

        case IR_PROC:
            /*
                Processing FFT result code and check threshhold...
            */
            if(past_some_threshhold){
                state = START; //past threshhold so we move on
            }else{
                state = IR_FFT; //recalculate FFT with new samples
            }
            break;
  }  
}

```

## Conclusion
We were able to integrate both the IR and microphone sensors to a single code base. This step is important as we will need to integrate all of the code from the different modules we made into the arduino. Looking forward, we would like to increase the ranges of the microphone sensors and IR sensors such that they will be able to detect the correct freqencies from farther away. This is important since the IR is used to detect other robots and the microphone is used to start the robot. We also want to explore ways to schedule our code because the sensing is a hard real time process where we have to stop the robot if it is in danger of collision and thus, we must find a way to quickly calculate the FFT and then have the robot react quick enough. A likely solution will be to use interrupts.  
