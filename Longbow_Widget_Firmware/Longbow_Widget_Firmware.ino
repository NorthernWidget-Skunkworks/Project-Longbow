/******************************************************************************
Longbow_Widget_Firmwave.cpp
Firmware to be placed on the Longbow Widget microcontroller to interface with Longbow type comunications
Bobby Schulz @ Northern Widget LLC
8/22/2018
https://github.com/NorthernWidget-Skunkworks/Project-Longbow

Version: 0.0.0

This code allows the Widget to send register read and write commands over I2C, which are passed on via RS485.

"The laws of nature are constructed in such a way as to make the universe as interesting as possible."
-Freeman Dyson

///////////////////ADD NEW QUOTE!

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Wire.h>

uint8_t ADR = 0x13; //FIX! Make address user settable 
uint8_t DataIn[8] = {0}; //Initalize the data input register, used to store data to send over RS485
char BulkDataFromRover[6] = {0}; //The bulk ascii data recived from the rover device 
float DataFromRover = 0; //Data incoming from sensor/device
float DataToRover = 0; //Data outgoing to sensor/device

bool DataRead = false; //Flag used to tell device to go get data after I2C transmission is over
bool DataWrite = false; //Flag used to tell device to send data to rover after I2C transmission is over 

uint8_t Format = 0;
uint8_t Reg = 0;



unsigned long Timeout = 1000; //Wait up to one second for readings

void setup() 
{
	Serial.begin(4800); //FIX! Auto baud rate detect?? Do not turn on UART until contacted over I2C? 
	Wire.begin(ADR); //Begin I2C with given slave address
	Wire.onRequest(requestEvent);     // register event
	Wire.onReceive(receiveEvent);
  DataRead = true;  //DEBUG!
  Reg = 0; //DEBUG! 
}

void loop() 
{
	if(DataRead) {
		Serial.print(Reg);
    Serial.print('\r');
//    Serial.flush();
    delay(100);
		unsigned long LocalTime = millis();
		while(Serial.available() == 0 && (LocalTime - millis()) < Timeout );
		int i = 0;
		while(Serial.available() > 0 && BulkDataFromRover[i] != '\r') {
			BulkDataFromRover[i] = Serial.read();  //Read in bulk data
//      Serial.println(BulkDataFromRover[i]); //DEBUG!
      i++;
		}
//		const char RegStr[2] = {BulkDataFromRover[i - 1], BulkDataFromRover[i - 2]};
//		Reg = (int)strtol(RegStr, NULL, 16); //Convert from hex into int
//		Format = BulkDataFromRover[i - 3]; //Grab format specifier 
//    Serial.println(i); //DEBUG!
//		for(int x = i - 1; x >= i - 3; x--) {  //REPLACE!!
//			BulkDataFromRover[x] = 0; //Clear last chunk of data from array
//		} 

//		String Temp = String(BulkDataFromRover);
    char Temp[i];  //DEBUG!
    float Val = 0;
//    String Temp = "";
    for(int x = 0; x < i; x++) {
//      Serial.println((BulkDataFromRover[x]));//DEBUG!
      Temp[x] = BulkDataFromRover[x];  //Fix using memcopy!!!!!
//      Temp = Temp + String(BulkDataFromRover[x]); //Manually concatonate because frak null termanators for the moment...
//      Temp.concat(BulkDataFromRover[x]);
    }
    Serial.println("BANG!"); //DEBUG!
//    Serial.println(BulkDataFromRover[1]);
    Val = (float)atof(Temp);
    Serial.println(Val);
    Serial.println(i); //DEBUG!
//		DataFromRover = Temp.toFloat();  //Store data as float to be read by I2C
		DataRead = false; //clear flag
	}

	if(DataWrite) {
	    if(Format == 'f') {
	    	Serial.print(DataToRover, 6); //FIX decimal hardcode
	    	// Serial.print('f'); //Print Format specifier
	    }

	    if(Format == 'd') {
	    	Serial.print(DataToRover, 0); //Print as rounded decimal
	    	// Serial.print('d'); //Print Format specifier
	    }

	    Serial.print(Format);  //Print format specifier 
	    Serial.print(Reg, HEX); //Print register to write data to
	    if(Reg < 0x10) Serial.print('0');  //Ensure reg value is always 2 chars 
    	Serial.print('\r'); //print EOF

    	DataWrite = false; //Clear flag
	}

}

void requestEvent()
{	
	// //Allow for repeated start condition 
	// if(RepeatedStart) {
	// 	for(int i = 0; i < 2; i++) {
	// 		Wire.write(Reg[RegID + i]);
	// 	}
	// }
	// else {
	// 	Wire.write(Reg[RegID]);
	// }
  char *Dat = (char *)&DataFromRover; 
	Wire.write(Dat, 4);  //Write data out to master 
	Wire.write(Format);
	Wire.write((int)BulkDataFromRover[3]); //DEBUG!
//  Wire.write(Reg); //DEBUG!
}

void receiveEvent(int DataLen) 
{
    //Write data to appropriate location

    // float DataOut = 0;

    if(DataLen > 1){
	    //Remove while loop?? 
	    while(Wire.available() < DataLen); //wait for data to be read in 
	    for(int i = 0; i < DataLen - 2; i++) {
	    	DataIn[i] = Wire.read(); //Read in data
	    }
	    Format = Wire.read();
	    Reg = Wire.read();
	    //Check for validity of write??

    	DataToRover = *( (float*) (DataIn) );  //Convert data reg into float

    	DataWrite = true; //Set flag to write data out to rover
	}

	if(DataLen == 1){
		Reg = Wire.read(); //Read in the register ID to be used for subsequent read
		DataRead = true; //Set flag to read data
	}
}
