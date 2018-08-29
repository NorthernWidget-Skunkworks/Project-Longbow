#include <Wire.h>
#include <stdlib.h>

uint8_t ADR = 0x21;
char ADR_Send[2] = {'1', '3'};
char RegID_Send[2] = {'0', '0'};

uint8_t Control = 0; //Control byte
uint8_t ADR_Out = 0; //Output address (from slave)
uint8_t RegID_Out = 0; //Output register ID (from slave)
float Data_Out = 0; //Output data (From slave)
char Format_Out = 0; //Output data fromatter (from slave)
uint8_t CRC_Out = 0; //Output crc check (from slave)

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Welcome to the Machine...");
}

void loop() {
//   // put your main code here, to run repeatedly:
//   SendData(0, 1); //Turn on UART
//   SendData(1, ADR_Send, 2);
//   SendData(2, RegID_Send, 2);
//   SendData(0, 1); //Set send bit
//   SendData(0, 3); //Set send bit, keep UART on
//   delay(1000);

//   Wire.beginTransmission(ADR);
//   Wire.write(3);
// //  Wire.write(1);
//   Wire.endTransmission();

//   Wire.requestFrom(ADR, 2);
// //  while(Wire.available() < 2) {
// //    delay(1);
// //  }
//   Serial.println(ADR, HEX);
//   Serial.println(Wire.read());
//   Serial.println(Wire.read());
//   delay(1000);

//   Wire.beginTransmission(0x13);
//   Wire.write(1);
// //  Wire.write(1);
//   Wire.endTransmission();
  
//   Wire.requestFrom(0x13, 2);
// //  while(Wire.available() < 2) {
// //    delay(1);
// //  }
//   Serial.println(0x13, HEX);
//   Serial.println(Wire.read());
//   Serial.println(Wire.read());

//   char Temp = Serial.read(); 
//   if(Temp == 'S') {
//     SendData(0, 0); //Turn off uart
//     while(Serial.read() != 's') {
//       delay(1);
//     }
//   }

//   if(Temp == 'A') {
//     Wire.beginTransmission(0x00);
//     Wire.write(99);
//     Wire.write(0x13);
//     Serial.println(Wire.endTransmission());
//   }

//   if(Temp == 'a') {
//     Wire.beginTransmission(0x00);
//     Wire.write(99);
//     Wire.write(0x21);
//     Serial.println(Wire.endTransmission());
//   }

//   if(Temp == 'B') {
//     Wire.beginTransmission(0x00);
//     Wire.write(97);
//     Wire.write(8);
//     Serial.println(Wire.endTransmission());
//   }

//   if(Temp == 'b') {
//     Wire.beginTransmission(0x00);
//     Wire.write(97);
//     Wire.write(4);
//     Serial.println(Wire.endTransmission());
//   }

for(int i = 0; i < 3; i++) {
	GetPacket(i, 13);
	Serial.println(Control);
	Serial.println(ADR_Out);
	Serial.println(RegID_Out);
	Serial.println(Data_Out);
	Serial.println(Format_Out);
	Serial.println(CRC_Out);
	Serial.print("\n");
	delay(10);
}

	GetPacket(5, 13);  //Update values
	Serial.println(Control);
	Serial.println(ADR_Out);
	Serial.println(RegID_Out);
	Serial.println(Data_Out);
	Serial.println(Format_Out);
	Serial.println(CRC_Out);
	Serial.print("\n\n\n\n");
	delay(2000);
  
}

uint8_t GetAddress() 
{

}

uint8_t GetRegID()
{

}

float GetData()
{

} 

char GetFormat()
{

}

uint8_t GetCRC()
{

}

uint8_t GetPacket(uint8_t Reg, uint8_t Adr)
{
	char ADR_Temp[2] = {0};
	char Reg_Temp[2] = {0};
	IntToArray(ADR_Temp, Adr); //Convert Adr to char array
	IntToArray(Reg_Temp, Reg); //Convert Reg to char array

	SendData(0, 1); //Turn on UART
	SendData(1, ADR_Temp, 2);  //Send address of device
	SendData(2, Reg_Temp, 2);  //Send desired register 
	SendData(0, 3); //Set send bit, keep UART on

	char DataTemp[10] = {0};
	char CRC_Temp[4] = {0};
	// char FormatTemp = 0;
	delay(100); //Wait for Longbow Widget to populate the registers, FIX! Read command register and wait instead 
	
	GetData(DataTemp, 0, 1);
	Control = DataTemp[0];

	GetData(DataTemp, 1, 2);
	ADR_Out = CharToInt(DataTemp);

	GetData(DataTemp, 2, 2);
	RegID_Out = CharToInt(DataTemp);
	
	GetData(DataTemp, 3, 10);
	// sscanf(DataTemp, "%f", &Data_Out);
	// Data_Out = (float)atof(DataTemp);
	Data_Out = strtod(DataTemp, NULL);
	// Serial.println(DataTemp[0]); //DEBUG!

	Format_Out = GetByte(4); //Read format character

	GetData(CRC_Temp, 5, 3);
	// sscanf(DataTemp, "%d", &CRC_Out);

	// for(int i = 0; i < 4; i++) {
	// 	Serial.print(CRC_Temp[i]);
	// 	Serial.print(" ");
	// }
	// Serial.print("\n");

	// Serial.print(DataTemp[0]);  //DEBUG!
	// Serial.print(DataTemp[1]); //DEBUG!
	// Serial.print(DataTemp[2]); //DEBUG!
	CRC_Out = strtol(CRC_Temp, NULL, 10);
	// sscanf(CRC_Temp, "%d", &CRC_Out); 
	// while()
	// CRC_Out = (CRC_Temp[0] - 48)*10 + (CRC_Temp[1] - 48);
	// Serial.println(DataTemp[2]); //DEBUG!
}

char GetByte(uint8_t RegID)
{
	Wire.beginTransmission(ADR);
	Wire.write(RegID);
	Wire.endTransmission();

	Wire.requestFrom(ADR, 1);
	while(Wire.available() < 1); 
	return Wire.read();
}

void GetData(char *Data, uint8_t RegID, uint8_t NumBytes)
{
	Wire.beginTransmission(ADR);
	Wire.write(RegID);
	Wire.endTransmission();

	Wire.requestFrom(ADR, NumBytes);
	int i = 0;
	for(i = 0; i < NumBytes; i++) {
		Data[i] = Wire.read();
	}
	Data[i + 1]  = '\0';
}

uint8_t CharToInt(char *Data) 
{
    return (Data[0] - 48)*10 + (Data[1] - 48);
}

void IntToArray(char *Reg, uint8_t Data)  //Convert Data value (0 ~ 99) to Char array
{
	Reg[1] = (Data % 10) + 48;
	Reg[0] = ((Data - (Data % 10))/10) + 48;
}

void SendData(uint8_t Reg, char *Data, uint8_t Len) 
{
  Wire.beginTransmission(ADR);
  Wire.write(Reg);
  for(int i = 0; i < Len; i++) {
    Wire.write(Data[i]);  
  }
//  Serial.println(Wire.endTransmission());
  Wire.endTransmission();
}

void SendData(uint8_t Reg, uint8_t Data) 
{
  Wire.beginTransmission(ADR);
  Wire.write(Reg);
  Wire.write(Data);
//  Serial.println(Wire.endTransmission());
  Wire.endTransmission();
}
