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
#include <Console.h>
#include <IRLibAll.h>
#include <IRLib_P01_NEC.h>
#include <IRLibDecodeBase.h>
#include <IRLibCombo.h>

//Pins
int Laser = 4;                // Pin for Laser        (Can be any pin)    **Can be edited**
int Motor = 6;                // Pin for Servo motor  (Require PWM)       **Can be edited**
int LED   = 13;               // Pin for LED checker  (Can be ignored)    **Can be edited**
IRrecvPCI myReceiver(2);      // Receiver connected to pin 2
                              // Transmitter pin 3 for uno and pin 9 for mega
// Task Oriented
long    TimeRec     = 0;                  // Main timer
long    TaskTimeRec = 0;                  // Task timer
bool    EnReceiver  = false;              // CHECKER for enabling IR receiver
bool    KeyBuilt    = false;              // CHECKER for key sent.
String  LastTask    = "";                 // STORE task
String  val         = "";                 // STORE IR transmission value
String  type        = "";                 // STORE IR transmission type 
long    ms          = 0;                  // SET   IR transmission delay
long    msTask      = 0;                  // SET   task delay
int     Loop        = 0;                  // Main loop count
bool    idle        = true;               // Disable timeout (Controller forced to idle)
long    msTimeOut   = 3000;               // SET  timeout time            **Can be edited**
String  LastVal     = "";                 // STORE last IR transmission value
String  LastType    = "";                 // STORE last IR transmission type
String  msg         = "";                 // STORE message string

//Position
int BiasesPos[4]    = {0,90,45,135};

//Checker
bool    LightRdy    = true;               // Force receiver to be able to activate this only once
bool    StartEn     = false;              // Force Quantum channel to activate with this on true
bool    Printing    = false;               // Disable to remove debugging prints       **Can be edited**
bool    Sending     = false;              // Force rotation of linear polariser activation
bool    ValiCheck   = false;              // Force system to check validation key.
bool    Commu       = false;
bool    MsgChannel  = false;
bool    Msg2Channel = false;
bool    BobTalking  = false;

// Key
int     State     = 0;
String  EncryKey  = "";
String  ValiKey   = "";

// Instances
Servo     BiasStepper;
IRsend    mySender;
IRdecode  myDecoder;                                          //create a decoder object

void setup()
{
    Serial.begin(9600);                                       // Begin serial communication
    pinMode(Laser, OUTPUT);                                   // Set pin for laser to output
    pinMode(LED,OUTPUT);                                      // Set onboard LED to output (For visual verification that IR is working)
    BiasStepper.attach(Motor);                                // Set pint for servo
    BiasStepper.write(0);                                     // Set servo to position 0 (0 to 180 degrees)
    digitalWrite(Laser, HIGH);                                // Set Laser to be turned on on default
    randomSeed(getRandomSeed());                              // Generate random seed with stray voltage from analog read
    Serial.println("System initialised, waiting for IR receiver");
    mySender.send(NEC, 691140, 38);                                 // To indicate laser is turned on
    TimeRec = millis();
    EnReceiver = true;
}

void loop()
{
    sendIRData();
    enableIR();
    if (myReceiver.getResults())
    {
        myDecoder.decode();
        if (Printing && myDecoder.value != 0)
        {
            Serial.print("--+ IR Checker received ");
            Serial.print(myDecoder.value);
            Serial.print(", ");
            Serial.print(Time());
            Serial.println("ms after IR is enabled.");
        }
        switch (myDecoder.value)
        {
            case 0:
                myReceiver.enableIRIn();
                if (Printing)
                    Serial.println("--+ Received 0, re-enabling IR receiver due to stray signal.");
                break;
            case 691139:
                if (LightRdy)
                {
                    sendIRData("691140", "long", 150);
                    RecordTimer();
                    LightRdy = false;
                }
                break;
            case 691141:
                LastTask = "StepperChecker";
                msTask = 0;
                RecordTaskTimer();
                idle = false;
                break;
            case 691143:
                Serial.println("-----------------------------");
                Serial.println("System and console setup complete.");     // Printing
                Serial.println("Key in 's' or 'S' to begin.");
                Serial.println("-----------------------------");
//                idle = true;
                StartEn = true;                                           // Enable reading of data.
                break;
            case 691145:
                ValiCheck = true;
                EnReceiver = true;
                RecordTimer();
                break;
            case 691148:
                if (Printing)
                    Serial.println("--+ Time out received!");
                sendIRData(LastVal, LastType, 500);
                RecordTimer();
                break;
            case 691150:
                LastTask = "Communication";
                msTask = 300;
                RecordTaskTimer();
                break;
            case 691154:
                Serial.println("Listening to Bob..");
                sendIRData("691155", "long", 500);
                RecordTimer();
                BobTalking = true;
                StartEn = false;
                break;
            case 691156:
                if(LastVal != "691155")
                {
                    if(Printing)
                    {
                        Serial.print("Bob sent: ");
                        Serial.print(LastVal.toInt());
                        Serial.print(" decipher to: ");
                        Serial.println(char(LastVal.toInt()^BinaryConverter(EncryKey)));
                    }
                    msg += char(LastVal.toInt()^BinaryConverter(EncryKey));
                    sendIRData("691155", "long", 500);
                }
                RecordTimer();
                break;
            case 691157:
                break;
            case 691158:
                Serial.print("Bob: ");
                Serial.println(msg);
                EnReceiver = true;
                msg = "";
                StartEn = true;
                BobTalking = false;
                break;
            default:
                if (EncryKey.length() < 16 && !KeyBuilt)
                {
                    int Bases = 2;
                    String DataToSend = "1";
                    Sending = true;
                    if (State == 0 || State == 1)
                        Bases = 1;
                    if (myDecoder.value == Bases)
                    {
                        DataToSend = "2";
                        if (State == 0 or State == 2)
                            EncryKey += String(1);
                        else
                            EncryKey += String(0);
                        if (EncryKey.length() == 16)
                        {
                            Serial.println("--+ Key length reached!");
                            Sending = false;
                            LastTask = "Validation";
                            msTask = 0;
                            RecordTaskTimer();
                            KeyBuilt = true;
                        }
                        Serial.print("Building key: ");
                        Serial.println(EncryKey);
                    }
                    sendIRData(DataToSend, "int", 500);
                    RecordTimer();
                }
                else if (ValiCheck)
                {
                    String returnVal = "691147";
                    if (myDecoder.value == ValiKey.toInt())
                    {
                        returnVal = "691146";
                        Serial.println("Validation key match!");
                        Serial.println("Communication channel is open. Type your message below.");
                    }
                    else
                    {
                        Serial.println("Validation key does not match!");
                        Serial.println("-----------------------------");
                        Serial.println("Incorrect key!");     // Printing
                        Serial.println("Key in 's' or 'S' to restart.");
                        Serial.println("-----------------------------");
                        resetKey();
                        resetIR();
                    }
                    StartEn = true;                                           // Enable reading of data.
                    idle = true;
                    ValiCheck = false;
                    sendIRData(returnVal, "int", 1000);
                }
                else if (Msg2Channel)
                {
                    if(Printing)
                    {
                      Serial.print("Last Value: ");
                      Serial.print(LastVal);
                      Serial.print(", comparing to received value: ");
                      Serial.println(myDecoder.value);
                    }
                    if (LastVal == String(myDecoder.value))
                    {
                        if(Printing)
                            Serial.println("--+ Last character sent matches.");
                        sendIRData("691151", "long", 200);
                    }
                    else
                    {
                        if(Printing)
                            Serial.println("--+ Last character sent does not match.");
                        sendIRData(LastVal, "long", 50);
                    }
                    Msg2Channel = false;
                }
                else if(BobTalking)
                {
                    sendIRData(String(myDecoder.value), "long", 500);
                    RecordTimer();
                }
                break;
        }
    }
    else if (Serial.available() > 0)
    {
        input(Serial.read());
    }
    if (LastTask != "" && TaskTime(msTask))
    {
        if (LastTask == "StepperChecker")
        {
            if (Loop == 0)
            {
                msTask = 700;
                BiasStepper.write(BiasesPos[2]);
                sendIRData("691142", "long", 200);
                idle = true;
            }
            else if (Loop == 1)
            {
                resetTask();
            }
            RecordTimer();
            RecordTaskTimer();
        }
        else if (LastTask == "Sending" && Sending)
        {
            int RandoVal = random(4);                                 // Get random integer from a range of 0-3
            Serial.print("==+ State: ");
            Serial.println(RandoVal);
            int Bias = BiasesPos[RandoVal];                           // Retrieve position angle with the given random integer
            BiasStepper.write(Bias);                                  // Write position to servo
            State = RandoVal;                                         // Retrieve data from loop
            Sending = false;
        }
        else if (LastTask == "Validation")
        {
            for (int i = 0; i < 8; i++)
            {
                ValiKey += String(String(EncryKey.charAt(i)).toInt() + 1);
            }
            String EncKey = "";
            for (int i = 8; i < 16; i++)
            {
                EncKey += EncryKey.charAt(i);
            }
            EncryKey = EncKey;
            Serial.println("----------");
            Serial.println("Computed Keys:");
            Serial.print("Validation Key: ");
            Serial.println(ValiKey);
            Serial.print("Encryption Key: ");
            Serial.println(EncryKey);
            Serial.println("----------");
            LastTask = "";
        }
        else if (LastTask == "Communication")
        {
            if (Loop < msg.length())
            {
                sendIRData(String(EncryptValue(msg.charAt(Loop))), "long", 100);
                RecordTimer();
                Msg2Channel = true;
                LastTask = "";
            }
            else
            {
                sendIRData("691153", "long", 100);
                resetTask();    
                MsgChannel = false;
                msg = "";
            }                
        }
        Loop++;
    }
    timeOut();
}

void timeOut()
{
    if (MsgChannel && Time(msTimeOut) && val == "")
    {
        if (Printing)
            Serial.println("--+ System timeout, requesting resend");
        sendIRData(LastVal, "long", 50);
    }
    else if (!idle && Time(msTimeOut) && val == "")
    {
        if (Printing)
            Serial.println("--+ System timeout, requesting resend");
        sendIRData("691148", "long", 50);
    }
    if (Time(500) && Commu)
    {
        sendIRData("691149", "long", 50);
        Serial.print("Alice: ");
        Serial.println(msg);
        Serial.println("--+ End of Message");
        Loop = 0;
        MsgChannel = true;
        Commu = false;
        RecordTimer();
    }
}
void input(char IVal)
{
    String IVal2 = String(IVal);
    IVal2.toUpperCase();
    if(StartEn && EncryKey != "")
    {
        Commu = true;
        msg += String(IVal);
        if (Printing)
            Serial.print(IVal);
        RecordTimer();
    }
    else if (IVal2 == "S" && StartEn)
    { 
        sendIRData("691144", "long", 300);
        LastTask = "Sending";
        Sending = true;
        idle = false;
        Serial.println("Start Command Received. Sending Data...");
        StartEn = false;
    }
}
void resetKey()
{
    KeyBuilt    = false;              // CHECKER for key sent.
    idle        = true;               // Disable timeout (Controller forced to idle)
    EncryKey  = "";
    ValiKey   = "";
}
void resetIR()
{
    LastVal     = "";                 // STORE last IR transmission value
    LastType    = "";                 // STORE last IR transmission type  
}
void resetTask()
{
    LastTask = "";
    msTask = 0;
    Loop = 0;
}
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
void RecordTimer()
{
    TimeRec = millis();
}
long Time()
{
    return millis() - TimeRec;
}
bool Time(long time)
{
    if (millis() - TimeRec >= time)
        return true;
    else
        return false;
}
void RecordTaskTimer()
{
    TaskTimeRec = millis();
}
long TaskTime()
{
    return millis() - TaskTimeRec;
}
bool TaskTime(long time)
{
    if (millis() - TaskTimeRec >= time)
        return true;
    else
        return false;
}
long EncryptValue (char Val)
{
    long EncryKeyInt = BinaryConverter(EncryKey);
    if (Printing)
    {
        Serial.print("--+ String: ");
        Serial.println(EncryKey);
        Serial.print("--+ Converted to Int ");
        Serial.println(EncryKeyInt);
        Serial.print("--+ Converted char ");
        Serial.println(Val ^ EncryKeyInt);
    }
    return Val ^ EncryKeyInt;
}
long BinaryConverter(String val)
{
    long val2 = 0;
    for(int i = 0; i < val.length(); i ++)
    {
        val2 += String(val.charAt(val.length()-1-i)).toInt()*round(pow(2, i));
    }
    return val2;
}

//https://forum.arduino.cc/t/how-random-is-arduino-random/369900/14 by aarg
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
