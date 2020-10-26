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

#include "images.h"

// Interrupción Timer
#include <Ticker.h>
Ticker scheduledTicker;

volatile int interrupts;
int number = random(10, 15);

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
const byte buttonPin1 = 15;
const byte buttonPin2 = 14;

// Pines donde se conectan los sensores
const byte sensorPin1 = 13;
const byte sensorPin2 = 12;

// Pin donde se conecta el parlante
const byte soundPin = 16;

// Melodía para alarma
int melody1[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
int noteDurations1[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

// Melodía para valoración negativa
int melody3[] = {
   NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0
};
int noteDurations3[] = {
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3
};

// Melodía para valoración positiva
int melody2[] = {
   NOTE_D5, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, 
  NOTE_D5, NOTE_G4, NOTE_G4,
  NOTE_E5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_FS5,
  NOTE_G5, NOTE_G4, NOTE_G4,
  NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4
};
int noteDurations2[] = {
  4, 8, 8, 8,
  4, 4, 4,
  4,
  8, 8, 8, 8,
  4, 4, 4,
  4,
  8, 8, 8, 8
};

// Variables para visualización
bool circle = false;

// Variables para nivel de conducta
short int cont=0, cont_aux=0;
bool q = false;

// Variables para envío y recepción de datos
bool data = false, alert = false;
String id = "1";
int mov=0;

// Función Interrupcion Timer
void onTime(){
  interrupts++;
  if(interrupts==number){
    data = true;
    alert = true;
    interrupts = 0;
    number = random(40, 60);
    Serial.println(number);
    scheduledTicker.detach();
  }
}

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
//  Serial.print("Conectando a ");
//  Serial.println(ssid);
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED){
//    delay(500);
//    Serial.print(".");
//  }
//  Serial.println("");
//  Serial.println("Conectado");
//  Serial.println("Dirección IP: ");
//  Serial.println(WiFi.localIP());
//
//  server.begin();
//  delay(100);

  // Iniciar Timer
  scheduledTicker.attach_ms_scheduled(1000, onTime);
  
  // Configuración OLED
  display.begin(SSD1306_SWITCHCAPVCC,0x3c);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.setTextWrap(false);
  display.dim(0);
  display.clearDisplay();

  // Activar resistores de PULL-UP
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  // Sensores como Entradas
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);

  // Parlante como salida
  //pinMode(soundPin, OUTPUT);

  visualize();
}

void loop(){
  receive_from_client();
  if(cont!=cont_aux){
    animation(cont,cont_aux);
    if(cont_aux<0){
      cont_aux=0;
    }
    cont = cont_aux;
    visualize();
    //send_data();
  }
  if(cont>5){
    cont=5;
    cont_aux=5;
  }
  if(data==true){
    data = false;
    if(alert==true){
      Serial.println("Alerta");
      attachInterrupt(digitalPinToInterrupt(buttonPin1), cont_plus, RISING);    
      attachInterrupt(digitalPinToInterrupt(buttonPin2), cont_minus, RISING);
      alarm(noteDurations1, melody1);
      alert = false;
      animation_question();
    }
  }
  if(digitalRead(sensorPin1)==HIGH && digitalRead(sensorPin2)==HIGH){
    mov++;
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

    String data_to_send = "number=" + String(cont) + "&mov=" + String(mov) +"&id=" + id;
    Serial.println(data_to_send);

    http.begin("http://192.168.1.21/datos-prueba.php");
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

// Función de Visualización OLED 
void visualize(){
  short int i=0; //j=26, k=0, i=2;
  display.clearDisplay();
  display.drawRect(105, 0, 13, 32, WHITE);
  for(i=0;i<=12;i++){
    display.fillRect(105,i,13,1,WHITE);
  }
  for(i=12;i<=12+cont*4;i++){
    display.fillRect(105,i,13,1,WHITE);
  }
  display.drawBitmap(0,0,emoji,128,64,WHITE);
//  while(i<=14+cont*2){
//    display.fillRect(k,j,10,i,WHITE);
//    k = k+10;
//    j = j-2;
//    i=i+2;
//  }
//  if(circle){
//    display.fillCircle(120,3,2,WHITE); 
//  }else{
//    display.drawCircle(120,3,2,WHITE);
//  }
//  circle=!circle;
  display.display();
}

// Función para animación
void animation(short int cont1, short int cont2){
  if(cont2>cont1){
    happy();
    alarm(noteDurations2, melody2);
  }else if(cont2<cont1){
    attent();
    alarm(noteDurations3, melody3);
  }
}

// Funciones de Visualización animaciones
void happy(){
  display.clearDisplay();
  for(int j=1;j<=2;j++){
    display.drawBitmap(0,0,feliz1,128,64,WHITE);
    display.display();
    delay(500);
    display.drawBitmap(0,0,feliz2,128,64,WHITE);
    display.display();
    delay(500);
    display.clearDisplay();
  }
}

void attent(){
  display.clearDisplay();
  for (int j=1;j<=2;j++){
    display.drawBitmap(0,0,at1,128,64,WHITE);
    display.display();
    delay(500);
    display.drawBitmap(0,0,at2,128,64,WHITE);
    display.display();
    delay(500);
    display.clearDisplay();
  }
}

// Función de Alerta(parlante)
void alarm(int noteDurations[], int melody[]){
  for(int thisNote=0; thisNote<8; thisNote++){
    int noteDuration = 1000/noteDurations[thisNote];
    tone(soundPin, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration*1.30;
    delay(pauseBetweenNotes);
    noTone(soundPin);
  }
}

// Función para animación Autoevaluación
void animation_question(){
  display.clearDisplay();
  q = true;
  display.drawBitmap(0,0,eyes,128,64, WHITE);
  display.display();
  while(q==true){
    delay(100);
    interrupts++;
    if (interrupts == 200){
      q = false;
      visualize();
      detachInterrupt(digitalPinToInterrupt(buttonPin1));
      detachInterrupt(digitalPinToInterrupt(buttonPin2));
    }
  }
  scheduledTicker.attach_ms_scheduled(1000, onTime);
  interrupts=0;
}
