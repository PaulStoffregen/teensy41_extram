#include <w25n01g_t4.h>

#define DHprint( a ) { Serial.print( #a); Serial.print(": ");Serial.println ( (uint32_t)a,HEX ); }
#define DTprint( a ) { Serial.println( #a); }
#define Dprint(a) Serial.print( a );

w25n01g_t4 myNAND;

uint8_t buffer[2048];
uint32_t tempAddress;


//define a struct of various data types
struct MYDATA_t {
	bool data_0;
	float data_1; 
	long data_2; 
	int data_3;
	byte data_4;
};

//define a struct joining MYDATA_t to an array of bytes to be stored
union MYDATA4I2C_t {
 MYDATA_t datastruct;
 uint8_t I2CPacket[sizeof(MYDATA_t)];
};

MYDATA4I2C_t mydata; //data to be written in memory
MYDATA4I2C_t readdata; //data read from memory

//random address to write from
uint16_t writeaddress = 2048*3;


void setup() {
  	Serial.begin(115200);
  	delay(1000);
  	Serial.println("Begin Init");

  	myNAND.begin();
  	//myNAND.eraseSector(writeaddress);

	uint16_t arraySize = sizeof(MYDATA_t);

	myNAND.readBytes(writeaddress, readdata.I2CPacket, arraySize);
    Serial.println("On Start Read Done - array loaded with read data");
	Serial.println("...... ...... ......");
	
	Serial.print("Data_0: ");
	if (readdata.datastruct.data_0) Serial.println("true");
	if (!readdata.datastruct.data_0) Serial.println("false");
	Serial.print("Data_1: ");
	Serial.println(readdata.datastruct.data_1, DEC);
	Serial.print("Data_2: ");
	Serial.println(readdata.datastruct.data_2, DEC);
	Serial.print("Data_3: ");
	Serial.println(readdata.datastruct.data_3, DEC);	
	Serial.print("Data_4: 0x");
	Serial.println(readdata.datastruct.data_4, HEX);
	Serial.println("...... ...... ......");
	Serial.println("Read Write test done - check data if successfull");
	Serial.println("...... ...... ......");	


	Serial.println("...... Read Array on Start ......");
	  Serial.println();
  	  memset(buffer, 0xFF, 2048);
	  myNAND.read(writeaddress, buffer, 2048);
	  Serial.println();
	  for (uint16_t j = 0; j < 12; j++) {
	    for (uint16_t i = 0; i < arraySize; i++) {
	      Serial.printf("0x%02x, ", buffer[j * arraySize + i]);
	    } Serial.println();
	  }
	 Serial.println();


//---------init data - load array
	mydata.datastruct.data_0 = true;
	Serial.print("Data_0: ");
	if (mydata.datastruct.data_0) Serial.println("true");
	if (!mydata.datastruct.data_0) Serial.println("false");
	mydata.datastruct.data_1 = 1.3575;
	Serial.print("Data_1: ");
	Serial.println(mydata.datastruct.data_1, DEC);
	mydata.datastruct.data_2 = 314159L;
	Serial.print("Data_2: ");
	Serial.println(mydata.datastruct.data_2, DEC);
	mydata.datastruct.data_3 = 142;
	Serial.print("Data_3: ");
	Serial.println(mydata.datastruct.data_3, DEC);	
	mydata.datastruct.data_4 = 0x50;
	Serial.print("Data_4: 0x");
	Serial.println(mydata.datastruct.data_4, HEX);
	Serial.println("...... ...... ......");
	

	//string test
	String string_test = "The Quick Brown Fox";
	string_test.toCharArray(mydata.datastruct.data_4,string_test.length()+1);
	//Serial.println(string_test);
	
	Serial.println();
	Serial.println("Init Done - array loaded");
	Serial.println("...... ...... ......");

  	memset(buffer, 0xFF, 2048);
	for(uint16_t i = 0; i < (512/arraySize); i++) {  //set up a 512-byte sub-page, 
		for(uint16_t j = 0; j < arraySize; j++) {
			buffer[j + i*arraySize]= mydata.I2CPacket[j];
		}
	}
	for(uint8_t j = 0; j < 4; j++) {
		myNAND.randomProgramDataLoad(writeaddress+j*512, buffer, 2048);
		myNAND.programExecute(writeaddress*j*512);
	}

    Serial.println("Write Done - array loaded in FRAM chip");
	Serial.println("...... ...... ......");

	
//---------read data from memory chip

	myNAND.readBytes(writeaddress, readdata.I2CPacket, arraySize);
    Serial.println("Read Done - array loaded with read data");
    Serial.println("            at Address (0)");
	Serial.println("...... ...... ......");
	
	Serial.print("Data_0: ");
	if (readdata.datastruct.data_0) Serial.println("true");
	if (!readdata.datastruct.data_0) Serial.println("false");
	Serial.print("Data_1: ");
	Serial.println(readdata.datastruct.data_1, DEC);
	Serial.print("Data_2: ");
	Serial.println(readdata.datastruct.data_2, DEC);
	Serial.print("Data_3: ");
	Serial.println(readdata.datastruct.data_3, DEC);	
	Serial.print("Data_4: 0x");
	Serial.println(readdata.datastruct.data_4, HEX);
	Serial.println("...... ...... ......");
	Serial.println("Read Write test done - check data if successfull");
	Serial.println("...... ...... ......");	

    Serial.println("Write Done - array loaded in FRAM chip");


	Serial.println("...... ...... ......");
	  Serial.println();
  	  memset(buffer, 0xFF, 2048);
	  myNAND.read(writeaddress, buffer, 2048);
	  Serial.println();
	  for (uint16_t j = 0; j < 12; j++) {
	    for (uint16_t i = 0; i < arraySize; i++) {
	      Serial.printf("0x%02x, ", buffer[j * arraySize + i]);
	    } Serial.println();
	  }

}

void loop() {
	// nothing to do
}