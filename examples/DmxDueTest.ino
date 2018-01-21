#include <DmxDue.h>

//------------------------------------
void setup()
{ 
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  Serial.println("Hello");
  
  DmxDue1.setTxMaxChannels(30);
  DmxDue1.begin();  //Start Transmission and receiving

  delay (500);
  Serial.println("Begin ok");
     
  // Fill-in output data array
  for (int i = 1; i < 30; i++)
  {
    DmxDue1.write(i, i + 1);
  }
  
  DmxDue1.setRxEvent(rxEv);  
}

int DataReceived=0;

void rxEv()
{
DataReceived=1;
}

void rxPrint()
{ int n;
  Serial.print(n=DmxDue1.getRxLength());Serial.print(":   ");
for (int i=1;i<=n;i++)
  {
    Serial.print(DmxDue1.read(i));Serial.print(";");
  }
Serial.println();
}

//------------------------------------
void loop()
{ 
  // just blinking ;)
 digitalWrite(13, HIGH);
 delay(1000);

 //And print data, if received
 if (DataReceived) 
 {
  rxPrint();
  DataReceived=0;
 }
 
 digitalWrite(13, LOW);
 delay(1000);
}
