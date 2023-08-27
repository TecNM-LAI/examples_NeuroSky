#include "BluetoothSerial.h"


// Condición de compilador si no está habilitado el BT Classic en la ESP32
#if !defined(CONFIG_BT_SPP_ENABLED)
  #error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif


// Definiciones de configuración de debug
#define BAUDRATE      115200
#define DEBUGOUTPUT   0
#define powercontrol  10
// Definiciones de salidas de actuadores
#define R             5
#define G             18
#define B             19
#define LED           2
#define M1A           27
#define M1B           26
#define M2A           25
#define M2B           33



// Objeto de puerto BT Classic serial
BluetoothSerial SerialBT;

// Variables de propiedades BT de la banda
String MACadd = "98:07:2D:7F:5E:93";  // MAC específica por banda
uint8_t address[6]  = {0x98, 0x07, 0x2D, 0x7F, 0x5E, 0x93};
String name = "MindWave Mobile";      // Nombre no intercambiable
const char *pin = "0000";             // Contraseña default
bool connected;

// Variables de checksum
byte generatedChecksum = 0;
byte checksum = 0; 
int payloadLength = 0;
byte payloadData[64] = {0};
// Variables de señales
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;
char arr_ATT[3] = {0};
char arr_MED[3] = {0};
char avg_ATT = 0;
char avg_MED = 0;

// Variables de sistema
long lastReceivedPacket = 0;
boolean bigPacket = false;



////////////////////////////////
// Leer datos de UART serial  //
////////////////////////////////
byte ReadOneByte() {
  int ByteRead;

  while(!SerialBT.available());
  ByteRead = SerialBT.read();

#if DEBUGOUTPUT  
  Serial.print((char)ByteRead);   // Echo del mismo byte por USB serial (propósito de debug)
#endif

  return ByteRead;
}


// Funciones de control de puente H
 void M1_B(void){
  digitalWrite(M1A,1);
  digitalWrite(M1B,0);
}
void M1_F(void){
  digitalWrite(M1A,0);
  digitalWrite(M1B,1);
}
void M1_H(void){
  digitalWrite(M1A,0);
  digitalWrite(M1B,0);
}
void M2_B(void){
  digitalWrite(M2A,1);
  digitalWrite(M2B,0);
}
void M2_F(void){
  digitalWrite(M2A,0);
  digitalWrite(M2B,1);
}
void M2_H(void){
  digitalWrite(M2A,0);
  digitalWrite(M2B,0);
}

// Función de movimiento por umbral
void movement(void){
  if(avg_MED >= 60){
    M1_F();
    M2_F();
  }
  else if(avg_ATT >= 70){
    M1_B();
    M2_B();
  }
  else{
    M1_H();
    M2_H();
  }
}



void setup() {
  // Inicialización de pines de control de puente H
  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(M2B, OUTPUT);
  
  // Inicialización de pines de LED RGB
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  
  // Inicilización de pin del LED embebido
  pinMode(LED, OUTPUT);

  // Chequeo de movimiento frontal
  M1_F();
  M2_F();
  delay(1000);

  // Chequeo de movimiento en reversa
  M1_B();
  M2_B();
  delay(1000);

  // Paro de motores
  M1_H();
  M2_H();
  delay(1000);

  // Inicialización de puerto serial para debug
  Serial.begin(BAUDRATE);
  SerialBT.begin("ESP32test", true); 
  SerialBT.setPin(pin);
  Serial.println("El dispositivo esta en modo maestro. Asegurese que el dispositivo BT remoto esta encendido!");

  /*
   * La conexión(dirección) es rápida (10 segs aprox.), la conexión(nombre) es lenta (30 segs aprox.) ya que necesita
   * resolver el nombre antes de direccionar, pero permite la conexión a varios dispositivos con nombres distintos.
   * Configurar el CoreDebugLevel to Info para visualizar la dirección BT y nombres de dispositivos.
   * 
   */
  //connected = SerialBT.connect(name);
  connected = SerialBT.connect(address);

  // Procedimiento de conexión a la banda
  if(connected) {
    Serial.println("Conexion establecida!");
  } else {
    while(!SerialBT.connected(10000)) {
      Serial.println("Fallo la conexion. Asegurese que el dispositivo remoto está encendido y en rango, luego reinicie."); 
    }
  }
  // disconnect() toma alrededor de 10 segs
  if (SerialBT.disconnect()) {
    Serial.println("Disconnected Succesfully!");
  }
  // La funcion reconecta por nombre(empleará la dirección, si está solucionada) o la dirección empleada con connect(nombre/dirección).
  SerialBT.connect();

}

void loop() {
  
  // Búsqueda de bytes de sincronización
  if(ReadOneByte() == 170) {
    if(ReadOneByte() == 170) {

      payloadLength = ReadOneByte();
      if(payloadLength > 169)                       // La longitud del payload no puede ser mayor a 169
          return;

      generatedChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) {  
        payloadData[i] = ReadOneByte();             // Almacenar el payload en memoria
        generatedChecksum += payloadData[i];
      }   

      checksum = ReadOneByte();                     // Leer el byte de checksum de la transmisión     
      generatedChecksum = 255 - generatedChecksum;  // Tomar el complemento 1 del checksum generado

        if(checksum == generatedChecksum) {    

        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for(int i = 0; i < payloadLength; i++) {    // Descifrar el payload
          switch (payloadData[i]) {
          case 2:
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:
            i++;
            attention = payloadData[i];                        
            break;
          case 5:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;      
            break;
          default:
            break;
          } // switch
        } // for loop

#if !DEBUGOUTPUT
        // Condicional de obtención de dato completo y con calidad aceptable
        if(bigPacket) {

        // Filtro FIR promediador de 3 taps para meditación y concentración
        avg_ATT = (arr_ATT[0] + arr_ATT[1] + arr_ATT[2]) / 3;
        avg_MED = (arr_MED[0] + arr_MED[1] + arr_MED[2]) / 3;
        movement();

        // Recorrido del ventaneo del filtro FIR para ambas variables
        arr_ATT[0] = arr_ATT[1];
        arr_ATT[1] = arr_ATT[2];
        arr_ATT[2] = attention;
        
        arr_MED[0] = arr_MED[1];
        arr_MED[1] = arr_MED[2];
        arr_MED[2] = meditation;

        // Formato de salida para el debug
        Serial.print("Q: ");
        Serial.print(poorQuality, DEC);
        Serial.print("\t");
        Serial.print("avg_ATT: ");
        Serial.print(avg_ATT, DEC);
        Serial.print("\t");
        Serial.print("avg_MED: ");
        Serial.print(avg_MED, DEC);
        Serial.print("\t");
        Serial.print("\n");

        // Impresión de valores recibidos
          if(poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
          digitalWrite(LED, LOW);
          Serial.print("Q: ");
          Serial.print(poorQuality, DEC);
          Serial.print("\t");
          Serial.print("ATT: ");
          Serial.print(attention, DEC);
          Serial.print("\t");
          Serial.print("MED: ");
          Serial.print(meditation, DEC);
          Serial.print("\t");
          Serial.print("t: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n");                     
        }
#endif        
        bigPacket = false;        
      }
      else {
        // Error en el checksum
      }  // cierre de if else para el checksum
    } // cierre de if en lectura de byte 0xAA
  } // cierre de if en lectura de byte 0xAA

}