//////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan

*/
//////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <String.h>

#include <ESP32Servo.h>

#include "Wire.h"

#include <SPI.h>
#include <LoRa.h>

//////////////////////////////////////////////////////////////////////////////////

// Pines LoRa
#define ss 5
#define rst 17
#define dio0 16

// I2C
#define SDA1 4
#define SCL1 25

#define SDA2 15
#define SCL2 14

// Pines LEDS
#define ledstop 0
#define ledwifi 2
#define ledlora 12
int dutyLeds = 30;

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

// Dirección I2C del ESP32 SLAVE
#define I2C_DEV_ADDR 0x55

// Dirección I2C del HDC1080
#define HDC1080_ADDRESS 0x40

// Registros del HDC1080
#define HDC1080_TEMP_REGISTER 0x00
#define HDC1080_HUM_REGISTER 0x01

// Concentrador Solar
const int analogPin1 = 36;  // Arriba 180
const int analogPin2 = 39;  // Abajo 0

const int pinSeguidor = 33; // Seguidor
const int pinReflector = 27; // Reflector

//////////////////////////////////////////////////////////////////////////////////

// LoRa
int esperarDato = 1;
byte datosCorruptos = 0;
int botonStop = 0;

// I2C
String msg;
String separacion_msg[7];

// HDC1080
// Variables necesarias
float tempCacao = 0.0;
float humedadCacao = 0.0;
float tempAmbiente = 0.0;
float humedadAmbiente = 0.0;
float mediaMoviltempCacao[3] = {0,0,0};
float mediaMovilhumeCacao[3] = {0,0,0};
float mediaMoviltempAmbiente[3] = {0,0,0};
float mediaMovilhumeAmbiente[3] = {0,0,0};

Servo servoSeguidor;
Servo servoReflector;

// Seguidor
int desfaseAnguloSeguidor = 100; // 0 GRADOS
int preAnguloLuzIn = desfaseAnguloSeguidor;
int igualdad = 0;

// Reflector
int desfaseAnguloReflector = 38; // 0 GRADOS
int anguloLuzIn = 0;
int anguloCompensacion = 0;

// Objeto
float anguloObjeto = 90.0;
float distanciaObjeto = 0.0; // Distancia desde la punta al objeto
// Definir cateto opuesto y cateto adyacente
int altura = 75;    // Altura de la base hasta el cristal en mm
int anchura = 40;  // Anchura del punto medio hasta la esquina de la base en mm

float compensacionUser = 0.5;
//float compensacionDistancia = 1.0;

//////////////////////////////////////////////////////////////////////////////////

// Prototipos de las tareas
void vTaskConcentrador(void *pvParameters);
void TomaEnvio(void *pvParameters);

//////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  
  // Configurar pines led
  pinMode(ledlora, OUTPUT);
  pinMode(ledwifi, OUTPUT);
  pinMode(ledstop, OUTPUT);

  // Inicia LoRa
  LoRa.setPins(ss, rst, dio0);
  //433E6 for Asia
  //868E6 for Europe
  //915E6 for North America
  for (int i=0; i<10; i++) {
    delay(150);
    LoRa.begin(915E6);
    Serial.print(".");
  }
  Serial.println();
  if (!LoRa.begin(915E6)) {
    Serial.println("Lora no Conectado!");
  }
  else { Serial.println("Lora Conectado!"); }

  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);

  servoSeguidor.attach(pinSeguidor);
  servoReflector.attach(pinReflector);
  
  servoSeguidor.write(desfaseAnguloSeguidor);
  delay(1000);
  servoReflector.write(desfaseAnguloReflector);
  delay(1000);

  I2Cone.begin(SDA1,SCL1);
  I2Ctwo.begin(SDA2,SCL2);

  xTaskCreatePinnedToCore(
    vTaskConcentrador,    // Función de la tarea
    "vTaskConcentrador",   // Nombre de la tarea
    4096,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    1,               // Prioridad
    NULL,            // Manejador de la tarea
    0                // Ejecutar en el Núcleo 0
  );

  // Crear la tarea para manejar el encoder en el Núcleo 0
  xTaskCreatePinnedToCore(
    TomaEnvio,    // Función de la tarea
    "TomaEnvio",   // Nombre de la tarea
    6144,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    2,               // Prioridad
    NULL,            // Manejador de la tarea
    1                // Ejecutar en el Núcleo 0
  );
}

void loop() { }

//////////////////////////////////////////////////////////////////////////////////

// Tarea Concentrador Solar en el Núcleo 0
// Prioridad 1
void vTaskConcentrador(void *pvParameters) {
  while (true) {
    // Control > 60 grados
    if ((tempCacao < 60) && (botonStop == 0)) {
      // Seguidor
      ///////////////////////////////////
      while (igualdad < 5) {
        // Leer los valores de los ADC
        int lectura1 = analogRead(analogPin1); // Arriba
        int lectura2 = analogRead(analogPin2); // Abajo
        
        unsigned int diferencia = abs(lectura1 - lectura2);
        Serial.println("Diferencia: " + String(diferencia) + " || Arriba: " + String(lectura1) + " || Abajo: " + String(lectura2));

        if (diferencia < 50) {
          igualdad++;
        }
        else {
          igualdad = 0;
          if (lectura1 > lectura2) {
            preAnguloLuzIn--;
          if (preAnguloLuzIn > 180) { preAnguloLuzIn = 180; }
          }
          else {
            preAnguloLuzIn++;
            if (preAnguloLuzIn < 0) { preAnguloLuzIn = 0; }
          }
        }
        servoSeguidor.write(preAnguloLuzIn);
        delay(100);
      }

      igualdad = 0;
      anguloLuzIn = map(preAnguloLuzIn, desfaseAnguloSeguidor, 180, 0, 90);
      Serial.println("Angulo Luz: " + String(anguloLuzIn));

      // Calculo Angulo Objeto
      ///////////////////////////////////
      distanciaObjeto = 24*10;
      // Calcular el ángulo en radianes a grados
      int anguloGrados = atan(altura / (anchura + distanciaObjeto)) * (180.0 / PI);
      // Convertir el ángulo de radianes a grados
      float compensacionDistancia = map(distanciaObjeto, 50, 320, 18, 20);
      if (compensacionDistancia < 0) { compensacionDistancia = 0; }
      //Serial.println("distanciaObjeto: " + String(distanciaObjeto) + " | compensacionDistancia: " + String(compensacionDistancia));
      anguloObjeto = (anguloGrados*compensacionDistancia/10) + 90;

      // Reflector
      ///////////////////////////////////
      anguloCompensacion = desfaseAnguloReflector + anguloLuzIn + (anguloObjeto-anguloLuzIn)*0.5;
      //anguloCompensacion = desfaseAnguloReflector + 38;

      if (anguloCompensacion > 180) { anguloCompensacion = 180; }
      else if (anguloCompensacion < 0) { anguloCompensacion = 0; }
      Serial.println("Angulo de Compensacion: " + String(anguloCompensacion));
      servoReflector.write(anguloCompensacion);
    }
    else {
      servoSeguidor.write(desfaseAnguloSeguidor);
      servoReflector.write(desfaseAnguloReflector);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

/////////////////////////////////////////////////

// Tarea toma y envio de datos en el Núcleo 1
// Prioridad 2
void TomaEnvio(void *pvParameters) {
  while (true) {
    digitalWrite(ledstop, 1);
    digitalWrite(ledwifi, 1);
    // Captura de Datos HDC1080 #1 y #2
    // Con Filtro de media movil (3)
    tempAmbiente = leerTemperatura_2();
    mediaMoviltempAmbiente[0] = mediaMoviltempAmbiente[1];
    mediaMoviltempAmbiente[1] = mediaMoviltempAmbiente[2];
    mediaMoviltempAmbiente[2] = tempAmbiente;
    tempAmbiente = (mediaMoviltempAmbiente[0]+mediaMoviltempAmbiente[1]+mediaMoviltempAmbiente[2])/3;

    vTaskDelay(pdMS_TO_TICKS(25));

    humedadAmbiente = leerHumedad_2();
    mediaMovilhumeAmbiente[0] = mediaMovilhumeAmbiente[1];
    mediaMovilhumeAmbiente[1] = mediaMovilhumeAmbiente[2];
    mediaMovilhumeAmbiente[2] = humedadAmbiente;
    humedadAmbiente = (mediaMovilhumeAmbiente[0]+mediaMovilhumeAmbiente[1]+mediaMovilhumeAmbiente[2])/3;

    vTaskDelay(pdMS_TO_TICKS(25));

    tempCacao = leerTemperatura_1();
    mediaMoviltempCacao[0] = mediaMoviltempCacao[1];
    mediaMoviltempCacao[1] = mediaMoviltempCacao[2];
    mediaMoviltempCacao[2] = tempCacao;
    tempCacao = (mediaMoviltempCacao[0]+mediaMoviltempCacao[1]+mediaMoviltempCacao[2])/3;

    vTaskDelay(pdMS_TO_TICKS(25));

    humedadCacao = leerHumedad_1();
    mediaMovilhumeCacao[0] = mediaMovilhumeCacao[1];
    mediaMovilhumeCacao[1] = mediaMovilhumeCacao[2];
    mediaMovilhumeCacao[2] = humedadCacao;
    humedadCacao = (mediaMovilhumeCacao[0]+mediaMovilhumeCacao[1]+mediaMovilhumeCacao[2])/3;

    vTaskDelay(pdMS_TO_TICKS(25));

    // Enviar datos LoRa
    /*
    Separacion: &
    0:  Temperatura principal (Cacao)
    1:  Humedad principal (Cacao)
    2:  Temperatura Ambiente
    3:  Humedad Ambiente
    4:  Stop
    */
    Serial.println("<1&" + String(tempCacao) + "&" + String(humedadCacao) + "&" + String(tempAmbiente) + "&" + String(humedadAmbiente) + "!");
    // Envio Datos
    LoRa.beginPacket();
    LoRa.print("<1&" + String(tempCacao) + "&" + String(humedadCacao) + "&" + String(tempAmbiente) + "&" + String(humedadAmbiente) + "!");
    LoRa.endPacket();

    vTaskDelay(pdMS_TO_TICKS(25));

    // Recibir datos LoRa
    while (esperarDato < 5) {
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        //Serial.println("Conexion LoRa");
        // Recibe el paquete
        while (LoRa.available()) {
          msg = LoRa.readString();
          Serial.println(msg);
        }
        // Separacion de los datos
        /*
        Separacion: &
        0: OK
        1: botonStop
        2: fecha
        3: dianoche
        4: tempCurrent
        5: humedadCurrent
        6: condicionCurrent
        7: pronosticoHora
        8: precipitacion
        */

        // Eliminar Rebote
        int contadorRebote = 0;
        while (true) {
          if (msg[contadorRebote] == '=') { break; }
          else { contadorRebote++; }
        }

        int x = 0;
        String separacion_msg[99];
        for (int i=1; i<msg.length()-1; i++) {
          if (msg[i+contadorRebote] != '&') { separacion_msg[x] = separacion_msg[x] + String(msg[i+contadorRebote]); }
          else { x++; }
        }
        
        Serial.println();

        // Comprobacion
        if ((x == 8) && (msg[0+contadorRebote] == '=') && (msg[msg.length()-1] == '-')) { datosCorruptos = 0; }
        else { datosCorruptos = 1; }

        // Guardado de Datos o Descarte
        if (datosCorruptos == 1) {
          Serial.println("Datos Corruptos");
        }
        else {
          for(int i=0;i<x+1;i++){ Serial.print("#" + String(i+1) + ": " + separacion_msg[i] + " || ");}
          botonStop = separacion_msg[1].toInt();
          Serial.println();
        }
        esperarDato = 10;
      }
      else {
        vTaskDelay(pdMS_TO_TICKS(25));
        esperarDato++;
      }
    }
    esperarDato = 0;
    digitalWrite(ledstop, 0);
    digitalWrite(ledwifi, 0);
    vTaskDelay(pdMS_TO_TICKS(2500));
  }
}

//////////////////////////////////////////////////////////////////////////////////

// HDC1080 #1
// Leer 1
float leerTemperatura_1() {
  // Enviar solicitud de lectura de temperatura
  I2Cone.beginTransmission(HDC1080_ADDRESS);
  I2Cone.write(HDC1080_TEMP_REGISTER);
  I2Cone.endTransmission();
  
  // Esperar que los datos estén listos
  delay(20);

  // Leer los 2 bytes de datos (16 bits)
  I2Cone.requestFrom(HDC1080_ADDRESS, 2);
  if (I2Cone.available() == 2) {
    uint16_t rawTemp = (I2Cone.read() << 8) | I2Cone.read();

    // Convertir los datos a grados Celsius
    return (rawTemp / 65536.0) * 165.0 - 40.0;
  }
  else {
    //Serial.println("Error al leer la temperatura");
    return 0.0; // Retorna 0 en caso de error
  }
}

float leerHumedad_1() {
  // Enviar solicitud de lectura de humedad
  I2Cone.beginTransmission(HDC1080_ADDRESS);
  I2Cone.write(HDC1080_HUM_REGISTER);
  I2Cone.endTransmission();
  
  // Esperar que los datos estén listos
  delay(20);

  // Leer los 2 bytes de datos (16 bits)
  I2Cone.requestFrom(HDC1080_ADDRESS, 2);
  if (I2Cone.available() == 2) {
    uint16_t rawHum = (I2Cone.read() << 8) | I2Cone.read();

    // Convertir los datos a porcentaje de humedad
    return (rawHum / 65536.0) * 100.0;
  }
  else {
    //Serial.println("Error al leer la humedad");
    return 0.0; // Retorna 0 en caso de error
  }
}

// HDC1080 #2
// Leer 2
float leerTemperatura_2() {
  // Enviar solicitud de lectura de temperatura
  I2Ctwo.beginTransmission(HDC1080_ADDRESS);
  I2Ctwo.write(HDC1080_TEMP_REGISTER);
  I2Ctwo.endTransmission();
  
  // Esperar que los datos estén listos
  delay(20);

  // Leer los 2 bytes de datos (16 bits)
  I2Ctwo.requestFrom(HDC1080_ADDRESS, 2);
  if (I2Ctwo.available() == 2) {
    uint16_t rawTemp = (I2Ctwo.read() << 8) | I2Ctwo.read();

    // Convertir los datos a grados Celsius
    return (rawTemp / 65536.0) * 165.0 - 40.0;
  }
  else {
    //Serial.println("Error al leer la temperatura");
    return 0.0; // Retorna 0 en caso de error
  }
}

float leerHumedad_2() {
  // Enviar solicitud de lectura de humedad
  I2Ctwo.beginTransmission(HDC1080_ADDRESS);
  I2Ctwo.write(HDC1080_HUM_REGISTER);
  I2Ctwo.endTransmission();
  
  // Esperar que los datos estén listos
  delay(20);

  // Leer los 2 bytes de datos (16 bits)
  I2Ctwo.requestFrom(HDC1080_ADDRESS, 2);
  if (I2Ctwo.available() == 2) {
    uint16_t rawHum = (I2Ctwo.read() << 8) | I2Ctwo.read();

    // Convertir los datos a porcentaje de humedad
    return (rawHum / 65536.0) * 100.0;
  }
  else {
    //Serial.println("Error al leer la humedad");
    return 0.0; // Retorna 0 en caso de error
  }
}

//////////////////////////////////////////////////////////////////////////////////