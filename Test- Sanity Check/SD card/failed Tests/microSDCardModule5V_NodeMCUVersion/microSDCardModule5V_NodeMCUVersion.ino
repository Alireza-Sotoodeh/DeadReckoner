/*
  SD-Card write and read test with ESP8266

  Connections:
    MicroSD Card
    Card Module       ESP8266
    --------------------------------
     3V3 -------------------- 3V3 
     SCK -------- GPIO 14 --- D5
     MISO ------- GPIO 12 --- D6
     MOSI ------- GPIO 13 --- D7
     CS --------- GPIO 15 --- D8
     GND -------------------- GND
*/

#include <SPI.h>
#include <SD.h> 

File Textfile;   

void setup() {
  Serial.begin(74880);
  Serial.println("Initializing SD card");

  if (!SD.begin(15)) {  
    Serial.println("Initialization failed!");  
    return;
  }

  Serial.println("Initialization completed"); 
  Textfile = SD.open("test.txt", FILE_WRITE);  

  if (Textfile) {
    Serial.println("Writing to Textfile...");  
    Textfile.println("First line");    
    Textfile.println("1, 2, 3, 4, 5");
    Textfile.println("a, b, c, d, e");
    Textfile.println();
    Textfile.close();     
    Serial.println("Completed");       
    Serial.println();
  }
  else {
    Serial.println("Text file could not be read");  
  }

  // READ TEXTFILE
  Textfile = SD.open("test.txt");      
  if (Textfile) {
    Serial.println("test.txt:");    
    while (Textfile.available()) {
      Serial.write(Textfile.read());     
    }
    Textfile.close();    
  }
  else  {
    Serial.println("Text file could not be opened"); 
  }
}

void loop()  {
}