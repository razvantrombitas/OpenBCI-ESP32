#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <string.h>
#include <SPIFFS.h>
#include <Firebase_ESP_Client.h>
#include <arduinoFFT.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_pass"

// Insert Firebase project API Key
#define API_KEY "your_api_key"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "test@test.com"
#define USER_PASSWORD "testtest"

const char* ssid = "your_ssid";
const char* password = "your_pass";

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "your_link"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 100;
unsigned int  k;

String uid;
String databasePath;
String float_ch1_path;
String float_ch2_path;
String float_ch3_path;
String float_ch4_path;
String stress_path;
String attention_path;
String consciousness_path;
String fft1_path;


// variables to hold the parsed data
float float_ch1 = 0.0;
float float_ch2 = 0.0;
float float_ch3 = 0.0;
float float_ch4 = 0.0;

String inputString = "";

// Constants for band power calculation
const int NUM_CHANNELS = 4;
const int NUM_SAMPLES = 16;
const float SAMPLING_RATE = 128.0;
const float DELTA_LOW = 0.5;
const float DELTA_HIGH = 4.0;
const float THETA_LOW = 4.0;
const float THETA_HIGH = 8.0;
const float ALPHA_LOW = 8.0;
const float ALPHA_HIGH = 12.0;
const float BETA_LOW = 12.0;
const float BETA_HIGH = 30.0;
const float GAMMA_LOW = 30;
const float GAMMA_HIGH = 100;

int alphaLowIndex = SAMPLING_RATE / ALPHA_HIGH;
int alphaHighIndex = SAMPLING_RATE / ALPHA_LOW;
int betaLowIndex = SAMPLING_RATE / BETA_HIGH;
int betaHighIndex = SAMPLING_RATE / BETA_LOW;
int thetaLowIndex = SAMPLING_RATE / THETA_HIGH;
int thetaHighIndex = SAMPLING_RATE / THETA_LOW;
int deltaLowIndex = SAMPLING_RATE / DELTA_HIGH;
int deltaHighIndex = SAMPLING_RATE / DELTA_LOW;
int gammaLowIndex = SAMPLING_RATE / GAMMA_HIGH;
int gammaHighIndex = SAMPLING_RATE / GAMMA_LOW;

// Variables to hold EEG data
float eegData[NUM_CHANNELS][NUM_SAMPLES];

// Variables to hold band power values
float deltaPower[NUM_CHANNELS];
float thetaPower[NUM_CHANNELS];
float alphaPower[NUM_CHANNELS];
float betaPower[NUM_CHANNELS];

AsyncWebServer server(80);

void setup() {
  Serial.begin(57600);
  //  SPIFFS.begin();

  //inputString.reserve(200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
  databasePath = "/UsersData/" + uid;
  float_ch1_path = databasePath + "/float_ch1";
  float_ch2_path = databasePath + "/float_ch2";
  float_ch3_path = databasePath + "/float_ch3";
  float_ch4_path = databasePath + "/float_ch4";
  stress_path    = databasePath + "/stress";
  attention_path = databasePath + "/attention";
  consciousness_path = databasePath + "/consciousness";
  fft1_path = databasePath + "/fft1";

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  inputString = "";

  // Start server
  server.begin();

  Serial.println("<ESP32 is ready>");
  //  delay(1000);
  WebSerial.println("Hello!");

  // Initialize band power calculation variables
  for (int i = 0; i < NUM_CHANNELS; i++) {
    deltaPower[i] = 0.0;
    thetaPower[i] = 0.0;
    alphaPower[i] = 0.0;
    betaPower[i] = 0.0;
  }
  k = 0;
}

void loop() {

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    while (Serial.available()) {

      char inChar = Serial.read();
      if (inChar == '[') {
        inputString.trim(); // remove any trailing whitespace
        int firstComma = inputString.indexOf(',');
        int secondComma = inputString.indexOf(',', firstComma + 1);
        int thirdComma = inputString.indexOf(',', secondComma + 1);
        float_ch1 = inputString.substring(0, firstComma).toFloat();
        float_ch2 = inputString.substring(firstComma + 1, secondComma).toFloat();
        float_ch3 = inputString.substring(secondComma + 1, thirdComma).toFloat();
        float_ch4 = inputString.substring(thirdComma + 1).toFloat();
        //        showParsedData();
        sendFloat(float_ch1_path, float_ch1);
        sendFloat(float_ch2_path, float_ch2);
        sendFloat(float_ch3_path, float_ch3);
        sendFloat(float_ch4_path, float_ch4);
        inputString = "";

        eegData[0][k] = float_ch1;
        eegData[1][k] = float_ch2;
        eegData[2][k] = float_ch3;
        eegData[3][k] = float_ch4;
        k++;
        WebSerial.print("SAMPLE =  ");
        WebSerial.println(k);
        delay(100);

        if (k == NUM_SAMPLES + 1) {

          k = 0;

//          // Shift all data over one position
//          for (int i = 1; i <= NUM_SAMPLES; i++) {
//            eegData[0][i - 1] = eegData[0][i];
//            eegData[1][i - 1] = eegData[1][i];
//            eegData[2][i - 1] = eegData[2][i];
//            eegData[3][i - 1] = eegData[3][i];
//          }
//
//
//          // Apply moving average filter to each channel
//          int n = 4; //filter length
//          for (int i = 0; i < NUM_CHANNELS; i++) {
//            for (int j = 0; j < NUM_SAMPLES; j++) {
//              int filter_start = j - n / 2;
//              int filter_end = j + n / 2;
//              if (filter_start < 0) filter_start = 0;
//              if (filter_end > NUM_SAMPLES) filter_end = NUM_SAMPLES;
//              float sum = 0;
//              for (int k = filter_start; k < filter_end; k++) {
//                sum += eegData[i][k];
//              }
//              eegData[i][j] = sum / (filter_end - filter_start);
//            }
//          }
//
//          // Remove remaining noise components
//          for (int i = 0; i < NUM_CHANNELS; i++) {
//            for (int j = 0; j < NUM_SAMPLES; j++) {
//              if (j < thetaLowIndex || j >= thetaHighIndex && j < alphaLowIndex || j >= alphaHighIndex && j < betaLowIndex || j >= betaHighIndex
//                  || j < deltaLowIndex || j >= deltaHighIndex && j < gammaLowIndex || j >= gammaHighIndex) {
//                eegData[i][j] = 0;
//              }
//            }
//          }
//
//
//          arduinoFFT FFT = arduinoFFT();
//          // Apply FFT to each channel
//          for (int i = 0; i < NUM_CHANNELS; i++) {
//            double channelData[NUM_SAMPLES];
//            double channelOutput[NUM_SAMPLES];
//
//            for (int j = 0; j < NUM_SAMPLES; j++) {
//              channelData[j] = eegData[i][j];
//              channelOutput[j] = 0;
//            }
//
//            FFT.Windowing(channelData, NUM_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
//            FFT.Compute(channelData, channelOutput, NUM_SAMPLES, FFT_FORWARD);
//            FFT.ComplexToMagnitude(channelData, channelOutput, NUM_SAMPLES);
//            for (int m = 0; m < NUM_SAMPLES; m++)
//              eegData[i][m] = channelData[m];
//          }


          for (int i = 0; i < NUM_SAMPLES; i++) {
            float freq = i * SAMPLING_RATE / NUM_SAMPLES;
            float mag = sqrt(pow(eegData[0][i], 2) + pow(eegData[1][i], 2) + pow(eegData[2][i], 2) + pow(eegData[3][i], 2));
          }
          // Calculate band power
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

          // Normalize band power values
          for (int i = 0; i < NUM_CHANNELS; i++) {
            deltaPower[i] /= NUM_SAMPLES;
            thetaPower[i] /= NUM_SAMPLES;
            alphaPower[i] /= NUM_SAMPLES;
            betaPower[i] /= NUM_SAMPLES;
          }


          float deltaRatio = deltaPower[0] / (deltaPower[0] + thetaPower[0]);
          float thetaRatio = thetaPower[0] / (thetaPower[0] + alphaPower[0]);
          float alphaRatio = alphaPower[0] / (alphaPower[0] + betaPower[0]);
          float deltaThetaRatio = deltaPower[0] / thetaPower[0];

          int attentionLevel = 0;
          int stressLevel = 0;
          int consciousnessLevel = 0;

          if (deltaRatio > 0.7) {
            attentionLevel = 1;
          } else if (thetaRatio > 0.7) {
            attentionLevel = 2;
          } else if (alphaRatio > 0.7) {
            attentionLevel = 3;
          } else {
            attentionLevel = 4;
          }

          if ((deltaPower[0] + thetaPower[0]) > (alphaPower[0] + betaPower[0])) {
            stressLevel = 1;
          } else {
            stressLevel = 2;
          }

          if (deltaThetaRatio > 0.7) {
            consciousnessLevel = 1;
          } else if (deltaThetaRatio < 0.3) {
            consciousnessLevel = 3;
          } else {
            consciousnessLevel = 2;
          }

          WebSerial.print("Attention Level ");
          WebSerial.println(attentionLevel);
          WebSerial.print("Stress Level ");
          WebSerial.println(stressLevel);
          WebSerial.print("Consciousness Level ");
          WebSerial.println(consciousnessLevel);

          sendInt(attention_path, attentionLevel);
          sendInt(stress_path, stressLevel);
          sendInt(consciousness_path, consciousnessLevel);

        }

        inChar = Serial.read();
        //        delay(100);
      }
      else inputString += inChar;
    }
  }
}

void showParsedData() {
  WebSerial.print("Channel 1 ");
  WebSerial.println(float_ch1);
  WebSerial.print("Channel 2 ");
  WebSerial.println(float_ch2);
  WebSerial.print("Channel 3 ");
  WebSerial.println(float_ch3);
  WebSerial.print("Channel 4 ");
  WebSerial.println(float_ch4);
}

void sendFloat(String path, float value) {
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)) {
    //    WebSerial.print("Writing value: ");
    //    WebSerial.print (value);
  }
  else {
    WebSerial.println("FAILED");
    WebSerial.println("REASON: " + fbdo.errorReason());
  }
}
void sendInt(String path, float value) {
  if (Firebase.RTDB.setInt(&fbdo, path.c_str(), value)) {
    //    WebSerial.print("Writing value: ");
    //    WebSerial.print (value);
  }
  else {
    WebSerial.println("FAILED");
    WebSerial.println("REASON: " + fbdo.errorReason());
  }
}
