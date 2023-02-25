# OpenBCI-ESP32
OpenBCI Cyton board &amp; ESP32 Project 

# 1. Project overview

This project uses an EEG headset with 8 channels, of which only 4 are used, to collect EEG signals. The signals are acquired using an OpenBCI Cyton board and transmitted via Bluetooth using a USB dongle to a serial port. The data is then read in real-time from the serial port and processed using an ESP32 microcontroller. The level of attention, concentration, and stress are determined and sent to Firebase for display on graphs. In addition, the EEG waveforms are plotted in real-time for the 4 channels used. A moving average filter and FFT are also applied to the input signal. The code for the project is written using the Arduino IDE. 

In terms of data processing, the project uses a number of mathematical calculations to determine the level of attention, concentration, and stress based on the band power values calculated using the FFT. 

{{:iothings:proiecte:2022:brain_waves.png?720|}}


The project also uses the moving average filter to smooth the raw EEG signals, to reduce noise and high frequency artifacts. This is a common technique used in EEG signal processing.

# 2. Hardware 

The hardware used in this project includes the OpenBCI Cyton board, an ESP32 microcontroller, a Bluetooth dongle, and a headset with EEG electrodes.

The OpenBCI Cyton board is used to acquire EEG data from the headset. It is a low-cost, open-source EEG acquisition system that supports a wide range of EEG applications. The Cyton board is connected to the headset via EEG electrodes and it is responsible for amplifying and filtering the EEG signals.

The ESP32 microcontroller is used to process the EEG data and transmit it to Firebase. The ESP32 is connected to the OpenBCI Cyton board via Bluetooth dongle. The ESP32 is also configured to run a web server which allows users to view the EEG data in a web browser.

The headset with EEG electrodes is used to place electrodes on the scalp to record brain activity. The headset is connected to the OpenBCI Cyton board via the cable to transmit the EEG signals to the board.

The acquired EEG signal was collected from the frontal lobe and temporal lobe from both hemispheres (left and right) using 4 electrodes positioned according to the 10-20 system. The electrodes were placed on the frontal and temporal lobes of both hemispheres, with the odd numbered channels (1, 3) representing the frontal and temporal lobes of the right hemisphere, and the even numbered channels (2, 4) representing the frontal and temporal lobes of the left hemisphere. This positioning allowed for the acquisition of EEG signals from both hemispheres, providing a more comprehensive understanding of brain activity.

{{:iothings:proiecte:2022:pozitionare_electrozi.jpg?360|}}

The entire hardware setup can be seen in the following picture: 

{{:iothings:proiecte:2022:setup.png?720|}}



# 3. Software 

The code provided includes WiFi, AsyncTCP, ESPAsyncWebServer, WebSerial, string, SPIFFS, Firebase_ESP_Client, and Arduino TFT libraries. It also includes the use of TokenHelper and RTDBHelper for generating tokens and printing RTDB payloads. The code uses a predefined constant for wifi credentials, Firebase project API key, Authorized Email and Corresponding Password, and RTDB URLs. It also has a number of variables for storing and processing EEG data, including arrays for holding EEG data, band power values, and indices for band power calculation. 

Additionally, it uses the ESPAsyncWebServer library to handle web requests and the WebSerial library to handle serial communication. The project also includes the use of the Arduino TFT library to perform FFT on the input EEG signals. This allows for the analysis of the frequency content of the signals, which can provide additional information about the brain activity being recorded. The project uses the following formulas to calculate the levels of attention, concentration, and stress:

```c
Attention = [(Alpha_Power + Gamma_Power) / (Delta_Power + Theta_Power + Alpha_Power + Beta_Power + Gamma_Power)] * 100

Concentration = [Beta_Power / (Delta_Power + Theta_Power + Alpha_Power + Beta_Power + Gamma_Power)] * 100

Stress = [(Delta_Power + Theta_Power) / (Delta_Power + Theta_Power + Alpha_Power + Beta_Power + Gamma_Power)] * 100
// Alpha_Power, Beta_Power, Gamma_Power, Delta_Power, Theta_Power are the band power values for each frequency band in each channel.
```

These lines of code define the formulas used to calculate the levels of attention, concentration, and stress based on the band power values. The Attention level is calculated by taking the sum of the Alpha and Gamma power and dividing it by the sum of all the band powers. The Concentration level is calculated by taking the Beta power and dividing it by the sum of all the band powers. The Stress level is calculated by taking the sum of Delta and Theta power and dividing it by the sum of all the band powers. The band powers are determined using the following code: 

```c
for (int i = 0; i < NUM_CHANNELS; i++) {
  for (int j = 0; j < NUM_SAMPLES; j++) {
      float freq = j * SAMPLING_RATE / NUM_SAMPLES;
       if (freq >= DELTA_LOW && freq < DELTA_HIGH) {
        deltaPower[i] += pow(eegData[i][j], 2);
       } else if (freq >= THETA_LOW && freq < THETA_HIGH) {
        thetaPower[i] += pow(eegData[i][j], 2);
       } else if (freq >= ALPHA_LOW && freq < ALPHA_HIGH) {
        alphaPower[i] += pow(eegData[i][j], 2);
       } else if (freq >= BETA_LOW && freq < BETA_HIGH) {
        betaPower[i] += pow(eegData[i][j], 2);
       }
  }
}
```


This function calculates the FFT of the EEG data. The FFT is an algorithm that can be used to transform time-domain data into the frequency domain. It takes in the channelData array and the vReal and vImag arrays as inputs. The for loop initializes the vReal and vImag arrays with the channelData and 0 respectively. It then creates an instance of the arduinoFFT class and uses the Compute and ComplexToMagnitude functions to calculate the FFT and convert it to magnitude.

```c
void calculateFFT(float channelData[], double vReal[], double vImag[]){
    for(int i = 0; i < NUM_SAMPLES; i++){
        vReal[i] = channelData[i];
        vImag[i] = 0;
    }
    arduinoFFT FFT = arduinoFFT();
    FFT.Compute(vReal, vImag, NUM_SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, NUM_SAMPLES);
}
```

Full code representation: 

{{:iothings:proiecte:2022:softdiag.png?720|}}



The code uses the Firebase_ESP_Client library to communicate with the Firebase Real-time Database (RTDB) to store and retrieve data. The RTDB stores the EEG data, attention, concentration, and stress levels, which are then displayed on graphs. The FirebaseAuth and FirebaseConfig classes are used to authenticate and configure the connection to the RTDB.

The web page is composed of a canvas element that is used to create a Chart.js plot to display the data, as well as a series of script tags that include the necessary libraries for Firebase and Chart.js. 

Once the data is obtained, a new Chart.js plot is created by passing the canvas element and the data to the Chart() function. The type of chart used is a line chart and the data for the chart is stored in the datasets array. The labels for the x-axis are stored in the labels array.

Finally, the script updates the chart with new data by periodically reading the data from the Firebase database and updating the data in the datasets array.






# 4. Results 

In this project, I used WebSerial for debugging purposes. WebSerial allows the user to view the serial output of the ESP32 in a web browser, which helps in identifying and troubleshooting any issues during the development process. This feature was very helpful in debugging the code and making sure that the data was being transmitted and processed correctly.


{{:iothings:proiecte:2022:webserial.jpg?720|}}


The data in Firebase has the following template: 

{{:iothings:proiecte:2022:firebase_data.jpg?720|}}

{{:iothings:proiecte:2022:graph2.jpg?720|}}

{{:iothings:proiecte:2022:graph1.jpg?360|}}

In addition to the real-time processing and visualization of EEG data, this project also allows for the extraction and analysis of EEG data in a post-processing manner. The OpenBCI Cyton board is able to save the EEG data in a .csv file format, which can be easily exported and analyzed using a script such as Python. By using a script, it is possible to perform more advanced and detailed analysis of the EEG data.

For example, by using a script, it is possible to determine the level of correlation between multiple users or multiple EEG recordings. This can be done by comparing the EEG data of different users or recordings and calculating the correlation coefficient between them. The correlation coefficient is a measure of the linear relationship between two variables and ranges from -1 to 1. A correlation coefficient of 1 indicates a perfect positive correlation, a coefficient of -1 indicates a perfect negative correlation, and a coefficient of 0 indicates no correlation. By calculating the correlation coefficient, it is possible to determine the similarity or dissimilarity between different EEG recordings.

Output from the Python script: 

{{:iothings:proiecte:2022:corr.jpg?720|}}



# 5. Conclusions

Overall, this project uses several concepts of signal processing, data visualization, and microcontroller programming to create a real-time EEG monitoring system. The use of EEG technology in combination with microcontroller programming and web technologies allows for the collection, processing, and visualization of brain activity data in a convenient and accessible way.

This project can be improved in several ways:

  * Incorporating more channels: By using more channels, the EEG data can be acquired from more regions of the brain, which will provide a more comprehensive understanding of the brain activity.

  * Improving the accuracy of the calculated attention, concentration and stress levels: This can be done by implementing more advanced algorithms and machine learning techniques to analyze the EEG data.

  * Improving the real-time display: The website displaying the processed data could be optimized for better performance and user experience.

  * Adding more features: Additional features such as online storage and sharing of the acquired data, or the ability to compare data between multiple users could be added.

  * Improving the security: The data should be encrypted in order to protect it from unauthorized access.

  * Improving the data visualization: The data visualization can be improved by adding more interactive elements, such as sliders to adjust the time range, or the ability to zoom in and out on specific parts of the data.

  * Improving the data analysis: By incorporating more advanced data analysis techniques, such as machine learning, the data can be analyzed in more depth, which can lead to new insights.


# 6. References 
 https://raphaelvallat.com/bandpower.html 

 https://rria.ici.ro/wp-content/uploads/2022/09/art._Chowdhuri_Mal.pdf

 https://openbci.com/
