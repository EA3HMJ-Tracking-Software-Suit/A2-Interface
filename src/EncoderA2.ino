//
// Interface encoder A2 to Modbus (c) ea3hmj 10/2023
//
//
#include <Arduino.h>
#include <Preferences.h>
#include <HardwareSerial.h>
#include "src\SoftwareSerial.h"
// Modbus server include
//#define LOG_LEVEL LOG_LEVEL_VERBOSE
//#include "src\Logging.h"
#include "src\ModbusServerRTU.h"
#define LED_BUILTIN 15
typedef union
{
  int32_t UINT32;
  byte Byte[4];
} myUint32;

enum {     
	// The first register starts at address 0
	ENCODERL,			// R0
	ENCODERH,			// R1
	RES,				// R2
	DIR,        // R3
	ORIGEN,	    // R4
  TOTAL_REGS_SIZE   // total number of registers for function 3 and 16 share the same register array

};
uint16_t   holdingRegs[TOTAL_REGS_SIZE*2]; // function 3 and 16 register array
myUint32 		pos;
Preferences preferences;
// Create a ModbusRTU server instance listening with 2000ms timeout
ModbusServerRTU MBserver(2000);
SoftwareSerial 		A2az(4,5);							// Puerto serie para encoder rx,tx pin 
SoftwareSerial 		A2el(2,3);							// Puerto serie para encoder rx,tx pin 
bool comm=true;                               // en el loop poder trabajar
byte buf[10];


// FC03: worker do serve Modbus function code 0x03 (READ_HOLD_REGISTER)
ModbusMessage FC03(ModbusMessage request) {
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  ModbusMessage response;     // response message to be sent back
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  // get request values
  request.get(2, address);
  request.get(4, words);
  // Address and words valid? We assume 10 registers here for demo
  if ((address + words) <= TOTAL_REGS_SIZE*2) {
    // Looks okay. Set up message with serverID, FC and length of data
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i) {
      response.add(holdingRegs[i]);
    }
  } else {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  return response;
}

// Worker function function code 0x06
ModbusMessage FC06(ModbusMessage request) {
  uint16_t addr = 0;        // Start address to read
  uint16_t value = 0;       // New value for register
  ModbusMessage response;
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  // Get addr and value from data array. Values are MSB-first, getValue() will convert to binary
  request.get(2, addr);
  request.get(4, value);
  Serial.println(addr);

  // address valid?
  if ( addr >= TOTAL_REGS_SIZE) {
    // No. Return error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
       return response;
  }
  // Fill in new value.
  if (addr==ORIGEN){ // le decimos que esta posicion es 0

      comm=false;
    vTaskDelay( pdMS_TO_TICKS(1000) );
    buf[0]=0xf0;
    buf[1]=0x01;
    A2az.write(buf,2);
    A2az.flush();  
    int cnt=0;
    while (A2az.available()<1 && cnt<50){
        vTaskDelay( pdMS_TO_TICKS(1) );
        cnt++;
    }		
    if (A2az.available()>0){
      A2az.readBytes(buf,1);
    }
    comm=true;
  }
  holdingRegs[addr] = value;
  preferences.begin("a2", false);
  char tmp[5];
  sprintf(tmp,"R%d",addr);
  preferences.putUInt(tmp, value);
  // Close the Preferences
  preferences.end();
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  return request;
}
// Setup() - initialization happens here
void setup() {
  preferences.begin("a2", false);
  char tmp[5];
  // inicializamos los offset
  for (int i=0;i<TOTAL_REGS_SIZE;i++){
	  sprintf(tmp,"R%d",TOTAL_REGS_SIZE+i);
	  holdingRegs[TOTAL_REGS_SIZE+i]=preferences.getUInt(tmp, 0);
  }
  // Close the Preferences
  preferences.end();
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
 // Init Serial monitor
  Serial.begin(115200);
  //while (!Serial) {}
  Serial.println("__ OK __");

	A2az.begin(9600);//,SERIAL_8N1,6,7);
	A2el.begin(9600);//,SERIAL_8N1,4,5);

// Init Serial2 connected to the RTU Modbus
// (Fill in your data here!)
  RTUutils::prepareHardwareSerial(Serial1);
  Serial1.begin(115200, SERIAL_8N1, GPIO_NUM_16, GPIO_NUM_17);

  // Define and start RTU server
    MBserver.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03// for serverID=1
   // MBserver.registerWorker(1, READ_INPUT_REGISTER, &FC04);     // FC=04 for serverID=1
    MBserver.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);     // FC=06 for serverID=1
 
// Start ModbusRTU background task
  MBserver.begin(Serial1);

}

void loop(){
  if (comm){
    buf[0]=0x10;
    buf[3]=0x0;
    buf[4]=0x0;
    A2az.write(buf,1);
    A2az.flush();  
    int cnt=0;
    while (A2az.available()<2 && cnt<50){
        vTaskDelay( pdMS_TO_TICKS(1) );
        cnt++;
    }		
    if (A2az.available()>1){
      A2az.readBytes(buf,2);
      buf[2]=buf[1];
      buf[3]=buf[0];
      pos.UINT32= *(int32_t*)&buf[2];
      holdingRegs[ENCODERL] = ((pos.Byte[3]<<8)|pos.Byte[2]);
      holdingRegs[ENCODERH]= ((pos.Byte[1]<<8)|pos.Byte[0]);
    }
  }
 vTaskDelay( pdMS_TO_TICKS(1) );
}
