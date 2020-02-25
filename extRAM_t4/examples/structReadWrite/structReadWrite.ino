#include <math.h>
#include <extRAM_t4.h>

//define a struct of various data types
typedef struct MYDATA_t {
  bool data_0;
  float data_1; 
  long data_2; 
  int data_3;
  byte data_4[1024];
  char data_5[32];
};

//define a struct joining MYDATA_t to an array of bytes to be stored
typedef union MYDATA4RAM_t {
 MYDATA_t datastruct;
 uint8_t Packet[sizeof(MYDATA_t)];
};
  
MYDATA4RAM_t mydata; //data to be written in memory
MYDATA4RAM_t readdata; //data read from memory

//random address to write from
uint16_t writeaddress = 0x00;

//Creating object for FRAM chip
extRAM_t4 mymemory;

void setup() {

  Serial.begin(9600);
  while (!Serial) ; //wait until Serial ready
  
  uint32_t arraySize = sizeof(MYDATA_t);
  
  Serial.println("Starting...");
    
  mymemory.begin();

  //.........................
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
  Serial.print("Data_4: 0x");
  for(uint32_t ii = 0; ii<1024; ii++){ 
    mydata.datastruct.data_4[ii] = 0x42;
    //Serial.print(mydata.datastruct.data_4[ii], HEX); 
    //Serial.print(" ");
  }
  Serial.println("Block of 1024 0x42's loaded in array");
  Serial.println();
    
  //mydata.datastruct.data_4 = 0x50;
  //Serial.print("Data_4: 0x");
  //Serial.println(mydata.datastruct.data_4, HEX);
  Serial.println("...... ...... ......");
  
  //string test
  String string_test = "The Quick Brown Fox";
  string_test.toCharArray(mydata.datastruct.data_5,string_test.length()+1);
  Serial.println(string_test);
    
  Serial.println("Init Done - array loaded");
  Serial.println("...... ...... ......");
  
  mymemory.writeArrayDMA(writeaddress, arraySize, mydata.Packet);

    //if (result == 0) Serial.println("Write Done - array loaded in FRAM chip");
    //if (result != 0) Serial.println("Write failed");
  Serial.println("...... ...... ......");
  
  
//---------read data from memory chip
  mymemory.readArrayDMA(writeaddress, arraySize, readdata.Packet);
    //if (result == 0) Serial.println("Read Done - array loaded with read data");
    //if (result != 0) Serial.println("Read failed");
  Serial.println("...... ...... ......");
  
//---------Send data to serial
  Serial.print("Data_0: ");
  if (readdata.datastruct.data_0) Serial.println("true");
  if (!readdata.datastruct.data_0) Serial.println("false");
  Serial.print("Data_1: ");
  Serial.println(readdata.datastruct.data_1, DEC);
  Serial.print("Data_2: ");
  Serial.println(readdata.datastruct.data_2, DEC);
  Serial.print("Data_3: ");
  Serial.println(readdata.datastruct.data_3, DEC);  
  Serial.print("Data_4: ");
  for(uint32_t ii = 0; ii<1024; ii++) {
    //Serial.print(readdata.datastruct.data_4[ii], HEX);
    //Serial.print(" ");
    if(0x42 != readdata.datastruct.data_4[ii])
      Serial.printf("Error Reading 0x42 at %d\n", ii);
  }
  Serial.println();
  Serial.print("Data_5: ");
    for (uint8_t j = 0; j < 19 + 1; j++) {
      Serial.print(readdata.datastruct.data_5[j]);
    }
  Serial.println();
  Serial.println("...... ...... ......");
  Serial.println("Read Write test done - check data if successfull");
  Serial.println("...... ...... ......"); 
  
}


void loop() {
  // put your main code here, to run repeatedly:

}