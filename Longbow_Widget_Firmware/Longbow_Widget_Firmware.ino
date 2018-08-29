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

volatile uint8_t ADR = 0x22; //FIX! Make address user settable 

//FIX in/out nomenclature 

#define UART_ON 0x01  //Bitmask for turning UART on
#define DATAWRITE 0x02  //Bitmask for data write of Crtl register 

#define SOF '['  //Define Start Of Frame character
#define EOF ']'  //Define End Of Frame character 

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

void setup() 
{
	if(EEPROM.read(0x00) != 'L') {  //If EEPROM is not initialized, like on initial firmware load
        EEPROM.write(0x01, ADR);  //Set default address value
        EEPROM.write(0x02, Baud);  //Set default baud value
        EEPROM.write(0x00, 'L');  //Set initialization flag for EEPROM
    }
    else {  //Normally, read address from EEPROM on startup
        ADR = EEPROM.read(0x01);
        Baud = EEPROM.read(0x02);
    }

	Wire.begin(ADR); //Begin I2C with given slave address //FIX! respond with general address call to to allow to set address to other values
	TWSAM = 0x01; //Allow for general address call, set secondary address to 0x00, set bit to allow matching on this address

	Wire.onRequest(requestEvent);     // register event
	Wire.onReceive(receiveEvent);
}

void loop() 
{
	//FIX add single read and no reading qualifiers //REPLACE!
	if(Serial.read() == SOF) {  //If anything is available to read

		while(Serial.available() < 4) {  //wait for mandatory data (ADR and Register ID) //FIX add timeout!
			delay(1);
		}

		ADR_Out[0] = Serial.read();  //Read in address
		ADR_Out[1] = Serial.read();

		delay(20);  //Garbage required delay, will be tracked down and terminated with extreme prejudice when time allows...
		// Serial.print("A0 = "); Serial.println('1');//Serial.println(ADR_Out[0]);  //DEBUG!

		RegID_Out[0] = Serial.read();  //Read in register ID
		RegID_Out[1] = Serial.read();

		int i = 0; //Counter 
		memset(Data_Out, 0, 10); //Clear Data_Out manually
		while(Serial.peek() < 65) {  //Read until formating character  //FIX add timeout, and out of range indicatior
			Data_Out[i++] = Serial.read(); 
		}

		unsigned char TestCRC = GetCRC(Data_Out, i);  //Calculate CRC given data packet 

		while(Serial.available() < 1) {  //Wait for formatting byte
			delay(1);
		}

		Format_Out = Serial.read();  //Read formatting character in

		i = 0;  //Clear counter to reuse
		memset(CRC_Out, 0, 4); //Clear CRC_Out manually

		while(Serial.available() < 2) {  //Wait for at least single digit CRC and EOF character 
			delay(1);
		}
		delay(20);
		
		CRC_Out[i++] = Serial.read();
		while(Serial.peek() != EOF) { //Read in CRC of variable (1~3 characters) length, looking for EOF character
			CRC_Out[i++] = Serial.read();
		}
		CRC_Out[i] = '\0';  //manually add terminator to string to that it can be parsed with strol


		Ctrl = Ctrl & 0xF7; //Clear bit 3, packet validity bit
		if(TestCRC != strtol(CRC_Out, NULL, 10)) Ctrl = Ctrl | 0x08; //Set packet error bit by checking calculated value against the converted value from message
	}

	if((Ctrl & DATAWRITE) == DATAWRITE) {  //If control bit is set, print registers to output
    	Serial.print(SOF);  //Print SOF to indicate message to come
    	PrintArray(ADR_In, 2);  //Print address
    	PrintArray(RegID_In, 2);  //print Register ID

    	if(Data_In[0] != 0) {  //Test if data is present, if so, write it out
    		PrintArray(Data_In, 10); //Print data array
    		PrintArray(CRC_In, 3);   //Print CRC array
    	}
    	Serial.print(EOF);  //Print end of message indicator 
    	Serial.print("\n"); //Print formatting character
    	Ctrl = Ctrl & 0xFD; //Clear data write bit  //FIX to be modular with masking!
	}

	if((Ctrl & UART_ON) == 1 && !UartActive) {  //Turn on UART only when directed to over I2C
		Serial.begin(Baud*1200);  //Begin Serial at the user defined speed
		UartActive = true; //Set uart running flag
	}

	if((Ctrl & UART_ON) == 0 && UartActive) {  //If UART ON bit is cleared, and UART is running, turn off UART
		Serial.end(); //Turn off serial
		UartActive = false; //Clear uart running flag
	}
	delay(1);

}

void PrintArray(char *Data, uint8_t Len) {  //Helper function for printing subset of array
	for(int i = 0; i < Len; i++) {
		Serial.print(Data[i]);
	}
}

unsigned char GetCRC(const unsigned char * data, const unsigned int size)  //CRC calculation
//Using CRC-8 From Dallas/Maxim group, with a polynomial of 0x8C, and an even parity (same as 1 wire bus)
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

void requestEvent()  //Respond to request for data
{	
	switch(Reg) {
		case 0:  //Write out control byte
			Wire.write(Ctrl);
			break;

		case 1:  //Write out address value
			Wire.write(ADR_Out[0]);
			Wire.write(ADR_Out[1]);
			break;

		case 2:  //Write out register ID
			Wire.write(RegID_Out[0]);
			Wire.write(RegID_Out[1]);
			break;

		case 3:  //Write out data
			for(int i = 0; i < 10; i++) {
				Wire.write(Data_Out[i]);  //write all data out 
			}
			break;

		case 4: //Write out format character
			Wire.write(Format_Out); 
			break;

		case 5:  //Write out CRC
			for(int i = 0; i < 3; i++) {
				Wire.write(CRC_Out[i]);  //write all data out 
			}
			break;

		case 96:  //Write out baud rate
			Wire.write(Baud); //Return baud rate
			break; 

		case 98:  //Write out address
			Wire.write(ADR); //Return address
			break;
	}
}

void receiveEvent(int DataLen) //respond to recipt of data
{
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

			case 2:  //Read in register ID
				RegID_In[0] = Wire.read();
				RegID_In[1] = Wire.read();
				break;

			case 3:  //Read in data 
				i = 0; //Counter
				memset(Data_In, 0, 10); //Manually clear Data_In, in case new data is shorter than previous data
				while(Wire.available() > 0) {  //FIX use length of data instead??
					Data_In[i++] = Wire.read();  //read all data into register 
				}
				break;

			case 4:  //Read in formatting character 
				Format_In = Wire.read();
				break;

			case 5:  //Read in CRC code
				i = 0; 
				memset(CRC_In, 0, 3); //Manually clear CRC_In, in case new crc is shorter than previous
				while(Wire.available() > 0) {  //FIX use length of data instead??
					CRC_In[i++] = Wire.read();
				}
				break;

			case 97:  //Read in baud rate setting
				EEPROM.write(0x02, Wire.read()); //Update EEPROM
				break;

			case 99:  //Read in address setting 
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
