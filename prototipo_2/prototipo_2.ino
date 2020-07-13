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
  visualize(cont);
  receive_from_client();
  if(cont!=cont_aux){
    animation(cont,cont_aux);
    if(cont_aux<0){
      cont_aux=0;
    }
    cont = cont_aux;
    //send_data();
  }
  if(cont==5){
    represent(heart,5000);
    cont=0;
    cont_aux=0;
  }
  if(data==true){
    data = false;
    if(alert==true){
      Serial.println("Alerta");
      alarm();
      alert = false;
      count();
    }
  }
//  if(digitalRead(sensorPin1)==HIGH && digitalRead(sensorPin2)==HIGH){
//    alarm();
//  }
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

// Función de Visualización OLED 
void visualize(int counter){
  lc.clearDisplay(0);
  switch(counter){
    case 1:
      escalera[0] = 240;
      escalera[1] = 112;
      escalera[2] = 48;
      escalera[3] = 16;
      escalera[4] = 0;
      escalera[5] = 0;
      escalera[6] = 0;
      break;
   case 2:
      escalera[0] = 248;
      escalera[1] = 120;
      escalera[2] = 56;
      escalera[3] = 24;
      escalera[4] = 8;
      escalera[5] = 0;
      escalera[6] = 0;
      break;
   case 3:
      escalera[0] = 252;
      escalera[1] = 124;
      escalera[2] = 60;
      escalera[3] = 28;
      escalera[4] = 12;
      escalera[5] = 4;
      escalera[6] = 0;
      break;
   case 4:
      escalera[0] = 254;
      escalera[1] = 126;
      escalera[2] = 62;
      escalera[3] = 30;
      escalera[4] = 14;
      escalera[5] = 6;
      escalera[6] = 2;
      break;
    default:
        escalera[0] = 224;
        escalera[1] = 96;
        escalera[2] = 32;
        escalera[3] = 0;
        escalera[4] = 0;
        escalera[5] = 0;
        escalera[6] = 0;
      break;    
    }
    represent(escalera,1000);
    escalera[7] = 1;
    represent(escalera,1000);
    escalera[7] = 0;
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
  }else if(cont2<cont1){
    represent(openeyes3,500);
    represent(openeyes2,500);
    represent(openeyes3,500);
    represent(openeyes2,500);
  }
}

// Función counter-tecla
void count(){
  int tecla=255;
  while(tecla==255){
    tecla = leer_tecla();
    Serial.println(tecla);
  }
  if(tecla==3){
    cont_aux++;
    tecla=255;
  }else if(tecla==2){
    cont_aux--;
    tecla=255;
  }else{
    data=true;
    alert=true;
    tecla=255;
  }
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
      if(884<valor_tecla && valor_tecla<904){
        tecla_pulsada=1;//Azul
      }
      if(855<valor_tecla && valor_tecla<875){
        tecla_pulsada=2;//Verde
      }
      if(979<valor_tecla && valor_tecla<999){
        tecla_pulsada=3;//Blanco
      }
      if(924<valor_tecla && valor_tecla<944){
        tecla_pulsada=4;//Rojo
      }
      if(826<=valor_tecla && valor_tecla<846){
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
