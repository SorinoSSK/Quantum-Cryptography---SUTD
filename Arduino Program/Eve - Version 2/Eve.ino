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
int MotorA = 6;                      // Pin for Servo motor    (Require PWM)       **Can be edited**
int MotorB = 5;                      // Pin for Servo motor    (Require PWM)       **Can be edited**
int Laser = 4;
IRrecvPCI myReceiver(2);            // Receiver connected to pin 2
                                    // Transmitter pin 3 for uno and pin 9 for mega
//Position                  
int BiasesPos[4]  = {0,0,0,0};      // STORE state position
int BiasesVal[4]  = {0,0,0,0};      // STORE state intensity value

int BiasesPosA[4]  = {0,90,45,135};      // STORE state position

//For Data Collection
int Data[16];                       //array to store the measurement for the 30 measurements
int index = 0;                      //Use multiple times for counting
int cut   = 0;                      //The cut off to decide if it is a high or a low
int angle = 0;

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
Servo     BiasStepperA;
Servo     BiasStepperB;
IRsend    mySender;
IRdecode  myDecoder;                      //create a decoder object  

void setup() 
{
    Serial.begin(9600);
    pinMode(Sense, INPUT);
    BiasStepperA.attach(MotorA);
    BiasStepperA.write(0);
    BiasStepperB.attach(MotorB);
    BiasStepperB.write(0);
    randomSeed(getRandomSeed());
    pinMode(Laser, OUTPUT);
    digitalWrite(Laser, HIGH);
    Serial.println("System initialised.");
    myReceiver.enableIRIn();
    
}

void loop(){
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
                msTask = 100;
                RecordTaskTimer();
                angle = 0;
                break;
            case 691142:                                    //Checking with polarisation state 2
                taskTracker = 2;                            //Ready task 2 after 500 ms
                BiasStepperB.write(45);
                msTask = 100;                               
                RecordTaskTimer();
                angle = 0;
                break;
            case 691144:                                    //Begin Quantum channel
                taskTracker = 3;                            
                msTask = 50;
                RecordTaskTimer();
                index = 0;
                Serial.println("Quantum Channel");
                break;
            case 691146:                                    //Matched Key
                Serial.print("It is a match. They don't know. Cipher Key: ");
                for (int i = 8; i<index;i++)
                  Cipher_Key = (Cipher_Key<<1) + Data[i];   //Generate Encryption Key using binary operation
                Serial.println (Cipher_Key);
                index = 16;
                break;
            case 691147:                                    //Dismacthed Key
                Serial.println("They know I am listening");
                index = 16;
                break;
            case 691148:                                    //Received timeout to resend last value
                break;
            case 691149:                                    //Alice is talking
                Serial.println("(Listening to Alice)");
                AliceTalking = true;
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
                }
                break;
            case 691152:                                  //Alice rejects the last character received
                break;
            case 691153:                                  //End of Alice message
                Serial.print("Alice: ");
                Serial.println(Message);
                Message = "";
                AliceTalking = false;
                break;
            case 691155:                                  //Alice confirmed that she is listening
                Serial.println("Alice is listening to Bob");
                break;
            default:                                      //When receive undetermined values
                if (index < 16)                           //If index is less than 16 so there are more data needed to form the key
                {
                    if (Printing) { Serial.print("Check bit from A: "); Serial.println(myDecoder.value); Serial.print("Current Index: "); Serial.println(index);}
                    
                    if (myDecoder.value == 2) {            //Alice confirmed that we use the same basis.
                        DisplayCurrentKey();
                        index++;
                    }
                               
                    if (index < 16) {                       //Have not reach the end of the key so continue doing task 3
                        taskTracker = 3; msTask = 40; RecordTaskTimer();
                    }                    
                    else {                                  //Reached the end of the key so do task 4
                        Serial.println("Key completed");
                        taskTracker = 4; msTask = 600; RecordTaskTimer();
                    }
                }
                else if(AliceTalking) {                   //If Alice is talking
                  LastVal = String(myDecoder.value);
                }
                break;
        }
        myReceiver.enableIRIn(); //reset IR Receiver
    }
    if (taskTracker != -1 && TaskTime(msTask))
    {
        if (taskTracker == 0)                                             //Task 0 is to search for highest value with respect to state 0
        {
            Serial.println("Laser is ready, begin checking for polarisation.");
            SearchHigh(0, angle);
            angle++;
            if (angle == 180){
              taskTracker = 1;
            }
        }
        else if (taskTracker == 1)                                        //Task 1 is to print what has been found so far
        {
            Polarisation(index);
        }
        else if (taskTracker == 2)                                        //Task 2 is to search for highest value with respect to state 2
        {
            SearchHigh(2, angle);
            angle++;
            if (angle == 180){
              taskTracker = 1;
            }
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
            Serial.print("Validation: ");
            Serial.println(tempVal);
            taskTracker = -1;
        }
        RecordTaskTimer();
    }
}

void SearchHigh(int x, int a){                                            //Function to search for highest value
    BiasStepperA.write(a);
    int Val = analogRead(Sense);
    if (Printing) { Serial.print(a); Serial.print(" : "); Serial.println(Val); }
    
    if (BiasesVal[x] <= Val){
        BiasesVal[x] = Val;
        BiasesPos[x] = a;
    }
    if (angle == 179){
        BiasStepperA.write(BiasesPos[x]);
        Serial.print("Highest Value: ");
        Serial.print(BiasesVal[x]);
        Serial.print(", Postion: ");
        Serial.println(BiasesPos[x]);
        if (BiasesPos[x] <= 90)      BiasesPos[x+1] = BiasesPos[x] + 90;
        else if(BiasesPos[x] >= 90)  BiasesPos[x+1] = BiasesPos[x] - 90;
    }
}

void Polarisation(int i)                                            //Function to display polarisation
{
    Serial.print("Position: ");
    Serial.println(i);
    BiasStepperA.write(BiasesPos[i]);
    delay(300);
    BiasesVal[i] = analogRead(Sense);
    Serial.print("Laser intensity ");
    Serial.print(BiasesVal[i]);
    Serial.print(", at step ");
    Serial.println(BiasesPos[i]);
    index++;
    if (i == 1)
    {
        taskTracker = -1;
    }
    else if (i==3){
      cut = (BiasesVal[0]+BiasesVal[1]+BiasesVal[0]+BiasesVal[1])/4;
      taskTracker = -1;
      Serial.print("Average intensity value: ");
      Serial.println(cut);
      Serial.println("Search Complete. System ready to go.");
      index = 0;
    }
}

void CaptureData(int choice)                                              //Function to display what is communicated in the quantum channel
{
    int Basis = BiasesPos[choice];
    BiasStepperA.write(Basis); //set servo to that angle
    delay(200);
    Data[index] = analogRead(Sense);
    Serial.print("State: ");
    Serial.print(choice);
    Serial.print(" Value: ");
    Serial.print(Data[index]);
    if ((Data[index]-cut)<0){
        if (choice == 0 || choice == 2){
            Data[index] = 0;
            Basis = new_choice(choice);
        }
        else {
            Data[index] = 1;
        }
    }
    else{
        if (choice == 0 || choice == 2)
            Data[index] = 1;
        else
            Data[index] = 0;
            Basis = new_choice(choice);
    }
    BiasStepperB.write(BiasesPosA[Basis]);
    Serial.print(" Bit decision: ");
    Serial.println(Data[index]);
    int holder;
    if (choice <2)
        holder = 1;
    else
        holder = 2;
    if(Printing){Serial.print("Axis: "); Serial.println(holder); }
}

int new_choice(int x){
  if(x == 0 || x == 2){
    return x + 1;
  }
  else return x - 1;
}

void DisplayCurrentKey()                                                  //Function to display current key
{
    Serial.print("Current Key: ");
    for (int i = 0; i<=index; i++)
        Serial.print(Data[i]);
    Serial.println("");
}

//########## The below section is for IR Communication only ##########

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
