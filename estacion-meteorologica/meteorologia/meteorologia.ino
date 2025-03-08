//////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan

*/
//////////////////////////////////////////////////////////////////////////////////

// Blynk
// ID de cada Usuario
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <Arduino.h>
#include <ArduinoJson.h>
#include <String.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Wire.h>

#include <SPI.h>
#include <LoRa.h>

#include <LiquidCrystal_I2C.h>

#include <Preferences.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>

//////////////////////////////////////////////////////////////////////////////////

// Pines LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

// Pines para el KY-040
#define SW_PIN 4
#define DT_PIN 15
#define CLK_PIN 13

// Pines LoRa
#define ss 5
#define rst 17
#define dio0 16

// Pines LEDS
#define ledstop 27
#define ledwifi 14
#define ledlora 12
int dutyLeds = 30;

// Pin STOP
#define stop 25

//////////////////////////////////////////////////////////////////////////////////

// Buffer formato de datos
char buffer[10];

// Guardado de datos
Preferences preferences;
String ssidNvs = "";
String passNvs = "";
String latNvs = "";
String longNvs = "";
String fechaNvs = "";
String condicionNvs = "";
String lluviaNvs = "";

// Contador de Errores
int contErrores = 0;

// WiFI
String SSID = "None";
String PASS = "None";
int wifiCont = 0;
byte wifiConect = 0;

// Blynk
bool cambioBotonStop = 1;
bool botonStop = 1;
int contadorTime = 0;

// LoRa
String msg;
byte loraConect = 0;
byte okLora = 0;
int loraContador = 0;

String LAT = "7.12"; // BUCARAMANGA
String LONG = "-73.11"; // BUCARAMANGA

// Timezonedb
// OpenWeatherMap
String URLtime = "http://api.timezonedb.com/v2.1/get-time-zone?key=";
const String apiKeytime = ""; // KEY PROPIA DE CADA USUARIO

// OpenWeatherMap
String URLopen = "https://api.openweathermap.org/data/2.5/weather?lat=";
const String apiKeyopen = ""; // KEY PROPIA DE CADA USUARIO

// AccuWeather
String URLaccu = "https://dataservice.accuweather.com/forecasts/v1/hourly/1hour/";
const String apiKeyaccu = ""; // KEY PROPIA DE CADA USUARIO

String localidad = "None";

String fecha = "AAAA-MM-DD HH:MM";
float tempCurrent = 0;
float humedadCurrent = 0;
String condicionCurrent = "Esperando informacion";
String estaLloviendoCurrent = "Null";
String pronosticoHora = "Esperando informacion...";
int precipitacion = 0;
String dianoche = "Null";

// KY-040
byte kyMov = 0;
int kySw = 0;
unsigned int calculoTiempo = 0;
int direction = 0;
int antdtState = -1;
int antClkState = -1;
int tiempoGiro = 200;

// Interfaz
/*
0: TEMP & HUMEDAD DEL SISTEMA
1: CLIMA ACTUAL
2: PRONOSTICO
3: INFO EXTRA
4: CONFIG (WIFI & ZONA)
5: CREDITOS
*/
int interfaz = 0;
int subInterfaz = 0;

int movLetras = 0;
byte blinkLCD = 0;
byte esperaLCD = 0;
byte intMov = 0;
byte obtenerPronostico = 0;
int irPronostico = 0;

int intConf_0 = 0; // Select
byte intConf_1 = 0; // SSID/LAT y PASS/LONG
int contadorSubConf  = 0;
byte okChange = 0;

//lcd.print("0123456789012345");
// INTERFAZ -1
//lcd.print(" [!] AAAA-MM-DD ");
//lcd.print("Esperando Inf...");
// INTERFAZ 0
//lcd.print("[T]   000.00 C  ");
//lcd.print("[H]   000.00 %  ");
// INTERFAZ 1
//lcd.print("AAAA-MM-DD HH...");
//lcd.print("000.00C  000.00%");
// INTERFAZ 2
//lcd.print(" Pronostico Hoy ");
//lcd.print("Esperando Inf...");
// INTERFAZ 3
//lcd.print("Informacion E...");
//lcd.print("   [ ENTER! ]   ");
// INTERFAZ 4
//lcd.print(" CONFIGURACION: ");
//lcd.print(" [ WiFi  ZONA ] ");
// INTERFAZ 5
//lcd.print("   HECHO POR:   ");
//lcd.print(" BRAYAN  & JUAN ");
//lcd.print();
                  //lcd.print("0123456789012345");
String palabraprecarInt[14] ={" [!] AAAA-MM-DD ", // -1
                              "Esperando Inf...",

                              "[T]     0.00 C  ",  // 0
                              "[H]     0.00 %  ",

                              "AAAA-MM-DD HH...",  // 1
                              "  0.00C    0.00%",

                              "   Pronostico   ",  // 2
                              "Esperando Inf...",

                              "Informacion E...",  // 3
                              "   [ ENTER! ]   ",

                              " Configuracion: ",  // 4
                              "[WiFi/ZONA/LoRa]",
                              
                              "   HECHO POR:   ",  // 5
                              " BRAYAN  & JUAN "};

int modoLetras = 0;
int modoZona = 0;
const String minuscula[26] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};
const String mayuscula[26] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
const String simbolo[23] = {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "-", "{", "}", "|", ":", ";", "<", ">", ".", "?", "/"};
const String numero[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
String letrasOutput = "a";
String confFinal_1 = "";
String confFinal_2 = "";

// Variables necesarias
float tempCacao = 000.00;
float humedadCacao = 000.00;
float tempAmbiente = 000.00;
float humedadAmbiente = 000.00;
byte datosCorruptos = 0;

//////////////////////////////////////////////////////////////////////////////////

// Prototipos de las tareas
void vTaskEncoder(void *pvParameters);
void vTaskLCD(void *pvParameters);
void vTaskWiFiDatos(void *pvParameters);
void vTaskLoRa(void *pvParameters);

//////////////////////////////////////////////////////////////////////////////////

// Funcion precarga interfaz
void precargaInt() {
  movLetras = 0;
  esperaLCD = 0;
  for (int i=0; i<16; i++) {
    switch (interfaz) {
      case -1:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[0][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[1][i]);
      break;

      case 0:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[2][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[3][i]);
      break;

      case 1:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[4][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[5][i]);
      break;

      case 2:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[6][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[7][i]);
      break;

      case 3:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[8][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[9][i]);
      break;

      case 4:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[10][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[11][i]);
      break;

      case 5:
        lcd.setCursor(i, 0);
        lcd.print(palabraprecarInt[12][i]);
        lcd.setCursor(i, 1);
        lcd.print(palabraprecarInt[13][i]);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void interfazPrin() {
  switch (interfaz) {
    case -1:
    {
      lcd.setCursor(5,0);
      lcd.print(fechaNvs + "                ");

      String msgInstant;
      msgInstant = "Probabilidad de Precipitacion: " + String(precipitacion) + "%";

      if (movLetras > 2) {
        lcd.setCursor(0,1);
        lcd.print(msgInstant.substring(movLetras-3, movLetras+13));
        if (movLetras-3 > msgInstant.length()-17) { movLetras = 0; }
      }
      else {
        lcd.setCursor(0,1);
        lcd.print(msgInstant.substring(0, 16));
      }
      movLetras++;
    }
    break;

    case 0:
    {
      sprintf(buffer, "%3d.%02d", int(tempCacao), int((tempCacao-int(tempCacao))*100));
      lcd.setCursor(6, 0);
      lcd.print(String(buffer));
      sprintf(buffer, "%3d.%02d", int(humedadCacao), int((humedadCacao-int(humedadCacao))*100));
      lcd.setCursor(6, 1);
      lcd.print(String(buffer));
    }
    break;
    
    case 1:
    {
      String msgInstant = fecha + " " + condicionCurrent;
      if (movLetras > 2) {
        lcd.setCursor(0,0);
        lcd.print(msgInstant.substring(movLetras-3, movLetras+13));
        if (movLetras-3 > msgInstant.length()-17) { movLetras = 0; }
      }
      else {
        lcd.setCursor(0,0);
        lcd.print(msgInstant.substring(0, 16));
      }
      movLetras++;

      sprintf(buffer, "%3d.%02d", int(tempCurrent), int((tempCurrent-int(tempCurrent))*100));
      lcd.setCursor(0, 1);
      lcd.print(String(buffer));
      sprintf(buffer, "%3d.%02d", int(humedadCurrent), int((humedadCurrent-int(humedadCurrent))*100));
      lcd.setCursor(9, 1);
      lcd.print(String(buffer));
    }
    break;

    case 2:
    {
      String msgInstant;
      msgInstant = pronosticoHora + "                ";
      
      if (movLetras > 2) {
        lcd.setCursor(0,1);
        lcd.print(msgInstant.substring(movLetras-3, movLetras+13));
        if (movLetras-3 > msgInstant.length()-17) { movLetras = 0; }
      }
      else {
        lcd.setCursor(0,1);
        lcd.print(msgInstant.substring(0, 16));
      }
      movLetras++;
    }
    break;

    case 3:
    {
      String msgInstant = "Informacion Extra";
      if (movLetras > 2) {
        lcd.setCursor(0,0);
        lcd.print(msgInstant.substring(movLetras-3, movLetras+13));
        if (movLetras-3 > msgInstant.length()-17) { movLetras = 0; }
      }
      else {
        lcd.setCursor(0,0);
        lcd.print(msgInstant.substring(0, 16));
      }
      movLetras++;
      lcd.setCursor(3, 1);
      if (blinkLCD == 1) { lcd.print("[ ENTER! ]"); }
      else { lcd.print("          "); }
    }
    break;

    case 4:
    {
      if (blinkLCD == 1) {
        lcd.setCursor(0, 1);
        lcd.print("[WiFi/ZONA/LoRa]");
      }
      else {
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
    }
    break;

    case 5:
    {
      if (blinkLCD == 1) {
        lcd.setCursor(1, 1);
        lcd.print("BRAYAN  & JUAN");
      }
      else {
        lcd.setCursor(1, 1);
        lcd.print("              ");
      }
    }
    break;
  }
}

// Interfaz info ++
void interfazInfo() {
  if (intConf_0 == 0) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" > Info. Wifi   ");
    lcd.setCursor(0,1);
    lcd.print(" Coordenadas    ");
  }
  else if (intConf_0 == 1) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" > Coordenadas   ");
    lcd.setCursor(0,1);
    lcd.print(" Localidad      ");
  }
  else if (intConf_0 == 2) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" > Localidad    ");
    lcd.setCursor(0,1);
    lcd.print(" Sensor Ambiente");
  }
  else if (intConf_0 == 3) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" > Sensor Amb...");
    lcd.setCursor(0,1);
    lcd.print(" Estado WiFi    ");
  }
  else if (intConf_0 == 4) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" > Estado WiFi  ");
    lcd.setCursor(0,1);
    lcd.print(" Estado LoRa    ");
  }
  else if (intConf_0 == 5) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789012345");
    lcd.print(" Estado WiFi    ");
    lcd.setCursor(0,1);
    lcd.print(" > Estado LoRa  ");
  }
}

void interfazInfo_1() {
  if (intConf_0 == 0) {
    lcd.setCursor(0,0);
    lcd.print(" SSID: " + ssidNvs  + "        ");
    lcd.setCursor(0,1);
    lcd.print(" PASS: *****" + passNvs.substring(passNvs.length()-3,passNvs.length()) + "    ");
  }
  else if (intConf_0 == 1) {
    lcd.setCursor(0,0);
  //lcd.print("0123456789013245");
  //lcd.print("[LAT]  -90.99*  ");
  //lcd.print("[LAT] -180.99*  ");
    lcd.print(" LAT:      ");
    lcd.setCursor(8,0);
    lcd.print(latNvs + "  ");

    lcd.setCursor(0,1);
    lcd.print(" LONG:     ");
    lcd.setCursor(7,1);
    lcd.print(longNvs + "  ");
  }
  else if (intConf_0 == 2) {
    lcd.setCursor(0,0);
    lcd.print("Localidad:                ");
    lcd.setCursor(0,1);
    lcd.print(localidad + "                ");
  }
  else if (intConf_0 == 3) {
    lcd.setCursor(0,0);
    lcd.print(" TEMP: -999.00C ");
    lcd.setCursor(0,1);
    lcd.print(" HUM:   100.00% ");
  }
  else if (intConf_0 == 4) {
    if (wifiConect == 0) {
      lcd.setCursor(0, 0);
      lcd.print("  Sin Conexion  ");
      lcd.setCursor(0, 1);
      lcd.print("      WiFi      ");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print(" Conexion  Wifi ");
      lcd.setCursor(0, 1);
      lcd.print("  Establecida!  ");
    }
  }

  else if (intConf_0 == 5) {
    if (loraConect == 0) {
      lcd.setCursor(0, 0);
      lcd.print("    LoRa  NO    ");
      lcd.setCursor(0, 1);
      lcd.print("   Conectado!   ");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print("      LoRa      ");
      lcd.setCursor(0, 1);
      lcd.print("   Conectado!   ");
    }
  }
}

// Interfaz Config
void interfazConfig() {
  if (intConf_0 == 0) {
    lcd.setCursor(0,0);
    lcd.print(" > Config. WiFi ");
    lcd.setCursor(0,1);
    lcd.print(" Config. Zona   ");
  }
  else if (intConf_0 == 1) {
    lcd.setCursor(0,0);
    lcd.print(" > Config. Zona ");
    lcd.setCursor(0,1);
    lcd.print(" Reiniciar Wifi ");
  }
  else if (intConf_0 == 2) {
    lcd.setCursor(0,0);
    lcd.print(" > Rein... Wifi ");
    lcd.setCursor(0,1);
    lcd.print(" Reiniciar LoRa ");
  }
  else if (intConf_0 == 3) {
    lcd.setCursor(0,0);
    lcd.print(" Reiniciar Wifi ");
    lcd.setCursor(0,1);
    lcd.print(" > Rein... LoRa ");
  }
}

// Extras
void interfazConfigNum() {
  // LAT
  if (modoZona == 0) {
  //lcd.print("0123456789013245");
  //lcd.print("[LAT]  -90.99*  ");
    if (blinkLCD == 1) {
      if (okChange == 0) {
        sprintf(buffer, "%3d", contadorSubConf);
        lcd.setCursor(0,0);
        lcd.print("[LAT]  " + String(buffer) + ".00   ");
        lcd.setCursor(0,1);
        lcd.print("  [SIGUIENTE]   ");
      }
      else {
        sprintf(buffer, "%02d", contadorSubConf);
        lcd.setCursor(11,0);
        lcd.print(String(buffer) + "   ");
        lcd.setCursor(0,1);
        lcd.print("  [SIGUIENTE]   ");
      }
    }
    else {
      if (okChange == 0) {
        lcd.setCursor(0,0);
        lcd.print("[LAT]     .00   ");
      }
      else {
        lcd.setCursor(11,0);
        lcd.print("     ");
      }
    }
  }
  // LONG
  else {
  //lcd.print("0123456789012345");
  //lcd.print("[LONG] -180.99° ");
    if (blinkLCD == 1) {
      if (okChange == 0) {
        sprintf(buffer, "%4d", contadorSubConf);
        lcd.setCursor(0,0);
        lcd.print("[LONG] " + String(buffer) + ".00  ");
        lcd.setCursor(0,1);
        lcd.print("  [CONFIRMAR]   ");
      }
      else {
        sprintf(buffer, "%02d", contadorSubConf);
        lcd.setCursor(12,0);
        lcd.print(String(buffer) + "  ");
        lcd.print("  [CONFIRMAR]   ");
      }
    }
    else {
        if (okChange == 0) {
        sprintf(buffer, "%4d", contadorSubConf);
        lcd.setCursor(0,0);
        lcd.print("[LONG]     .00  ");
      }
      else {
        sprintf(buffer, "%02d", contadorSubConf);
        lcd.setCursor(12,0);
        lcd.print("    ");
      }
    }
  }
}

void interfazConfigLetras() {
  if (okChange == 0) {
    if (modoLetras == 5) {
      if (blinkLCD == 1) {
        lcd.setCursor(0,0);
        lcd.print("[SSID]  [ OK ]  ");
      }
      else {
        lcd.setCursor(0,0);
        lcd.print("[SSID]          ");
      }
    }
    else if (modoLetras == 4) {
      if (blinkLCD == 1) {
        lcd.setCursor(0,0);
        lcd.print("[SSID]  [ <- ]  ");
      }
      else {
        lcd.setCursor(0,0);
        lcd.print("[SSID]          ");
      }
    }
    else {
      lcd.setCursor(0,0);
      lcd.print("[SSID]   > " + String(letrasOutput) + " <  ");
    }
    lcd.setCursor(0,1);
    lcd.print(confFinal_1 + "                ");
  }
  else {
    if (modoLetras == 5) {
      if (blinkLCD == 1) {
        lcd.setCursor(0,0);
        lcd.print("[PASS]  [ OK ]  ");
      }
      else {
        lcd.setCursor(0,0);
        lcd.print("[PASS]          ");
      }
    }
    else if (modoLetras == 4) {
      if (blinkLCD == 1) {
        lcd.setCursor(0,0);
        lcd.print("[PASS]  [ <- ]  ");
      }
      else {
        lcd.setCursor(0,0);
        lcd.print("[PASS]          ");
      }
    }
    else {
      lcd.setCursor(0,0);
      lcd.print("[PASS]   > " + String(letrasOutput) + " <  ");
    }
    lcd.setCursor(0,1);
    lcd.print(confFinal_2 + "                ");
  }
}

void letrasOut() {
  switch (modoLetras) {
    case 0:
      letrasOutput = minuscula[contadorSubConf];
    break;

    case 1:
      letrasOutput = mayuscula[contadorSubConf];
    break;

    case 2:
      letrasOutput = simbolo[contadorSubConf];
    break;

    case 3:
      letrasOutput = numero[contadorSubConf];
    break;
  }
}

//////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  // Configurar pines led
  pinMode(ledlora, OUTPUT);
  pinMode(ledwifi, OUTPUT);
  pinMode(ledstop, OUTPUT);
  
  // Iniciar NVS
  preferences.begin("my-settings", false); // Abre o crea el espacio de almacenamiento "my-settings"
  // Leer las cadenas
  ssidNvs = preferences.getString("ssidNvs", "None");
  passNvs = preferences.getString("passNvs", "None");
  latNvs = preferences.getString("latNvs", "7.12"); // BUCARAMANGA
  longNvs = preferences.getString("longNvs", "-73.11"); //
  fechaNvs = preferences.getString("fechaNvs", "AAAA-MM-DD");
  condicionNvs = preferences.getString("condicionNvs", "Esperando informacion...");
  lluviaNvs = preferences.getString("lluviaNvs", "0");

  Serial.println();
  Serial.println(ssidNvs);
  Serial.println(passNvs);
  Serial.println(latNvs);
  Serial.println(longNvs);
  Serial.println(fechaNvs);
  Serial.println(condicionNvs);
  Serial.println(lluviaNvs);
  Serial.println();

  preferences.end(); // Cierra el espacio de almacenamiento

  if (SSID != ssidNvs) { SSID = ssidNvs; }
  if (PASS != passNvs) { PASS = passNvs; }
  if (LAT != latNvs) { LAT = String(latNvs.toFloat()); }
  if (LONG != longNvs) { LONG = String(longNvs.toFloat()); }

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando  Wifi");

  // Start WiFi
  WiFi.disconnect(true, true);
  WiFi.begin(SSID, PASS);
  Serial.println("Conectando a WiFi");
  for (int i=0; i<10; i++) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true, true);
    lcd.setCursor(0, 0);
    lcd.print("  Sin Conexion  ");
    lcd.setCursor(0, 1);
    lcd.print("      WiFi      ");
    Serial.println("Wifi no Conectado");
    analogWrite(ledwifi, 0);
    wifiConect = 0;
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print(" Conexion  Wifi ");
    lcd.setCursor(0, 1);
    lcd.print("  Establecida!  ");
    Serial.println("WiFi Conectado");
    analogWrite(ledwifi, dutyLeds);
    wifiConect = 1;
  }
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Iniciando LoRa ");

  // Inicia LoRa
  LoRa.setPins(ss, rst, dio0);
  Serial.println("Iniciando LoRa!");
  //433E6 for Asia
  //868E6 for Europe
  //915E6 for North America
  for (int i=0; i<10; i++) {
    delay(250);
    LoRa.begin(915E6);
    Serial.print(".");
  }
  Serial.println();
  if (!LoRa.begin(915E6)) {
    lcd.setCursor(0, 0);
    lcd.print("    LoRa  NO    ");
    lcd.setCursor(0, 1);
    lcd.print("   Conectado!   ");
    Serial.println("Lora no Conectado!");
    analogWrite(ledlora, 0);
    loraConect = 0;
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("      LoRa      ");
    lcd.setCursor(0, 1);
    lcd.print("   Conectado!   ");
    Serial.println("Lora Conectado!");
    // Change sync word (0xF3) to match the receiver
    // The sync word assures you don't get LoRa messages from other LoRa transceivers
    // ranges from 0-0xFF
    LoRa.setSyncWord(0xF3);
    analogWrite(ledlora, dutyLeds);
    loraConect = 1;
  }

  delay(1500);

  lcd.setCursor(0, 0);
  lcd.print("  PROYECTO  DE  ");
  lcd.setCursor(0, 1);
  lcd.print("     GRADO!     ");

  delay(1500);

  precargaInt();

  // Configurar pines del KY-040 como entradas
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  // Configurar pin STOP
  pinMode(stop, INPUT_PULLUP);

  if (wifiConect == 1) { Blynk.begin(BLYNK_AUTH_TOKEN, SSID.c_str(), PASS.c_str()); }

  // ENCODER
  xTaskCreatePinnedToCore(
    vTaskEncoder,    // Función de la tarea
    "vTaskEncoder",   // Nombre de la tarea
    2048,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    2,               // Prioridad
    NULL,            // Manejador de la tarea
    0                // Ejecutar en el Núcleo 0
  );

  // LoRa
  xTaskCreatePinnedToCore(
    vTaskLoRa,    // Función de la tarea
    "vTaskLoRa",   // Nombre de la tarea
    4096,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    4,               // Prioridad
    NULL,            // Manejador de la tarea
    0                // Ejecutar en el Núcleo 0
  );

  // Wifi
  xTaskCreatePinnedToCore(
    vTaskWiFiDatos,       // Función de la tarea
    "vTaskWiFiDatos",      // Nombre de la tarea
    20000,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    2,               // Prioridad
    NULL,            // Manejador de la tarea
    1                // Ejecutar en el Núcleo 1
  );

  // LCD
  xTaskCreatePinnedToCore(
    vTaskLCD,       // Función de la tarea
    "vTaskLCD",      // Nombre de la tarea
    2048,            // Tamaño de la pila
    NULL,            // Parámetros de entrada
    5,               // Prioridad
    NULL,            // Manejador de la tarea
    1                // Ejecutar en el Núcleo 1
  );


}

//////////////////////////////////////////////////////////////////////////////////

String convertirUNIXATiempo(unsigned long unixTime) {
  // Ajuste manual para GMT-0500 (restar 5 horas en segundos)
  const int timezoneOffset = -5 * 3600;  // -5 horas en segundos
  time_t rawTime = unixTime + timezoneOffset;

  // Convertir UNIX a hora legible (hora:minutos)
  struct tm* ti = localtime(&rawTime);
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", ti);  // Formato 24h sin segundos
  return String(buffer);
}

/*
String precipitacionEs(String msg) {
  if (msg == "None") { return "Nula"; }
  else if (msg == "Light") { return "Ligera"; }
  else if (msg == "Moderate") { return "Moderada"; }
  else if (msg == "Heavy") { return "Fuerte"; }
  else if (msg == "Extreme") { return "Extrema"; }
  else { return "Null"; }
}*/

void obtenerFechaHora() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // URL de la API de TimeZoneDB
    String urlTZDB = URLtime + apiKeytime + "&format=json&by=position&lat=" + LAT + "&lng=" + LONG;

    http.begin(urlTZDB);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();

      // Analizar JSON para extraer datos
      DynamicJsonDocument docTZDB(1024);
      deserializeJson(docTZDB, payload);
      
      // Extraer información
      String formatted = docTZDB["formatted"]; // Fecha y hora formateadas
      fecha = formatted.substring(0, 16);

      // Mostrar resultados en Serial
      Serial.println("Fecha y Hora: " + fecha);
    }
    else { Serial.println("Error en la solicitud HTTP a TimeZoneDB."); }
    http.end();
  }
  else {}
}

void obtenerDatosClima() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // URL de la API de OpenWeatherMap
    String urlOWM = URLopen + LAT + "&lon=" + LONG + "&appid=" + apiKeyopen + "&lang=es&units=metric";

    http.begin(urlOWM);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();

      // Analizar JSON para extraer datos
      DynamicJsonDocument docOWM(1024);
      deserializeJson(docOWM, payload);
  
      // Extraer información
      String name = docOWM["name"]; // Nombre de la localidad
      float temp = docOWM["main"]["temp"]; // Temperatura
      float humidity = docOWM["main"]["humidity"]; // Humedad
      String main = docOWM["weather"][0]["main"]; // Condicion general
      String description = docOWM["weather"][0]["description"]; // Condicion detallada
      float sunrise = docOWM["sys"]["sunrise"];  // Hora de amanecer en UNIX
      float sunset = docOWM["sys"]["sunset"];    // Hora de atardecer en UNIX
      
      localidad = name;
      tempCurrent = temp;
      humedadCurrent = humidity;
      estaLloviendoCurrent = main;
      condicionCurrent = description;
      if ((fecha.substring(11,16) >= convertirUNIXATiempo(sunrise)) && (fecha.substring(11,16) < convertirUNIXATiempo(sunset))) { dianoche = "Dia"; }
      else { dianoche = "Noche"; }

      // Mostrar resultados en Serial
      Serial.println("Localidad: " + name);
      Serial.println("Temperatura Current: " + String(temp) + " °C");
      Serial.println("Humedad Current: " + String(humidity) + " %");
      Serial.println("Condicion general Current: " + main);
      Serial.println("Condicion detallada Current: " + description);
      // Convertir y mostrar la hora en formato legible
      Serial.println("Es de: " + dianoche);
    }
    else { Serial.println("Error en la solicitud HTTP a OpenWeatherMap."); }
    http.end();
  }
  else {}
}

void obtenerDatosPronostico() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Obtener el código de ubicación basado en las coordenadas
    String locationUrl = "http://dataservice.accuweather.com/locations/v1/cities/geoposition/search?apikey=" + apiKeyaccu + "&q=" + LAT + "," + LONG + "&language=es";
    
    http.begin(locationUrl);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String locationPayload = http.getString();
      DynamicJsonDocument locationDoc(1024);
      deserializeJson(locationDoc, locationPayload);
      
      String locationKey = locationDoc["Key"]; // Obtener el Key de la ubicación
      String localidad = locationDoc["LocalizedName"]; // Obtener el nombre de la localidad

      // Ahora que tenemos el Key, podemos obtener el pronóstico
      String forecastUrl = URLaccu + locationKey + "?apikey=" + apiKeyaccu + "&language=es&metric=true";
      http.begin(forecastUrl);
      httpResponseCode = http.GET();
      
      if (httpResponseCode > 0) {
        String forecastPayload = http.getString();

        // Analizar JSON para extraer datos
        DynamicJsonDocument forecastDoc(2048);
        deserializeJson(forecastDoc, forecastPayload);
        
        // Extraer y mostrar Pronostico y probabilidad de precipitaciones
        String IconPhrase = forecastDoc[0]["IconPhrase"]; // Pronostico del Dia
        int PrecipitationProbability = forecastDoc[0]["PrecipitationProbability"];
        
        fechaNvs = fecha;
        condicionNvs = IconPhrase;
        lluviaNvs = String(PrecipitationProbability);

        preferences.begin("my-settings", false); // Abre o crea el espacio de almacenamiento "my-settings"
        // Guardar las cadenas
        preferences.putString("fechaNvs", fechaNvs);
        preferences.putString("condicionNvs", condicionNvs);
        preferences.putString("lluviaNvs", lluviaNvs);

        Serial.println("Guardando fecha: " + fechaNvs);
        Serial.println("Guardando condicion: " + condicionNvs);
        Serial.println("Guardando precipitacion: " + lluviaNvs);
        preferences.end(); // Cierra el espacio de almacenamiento

        obtenerPronostico = 0;
      }
      else { Serial.println("Error en la solicitud HTTP para el pronóstico."); }
    }
    else { Serial.println("Error en la solicitud HTTP para la ubicación."); }
    http.end();
  }
  else {}
}

BLYNK_WRITE(V12) { // V12 es el Virtual Pin para el botón
  if (param.asInt() == 1) { botonStop = !botonStop; }
}

void loop() {}

//////////////////////////////////////////////////////////////////////////////////

// Tarea que maneja el encoder en el Núcleo 0
// Prioridad 2
void vTaskEncoder(void *pvParameters) {
  while (true) {
    byte dtState = digitalRead(DT_PIN);
    byte clkState = digitalRead(CLK_PIN);

    ///////////////////////////////
    if (digitalRead(SW_PIN) == LOW) {
      kySw = 1;
      vTaskDelay(pdMS_TO_TICKS(250));
    }

    if (kySw == 2) {
      kySw = 0;
      calculoTiempo = 0;
      Serial.println("Pulsar");
      // Press Interfaz
      switch (subInterfaz) {
        case 1:
          // Informacion ++
          if (interfaz == 3) {
            if (intConf_0 == 0) {
              subInterfaz = 2;
            }
            if (intConf_0 == 1) {
              subInterfaz = 2;
            }
            if (intConf_0 == 2) {
              subInterfaz = 2;
            }
            if (intConf_0 == 3) {
              subInterfaz = 2;
              movLetras = 0;
            }
            if (intConf_0 == 4) {
              subInterfaz = 2;
            }
            if (intConf_0 == 5) {
              subInterfaz = 2;
            }
          }
          // WiFi & Zona
          else if (interfaz == 4) {
            // Entrar WiFi
            if (intConf_0 == 0) {
              subInterfaz = 2;
              contadorSubConf = 0;
              okChange = 0;
              modoLetras = 0;
              letrasOutput = "a";
              confFinal_1 = "";
              confFinal_2 = "";
            }
            // Entrar Zona
            else if (intConf_0 == 1) {
              subInterfaz = 2;
              contadorSubConf = 0;
              okChange = 0;
              modoZona = 0;
              tiempoGiro = 25;
              confFinal_1 = "";
              confFinal_2 = "";
            }
            // Entrar Reset WiFi
            else if (intConf_0 == 2) {
              subInterfaz = 3;
            }
            // Entrar Reset LoRa
            else if (intConf_0 == 3) {
              subInterfaz = 4;
            }
          }
        break;

        case 2:
          // Informacion ++ 
          if (interfaz == 3) {
            subInterfaz = 1;
          }
          // WiFi & Zona
          else if (interfaz == 4) {
            // WiFi
            if (intConf_0 == 0) {
              modoLetras++;
              if (modoLetras == 6) { modoLetras = 0; }
              // Simbolos
              if ((contadorSubConf > 22) && (modoLetras == 2)) { contadorSubConf = 22; }
              // Numeros
              else if ((contadorSubConf > 9) && (modoLetras == 3)) { contadorSubConf = 9; }
              // Config Final
              else if ((contadorSubConf > 0) && (modoLetras == 4)) { contadorSubConf = 0; }
            }
            // Zona
            else if (intConf_0 == 1) {
              // ACEPTAR CONFIG ZONA
              // LAT
              if (modoZona == 0) {
                if (okChange == 0) {
                  sprintf(buffer, "%3d", contadorSubConf);
                  confFinal_1 = String(buffer);
                  contadorSubConf = 0;
                  okChange = 1;
                }
                else {
                  sprintf(buffer, "%02d", contadorSubConf);
                  confFinal_1 = confFinal_1 + "." + String(buffer);
                  contadorSubConf = 0;
                  okChange = 0;
                  modoZona = 1;
                }
              }
              // LONG
              else {
                if (okChange == 0) {
                  sprintf(buffer, "%4d", contadorSubConf);
                  confFinal_2 = String(buffer);
                  contadorSubConf = 0;
                  okChange = 1;
                }
                else {
                  sprintf(buffer, "%02d", contadorSubConf);
                  confFinal_2 = confFinal_2 + "." + String(buffer);
                  Serial.println("Configuracion final: " + String (confFinal_1) + " | " + String(confFinal_2));
                  tiempoGiro = 200;
                  intMov = 1;
                  interfaz = 0;
                  subInterfaz = 0;
                  obtenerPronostico = 1;

                  preferences.begin("my-settings", false); // Abre o crea el espacio de almacenamiento "my-settings"
                  // Guardar las cadenas
                  latNvs = confFinal_1;
                  preferences.putString("latNvs", confFinal_1);
                  longNvs = confFinal_2;
                  preferences.putString("longNvs", confFinal_2);
                  LAT = String(latNvs.toFloat());
                  LONG = String(longNvs.toFloat());
                  preferences.end(); // Cierra el espacio de almacenamiento
                }
              }
            }
          }
        break;

        case 3:
        break;

        default:
          if (interfaz == -1) {
            lcd.backlight();
            irPronostico = 0;
            intMov = 1;
            interfaz = 0;
          }
          // Entrar Control Manual
          else if (interfaz == 3) {
            irPronostico = 0;
            subInterfaz = 1;
            intConf_0 = 0;
          }
          // Entrar WiFi & Zona
          else if (interfaz == 4) {
            irPronostico = 0;
            subInterfaz = 1;
            intConf_0 = 0;
          }
          else { irPronostico = 0; }
      }
    }
    else if (kySw == 1) {
      kySw++;
      // Calculo Mantener
      calculoTiempo = calculoTiempo + 270;
      // MANTENER
      if (calculoTiempo == 270*3) {
        kySw = 0;
        calculoTiempo = 0;
        if ((subInterfaz > 1) && (subInterfaz < 3) && (interfaz == 4)) {
          // OK
          // INTERFAZ CONFIG
          if ((intConf_0 == 0) && (interfaz == 4)) {
            if (okChange == 0) {
              if (modoLetras == 4) {
                if (confFinal_1.length() < 0) { confFinal_1 = ""; }
                else { confFinal_1 = confFinal_1.substring(0,confFinal_1.length()-1); }
              }
              else if (modoLetras == 5) {
                  modoLetras = 0;
                  contadorSubConf = 0;
                  okChange = 1;
                }
              else { confFinal_1 = confFinal_1 + letrasOutput; }
            }
            else {
              if (modoLetras == 4) {
                if (confFinal_2.length() < 0) { confFinal_2 = ""; }
                else { confFinal_2 = confFinal_2.substring(0,confFinal_2.length()-1); }
              }
              else if (modoLetras == 5) {
                Serial.println("Configuracion final: " + String (confFinal_1) + " | " + String(confFinal_2));
                subInterfaz = 3;

                preferences.begin("my-settings", false); // Abre o crea el espacio de almacenamiento "my-settings"
                // Guardar las cadenas
                ssidNvs = confFinal_1;
                preferences.putString("ssidNvs", confFinal_1);
                passNvs = confFinal_2;
                preferences.putString("passNvs", confFinal_2);
                SSID = ssidNvs;
                PASS = passNvs;
                preferences.end(); // Cierra el espacio de almacenamiento
              }
              else { confFinal_2 = confFinal_2 + letrasOutput; }
            }
          }
        }
        else {
          // RESET
          Serial.println("Reset");
          intMov = 1;

          interfaz = 0;
          subInterfaz = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }

    ///////////////////////////////
    if ((antdtState == 1) && (antClkState == 1)) {
      if ((dtState == 1)&&(clkState == 0)) {
        kyMov = 1;
        direction = 2;
      }
      else if ((dtState == 0)&&(clkState == 1)) {
        kyMov = 1;
        direction = 1;
      }
    }  

    ///////////////////////////////
    if (kyMov == 1) {
      if (direction == 2) {
        switch (subInterfaz) {
          case 1:
            // Informacion ++ 
            if (interfaz == 3) {
              intConf_0++;
              if (intConf_0 > 5) { intConf_0 = 5; }
            }
            // WiFi & Zona
            else if (interfaz == 4) {
              intConf_0++;
              if (intConf_0 > 3 ) { intConf_0 = 3; }
            }
          break;

          case 2:
            contadorSubConf++;
            // Informacion ++ 
            if (interfaz == 3) {}
            // WiFi & Zona
            else if (interfaz == 4) {
              // WIFI
              if (intConf_0 == 0) {
                // Letras
                if ((modoLetras == 0) || (modoLetras == 1)) {
                  if (contadorSubConf > 25) { contadorSubConf = 0; }
                }
                // Simbolos
                else if (modoLetras == 2) {
                  if (contadorSubConf > 22) { contadorSubConf = 0; }
                }
                // Numeros
                else if (modoLetras == 3) {
                  if (contadorSubConf > 9) { contadorSubConf = 0; }
                }
                // Config Final
                else if (modoLetras == 4) {
                  if (contadorSubConf > 0) { contadorSubConf = 0; }
                }
              }
              // ZONA
              else {
                // LAT
                if (modoZona == 0) {
                  // 90 a -90
                  if (okChange == 0) { if (contadorSubConf > 90) { contadorSubConf = -90; } }
                  // 99 a 0
                  else { if (contadorSubConf > 99) { contadorSubConf = 0; } }
                }
                // LONG
                else {
                  // 180 a -180
                  if (okChange == 0) { if (contadorSubConf > 180) { contadorSubConf = -180; } }
                  // 99 a 0
                  else { if (contadorSubConf > 99) { contadorSubConf = 0; } }
                }
              }
            }
          break;

          case 3:
          break;

          // Principal
          default:
            if (interfaz == -1) {
              lcd.backlight();
              irPronostico = 0;
              intMov = 1;
              interfaz = 0;
            }
            else {
              irPronostico = 0;
              intMov = 1;
              interfaz++;
              if (interfaz > 5) {
                interfaz = 0;
              }
            }
        }
        
        Serial.println("Girando a la derecha");
        vTaskDelay(pdMS_TO_TICKS(tiempoGiro));
      }
      else if (direction == 1) {
        switch (subInterfaz) {
          case 1:
            // Informacion ++ 
            if (interfaz == 3) {
              intConf_0--;
              if (intConf_0 < 0) { intConf_0 = 0; }
            }
            // WiFi & Zona
            else if (interfaz == 4) {
              intConf_0--;
              if (intConf_0 < 0) { intConf_0 = 0; }
            }
          break;

          case 2:
            contadorSubConf--;
            // Informacion ++ 
            if (interfaz == 3) {}
            // WiFi & Zona
            else if (interfaz == 4) {
              // WIFI
              if (intConf_0 == 0) {
                // Letras
                if ((modoLetras == 0) || (modoLetras == 1)) {
                  if (contadorSubConf < 0) { contadorSubConf = 25; }
                }
                // Simbolos
                else if (modoLetras == 2) {
                  if (contadorSubConf < 0) { contadorSubConf = 22; }
                }
                // Numeros
                else if (modoLetras == 3) {
                  if (contadorSubConf < 0) { contadorSubConf = 9; }
                }
                // Config Final
                else if (modoLetras == 4) {
                  if (contadorSubConf < 0) { contadorSubConf = 0; }
                }
              }
              // ZONA
              else {
                // LAT
                if (modoZona == 0) {
                  // 90 a -90
                  if (okChange == 0) { if (contadorSubConf < -90) { contadorSubConf = 90; } }
                  // 99 a 00
                  else { if (contadorSubConf < 0) { contadorSubConf = 99; } }
                }
                // LONG
                else {
                  // 180 a -180
                  if (okChange == 0) { if (contadorSubConf < -180) { contadorSubConf = 180; } }
                  // 99 a 00
                  else { if (contadorSubConf < 0) { contadorSubConf = 99; } }
                }
              }
            }
          break;

          case 3:
          break;

          // Principal
          default:
            if (interfaz == -1) {
              lcd.backlight();
              irPronostico = 0;
              intMov = 1;
              interfaz = 0;
            }
            else {
              irPronostico = 0;
              intMov = 1;
              interfaz--;
              if (interfaz < 0) {
                interfaz = 5;
              }
            }
        }

        Serial.println("Girando a la izquierda");
        vTaskDelay(pdMS_TO_TICKS(tiempoGiro));
      }
    }

    ///////////////////////////////
    kyMov = 0;
    antdtState = dtState;
    antClkState = clkState;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Tarea LoRa en el Núcleo 0
// Prioridad 4
void vTaskLoRa(void *pvParameters) {
  while (true) {
    if (loraConect == 1) {
      if (digitalRead(stop) == 0) {
        botonStop = !botonStop;
        vTaskDelay(pdMS_TO_TICKS(250));
      }

      // Recibir datos LoRa
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        loraContador = 0;
        analogWrite(ledlora, 0);
        //Serial.println("Conexion LoRa");
        // Recibe el paquete
        while (LoRa.available()) {
          msg = LoRa.readString();
          //Serial.println(msg);
        }
        // Separacion de los datos
        /*
        Separacion: &
        0:  OK
        1:  Temperatura principal (Cacao)
        2:  Humedad principal (Cacao)
        3:  Temperatura Ambiente
        4:  Humedad Ambiente
        5:  Stop
        */

        int x = 0;
        String separacion_msg[99];
        for (int i=1; i<msg.length()-1; i++) {
          if (msg[i] != '&') { separacion_msg[x] = separacion_msg[x] + String(msg[i]); }
          else { x++; }
        }

        // Comprobacion
        if ((x == 4) && (msg[0] == '<') && (msg[msg.length()-1] == '!')) { datosCorruptos = 0; }
        else { datosCorruptos = 1; }

        // Guardado de Datos o Descarte
        if (datosCorruptos == 1) {
          Serial.println("Datos Corruptos");
        }
        else {
          //for(int i=0;i<x+1;i++){ Serial.print("#" + String(i+1) + ": " + separacion_msg[i] + " || "); }
          //Serial.println();
          okLora = separacion_msg[0].toInt();
          tempCacao = separacion_msg[1].toFloat();
          humedadCacao = separacion_msg[2].toFloat();
          tempAmbiente = separacion_msg[3].toFloat();
          humedadAmbiente = separacion_msg[4].toFloat();
        }

        vTaskDelay(pdMS_TO_TICKS(25));

        // Enviar datos LoRa
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
        
        // Envio Datos
        for (int i=0; i<3; i++) {
          LoRa.beginPacket();
          Serial.println("=1&" + String(botonStop) + "&" + fecha + "&" + dianoche + "&" + String(tempCurrent) + "&" + String(humedadCurrent) + "&" + condicionCurrent + "&" + pronosticoHora + "&" + String(precipitacion) + "-");
          LoRa.print("<1&100.00&100.00&100.00&100.00!????????=1&" + String(botonStop) + "&" + fecha + "&" + dianoche + "&" + String(tempCurrent) + "&" + String(humedadCurrent) + "&" + condicionCurrent + "&" + pronosticoHora + "&" + String(precipitacion) + "-");
          LoRa.endPacket();
          vTaskDelay(pdMS_TO_TICKS(25/2));
        }

        vTaskDelay(pdMS_TO_TICKS(25));
        analogWrite(ledlora, dutyLeds);
      }
      else {
        //Serial.println(loraContador);
        loraContador++;
        if (loraContador == 2400) {
          Serial.println("No hay conexion con el Concentrador");
          loraContador = 0;
          okLora = 0;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(25));
    }
    else { vTaskDelay(pdMS_TO_TICKS(10*1000)); }
  }
}

/////////////////////////////////////////

// Tarea WiFi en el Núcleo 1
// Prioridad 4
void vTaskWiFiDatos(void *pvParameters) {
  while (true) {

    vTaskDelay(pdMS_TO_TICKS(100));
    contadorTime++;

    //Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      analogWrite(ledwifi, 0);
      vTaskDelay(pdMS_TO_TICKS(50));

      Blynk.run();

      if (cambioBotonStop != botonStop) {
        contErrores++;
        Serial.println("Cambio boton: " + String(cambioBotonStop) + " | " + String(botonStop));
        
        if (okLora == 1) {
          if (botonStop == 0) {
            Serial.println("Nuevo proceso de secado ha comenzado.");
            Blynk.logEvent("start","Nuevo proceso de secado ha comenzado.");
          }
          else {
            Serial.println("El proceso de secado se ha detenido.");
            Blynk.logEvent("start","El proceso de secado se ha detenido.");
          }
        }
        else {
          Serial.println("No hay datos del concentrador. Revisar conexión.");
          Blynk.logEvent("start","No hay datos del concentrador. Revisar conexión.");
          botonStop = 1;
        }

        cambioBotonStop = botonStop;
      }

      digitalWrite(ledstop, botonStop);
      Blynk.virtualWrite(V13, botonStop);
      Blynk.virtualWrite(V15, !botonStop);
      Blynk.virtualWrite(V14, okLora);

      // ESPERAR 20 SEGUNDOS
      if (contadorTime == 200) {
        Serial.println("Obteniendo datos WiFi");
        contadorTime = 0;
        
        Serial.println();
        obtenerFechaHora();
        obtenerDatosClima();
        if ((fecha.substring(0, 13) != fechaNvs.substring(0, 13)) || (obtenerPronostico == 1)) { obtenerDatosPronostico(); }
        Serial.println("Pronostico: " + condicionNvs);
        Serial.println("Precipitacion: " + lluviaNvs + "%");
        pronosticoHora = condicionNvs;
        precipitacion = lluviaNvs.toInt();

        if (botonStop == 0) {
          Blynk.virtualWrite(V0, tempCacao);
          Blynk.virtualWrite(V1, humedadCacao);
          Blynk.virtualWrite(V2, tempAmbiente);
          Blynk.virtualWrite(V3, humedadAmbiente);
        }

        Blynk.virtualWrite(V4, localidad);
        Blynk.virtualWrite(V5, fecha);
        Blynk.virtualWrite(V6, tempCurrent);
        Blynk.virtualWrite(V7, humedadCurrent);
        Blynk.virtualWrite(V8, condicionCurrent);
        Blynk.virtualWrite(V9, pronosticoHora);
        Blynk.virtualWrite(V10, precipitacion);
        Blynk.virtualWrite(V11, dianoche);
        Blynk.virtualWrite(V16, contErrores);
        
        if ((estaLloviendoCurrent == "Rain") || (estaLloviendoCurrent == "Drizzle")  || (estaLloviendoCurrent == "Thunderstorm")) { Blynk.logEvent("clima","Actualmente en la zona hay una condición de " + condicionCurrent + ". Se recomienda tener cuidado."); }
        if (precipitacion > 50) { Blynk.logEvent("pronostico","¡El porcentaje de precipitación es ALTO! Hay una probabilidad de " + String(precipitacion) + "% de lluvia. Se recomienda estar atento a cambios climáticos en la siguiente hora."); }
      }

      analogWrite(ledwifi, dutyLeds);
    }
    else { 
      //Serial.println("Sin conexion WiFi.");
      analogWrite(ledwifi, 0);
    }
  }
}

// Tarea LCD en el Núcleo 1
// Prioridad 5
void vTaskLCD(void *pvParameters) {
  while (true) {

    blinkLCD = !blinkLCD;

    switch (subInterfaz) {
      case 1:
        // Informacion ++
        if (interfaz == 3) { interfazInfo(); }
        // WiFi & Zona
        else if (interfaz == 4) { interfazConfig(); }
      break;
      
      case 2:
        // Informacion ++
        if (interfaz == 3) {
          interfazInfo_1();
        }
        // WiFi & Zona
        else if (interfaz == 4) {
          if (intConf_0 == 0) {
            letrasOut();
            interfazConfigLetras();
          }
          else { interfazConfigNum(); }
        }
      break;

      case 3:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Conectando  Wifi");
        Serial.println("Conectando a WiFi");
        // Start WiFi
        WiFi.disconnect(true, true);
        WiFi.begin(SSID, PASS);
        for (int i=0; i<10; i++) {
          vTaskDelay(pdMS_TO_TICKS(250));
          Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() != WL_CONNECTED) {
          WiFi.disconnect(true, true);
          lcd.setCursor(0, 0);
          lcd.print("  Sin Conexion  ");
          lcd.setCursor(0, 1);
          lcd.print("      WiFi      ");
          Serial.println("Wifi no Conectado");
          analogWrite(ledwifi, 0);
          wifiConect = 0;
        }
        else {
          lcd.setCursor(0, 0);
          lcd.print(" Conexion  Wifi ");
          lcd.setCursor(0, 1);
          lcd.print("  Establecida!  ");
          Serial.println("WiFi Conectado");
          analogWrite(ledwifi, dutyLeds);
          wifiConect = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        if (wifiConect == 1) { Blynk.begin(BLYNK_AUTH_TOKEN, SSID.c_str(), PASS.c_str()); }
        intMov = 1;
        interfaz = 0;
        subInterfaz = 0;
        // Configurar pines del KY-040 como entradas
        pinMode(CLK_PIN, INPUT);
        pinMode(DT_PIN, INPUT);

        // Configurar pin STOP
        pinMode(stop, INPUT_PULLUP);
      break;
        
      case 4:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Iniciando LoRa ");

        // Inicia LoRa
        LoRa.setPins(ss, rst, dio0);
        Serial.println("Iniciando LoRa!");
        //433E6 for Asia
        //868E6 for Europe
        //915E6 for North America
        for (int i=0; i<10; i++) {
          vTaskDelay(pdMS_TO_TICKS(250));
          LoRa.begin(915E6);
          Serial.print(".");
        }
        Serial.println();
        if (!LoRa.begin(915E6)) {
          lcd.setCursor(0, 0);
          lcd.print("    LoRa  NO    ");
          lcd.setCursor(0, 1);
          lcd.print("   Conectado!   ");
          Serial.println("Lora no Conectado!");
          analogWrite(ledlora, 0);
          loraConect = 0;
        }
        else {
          lcd.setCursor(0, 0);
          lcd.print("      LoRa      ");
          lcd.setCursor(0, 1);
          lcd.print("   Conectado!   ");
          Serial.println("Lora Conectado!");
          // Change sync word (0xF3) to match the receiver
          // The sync word assures you don't get LoRa messages from other LoRa transceivers
          // ranges from 0-0xFF
          LoRa.setSyncWord(0xF3);
          analogWrite(ledlora, dutyLeds);
          loraConect = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
        intMov = 1;
        interfaz = 0;
        subInterfaz = 0;
      break;

      default:
        if (intMov == 1) {
          dutyLeds = 30;
          if (loraConect == 1) { analogWrite(ledlora, dutyLeds); }
          if (wifiConect == 1) { analogWrite(ledwifi, dutyLeds); }
          intMov = 0;
          precargaInt();
        }
        interfazPrin();
        if (irPronostico == 120) {
          Serial.println("Entrando en Alerta");
          irPronostico++;
          intMov = 1;
          interfaz = -1;
        }
        else if (irPronostico == 240) { 
          lcd.noBacklight();
          dutyLeds = 15;
          if (loraConect == 1) { analogWrite(ledlora, dutyLeds); }
          if (wifiConect == 1) { analogWrite(ledwifi, dutyLeds); }
        }
        else if (irPronostico < 240)  { irPronostico++; }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

//////////////////////////////////////////////////////////////////////////////////