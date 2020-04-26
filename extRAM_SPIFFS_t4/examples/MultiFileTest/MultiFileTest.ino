#include <extRAM_t4.h>
#include <spiffs.h>

//Setup 2 files IO
spiffs_file file1;
spiffs_file file2;
spiffs_file file3;

extRAM_t4 eRAM;
//uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 1; //0 - flash, 1 - eram
                           //2 - 2 4meg eram pseudo partitions
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_FLASH_PSRAM

uint8_t config = INIT_PSRAM_ONLY;

char buf[1024] = "";
char fname[32] = "my_file1";
int szLen = strlen( buf );
elapsedMicros my_us;

//define a struct of various data types
 struct MYDATA_t {
  bool data_0;
  float data_1; 
  long data_2; 
  int data_3;
  char data_4[32];
};

//define a struct joining MYDATA_t to an array of bytes to be stored
 union MYDATA4RAM_t {
 MYDATA_t datastruct;
 char Packet[sizeof(MYDATA_t)];
};

MYDATA4RAM_t mydata; //data to be written in memory
MYDATA4RAM_t readdata; //data read from memory

void setup() {
  while (!Serial);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
#if 1
  Serial.println("\n Enter 'y' in 6 seconds to format FlashChip - other to skip");
  uint32_t pauseS = millis();
  char chIn = 9;
  while ( pauseS + 6000 > millis() && 9 == chIn ) {
    if ( Serial.available() ) {
      do {
        if ( chIn != 'y' )
          chIn = Serial.read();
        else
          Serial.read();
      }
      while ( Serial.available() );
    }
  }
  if ( chIn == 'y' ) {
    int8_t result = eRAM.begin(config);
    if(result == 0){
      eRAM.eraseFlashChip();
    } else {
      eRAM.eraseDevice();
    }
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config);
  eRAM.fs_mount();

  Serial.println();
  Serial.println("Initial Directory contents:");
  eRAM.fs_listDir();

  //test of print
  Serial.println();
  Serial.println("Using println and printf to printoutput file");
  eRAM.f_open(file1, "PRINTOUTPUT1", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  eRAM.f_open(file2, "PRINTOUTPUT2", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);

  Serial.printf("File handle for File1: %d\n", file1);
  Serial.printf("File handle for File2: %d\n", file2);

  for(uint8_t i = 0; i < 10; i++) {
    eRAM.printTo(file1);
    eRAM.f_write(file1, "abcdefghijklmnopqrstuvwxyz",26);
    eRAM.printf("\nRec: %d, Float: %f, Int: %d\n",i,i+26.4, i+98);
    eRAM.printTo(file2);
    eRAM.f_write(file2, "ABCDEFGHIJKLMNOPQRSTUVWXYZ",26);
    //eRAM.println("THIS IS A TEST");
    eRAM.printf("\nRec: %d, Float: %f, Int: %d\n",i,i+56.4, i+198);
  }
  eRAM.f_close(file1);
  eRAM.f_close(file2);

  Serial.println();
  Serial.println("Second Directory contents:");
  eRAM.fs_listDir();

  Serial.println("-------------------------------");
  Serial.println("File1 contents:");
  Serial.println("-------------------------------");

  eRAM.f_open(file1, "PRINTOUTPUT1", SPIFFS_RDONLY);
  for(uint8_t i = 0; i <30; i++){
    if(eRAM.f_eof(file1)) //break loop on EOF
        break;
    eRAM.f_read(file1, buf, 1024);
    Serial.println(buf);
  }
  eRAM.f_close(file1);

  //Structured data writes to SPIFFS ala MB85RC I2C FRAM library
  structuredWrite();

}

void loop() {
  // put your main code here, to run repeatedly:
}

void structuredWrite(){
  Serial.println("-------------------------------");
  Serial.println("File3 byte conversion test:");
  Serial.println("-------------------------------");
  Serial.println();
  
  uint32_t arraySize = sizeof(MYDATA_t);
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
  //string test
  String string_test = "The Quick Brown Fox";
  string_test.toCharArray(mydata.datastruct.data_4,string_test.length()+1);
  Serial.println(string_test);
    
  Serial.println("Init Done - array loaded");
  Serial.println("...... ...... ......");

  //lets try something more interesting and complicated
  eRAM.f_open(file3, "logger.txt", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
   for(int32_t i = 0; i < 10; i++) {
    eRAM.f_write(file3, mydata.Packet, arraySize);
  }
  eRAM.f_close(file3);

  Serial.println();
  Serial.println("3rd Directory contents:");
  eRAM.fs_listDir();
  Serial.println();
  
  eRAM.f_open(file3, "logger.txt", SPIFFS_RDONLY);
  for(uint8_t i = 0; i <30; i++){
    if(eRAM.f_eof(file3)) //break loop on EOF
        break;
    eRAM.f_read(file3, readdata.Packet, arraySize);
      //---------Send data to serial
      if (readdata.datastruct.data_0) Serial.print("true");
      if (!readdata.datastruct.data_0) Serial.print("false");
      Serial.print(", ");
      Serial.print(readdata.datastruct.data_1, DEC);
      Serial.print(", ");
      Serial.print(readdata.datastruct.data_2, DEC);
      Serial.print(", ");
      Serial.print(readdata.datastruct.data_3, DEC);  
      Serial.print(", ");
        for (uint8_t j = 0; j < 19 + 1; j++) {
          Serial.print(readdata.datastruct.data_4[j]);
        }
      Serial.println();
  }
  eRAM.f_close(file3);
}