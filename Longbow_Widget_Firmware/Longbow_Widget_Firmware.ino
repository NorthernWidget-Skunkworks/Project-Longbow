/******************************************************************************
Longbow_Widget_Firmwave.cpp
Firmware to be placed on the Longbow Widget microcontroller to interface with Longbow type comunications
Bobby Schulz @ Northern Widget LLC
8/22/2018
https://github.com/NorthernWidget-Skunkworks/Project-Longbow

Version: 0.0.0

This code allows the Widget to send register read and write commands over I2C, which are passed on via RS485.

"I feel that I have at last struck the solution of a great problem—and the day is coming when telegraph wires will be laid on to houses 
just like water or gas—and friends converse with each other without leaving home."
-Alexander Graham Bell

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Wire.h>
#include <EEPROM.h>

volatile uint8_t ADR = 0x13; //FIX! Make address user settable 
// char DataIn[10] = {0}; //Initalize the data input register, used to store data to send over RS485
// char BulkDataFromRover[10] = {0}; //The bulk ascii data recived from the rover device 
// uint8_t ADR = 0;  //Used to store the sent/rec address
// uint8_t RegID = 0;  //Used to store the sent/rec register ID
// uint8_t CRC = 0; //Used to store the sent/rec CRC //FIX add CRC check??
// float DataFromRover = 0; //Data incoming from sensor/device
// float DataToRover = 0; //Data outgoing to sensor/device

// bool DataRead = false; //Flag used to tell device to go get data after I2C transmission is over
// bool DataWrite = false; //Flag used to tell device to send data to rover after I2C transmission is over 

// uint8_t Format = 0;
// uint8_t Reg = 0;

//FIX in/out nomenclature 

#define UART_ON 0x01  //Bitmask for turning UART on
#define DATAWRITE 0x02  //Bitmask for data write of Crtl register 

// #define DATAWRITE 0x01  //Bitmask for data write of Crtl register 
#define SOF '['
#define EOF ']'

volatile uint8_t Ctrl = 0; //Control register to determine function of device 
volatile uint8_t Baud = 4; //Used to set baud rate, 4800 by default, FIX!
volatile char ADR_In[2] = {0};  //Input address
volatile char RegID_In[2] = {0}; //Input register ID
volatile char Data_In[10] = {0}; //Input data
volatile char Format_In = 0;
volatile char CRC_In[3] = {0}; //Input CRC check

volatile char ADR_Out[2]; //Output address (from slave)
volatile char RegID_Out[2] = {0}; //Output register ID (from slave)
volatile char Data_Out[10] = {0}; //Output data (From slave)
volatile char Format_Out = 0; //Output data fromatter (from slave)
volatile char CRC_Out[4] = {0}; //Output crc check (from slave)

volatile uint8_t Reg = 0; //Used to point to which register to read out

bool UartActive = false; //Used to keep track if UART is on

// byte Dummy = '2'; //DEBUG! 
// unsigned long Timeout = 1000; //Wait up to one second for readings

void setup() 
{
	//FIX! Set address, baud rate, based on EEPROM values 
	// Serial.begin(4800); //FIX! Auto baud rate detect?? Do not turn on UART until contacted over I2C? 
	//FIX! Make baud rate user settable 
	// Serial.println("TEST!"); //DEBUG!
	if(EEPROM.read(0x00) != 'L') {  //If EEPROM is not initialized, like on initial firmware load
        EEPROM.write(0x01, ADR);  //Set default address value
        EEPROM.write(0x02, Baud);  //Set default baud value
        EEPROM.write(0x00, 'L');  //Set initialization flag for EEPROM
    }
    else {  //Normally, read address from EEPROM on startup
        ADR = EEPROM.read(0x01);
        Baud = EEPROM.read(0x02);
        // Serial.begin(4800);
        // Serial.println(ADR); //DEBUG!
        // Serial.println(Baud); //DEBUG!
        // Serial.end();
    }

	Wire.begin(ADR); //Begin I2C with given slave address //FIX! respond with general address call to to allow to set address to other values
	TWSAM = 0x01; //Allow for general address call, set secondary address to 0x00, set bit to allow matching on this address
	// TWSAM = 0xC7;
	Wire.onRequest(requestEvent);     // register event
	Wire.onReceive(receiveEvent);

  // DataRead = true;  //DEBUG!
  // Reg = 0; //DEBUG! 
}

void loop() 
{
	// Serial.println(ADR_Out[0]); //DEBUG!
	// Dummy = ADR_Out[0]; //DEBUG!
	//FIX add single read and no reading qualifiers //REPLACE!
	if(Serial.read() == SOF) {  //If anything is available to read
		// while(Serial.read() != SOF);  //FIX add timeout! 
		// Serial.println("BANG!"); //DEBUG!
		while(Serial.available() < 4) {  //FIX add timeout!
			delay(1);
		}

		ADR_Out[0] = Serial.read();
		ADR_Out[1] = Serial.read();
		// Dummy = '1'; //DEBUG!
		delay(20);
		// Serial.print("A0 = "); Serial.println('1');//Serial.println(ADR_Out[0]);  //DEBUG!

		RegID_Out[0] = Serial.read();
		RegID_Out[1] = Serial.read();
		// Serial.println(RegID_Out[0]); //DEBUG!
		int i = 0; //Counter 
		memset(Data_Out, 0, 10); //Clear Data_Out manually
		while(Serial.peek() < 65) {  //Read until formating character  //FIX add timeout, and out of range indicatior
			Data_Out[i++] = Serial.read(); 
		}

		unsigned char TestCRC = GetCRC(Data_Out, i);
		while(Serial.available() < 1) {
			delay(1);
		}
		Format_Out = Serial.read();
		i = 0;  //Clear counter to reuse
		memset(CRC_Out, 0, 4); //Clear CRC_Out manually
		char Temp = 0;
		while(Serial.available() < 2) {
			delay(1);
		}
		delay(20);
		// CRC_Out[0] = Serial.read(); //DEBUG!
		i = 0; //DEBUG!
		// Temp = Serial.read();
		CRC_Out[i++] = Serial.read();
		while(Serial.peek() != EOF) { //FIX???
			// Serial.print(Temp);
			// Temp = Serial.read();
			CRC_Out[i++] = Serial.read();
			// Serial.print(":"); //DEBUG!
		    // CRC_Out[(i++)%3] = Temp;
		}
		CRC_Out[i] = '\0';


		Ctrl = Ctrl & 0xF7; //Clear bit 3, packet validity bit
		if(TestCRC != strtol(CRC_Out, NULL, 10)) Ctrl = Ctrl | 0x08; //Set packet error bit
		// Ctrl = Ctrl | (CheckCRC() << 3); //Set status of packet validity 
		// CRC_Out[0] = i; //DEBUG!
		// Serial.println(i); //DEBUG!
		// Serial.print("A0 = "); Serial.println(ADR_Out[0]);  ///DEBUG!
	}
	// delay(20);
	// Serial.print("A0 = "); Serial.println(ADR_Out[0]);  ///DEBUG!


// 	if(DataRead) {
// 		Serial.print(Reg);
//     Serial.print('\r');
// //    Serial.flush();
//     delay(100);
// 		unsigned long LocalTime = millis();
// 		while(Serial.available() == 0 && (LocalTime - millis()) < Timeout );
// 		int i = 0;
// 		while(Serial.available() > 0 && BulkDataFromRover[i] != '\r') {
// 			BulkDataFromRover[i] = Serial.read();  //Read in bulk data
// //      Serial.println(BulkDataFromRover[i]); //DEBUG!
//       i++;
// 		}
// //		const char RegStr[2] = {BulkDataFromRover[i - 1], BulkDataFromRover[i - 2]};
// //		Reg = (int)strtol(RegStr, NULL, 16); //Convert from hex into int
// //		Format = BulkDataFromRover[i - 3]; //Grab format specifier 
// //    Serial.println(i); //DEBUG!
// //		for(int x = i - 1; x >= i - 3; x--) {  //REPLACE!!
// //			BulkDataFromRover[x] = 0; //Clear last chunk of data from array
// //		} 

// //		String Temp = String(BulkDataFromRover);
//     char Temp[i];  //DEBUG!
//     float Val = 0;
// //    String Temp = "";
//     for(int x = 0; x < i; x++) {
// //      Serial.println((BulkDataFromRover[x]));//DEBUG!
//       Temp[x] = BulkDataFromRover[x];  //Fix using memcopy!!!!!
// //      Temp = Temp + String(BulkDataFromRover[x]); //Manually concatonate because frak null termanators for the moment...
// //      Temp.concat(BulkDataFromRover[x]);
//     }
//     Serial.println("BANG!"); //DEBUG!
// //    Serial.println(BulkDataFromRover[1]);
//     Val = (float)atof(Temp);
//     Serial.println(Val);
//     Serial.println(i); //DEBUG!
// //		DataFromRover = Temp.toFloat();  //Store data as float to be read by I2C
// 		DataRead = false; //clear flag
// 	}
	// Serial.println(Ctrl); //DEBUG!
	if((Ctrl & DATAWRITE) == DATAWRITE) {
		// Serial.print('x'); //DEBUG!
	    // if(Format == 'f') {
	    // 	Serial.print(DataToRover, 6); //FIX decimal hardcode
	    // 	// Serial.print('f'); //Print Format specifier
	    // }

	    // if(Format == 'd') {
	    // 	Serial.print(DataToRover, 0); //Print as rounded decimal
	    // 	// Serial.print('d'); //Print Format specifier
	    // }

	    // Serial.print(Format);  //Print format specifier 
	    // Serial.print(Reg, HEX); //Print register to write data to
	    // if(Reg < 0x10) Serial.print('0');  //Ensure reg value is always 2 chars 
    	// Serial.print('\r'); //print EOF
    	// Serial.print(Ctrl, HEX); //DEBUG!
    	Serial.print(SOF);
    	PrintArray(ADR_In, 2);
    	PrintArray(RegID_In, 2);
    	// Serial.print(ADR_In);
    	// Serial.print(RegID_In);
    	if(Data_In[0] != 0) {  //Test if data is present, if so, write it out
    		PrintArray(Data_In, 10); //Fix length??
    		PrintArray(CRC_In, 3);  //Fix length??
    	}
    	Serial.print(EOF);
    	Serial.print("\n"); //Print formatting character
    	Ctrl = Ctrl & 0xFD; //Clear data write bit  //FIX to be modular with masking!
	}

	if((Ctrl & UART_ON) == 1 && !UartActive) {  //Turn on UART only when directed to over I2C
		Serial.begin(Baud*1200);
		UartActive = true; //Set uart running flag
	}


	if((Ctrl & UART_ON) == 0 && UartActive) {  //If UART ON bit is cleared, and UART is running, turn off UART
		Serial.end(); //Turn off serial
		UartActive = false; //Clear uart running flag
	}
	delay(1);

}

void PrintArray(char *Data, uint8_t Len) {
	for(int i = 0; i < Len; i++) {
		Serial.print(Data[i]);
	}
}

unsigned char GetCRC(const unsigned char * data, const unsigned int size)
{
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned char inbyte = data[i];
        for ( unsigned char j = 0; j < 8; ++j )
        {
            unsigned char mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if ( mix ) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
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
 //  char *Dat = (char *)&DataFromRover; 
	// Wire.write(Dat, 4);  //Write data out to master 
	// Wire.write(Format);
	// Wire.write((int)BulkDataFromRover[3]); //DEBUG!
//  Wire.write(Reg); //DEBUG!

	switch(Reg) {
		case 0:  //Write out control byte
			Wire.write(Ctrl);
			// Wire.write(0x0A); //DEBUG!
			break;

		case 1:  //Write out address value
			// Wire.write(Reg);
			// Wire.write(Dummy); //DEBUG!
			// Wire.write((byte*)ADR_Out, 2);
			Wire.write(ADR_Out[0]);
			Wire.write(ADR_Out[1]);
			break;

		case 2:  //Write out register ID
			// Wire.write(0x0B); //DEBUG!
			Wire.write(RegID_Out[0]);
			Wire.write(RegID_Out[1]);
			break;

		case 3:
			for(int i = 0; i < 10; i++) {
				Wire.write(Data_Out[i]);  //write all data out 
			}
			break;

		case 4: //Write out format character
			Wire.write(Format_Out); 
			break;

		case 5:  //FIX calulcate CRC intenrally??
			for(int i = 0; i < 3; i++) {
				Wire.write(CRC_Out[i]);  //write all data out 
			}
			break;

		case 96:
			Wire.write(Baud); //Return baud rate
			break; 

		case 98:
			Wire.write(ADR); //Return address
			break;
	}
}

void receiveEvent(int DataLen) 
{
    //Write data to appropriate location

    // float DataOut = 0;

 //    if(DataLen > 1){
	//     //Remove while loop?? 
	//     while(Wire.available() < DataLen); //wait for data to be read in 
	//     for(int i = 0; i < DataLen - 2; i++) {
	//     	DataIn[i] = Wire.read(); //Read in data
	//     }
	//     Format = Wire.read();
	//     Reg = Wire.read();
	//     //Check for validity of write??

 //    	DataToRover = *( (float*) (DataIn) );  //Convert data reg into float

 //    	DataWrite = true; //Set flag to write data out to rover
	// }

	// if(DataLen == 1){
	// 	Reg = Wire.read(); //Read in the register ID to be used for subsequent read
	// 	DataRead = true; //Set flag to read data
	// }
	// Ctrl = 1; //DEBUG!
	int i = 0; //Initialize general counter (switch cases get anrgy about redefinition)
	Reg = Wire.read(); //Read first byte of data, this will determine what is being sent 
	if(DataLen > 1) {
		switch(Reg) {
			case 0:  //Read in control byte
				Ctrl = Wire.read(); //DEBUG!
				break;

			case 1:  //Read in address value
				ADR_In[0] = Wire.read();
				ADR_In[1] = Wire.read();
				break;

			case 2:
				RegID_In[0] = Wire.read();
				RegID_In[1] = Wire.read();
				break;

			case 3:
				i = 0; //Counter
				memset(Data_In, 0, 10); //Manually clear Data_In, in case new data is shorter than previous data
				while(Wire.available() > 0) {  //FIX use length of data instead??
					Data_In[i++] = Wire.read();  //read all data into register 
				}
				break;

			case 4:
				Format_In = Wire.read();
				break;

			case 5:  //FIX calulcate CRC intenrally??
				i = 0; 
				memset(CRC_In, 0, 3); //Manually clear CRC_In, in case new crc is shorter than previous
				while(Wire.available() > 0) {  //FIX use length of data instead??
					CRC_In[i++] = Wire.read();
				}
				break;

			case 97:
				// Baud = Wire.read();
				EEPROM.write(0x02, Wire.read()); //Update EEPROM
				break;

			case 99:
				// ADR = Wire.read();
				EEPROM.write(0x01, Wire.read()); //Update EEPROM
				break;


			default:  //Clear buffer by default
				while(Wire.available()) {
					Wire.read();
				}
				break;
		}
	}
}
