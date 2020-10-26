/*
 * Proyecto SESCCA
 * Desarrollo de prototipo 3 de tipo conductual para estudiantes.
 * Controlador: ESP8266
 * Actuadores: 2 botones de tipo NA, Cinta LED Neopixel", Motor DC 3v(vibraciones), 2 leds
 * Sensores: 2 sensores de proximidad
 * Envío de datos como Cliente a través de HTTPCLient
 * Recepción de datos como Servidor
 * Elaborado por : Juan Sebastián Bravo Meneses
 * Versión: 1.1
 */

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Interrupción Timer
//#include <Ticker.h>
//Ticker scheduledTicker;

volatile int interrupts;
//int number = random(10, 15);

// Pin salida Neopixel
#define LED 13

// Número de LEDS
#define NUMPIXELS 8

Adafruit_NeoPixel pixels(NUMPIXELS, LED, NEO_GRB+NEO_KHZ800);

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "pitches.h"


// Conexión a Red
const char* ssid     = "MOVISTAR_FIBRA_8EF0"; // Nombre Red
const char* password = "8227854698"; // Contraseña Red

// Puerto para servidor
WiFiServer server(80);

// Variable para guardar HTTP request
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Pines donde se conectan los botones
const byte buttonPin1 = 4;
const byte buttonPin2 = 5;

// Pines donde se conectan los sensores
const byte sensorPin1 = 14;
const byte sensorPin2 = 12;

// Pin donde se conecta el motor
const byte vibePin = 15;

// Pines donde se conecta leds(ojos)
const byte led1 = 16;
const byte led2 = 0;

// Variables para nivel de conducta
short int cont=0, cont_aux=0;
bool q =false;

// Variables para envío y recepción de datos
bool data = false, alert = false;
String id="23";
int mov=0;

// Función Interrupcion Timer
//void onTime(){
//  interrupts++;
//  if(interrupts==number){
//    data = true;
//    alert = true;
//    interrupts = 0;
//    number = random(40, 60);
//    Serial.println(number);
//    scheduledTicker.detach();
//  }
//}
//void ICACHE_RAM_ATTR onTime(){
//  interrupts++;
//  timer1_write(2500000);
//  Serial.println(interrupts);
//  if(interrupts==number){
//    data = true;
//    alert = true;
//    interrupts = 0;
//    number = random(10, 25)*2;
//    Serial.println(number);
//  }
//}

// Funciones Interrupcion Botones
ICACHE_RAM_ATTR void cont_plus(){
  cont_aux++;
  detachInterrupt(digitalPinToInterrupt(buttonPin1));
  detachInterrupt(digitalPinToInterrupt(buttonPin2));
  q = false;
}
ICACHE_RAM_ATTR void cont_minus(){
  cont_aux--;
  detachInterrupt(digitalPinToInterrupt(buttonPin1));
  detachInterrupt(digitalPinToInterrupt(buttonPin2));
  q = false;
}

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  delay(100);

  // Iniciar Timer
//  scheduledTicker.attach_ms_scheduled(1000, onTime);
//  timer1_isr_init();
//  timer1_attachInterrupt(onTime);
//  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
//  timer1_write(2500000);

  //Serial.println(number);
  
  // Configuración LED
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
  #endif
  pixels.begin();
  // Activar resistores de PULL-UP
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  // Sensores como Entradas
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);

  // Parlante como salida
  pinMode(vibePin, OUTPUT);

  // Leds
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  pinMode(LED, OUTPUT);

  visualize(cont);
}

void loop(){
  //pixels.clear();  
  //visualize(cont);
  receive_from_client();
  if(cont!=cont_aux){
    animation(cont,cont_aux);
    if(cont_aux<0){
      cont_aux=0;
    }
    cont = cont_aux;
    visualize(cont);
    send_data();
  }
  if(data==true){
    data = false;
    if(alert==true){
      Serial.println("Alerta");
      attachInterrupt(digitalPinToInterrupt(buttonPin1), cont_plus, RISING);    
      attachInterrupt(digitalPinToInterrupt(buttonPin2), cont_minus, RISING);
      alarm();
      alert = false;
      animation_question();
    }
  }
  if(cont>5){
    cont=5;
    cont_aux=5;
  }
  if(digitalRead(sensorPin1)==HIGH && digitalRead(sensorPin2)==HIGH){
    //mov++;
    //send_data();
  }
}

// Función para recepción de datos desde los clientes
void receive_from_client(){
  WiFiClient client = server.available();

  if(client){
    Serial.println("Nuevo Cliente");
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;
    while(client.connected() && currentTime-previousTime<=timeoutTime){
      currentTime = millis();
      if(client.available()){
        char c = client.read();
        Serial.write(c);
        header += c;
        if(c=='\n'){
          if(currentLine.length()==0){
            client.println("HTTP/1.1 200 0K");
            client.println("Conexión: Cerrada");
            client.println();

            if(header.indexOf("GET /?plus=true")>=0){
              cont_aux++;
            } else if(header.indexOf("GET /?minus=true")>=0){
              cont_aux--;
            } else if(header.indexOf("GET /?data=alert")>=0){
              alert = true;
            }
            data = true;
            break;
          }else{
            currentLine = "";
          }
        } else if(c!='\r'){
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Cliente desconectado.");
    Serial.println("");
  }
}

// Función para envío de datos como cliente
void send_data(){
  server.stop();
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;

    String data_to_send = "number=" + String(cont) + "&mov=" + String(mov) + "&id=" + id;
    Serial.println(data_to_send);

    http.begin("http://192.168.1.200/datos-prueba.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int code_request = http.POST(data_to_send);

    if(code_request>0){
      Serial.println("Código HTTP > " + String(code_request));

      if(code_request==200){
        String body_request = http.getString();
        Serial.println(body_request);
      }
    }else{
      Serial.print("Error enviando POST, código: ");
      Serial.println(code_request);
    }
    http.end();
  }else{
    Serial.println("Error en la conexión");
  }
  delay(2000);
  server.begin();
}



// Función para animación
void animation(short int cont1, short int cont2){
  int j=0;
  if(cont2>cont1){
    while(j<2){
      pixels.clear();
      for(int i=0; i<cont2+3;i++){
        pixels.setPixelColor(i, pixels.Color(50, 220, 0));
        pixels.show();
        delay(200);
      }
      //alarm();
      j++;
    }
    j=0;
  }else if(cont2<cont1){
    while(j<2){
      pixels.clear();
      for(int i=NUMPIXELS-1; i>=0;i--){
        pixels.setPixelColor(i, pixels.Color(220, 50, 0));
        pixels.show();
        delay(200);
      }
      //alarm();
      j++;
    }
    j=0;
  }
}

// Función de Visualización LED 
void visualize(int counter){
  pixels.clear();
  for(int i=0; i<counter+3;i++){
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
  }
  pixels.show();
}

// Función de Motor(vibración)
void alarm(){
  int i = 0;
  for(i=0;i<3;i++){
    analogWrite(vibePin, 800);
    delay(300);
    analogWrite(vibePin, 0);
    delay(700);
  }
}

// Función para animación Autoevaluación
void animation_question(){
  q = true;
  while(q==true){
    // Serial.println(interrupts);
    pixels.clear();
//    analogWrite(vibePin, 800);
//    delay(200);
//    analogWrite(vibePin, 0);
    for(int i=NUMPIXELS-1; i>=0;i--){
        pixels.setPixelColor(i, pixels.Color(120, 50, 50));
        pixels.show();
        digitalWrite(led1, HIGH);
        digitalWrite(led2, HIGH);
        delay(100);
    }
    pixels.clear();
    for(int i=0; i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(120, 50, 50));
        pixels.show();
        digitalWrite(led1, LOW);
        digitalWrite(led2, LOW);
        delay(100);
    }
    interrupts = interrupts+2;
    if (interrupts == 8){
      q = false;
      pixels.clear();
      analogWrite(vibePin, 0);
      visualize(cont);
      detachInterrupt(digitalPinToInterrupt(buttonPin1));
      detachInterrupt(digitalPinToInterrupt(buttonPin2));
    }
  }
  //scheduledTicker.attach_ms_scheduled(1000, onTime);
  interrupts=0;
}