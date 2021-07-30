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
int Laser = 4;
int Motor = 6;
int LED   = 13;
IRrecvPCI myReceiver(2);                                    //Receiver conected to pin 2
                                                            //Transmitter pin 3 for uno and pin 9 for mega
// Task Oriented
long    TimeRec     = 0;
long    TaskTimeRec = 0;
bool    EnReceiver  = false;
String  LastTask    = "";
String  val         = "";                                   // IR Timer
String  type        = "";                                   // IR Timer
long    ms          = 0;                                    // IR Timer
long    msTask      = 0;                                    // Task
int     Loop        = 0;

//Position
int BiasesPos[4]    = {0,90,45,135};

//Checker
bool    LightRdy    = true;
bool    StartEn     = false;
bool    Printing    = true;
bool    Sending     = false;
bool    ValiCheck   = false;

// Key
int     State     = 0;
String  EncryKey  = "";
String  ValiKey   = "";

// Instances
Servo     BiasStepper;
IRsend    mySender;
IRdecode  myDecoder;                                          //create a decoder object  '

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
            Serial.print("IR Checker received ");
            Serial.print(myDecoder.value);
            Serial.print(", ");
            Serial.print(Time());
            Serial.println("ms after IR is enabled.");
        }
        switch (myDecoder.value)
        {
            case 0:
                myReceiver.enableIRIn();
                Serial.println("Received 0, re-enabling IR receiver due to stray signal.");
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
                break;
            case 691143:
                Serial.println("-----------------------------");
                Serial.println("System and console setup complete.");     // Printing
                Serial.println("Key in 's' or 'S' to begin.");
                Serial.println("-----------------------------");
                StartEn = true;                                           // Enable reading of data.
                break;
            case 691145:
                ValiCheck = true;
                myReceiver.enableIRIn();
                break;
            default:
                if (EncryKey.length() < 16)
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
                            Sending = false;
                            LastTask == "Validation";
                        }
                        if (Printing)
                        {
                            Serial.print("Key: ");
                            Serial.println(EncryKey);
                        }
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
                    }
                    else
                        Serial.println("Validation key does match!");
                    sendIRData(returnVal, "int", 500);
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
            }
            else if (Loop == 1)
            {
                BiasStepper.write(BiasesPos[3]);
                resetTask();
            }
            RecordTimer();
            RecordTaskTimer();
        }
        else if (LastTask == "Sending" && Sending)
        {
            int RandoVal = random(4);                                 // Get random integer from a range of 0-3
            int Bias = BiasesPos[RandoVal];                           // Retrieve position angle with the given random integer
            BiasStepper.write(Bias);                                  // Write position to servo
            State = RandoVal;                                         // Retrieve data from loop
            Sending = false;
        }
        else if (LastTask == "Validation")
        {
            for (int i = 0; i <= 8; i++)
            {
                ValiKey += String(String(EncryKey.charAt(i)).toInt() + 1);
            }
            String EncKey = "";
            for (int i = 8; i <= 16; i++)
            {
                EncKey += EncryKey.charAt(i);
            }
            EncryKey = EncKey;
            LastTask == "";
        }
        Loop++;
    }
}

void input(char IVal)
{
    String IVal2 = String(IVal);
    IVal2.toUpperCase();
    if (IVal2 == "S" && StartEn)
    { 
        sendIRData("691144", "long", 300);
        LastTask = "Sending";
        Sending = true;
        Serial.println("Start Command Received. Sending Data...");
        StartEn = false;
    }
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
            Serial.print("IR enabled after ");
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
            Serial.print("Sent value ");
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
