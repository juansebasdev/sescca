/*
 * Proyecto SESCCA
 * Desarrollo de prototipo conductual para estudiantes.
 * Controlador: ESP8266
 * Actuadores: 2 botones de tipo NA, Cinta LED Neopixel", Motor DC 3v(vibraciones), 2 leds
 * Sensores: 2 sensores de proximidad
 * Envío de datos como Cliente a través de HTTPCLient
 * Recepción de datos como Servidor
 * Elaborado por : Juan Sebastián Bravo Meneses
 * Versión: 1.2
 */

#include <Adafruit_NeoPixel.h>
#include <stdio.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


// Pin salida Neopixel
#define LED 13

// Número de LEDS
#define NUMPIXELS 8

Adafruit_NeoPixel pixels(NUMPIXELS, LED, NEO_GRB+NEO_KHZ800);

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


// Conexión a Red
const char* ssid     = "unifi_network"; // Nombre Red
const char* password = "unifi_network"; // Contraseña Red

// Puerto para servidor

int boardIP[4] = {10, 20, 20, 17}; // Último dígito del arreglo es el ID de la tarjeta
int raspiIP[4] = {10, 20, 20, 150};
int gateIP[4] = {10, 20, 20, 1};
String raspiIPString = String(raspiIP[0])+"."+String(raspiIP[1])+"."+String(raspiIP[2])+"."+String(raspiIP[3]);
//String raspiIPString = "localhost";

WiFiServer server(80);
IPAddress local_IP(boardIP[0], boardIP[1], boardIP[2], boardIP[3]); // dirección IP
IPAddress gateway(gateIP[0], gateIP[1], gateIP[2], gateIP[3]); // Gateway IP 
IPAddress subnet(255, 255, 255, 0); //(255, 255, 0, 0)
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

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
//short int cont=3, cont_aux=3; // change made
short int cont=0, cont_aux=0;
unsigned int mov=0, nomov=0;
short int disrupt = 0;
bool q = false, complete = false;

// Variables para envío y recepción de datos
bool data = false, alert = false;
String id=String(boardIP[3]);

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
  while (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)){
    Serial.println("STA Failed to configure");
  }
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
  delay(200);
  
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
  receive_from_client();
  if(data==true){
    data = false;
    if(alert==true){
      // Serial.println("Alerta");
      attachInterrupt(digitalPinToInterrupt(buttonPin1), cont_plus, RISING);    
      attachInterrupt(digitalPinToInterrupt(buttonPin2), cont_minus, RISING);
      animation_question();
    }
  }
  if(cont!=cont_aux){
    if(cont_aux<0){
      cont_aux=0;
    } else if(cont_aux>8){
      cont_aux = 8;
    }
    if (cont_aux > cont){
      /*Previous code*/ 
      //cont = cont_aux;
      //send_data(1);
      /**/
      cont = cont_aux;
      if (alert==true){
        send_data(1);  
        Serial.println("Sending data...");
        alert=false;
      }
    } else {
      //cont = cont_aux;
      //send_data(0);
      cont = cont_aux;
      if (alert==true){
        send_data(0);
        Serial.println("Sending data...");
        alert=false;  
      }
    }
    visualize(cont);
  }
  /*
  if(data==true){
    data = false;
    if(alert==true){
      // Serial.println("Alerta");
      attachInterrupt(digitalPinToInterrupt(buttonPin1), cont_plus, RISING);    
      attachInterrupt(digitalPinToInterrupt(buttonPin2), cont_minus, RISING);
      alert = false;
      animation_question();
    }
  }
  */
  if(digitalRead(sensorPin1)==HIGH && digitalRead(sensorPin2)==HIGH){
    mov++;
    delay(100);
    // Serial.println("Mov ");
    // Serial.println(mov);
    if(mov == 200) {
      mov = 0;
      nomov = 0;
      disrupt = 1;
      send_data(2);
    }
  }
  if(digitalRead(sensorPin1)==LOW || digitalRead(sensorPin2)==LOW){
    if (mov > 0){
      nomov++;
      delay(100);
      // Serial.println("NoMov ");
      // Serial.print(nomov);
      if(nomov == 200){
        disrupt = 0;
        mov = 0;
        nomov = 0;
        send_data(2);
      }
    }
  }
  if(cont == 8){
    complete = true;
    delay(5000);
    while(cont==8){
      receive_from_client();
      pixels.clear();
      for(int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, pixels.Color(6, 5, 5));
        pixels.show();
        delay(100);
      }
      pixels.clear();
      for(int i = NUMPIXELS-1; i >= 0; i--){
        pixels.setPixelColor(i, pixels.Color(6, 5, 5));
        pixels.show();
        delay(100);
      }
    }
    if(data == true)
        data = false;
    visualize(cont);    
  }
}

// Función de Visualización LED 
void visualize(int counter){
  pixels.clear();
  for(int i=0; i<counter;i++){
    pixels.setPixelColor(i, pixels.Color(0, 3, 0));
  }
  pixels.show();
}

// Función para animación Autoevaluación
void animation_question(){
  pixels.clear();
  int i = 0;
  q = true;
  while (q = true){
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
    delay(400);
    analogWrite(vibePin, 600);
    delay(300);
        
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    analogWrite(vibePin, 0);
    delay(300);

    i++;
    if(i==5){
      q = false;
      visualize(cont);
      digitalWrite(led1, LOW);
      digitalWrite(led2, LOW);
      analogWrite(vibePin, 0);
      detachInterrupt(digitalPinToInterrupt(buttonPin1));
      detachInterrupt(digitalPinToInterrupt(buttonPin2));
      break;
    }
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
            } else if (header.indexOf("GET /?recount=true")>=0){
              complete = false;
              cont_aux = 0;
              cont = cont_aux;
              visualize(cont);
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
void send_data(int state){
  server.stop();
  WiFiClient client;
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String data_to_send;
    int code_request = 0;
    if (state < 2){
      data_to_send = "number=" + String(cont) + "&id=" + id + "&state=" + state + "&drpt=" + String(disrupt);

      Serial.println(data_to_send);

      // http.begin(client, "http://192.168.43.200/data_from_board.php");
      // http.begin(client, "http://sescca.duckdns.org/receive/");
      // http.begin(client, "http://192.168.101.74:8000/evaluation/receive/?" + data_to_send);
      http.begin(client, "http://"+raspiIPString+":8000/evaluation/receive/?"+data_to_send);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      code_request = http.GET();
    } else {
      data_to_send = "drpt=" + String(disrupt) + "&id=" + id;

      Serial.println(data_to_send);

      //http.begin(client, "http://192.168.101.74/disrupt_from_board.php");
      http.begin(client, "http://"+raspiIPString+"/disrupt_from_board.php");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      code_request = http.POST(data_to_send);
    }
    if(code_request>0){
      Serial.println("Código HTTP > " 
      + String(code_request));

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
