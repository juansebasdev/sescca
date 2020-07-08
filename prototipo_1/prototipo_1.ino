/*
 * Proyecto SESCCA
 * Desarrollo de prototipo 1 de tipo conductual para estudiantes.
 * Controlador: ESP8266
 * Actuadores: 2 botones de tipo NA, Pantalla OLED 0.96", Parlante de 0.25 w 
 * Sensores: 2 sensores de proximidad
 * Envío de datos como Cliente a través de HTTPCLient
 * Recepción de datos como Servidor
 * Elaborado por : Juan Sebastián Bravo Meneses
 * Versión: 1.0
 */

#define SSD1306_128_64
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display;

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "pitches.h"


// Conexión a Red
const char* ssid     = "MOVISTAR_FIBRA_0440"; // Nombre Red
const char* password = "8227854698"; // Contraseña Red

// Puerto para servidor
WiFiServer server(80);

// Variable para guardar HTTP request
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Pines donde se conectan los botones
const byte buttonPin1 = 2;
const byte buttonPin2 = 14;

// Pines donde se conectan los sensores
const byte sensorPin1 = 13;
const byte sensorPin2 = 12;

// Pin donde se conecta el parlante
const byte soundPin = 15;

// Melodía para alarma
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

// Variables para nivel de conducta
short int cont=0, cont_aux=0;

// Variables para envío y recepción de datos
bool data = false, alert = false;

// Funciones Interrupcion Botones
ICACHE_RAM_ATTR void cont_plus(){
  cont_aux++;
  detachInterrupt(digitalPinToInterrupt(buttonPin1));
  detachInterrupt(digitalPinToInterrupt(buttonPin2));
}
ICACHE_RAM_ATTR void cont_minus(){
  cont_aux--;
  detachInterrupt(digitalPinToInterrupt(buttonPin1));
  detachInterrupt(digitalPinToInterrupt(buttonPin2));
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

  // Activar resistores de PULL-UP
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  // Sensores como Entradas
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);

  // Parlante como salida
  pinMode(soundPin, OUTPUT);
}

void loop(){
  
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

            if(header.indexOf("GET /cont/plus")>=0){
              cont_aux++;
            } else if(header.indexOf("GET /cont/minus")>=0){
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

    String data_to_send = "number=" + String(cont);
    Serial.println(data_to_send);

    http.begin("http://192.168.1.10/datos-prueba.php");
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

// Función de Alerta(parlante)
void alarm(){
  for(int thisNote=0; thisNote<8; thisNote++){
    int noteDuration = 1000/noteDurations[thisNote];
    tone(soundPin, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration*1.30;
    delay(pauseBetweenNotes);
    noTone(soundPin);
  }
}
