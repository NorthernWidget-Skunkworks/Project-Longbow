/******************************************************************************
Longbow_Interface_I2C.ino
Software to be used on an Arduino or data logger to comunicate with a Longbow Widget over I2C for demo purposes
Bobby Schulz @ Northern Widget LLC
8/29/2018
https://github.com/NorthernWidget-Skunkworks/Project-Longbow

This code allows the user to talk to an end device (designed for the TP-Downhole Longbow) using the Longbow Widget
as an intermediary device. The comunication between the Arduino and the Longbow widget is I2C, and in turn, the Widget and
the TP-Downhole comunicate over RS485 using the Longbow protocol 

"I feel that I have at last struck the solution of a great problem—and the day is coming when telegraph wires will be laid on to houses 
just like water or gas—and friends converse with each other without leaving home."
-Alexander Graham Bell

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

uint8_t ADR = 0x22;  //Address of I2C device (Longbow Widget)

uint8_t ADR_Slave = 13; //Address of the slave device on RS485 (TP-Downhole Longbow)

uint8_t Control = 0; //Control byte
uint8_t ADR_Out = 0; //Output address (from slave)
uint8_t RegID_Out = 0; //Output register ID (from slave)
float Data_Out = 0; //Output data (From slave)
char Format_Out = 0; //Output data fromatter (from slave)
uint8_t CRC_Out = 0; //Output crc check (from slave)

void setup() {
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  Wire.begin();  //Begin I2C
  Serial.begin(9600);  //Begin Serial
  Serial.println("Welcome to the Machine...");  //Send ubiquitious alive message 
}

void loop() {
	// digitalWrite(19, HIGH);
	// delay(5000);
	// digitalWrite(19, LOW);
	// delay(5000);
	SetWidgetAddress(ADR); 
	Reset();
	Serial.println("BANG1!"); //DEBUG!
	GetPacket(5, ADR_Slave);  //Update values
	Serial.println("BANG2!"); //DEBUG!
	for(int i = 0; i < 3; i++) {  //Get data packets
		GetPacket(i, ADR_Slave);
    	Serial.println("SOF");
		Serial.println(Control);
		Serial.println(ADR_Out);
		Serial.println(RegID_Out);
		Serial.println(Data_Out);
		Serial.println(Format_Out);
		Serial.println(CRC_Out);
		Serial.print("EOF\n");
		delay(10);
	}
	// Serial.println("BANG!"); //DEBUG!
	// SetAddress(0x45, 13); //Change address

	GetPacket(98, 0x45);
	Serial.print("ADR 0x45 = ");
	Serial.println(Data_Out);
	GetPacket(98, ADR_Slave);
	Serial.print("ADR 13 = ");
	Serial.println(Data_Out);

	Reset();
	Serial.println("RESET!");

	GetPacket(98, 0x45);
	Serial.print("ADR 0x45 = ");
	Serial.println(Data_Out);
	GetPacket(98, 13);
	Serial.print("ADR 13 = ");
	Serial.println(Data_Out);

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

uint8_t SetAddress(uint8_t NewAdr, uint8_t Adr) 
{
	char ADR_Temp[2] = {0};
	char ADR_New[2] = {0};
	char AddressReg[2] = {'9', '9'}; 
	IntToArray(ADR_Temp, Adr); //Convert Adr to char array
	IntToArray(ADR_New, NewAdr); //Convert new address to char array

	SendData(0, 1);  //Turn on UART
	SendData(1, ADR_Temp, 2);  //Load I2C register 
	SendData(2, AddressReg, 2);  //Load downhole register
	SendData(3, ADR_New, 2);  //Load data
	SendData(0, 3); //Set send bit, keep UART on
}

uint8_t SetWidgetAddress(uint8_t NewAdr)
{
	Wire.beginTransmission(0x00);  //Use general call address
	Wire.write(99);  //Write to address set register
	Wire.write(NewAdr);  //Write new address
	Serial.println(Wire.endTransmission()); //DEBUG!
}

uint8_t Reset() 
{
  SendData(0, 1); //Turn on UART
  SendData(0, 0x81); //Keep UART on, command a reset
  delay(100);
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

	delay(100); //Wait for Longbow Widget to populate the registers, FIX! Read command register and wait instead 
	
	GetData(DataTemp, 0, 1);  //Call for control data
	Control = DataTemp[0];  

	GetData(DataTemp, 1, 2);  //Call for Address data
	ADR_Out = CharToInt(DataTemp);  //Convert to int

	GetData(DataTemp, 2, 2);  //Call for Register ID 
	RegID_Out = CharToInt(DataTemp);  //Convert to int
	
	GetData(DataTemp, 3, 10);  //Call for raw data
	Data_Out = strtod(DataTemp, NULL);  //Convert to float 

	Format_Out = GetByte(4); //Read format character

	GetData(CRC_Temp, 5, 3);  //Call for CRC
	CRC_Out = strtol(CRC_Temp, NULL, 10);  //Convert to int
}

char GetByte(uint8_t RegID)  //Read single byte of data
{
	Wire.beginTransmission(ADR);  //Send register value
	Wire.write(RegID);
	Wire.endTransmission();

	Wire.requestFrom(ADR, 1);  //Read from register
	// while(Wire.available() < 1); 
	return Wire.read();
}

void GetData(char *Data, uint8_t RegID, uint8_t NumBytes)  //Call for an array of data
{
	Wire.beginTransmission(ADR);  //Send register value
	Wire.write(RegID);
	Wire.endTransmission();

	Wire.requestFrom(ADR, NumBytes);  //Read multiple bytes from register
	int i = 0;
	for(i = 0; i < NumBytes; i++) {
		Data[i] = Wire.read();
	}
	Data[i + 1]  = '\0';  //Add null terminator 
}

uint8_t CharToInt(char *Data) //Convert 2 char bytes (0 ~ 99) to integer
{
    return (Data[0] - 48)*10 + (Data[1] - 48);
}

void IntToArray(char *Reg, uint8_t Data)  //Convert Data value (0 ~ 99) to Char array
{
	Reg[1] = (Data % 10) + 48;
	Reg[0] = ((Data - (Data % 10))/10) + 48;
}

void SendData(uint8_t Reg, char *Data, uint8_t Len) //Writes out array to given register on reciver
{
  Wire.beginTransmission(ADR);
  Wire.write(Reg);
  for(int i = 0; i < Len; i++) {  //Write multiple bytes 
    Wire.write(Data[i]);  
  }
  Serial.println(Wire.endTransmission()); //DEBUG!
}

void SendData(uint8_t Reg, uint8_t Data) //Writes out single byte to register on reciver 
{
  Wire.beginTransmission(ADR);
  Wire.write(Reg);
  Wire.write(Data);
  Wire.endTransmission();
}
