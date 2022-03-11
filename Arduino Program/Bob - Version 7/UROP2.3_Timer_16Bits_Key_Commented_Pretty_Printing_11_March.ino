 /* Code Representation 2
  * 691139 - Sensor has completed setup
  * 691140 - Laser Tranmitter has completed setup
  * 691141 - Laser Receiver completed 180 degree light check
  *        - rotate to check for next basis
  * 691142 - Feedback Transmitter moved on request 
  * 691143 - Laser receiver recorded ambience value for offset   
  * 691144 - Begin Quantum Crypto transmission
  * 691145 - Bob request to send Alice data
  * 691146 - Alice stating validation key is correct
  * 691147 - Alice stating validation key is wrong
  * 691148 - Timeout request for last sent data.
  * 
  * Functions
  * getRandomSeed()                   - Return randomised seed values from noise voltage
  * Time()                            - Return waited time
  * Time(Long)                        - Return True/False of time reached
  * RecordTimer()                     - Records new timestamp
  * sendIRData(String, String, Long)  - Set data to be sent later
  * sendIRData()                      - Sends data through IR with preset data
  * enableIR()                        - Enable IR after time reached 100
  * resetTask()                       - Reset task values
  * TaskTimer()                       - Return task's waited time
  * TaskTimer(Long)                   - Return True/False of task's time reached
  * RecordTaskTimer()                 - Records new task timestamp
  * timeOut()                         - Count for timeout and activate timeout sequence
  * 
  * After Value 100022 require more than 500ms
  */
#include <Servo.h>
#include "IRLibAll.h" //library for IR
#include <IRLib_P01_NEC.h>
#include <IRLibDecodeBase.h>
#include <IRLibCombo.h>
#include <string.h>

//Pins
int Sense = A3;                     // Pin for OPT101         (Can be any pin)    **Can be edited**
int Motor = 6;                      // Pin for Servo motor    (Require PWM)       **Can be edited**
IRrecvPCI myReceiver(2);            // Receiver connected to pin 2
                                    // Transmitter pin 3 for uno and pin 9 for mega
//Position                  
int BiasesPos[4]  = {0,0,0,0};      // STORE state position
int BiasesVal[4]  = {0,0,0,0};      // STORE state intensity value

//For Data Collection
int Data[16];                       //array to store the measurement for the 30 measurements
int index = 0;                      //Use multiple times for counting
int cut   = 0;                      //The cut off to decide if it is a high or a low

//Checker Variable
int     taskTracker = -1;           //Too track of the task
bool    Printing    = false;        //To swtich between detailed printing and less printing

// Variables for Key
long   Cipher_Key      = 0;         //Encrytion Key

// Variables for timer
long    TimeRec     = 0;                  // Main timer
long    TaskTimeRec = 0;                  // Task timer
bool    EnReceiver  = false;              // CHECKER for enabling IR receiver
String  val         = "";                 // STORE IR transmission value
String  type        = "";                 // STORE IR transmission type 
long    ms          = 0;                  // SET   IR transmission delay
long    msTask      = 0;                  // SET   task delay
bool    idle        = true;               // Disable timeout (Controller forced to idle)
long    msTimeOut   = 3000;               // SET  timeout time            **Can be edited**
String  LastVal     = "";                 // STORE last IR transmission value
String  LastType    = "";                 // STORE last IR transmission type
bool MsgChannel = true;                   //For timeout

// For Users input
String Message = "";                      //For Alice message
bool AliceTalking = false;                //To take note that Alice is talking

String myMessage = "";                    //For Bob's messsage
bool BobTalking = false;                  //To take note that Bob is talking
int TextLength = 0;                       //To check how many characters are to be sent
int sendIndex = 0;                        //To keep track of the current charcater position

//Instances
Servo     BiasStepper;
IRsend    mySender;
IRdecode  myDecoder;                      //create a decoder object  

void setup() 
{
    Serial.begin(9600);
    pinMode(Sense, INPUT);
    BiasStepper.attach(Motor);
    BiasStepper.write(0);
    randomSeed(getRandomSeed());
    if (Printing){
      Serial.println("Debugging mode is enabled");
    }
    else {
      Serial.println("Debugging mode is disabled");
    }
    
    Serial.println("System initialised.");
    delay(100);
    //setting up the IR receiver
    Serial.println("Ready to receive IR signals. Waiting for Alice to be ready...");
    sendIRData("691139", "long", 500);                      //To tell Alice that Bob is ready
    RecordTimer();
    TimeRec = millis();
    EnReceiver = true;
}

void loop() 
{
    sendIRData();                                           //To send message
    enableIR();                                             //Ensure receiving of IR signals
    if (myReceiver.getResults()){                           //If receive anything from IR
        myDecoder.decode();                                 //Decode the IR Signal
        if (Printing) {                                     //Printing what is received and when it is received
            Serial.print("--+ IR Checker received ");
            Serial.print(myDecoder.value);
            Serial.print(", ");
            Serial.print(Time());
            Serial.println("ms after IR is enabled.");
        }
        switch(myDecoder.value)                             //For the different cases of IR signal
        {
            case 0:
                myReceiver.enableIRIn();
                if (Printing)
                    Serial.println("--+ Received 0, re-enabling IR receiver due to stray voltage.");
                break;
            case 691140:                                    //Checking with polarisation state 0
                taskTracker = 0;                            //Ready task 0 after 500 ms
                msTask = 500;
                RecordTaskTimer();
                MsgChannel = false;                         //No need for timeout anymore
                break;
            case 691142:                                    //Checking with polarisation state 2
                taskTracker = 2;                            //Ready task 2 after 500 ms
                msTask = 500;                               
                RecordTaskTimer();
                MsgChannel = false;
                break;
            case 691144:                                    //Begin Quantum channel
                taskTracker = 3;                            
                msTask = 50;
                RecordTaskTimer();
                index = 0;
                MsgChannel = false;
                Serial.println("-----------------------------");
                Serial.println("Quantum Channel");
                break;
            case 691146:                                    //Matched Key
                Serial.println("Validation Key matched!");
                Serial.println("Communication channel is open. Type your message below.");
                for (int i = 8; i<16;i++)
                  Cipher_Key = (Cipher_Key<<1) + Data[i];   //Generate Encryption Key using binary operation
                if (Printing){
                  Serial.print("Cipher_Key: ");
                  Serial.println (Cipher_Key);
                }
                
                break;
            case 691147:                                    //Dismacthed Key
                Serial.println("It is not a macth. Eve could be listening.");
                break;
            case 691148:                                    //Received timeout to resend last value
                if (Printing) Serial.println("--+ Time out received!");
                sendIRData(LastVal, LastType, 500);
                RecordTimer();
                break;
            case 691149:                                    //Alice is talking
                Serial.println("(Listening to Alice. Console closed)");
                sendIRData("691150", "long", 500);
                RecordTimer();
                AliceTalking = true;
                MsgChannel = true;
                break;
            case 691151:                                    //Alice confirmed that the character received is correct
                if(LastVal != "691150"){
                    if(Printing){
                        Serial.print("Alice sent: ");
                        Serial.print(LastVal.toInt());
                        Serial.print(" decipher to: ");
                        Serial.println(char(LastVal.toInt()^Cipher_Key));
                    }
                        Message += char(LastVal.toInt()^Cipher_Key);
                        sendIRData("691150", "long", 500);
                        RecordTimer();
                }
                MsgChannel = true;
                break;
            case 691152:                                  //Alice rejects the last character received
                break;
            case 691153:                                  //End of Alice message
                Serial.print("Alice: ");
                Serial.println(Message);
                Message = "";
                MsgChannel = false;
                AliceTalking = false;
                break;
            case 691155:                                  //Alice confirmed that she is listening
                if (Printing) Serial.println("Alice is listening.");
                sendIRData(String(myMessage[sendIndex]^Cipher_Key), "long", 500);
                RecordTimer();
                break;
            default:                                      //When receive undetermined values
                if (index < 16)                           //If index is less than 16 so there are more data needed to form the key
                {
                    if (Printing) {
                    Serial.print("Check bit from A: ");
                    Serial.println(myDecoder.value);
                    Serial.print("Current Index: ");
                    Serial.println(index);
                    }
                    else{
                      if (myDecoder.value == 1){
                        Serial.println("Base mismatch");
                      }
                      else{
                        Serial.println("Base matched");
                      }
                    }
                    
                    if (myDecoder.value == 2)             //Alice confirmed that we use the same basis.
                    {
                        DisplayCurrentKey();
                        index++;
                    }                    
                    if (index < 16)                       //Have not reach the end of the key so continue doing task 3
                    {
                        taskTracker = 3; msTask = 50; RecordTaskTimer();
                    }                    
                    else                                  //Reached the end of the key so do task 4
                    {
                        Serial.println("⥬ Key length reached");
                        sendIRData("691145", "long", 500);
                        RecordTimer();

                        taskTracker = 4; msTask = 600; RecordTaskTimer();
                    }
                }
                else if(AliceTalking) {                   //If Alice is talking
                  sendIRData(String(myDecoder.value), "long", 500);
                  RecordTimer();
                  MsgChannel = false;
                }
                else if (BobTalking){                     //If Bob is talking
                  if(myDecoder.value == myMessage[sendIndex]^Cipher_Key){ //Alice received the correct value
                      if (sendIndex == (myMessage.length()-1)){           //If it has reached the length of the message
                          sendIRData("691158", "long", 500);
                          RecordTimer();
                          MsgChannel = false;
                          Serial.println("✓");
                      }
                      else{                                               //If it has not reached the length of the message
                          sendIndex++;
                          sendIRData("691156", "long", 500);
                          RecordTimer();
                      }
                  }
                  else{                                                   //Alice did not receive the correct value
                      sendIRData("691157", "long", 500);
                      RecordTimer();
                      taskTracker = 5; msTask = 500; RecordTaskTimer();
                  }
                }
                break;
        }
        myReceiver.enableIRIn(); //reset IR Receiver
    }
    if (taskTracker != -1 && TaskTime(msTask))
    {
        if (taskTracker == 0)                                             //Task 0 is to search for highest value with respect to state 0
        {
            Serial.println("Alice indicated laser is ready. Begin intensity caliberation.");
            Serial.println("-----------------------------");
            SearchHigh(0);
            msTask = 100;
            taskTracker = 1;
        }
        else if (taskTracker == 1)                                        //Task 1 is to print what has been found so far
        {
            Polarisation(index);
            msTask = 400;
        }
        else if (taskTracker == 2)                                        //Task 2 is to search for highest value with respect to state 2
        {
            SearchHigh(2);
            taskTracker = 1;
            msTask = 900;
        }
        else if(taskTracker == 3)                                         //Task 3 is to start quantum channel between Alice and Bob
        {
            CaptureData(random(4));
            taskTracker = -1;
        }
        else if(taskTracker == 4)                                         //Task 4 is to send validation bits
        {
            index = 16;
            long tempVal = 0;
            for (int i = 0; i<8;i++)
                tempVal = tempVal*10 + (Data[i]+1);

            String Validation = "";
            for (int i = 0; i<8;i++)
                Validation += String(Data[i]);
            Serial.println("-----------------------------");
            Serial.print("Validation Key: ");
            if (Printing){
              Serial.println(tempVal);
            }
            else {
              Serial.println(Validation);
            }

            String Encryption = "";
            for (int i = 8; i<16;i++)
                Encryption += String(Data[i]);
            
            Serial.print("Encryption Key: ");
            Serial.println(Encryption);
            Serial.println("-----------------------------");
          
            
            sendIRData(String(tempVal), "long", 600);
            RecordTimer();
            taskTracker = -1;
        }
        else if (taskTracker == 5){                                       //Task 5 is to send encrypted charcater value
            sendIRData(String(myMessage[sendIndex]^Cipher_Key), "long", 700);
            RecordTimer();
            taskTracker = -1;
        }
        RecordTaskTimer();
    }
    if (Serial.available() != 0 && AliceTalking == false){                //If receive message from console while Alice is not talking
      myMessage = Serial.readString();
      sendIndex = 0;
      String tempString = "";
      for (int i=0; i<myMessage.length()-1; i++){
        tempString += myMessage.charAt(i);
      }
      Serial.print("Bob: ");
      Serial.print(tempString);
      Serial.print(" ✓");
      BobTalking = true;
      sendIRData("691154", "long", 500);
      RecordTimer();
      MsgChannel = true;
    }
    timeOut();
}

void timeOut()                                                            //Timeout function to send IR signal
{
    if (MsgChannel && Time(msTimeOut) && val == "")
    {
        if (Printing)
            Serial.println("--+ System timeout, requesting resend");
        sendIRData(LastVal, "long", 50);
        RecordTimer();
    }
}

void DisplayCurrentKey()                                                  //Function to display current key
{
    Serial.print("Agreed Key: ");
    for (int i = 0; i<=index; i++)
        Serial.print(Data[i]);
    Serial.println("");
}

void CaptureData(int choice)                                              //Function to display what is communicated in the quantum channel
{
    int Basis = BiasesPos[choice];
    BiasStepper.write(Basis); //set servo to that angle
    delay(400);
    Data[index] = analogRead(Sense);
    Serial.print("⟹ State: ");
    if (Printing){
      Serial.print(choice);
      Serial.print(" Value: ");
      Serial.print(Data[index]);
    }
    else {
      if (choice == 0){
        Serial.print("|V〉");
      }
      else if (choice == 1){
        Serial.print("|H〉");
      }
      else if (choice == 2){
        Serial.print("|-〉");
      }
      else if (choice == 3){
        Serial.print("|+〉");
      }
    }
    if ((Data[index]-cut)<0)
    {
        if (choice == 0 || choice == 2)
            Data[index] = 0;
        else
            Data[index] = 1;
    }
    else
    {
        if (choice == 0 || choice == 2)
            Data[index] = 1;
        else
            Data[index] = 0;
    }
    Serial.print(" Bit decision: ");
    Serial.println(Data[index]);
    int holder;
    if (choice <2)
        holder = 1;
    else
        holder = 2;
    if(Printing){
      Serial.print("Axis: ");
      Serial.println(holder);
    }
    sendIRData(String(holder), "long", 600);
    RecordTimer();
}

void SearchHigh(int x){                                            //Function to search for highest value
    for (int angle = 0; angle<180 ; angle++)
    {
        BiasStepper.write(angle);
        delay(100);
        int Val = analogRead(Sense);
        if (Printing) {
          Serial.print(angle);
          Serial.print(" : ");
          Serial.println(Val);
        }
        if (BiasesVal[x] <= Val)
        {
            BiasesVal[x] = Val;
            BiasesPos[x] = angle;
        }
    }
    BiasStepper.write(BiasesPos[x]);
    Serial.print("Peak laser intensity: ");
    Serial.print(BiasesVal[x]);
    Serial.print(", Stepper angle: ");
    Serial.println(BiasesPos[x]);
    if (BiasesPos[x] <= 90)      BiasesPos[x+1] = BiasesPos[x] + 90;
    else if(BiasesPos[x] >= 90)  BiasesPos[x+1] = BiasesPos[x] - 90;
    Serial.println("");
}

void Polarisation(int i)                                            //Function to display polarisation
{
    Serial.print("Position: ");
    if (Printing){
      Serial.println(i);
    }
    else {
      if (i == 0){
        Serial.println("|V〉");
      }
      else if (i == 1){
        Serial.println("|H〉");
      }
      else if (i == 2){
        Serial.println("|-〉");
      }
      else if (i == 3){
        Serial.println("|+〉");
      }
    }
    
    BiasStepper.write(BiasesPos[i]);
    delay(300);
    BiasesVal[i] = analogRead(Sense);
    Serial.print("Laser intensity ");
    Serial.print(BiasesVal[i]);
    Serial.print(", stepper angle ");
    Serial.println(BiasesPos[i]);
    Serial.println("");
    index++;
    if (i == 1)
    {
        sendIRData("691141", "long", 700);
        RecordTimer();
        taskTracker = -1;
        MsgChannel = true;
    }
    else if (i==3){
      cut = (BiasesVal[0]+BiasesVal[1]+BiasesVal[0]+BiasesVal[1])/4;
      taskTracker = -1;
      Serial.print("Average intensity value: ");
      Serial.println(cut);
      Serial.println("-----------------------------");
      Serial.println("Intensity caliberation completed. Waiting for Alice to begin quantum channel.");
   
      index = 0;
      sendIRData("691143", "long", 600);
      RecordTimer();
      MsgChannel = true;
    }
}

//########## The below section is for IR Communication only ##########

void enableIR()
{
    if (Time(100) && EnReceiver)
    {
        myReceiver.enableIRIn();                                  // Start the receiver
        if (Printing)
        {
            Serial.print("--+ IR enabled after ");
            Serial.print(Time());
            Serial.println("ms");
        }
        EnReceiver = false;
        RecordTimer();
    }
}

void sendIRData()
{
    if (Time(ms) && val != "")
    {
        type.toUpperCase();
        myReceiver.disableIRIn();
        if (type == "LONG" || type == "INT")
            mySender.send(NEC, val.toInt(), 38);
        else if (type == "CHAR")
            mySender.send(NEC, val.charAt(0), 38);
        if (Printing)
        {
            Serial.print("--+ Sent value ");
            Serial.print(val);
            Serial.print(" after ");
            Serial.print(Time());
            Serial.println("ms");
        }
        type = "";
        val = "";
        ms = 0;
        RecordTimer();
        EnReceiver = true;
    }  
}

void sendIRData(String valIR, String typeIR, long msIR)
{
    val = valIR;
    type = typeIR;
    ms = msIR;
    LastVal = val;
    LastType = type;
}

void RecordTimer() { TimeRec = millis(); }

long Time() { return millis() - TimeRec; }

bool Time(long time)
{
    if (millis() - TimeRec >= time)
        return true;
    else
        return false;
}

void RecordTaskTimer() { TaskTimeRec = millis(); }

long TaskTime() { return millis() - TaskTimeRec; }

bool TaskTime(long time)
{
    if (millis() - TaskTimeRec >= time)
        return true;
    else
        return false;
}

//########## https://forum.arduino.cc/t/how-random-is-arduino-random/369900/14 by aarg ##########
int getRandomSeed()
{
    const int baseIntervalMs = 250UL;
    const byte sampleSignificant = 13;
    const byte sampleMultiplier = 50;
    const byte hashIterations = 6;
    int intervalMs = 0;
    unsigned long reading;
    int result = 0;
    for (int i = 0; i < hashIterations; i++)
    {
        pinMode(A0, INPUT_PULLUP);
        pinMode(A0, INPUT);
        delay(baseIntervalMs + intervalMs);
        reading = analogRead(A0);
        result |= (reading & 1) << i;
        intervalMs = (reading % sampleSignificant) * sampleMultiplier;
    }
    return result;
}
