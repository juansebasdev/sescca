/*
 * Proyecto SESCCA
 * Desarrollo de prototipo 2 de tipo conductual para estudiantes.
 * Controlador: ESP8266
 * Actuadores: Teclado 5 botones analógico, Matriz LED 8x8, Parlante de 0.25 w 
 * Sensores: 2 sensores de proximidad
 * Envío de datos como Cliente a través de HTTPCLient
 * Recepción de datos como Servidor
 * Elaborado por : Juan Sebastián Bravo Meneses
 * Versión: 1.0
 */

#include "LedControlMS.h"
LedControl lc = LedControl(13,14,2,1);

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "pitches.h"

#include "images.h"

// Interrupción Timer
#include <Ticker.h>
Ticker scheduledTicker;

volatile int interrupts;
int number = random(10, 15);

// Habilitar pin analógico para teclado
const int analogInPin=A0;

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


// Pines donde se conectan los sensores
const byte sensorPin1 = 5;
const byte sensorPin2 = 4;

// Pin donde se conecta el parlante
const byte soundPin = 15;

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

// Variables para nivel de conducta
short int cont=4, cont_aux=4;

// Variables para envío y recepción de datos
bool data = false, alert = false;
String id = "2";
int mov = 0;

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
  
  // Configuración Matriz LED
  lc.shutdown(0,false);
  lc.setIntensity(0,1); // Los valores están entre 1 y 15
  lc.clearDisplay(0);

  // Sensores como Entradas
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);

  // Parlante como salida
  pinMode(soundPin, OUTPUT);
}

void loop(){
  receive_from_client();
  if(cont!=cont_aux){
    animation(cont,cont_aux);
    if(cont_aux<0){
      cont_aux=0;
    }
    cont = cont_aux;
    //send_data();
  }
  if(cont>=5){
    //represent(heart,5000);
    cont=5;
    cont_aux=5;
  }
  if(data==true){
    data = false;
    if(alert==true){
      Serial.println("Alerta");
      alarm(noteDurations1, melody1);
      alert = false;
      count();
    }
  }
  if(digitalRead(sensorPin1)==HIGH && digitalRead(sensorPin2)==HIGH){
    mov++;
    //send_data();
  }
  visualize(cont);
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

    String data_to_send = "number=" + String(cont) + "&mov=" + String(mov) + "&id=" + id;
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
void visualize(int counter){
  lc.clearDisplay(0);
  switch(counter){
    case 1:
      escalera[1] = 0;
      escalera[2] = 0;
      escalera[3] = 0;
      escalera[4] = 0;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
    case 2:
      escalera[1] = 0;
      escalera[2] = 0;
      escalera[3] = 0;
      escalera[4] = 15;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
      break;
   case 3:
      escalera[1] = 0;
      escalera[2] = 0;
      escalera[3] = 31;
      escalera[4] = 15;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
      break;
   case 4:
      escalera[1] = 0;
      escalera[2] = 63;
      escalera[3] = 31;
      escalera[4] = 15;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
      break;
   case 5:
      escalera[1] = 127;
      escalera[2] = 63;
      escalera[3] = 31;
      escalera[4] = 15;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
      break;
  default:
      escalera[1] = 0;
      escalera[2] = 0;
      escalera[3] = 0;
      escalera[4] = 0;
      escalera[5] = 7;
      escalera[6] = 3;
      escalera[7] = 1;
      break;    
    }
    represent(escalera,1000);
    escalera[0] = 128;
    represent(escalera,1000);
    escalera[0] = 0;
}

// Función Mostrar Matriz
void represent(byte *Datos, int delay_){
  for(int i=0;i<8;i++){
    lc.setColumn(0,i,Datos[7-i]);
  }
  delay(delay_);
}

// Función para animación
void animation(short int cont1, short int cont2){
  if(cont2>cont1){
    represent(happy, 2000);
    alarm(noteDurations2, melody2);
  }else if(cont2<cont1){
    represent(openeyes3,500);
    represent(openeyes2,500);
    represent(openeyes3,500);
    represent(openeyes2,500);
    alarm(noteDurations3, melody3);
  }
}

// Función counter-tecla
void count(){
  
  int tecla=255;
  interrupts = 0;
  while(tecla==255){
    trans();
    tecla = leer_tecla();
    Serial.println(interrupts);
    interrupts++;
    delay(100);
    if (interrupts > 20){
      break;
    }
  }
  if(tecla==3){
    cont_aux++;
    tecla=255;
  }else if(tecla==1){
    cont_aux--;
    tecla=255;
  }else if (interrupts != 0){ 
    interrupts = 0;
    visualize(cont);
  }
  else{
    data=true;
    alert=true;
    tecla=255;
  }
  scheduledTicker.attach_ms_scheduled(1000, onTime);
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

// Función lectura Teclado Analógico
int leer_tecla(){
  int valor_tecla, tecla_pulsada, t, valor_original, cont_veces;
  tecla_pulsada=255;
  valor_tecla=analogRead(analogInPin);
  if (valor_tecla > 800){
    for (t=0;t<50;t++){
      delay(1);
    }
    valor_original=valor_tecla;
    cont_veces=0;

    for (t=1;t<=40;t++){
      valor_tecla=analogRead(analogInPin);
      if(817<valor_tecla && valor_tecla<857){
        tecla_pulsada=1;//Azul
      }
      if(781<valor_tecla && valor_tecla<821){
        tecla_pulsada=2;//Verde
      }
      if(926<valor_tecla && valor_tecla<966){
        tecla_pulsada=3;//Blanco
      }
      if(865<valor_tecla && valor_tecla<905){
        tecla_pulsada=4;//Rojo
      }
      if(749<=valor_tecla && valor_tecla<789){
        tecla_pulsada=5;//Amarillo 
      }

      if(valor_original==valor_tecla){
        cont_veces++;
      }
    }
  }
  if(cont_veces > 2){
    for (t=0;t<100;t++){
      delay(1);
    }
    return tecla_pulsada;
  }
  else{
    return 255;
  }
}

void trans(){
  for (int row=0; row<8; row++){
    for (int col=0; col<8; col++){
    lc.setLed(0,col,row,true); 
      delay(5);
    }
  }
  for (int row=0; row<8; row++){
    for (int col=0; col<8; col++){
      lc.setLed(0,col,row,false); 
      delay(5);
    }
  }
}
