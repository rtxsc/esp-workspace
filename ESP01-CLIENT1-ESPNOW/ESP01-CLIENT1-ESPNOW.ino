/*
  SimpleEspNowConnectionClient
  Basic EspNowConnection Client implementation
  Edited on macOS 14 March 2022
*/

#include "SimpleEspNowConnection.h"

SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);

String inputString;
String serverAddress;

typedef struct struct_message {
  char type;
  char a[32];
  int b;
  float c;
  bool e;
} struct_message;

bool sendBigMessage()
{
  char bigMessage[] = "\n\
There was once a woman who was very, very cheerful, though she had little to make her so; for she was old, and poor, and lonely. She lived in a little bit of a cottage and earned a scant living by running errands for her neighbours, getting a bite here, a sup there, as reward for her services. So she made shift to get on, and always looked as spry and cheery as if she had not a want in the world.\n\
Now one summer evening, as she was trotting, full of smiles as ever, along the high road to her hovel, what should she see but a big black pot lying in the ditch!\n\
\"Goodness me!\" she cried, \"that would be just the very thing for me if I only had something to put in it! But I haven't! Now who could have left it in the ditch?\"\n\
And she looked about her expecting the owner would not be far off; but she could see nobody.\n\
\"Maybe there is a hole in it,\" she went on, \"and that's why it has been cast away. But it would do fine to put a flower in for my window; so I'll just take it home with me.\"\n\
And with that she lifted the lid and looked inside. \"Mercy me!\" she cried, fair amazed. \"If it isn't full of gold pieces. Here's luck!\"\n\
And so it was, brimful of great gold coins. Well, at first she simply stood stock-still, wondering if she was standing on her head or her heels. Then she began saying:\n\
\"Lawks! But I do feel rich. I feel awful rich!\"\n\
";   

 return(simpleEspConnection.sendMessage(bigMessage));  
}

bool sendStructMessage()
{
  struct_message myData;
  
  myData.type = '#'; // just to mark first byte. It's on you how to distinguish between struct and text message
  sprintf (myData.a, "Greetings from %s", simpleEspConnection.myAddress.c_str());
  myData.b = random(1,20);
  myData.c = (float)random(1,100000)/(float)10000;
  myData.e = true;
    
  return(simpleEspConnection.sendMessage((uint8_t *)&myData, sizeof(myData)));
}

void OnSendError(uint8_t* ad)
{
  Serial.println("[OnSendError from CLIENT] SENDING TO '"+simpleEspConnection.macToStr(ad)+"' WAS NOT POSSIBLE!");
}

void OnMessage(uint8_t* ad, const uint8_t* message, size_t len)
{
  if((char)message[0] == '#') // however you distinguish....
  {
    struct_message myData;
    
    memcpy(&myData, message, len);
    Serial.printf("Structure:\n");    
    Serial.printf("a:%s\n", myData.a);    
    Serial.printf("b:%d\n", myData.b);    
    Serial.printf("c:%f\n", myData.c);    
    Serial.printf("e:%s\n", myData.e ? "true" : "false");   
    simpleEspConnection.sendMessage("ACK-1 CLIENT1"); // to acknowledge the receipt of struct
 
  }
  else{
    Serial.printf("MESSAGE:[%d]%s from %s\n", len, (char *)message, simpleEspConnection.macToStr(ad).c_str());
    simpleEspConnection.sendMessage("ACK-2 CLIENT1"); // to acknowledge the receipt of msg
  }
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{  
  Serial.println("New GatewayAddress '"+ad+"'");
  serverAddress = ad;

  simpleEspConnection.setServerMac(ga);
}

void setup() 
{
  Serial.begin(115200);
  Serial.println();

  Serial.println("Setup...");

  simpleEspConnection.begin();
//  simpleEspConnection.setPairingBlinkPort(2);  
// E8DB84988D42 ESP01
// AC67B2258578 ESP32 
  serverAddress = "AC67B2258578"; // ESP32 Server Gateway
   simpleEspConnection.setServerMac(serverAddress);
  simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
  simpleEspConnection.onSendError(&OnSendError);  
  simpleEspConnection.onMessage(&OnMessage);  
  
  Serial.println(WiFi.macAddress());  
}

void loop() 
{
  // needed to manage the communication in the background!  
  simpleEspConnection.loop();
  
  while (Serial.available()) 
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n') // must set Serial Monitor's line ending to Newline
    {
      Serial.println(inputString);

      if(inputString == "startpair")
      {
        simpleEspConnection.startPairing(30);
      }
      else if(inputString == "endpair")
      {
        simpleEspConnection.endPairing();
      }      
      else if(inputString == "changepairingmac")
      {
        uint8_t np[] {0xCE, 0x50, 0xE3, 0x15, 0xB7, 0x33};
        
        simpleEspConnection.setPairingMac(np);
      }      
      else if(inputString == "textsend")
      {
        if(!simpleEspConnection.sendMessage("This comes from the Client"))
        {
          Serial.println("SENDING textsend TO '"+serverAddress+"' WAS NOT POSSIBLE!");
        }
      }
      else if(inputString == "structsend")
      {
        if(!sendStructMessage())
        {
          Serial.println("SENDING structsend TO '"+serverAddress+"' WAS NOT POSSIBLE!");
        }
      }
      else if(inputString == "bigsend")
      {
        if(!sendBigMessage())
        {
          Serial.println("SENDING bigsend TO '"+serverAddress+"' WAS NOT POSSIBLE!");
        }
      }
      
      inputString = "";
    }
    else
    {
      inputString += inChar;
    }
  }
}