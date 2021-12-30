 /* Program written by Seow Sin Kiat in collaboration of Ivan Feng
  * Credit of randomisation function is have to be given to user, aarg.
 */
 /* Code Representation ver 2
  * 691139 - Sensor has completed setup.
  * 691140 - Laser Tranmitter has completed setup.
  * 691141 - Laser Receiver completed 180 degree light check.
  *        - rotate to check for next basis.
  * 691142 - Feedback Transmitter moved on request.
  * 691143 - Laser receiver recorded ambience value for offset.
  * 691144 - Begin Quantum Crypto transmission.
  * 691145 - Bob request to send Alice data.
  * 691146 - Alice stating validation key is correct.
  * 691147 - Alice stating validation key is wrong.
  * 691148 - Timeout request for last sent data.
  * 691149 - After timeout indication for Serial Monitor to inform Bob on the start of sending encrypted message. 
  *        - (no more characters received, String is read character by character)
  * 691150 - Bob acknowledge acknowledge the sending of encrypted message and reply.
  * 691151 - Alice inform Bob that there is no error in the character he receive.
  * 691152 - Alice inform Bob that there is error in the character he receive.
  * 691153 - Alice inform Bob that the full message have been sent.
  * 691154 - After timeout indication for Serial Monitor to inform Alice on the start of sending encrypted message. 
  *        - (no more characters received, String is read character by character)
  * 691155 - Alice acknowledge the sending of encrypted message and reply.
  * 691156 - Bob inform Alice that there is no error in the character he receive.
  * 691157 - Bob inform Alice that there is error in the character he receive.
  * 691158 - Bob inform Alice that the full message have been sent.
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
  * input(char IVal)                  - Process Serial Monitor input (Start command or sending text to Bob)
  * BinaryConverter(String Val)       - Convert String received from IR to Binary and store as long
  * EncryptValue(char Val)            - XOR Encryption
  * resetIR()                         - Clear IR transmission values and state
  * resetKey()                        - Clear encryption key built and state
  * 
  * After Value 100022 require more than 500ms
  */

/*----- Libraries -----*/
#include <Servo.h>
#include <Console.h>
#include <IRLibAll.h>
#include <IRLib_P01_NEC.h>
#include <IRLibDecodeBase.h>
#include <IRLibCombo.h>

/*----- Pins -----*/
int Laser = 4;                            // Pin for Laser        (Can be any pin)    **Can be edited**
int Motor = 6;                            // Pin for Servo motor  (Require PWM)       **Can be edited**
int LED   = 13;                           // Pin for LED checker  (Can be ignored)    **Can be edited**
IRrecvPCI myReceiver(2);                  // Receiver connected to pin 2
                                          // Transmitter pin 3 for uno and pin 9 for mega
/*----- Task Oriented -----*/
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
long    msTimeOut   = 3000;               // SET  timeout time                        **Can be edited**
String  LastVal     = "";                 // STORE last IR transmission value
String  LastType    = "";                 // STORE last IR transmission type
String  msg         = "";                 // STORE message string

/*----- Position -----*/
int BiasesPos[4]    = {0,90,45,135};

//Checker
bool    LightRdy    = true;               // Force receiver to be able to activate this only once
bool    StartEn     = false;              // Force Quantum channel to activate with this on true
bool    Printing    = true;               // Disable to remove debugging prints       **Can be edited**
bool    Sending     = false;              // Force rotation of linear polariser activation
bool    ValiCheck   = false;              // Force system to check validation key.
bool    Commu       = false;              // Timeout to get last character of message sent
bool    MsgChannel  = false;              // Enable timeout for sending message
bool    Msg2Channel = false;              // Force system to check validity of last message sent
bool    BobTalking  = false;              // Check if Bob is sending any message

/*----- Key -----*/
int     State     = 0;                    // Store the last state of linear polariser
String  EncryKey  = "";                   // To build encryption key
String  ValiKey   = "";                   // Sacrificical bits to check with Bob

/*----- Instances -----*/
Servo     BiasStepper;                    // Create Servo object
IRsend    mySender;                       // Create IR transmitter object
IRdecode  myDecoder;                      // Create decoder object

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
    mySender.send(NEC, 691140, 38);                           // To indicate laser is turned on
    TimeRec = millis();                                       // Initialise timestamp
    EnReceiver = true;                                        // Enable receiver in 100ms
}

/*----- Program flow -----*/
/* 1) Send data through IR if available
 * 2) Enable IR receiver in 100ms if enabled
 * 3) Read and perfomer task if IR receiver received data
 * 4) Perform task if enabled
 * 5) Timeout of required
*/
void loop()
{
    sendIRData();                                     // Send IR data
    enableIR();                                       // Enable IR receiver
    if (myReceiver.getResults())                      // IR receiver's message
    {
        myDecoder.decode();
        if (Printing && myDecoder.value != 0)         // To debug on IR data received and the timing it received
        {
            Serial.print("--+ IR Checker received ");
            Serial.print(myDecoder.value);
            Serial.print(", ");
            Serial.print(Time());
            Serial.println("ms after IR is enabled.");
        }
        switch (myDecoder.value)
        {
            case 0:                                   // Ignore if receiver received 0, interferred data read by IR receiver will be read as 0.
                myReceiver.enableIRIn();
                if (Printing)
                    Serial.println("--+ Received 0, re-enabling IR receiver due to stray signal.");
                break;
            case 691139:                              // Alice informing Bob that her photosensor is ready.
                if (LightRdy)                         // Alice can only inform Bob to start once.
                {
                    sendIRData("691140", "long", 150);    // Bob giving feedback to start calibration.
                    RecordTimer();                        // Reset timer to send data
                    LightRdy = false;                     // Disable 691139
                }
                break;
            case 691141:                              // Alice requesting bob to rotate linear polariser to the next base.
                LastTask = "StepperChecker";              // Set task to be worked on
                msTask = 0;                               // Set delay for task
                RecordTaskTimer();                        // Reset timer for task
                idle = false;                             // Enable time out
                break;
            case 691143:                              // Alice informing Bob that calibration is completed.
                Serial.println("-----------------------------");
                Serial.println("System and console setup complete.");     // Printing
                Serial.println("Key in 's' or 'S' to begin.");
                Serial.println("-----------------------------");
                StartEn = true;                                           // Enable reading of data.
                break;
            case 691145:                              // Alice's start byte to send validation bits 
                ValiCheck = true;                           // Set task to verify validation bits
                EnReceiver = true;                          // Enable IR receiver in 100ms
                RecordTimer();                              // Reset timer for IR to start
                break;
            case 691148:                              // Timeout received from Bob
                if (Printing)
                    Serial.println("--+ Time out received!");
                sendIRData(LastVal, LastType, 500);         // Send last value and type sent.
                RecordTimer();                              // Reset timer for sending.
                break;
            case 691150:                              // Alice's reply from Bob's request to send encrypted message.
                LastTask = "Communication";                 // Set task to communication mode
                msTask = 300;                               // Set task delay to 300ms
                RecordTaskTimer();                          // Reset task timer
                break; 
            case 691154:                              // Bob request to send Alice message
                Serial.println("Listening to Bob..");       // 
                sendIRData("691155", "long", 500);          // Reply Bob to send the message.
                RecordTimer();                              // Reset timer to send message
                BobTalking = true;                          // Force system to be reading from Bob
                StartEn = false;                            // Disable Serial Com
                break;
            case 691156:                              // Bob informing Alice that the message he received is correct.
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
                    sendIRData("691155", "long", 500);      // Replying Bob to send the next character
                }
                RecordTimer();                              // Reset timer to send message
                break;
            case 691157:                              // Bob informing Alice that the message he received is incorrect.
                EnReceiver = true;                          // Do nothing and wait for message
                RecordTimer();                              // Reset timer to enable receiver
                break;
            case 691158:                              // Bob informing Alice that he have finish sending his messages.
                Serial.print("Bob: ");
                Serial.println(msg);
                EnReceiver = true;                          // Enable receiver after 100ms  
                msg = "";                                   // Reset message string
                StartEn = true;                             // Enable Serial Com input
                BobTalking = false;                         // Reset listening to bob
                break;
            default:                                  // Any other data that is not filtered
                if (EncryKey.length() < 16 && !KeyBuilt)    // if building encryption key
                {
                    int Bases = 2;                          // Init Base state to |psi>
                    String DataToSend = "1";                // Init data state to 1
                    Sending = true;                         // Enable sending for data
                    if (State == 0 || State == 1)           // if state is not in base |psi>
                        Bases = 1;                          // change base to |phi>
                    if (myDecoder.value == Bases)           // if Bob has the same base as Alice
                    {
                        DataToSend = "2";                   // reply acknowledgement
                        if (State == 0 or State == 2)       // check for the state in the base
                            EncryKey += String(1);
                        else
                            EncryKey += String(0);
                        if (EncryKey.length() == 16)        // If key is fully built
                        {
                            Serial.println("--+ Key length reached!");
                            Sending = false;                // Disable sending
                            LastTask = "Validation";        // Change task to validation mode
                            msTask = 0;                     // Reset task delay to 0
                            RecordTaskTimer();              // Reset task timer
                            KeyBuilt = true;                // Record key is built
                        }
                        Serial.print("Building key: ");
                        Serial.println(EncryKey);           // Prints key while it builds
                    }
                    sendIRData(DataToSend, "int", 500);     // Send key info through IR to Bob
                    RecordTimer();                          // Reset IR transmission timer
                }
                else if (ValiCheck)                         // If program finish building of key and enter validation check
                {
                    String returnVal = "691147";            // Init code to be sent to Bob (Validation key is wrong)
                    if (myDecoder.value == ValiKey.toInt()) // If validation key matches Bob
                    {
                        returnVal = "691146";               // Change code to be sent to Bob (Validation key is correct)
                        Serial.println("Validation key match!");
                        Serial.println("Communication channel is open. Type your message below.");
                    }
                    else                                    // Prompt to restart building key
                    {
                        Serial.println("Validation key does not match!");
                        Serial.println("-----------------------------");
                        Serial.println("Incorrect key!");   // Printing
                        Serial.println("Key in 's' or 'S' to restart.");
                        Serial.println("-----------------------------");
                        resetKey();                         // Reset key built
                        resetIR();                          // Reset IR values
                    }
                    StartEn = true;                         // Enable reading of data.
                    idle = true;                            // Enable device idling (Disable timeout)
                    ValiCheck = false;                      // End Validation mode
                    sendIRData(returnVal, "int", 1000);     // Send IR value to Bob
                }
                else if (Msg2Channel)                       // If sending message to Bob (Validation key matches)
                {
                    if(Printing)                            // If printing is enabled (for debugging)
                    {
                      Serial.print("Last Value: ");
                      Serial.print(LastVal);
                      Serial.print(", comparing to received value: ");
                      Serial.println(myDecoder.value);
                    }
                    if (LastVal == String(myDecoder.value)) // If sent value matches Bob's reply
                    {
                        if(Printing)                        // If printing is enabled (for debugging)
                            Serial.println("--+ Last character sent matches.");
                        sendIRData("691151", "long", 200);  // Send code to Bob to inform him to receive next byte of the message.
                    }
                    else                                    // Else error occur in the message sent to Bob.
                    {
                        if(Printing)                        // If printing is enabled (for debugging)
                            Serial.println("--+ Last character sent does not match.");
                        sendIRData(LastVal, "long", 50);    // Resend message sent to Bob
                    }
                    Msg2Channel = false;                    // Turn off message sending mode
                }
                else if(BobTalking)                         // If Bob is sending message to Alice
                {
                    sendIRData(String(myDecoder.value), "long", 500); // Reply Bob on whatever he send
                    RecordTimer();                          // Reset IR transmission delay timer
                }
                break;
        }
    }
    else if (Serial.available() > 0)                        // If Serial Monitor receive input
    {
        input(Serial.read());                               // Transfer input received to function and to be processed
    }
    if (LastTask != "" && TaskTime(msTask))                 // If there are still task to be performed and delay have been reached
    {
        if (LastTask == "StepperChecker")                   // Check if task is stepper motor's movement
        {
            if (Loop == 0)                                  // If linear polariser is performing 1st stage synchronisation check
            {
                msTask = 700;                               // Set task delay (wait for servo motor to finish moving)
                BiasStepper.write(BiasesPos[2]);            // Set stepper motor to 1st stage position
                sendIRData("691142", "long", 200);          // Inform Bob that 1st stage position check is completed
                idle = true;                                // Enable Idling (Disable IR transmission time out)
            }
            else if (Loop == 1)                             // If linear polariser is performing 2nd stage synchronisation check
            {
                resetTask();                                // Reset Task
            }
            RecordTimer();                                  // Reset IR transmission delay
            RecordTaskTimer();                              // Reset Task delay
        }
        else if (LastTask == "Sending" && Sending)          // If Task is on key value comparison. (Compare laser's base)
        {
            int RandoVal = random(4);                       // Get random integer from a range of 0-3
            Serial.print("==+ State: ");
            Serial.println(RandoVal);
            int Bias = BiasesPos[RandoVal];                 // Retrieve position angle with the given random integer
            BiasStepper.write(Bias);                        // Write position to servo
            State = RandoVal;                               // Retrieve data from loop
            Sending = false;                                // Disable the sending of message
        }
        else if (LastTask == "Validation")                  // If encryption key is built
        {                                                   // Divide 16 bit encryption key to 8bit validation and 8bit encryption key
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
        else if (LastTask == "Communication")                 // If Alice is sending Bob a message (After validation key matches).
        {
            if (Loop < msg.length())                          // If message is not fully sent. (send message in byte, 1 character = 1 byte)
            {
                sendIRData(String(EncryptValue(msg.charAt(Loop))), "long", 100);
                RecordTimer();
                Msg2Channel = true;
                LastTask = "";
            }
            else                                              // If the full message have been sent
            {
                sendIRData("691153", "long", 100);            // Send message ending code (Inform Bob to start decoding the full message)
                resetTask();    
                MsgChannel = false;
                msg = "";
            }                
        }
        Loop++;
    }
    timeOut();                                                // Time out there is no more message to be sent
}

/*----- Timeout function -----*/
void timeOut()
{
    if (MsgChannel && Time(msTimeOut) && val == "")           // If timeout occur during the sending of message
    {
        if (Printing)                                         // If printing is enabled (for debugging)
            Serial.println("--+ System timeout, requesting resend");
        sendIRData(LastVal, "long", 50);                      // Resend last Val
    }
    else if (!idle && Time(msTimeOut) && val == "")           // If timeout occur during non-idle mode
    {
        if (Printing)                                         // If printing is enabled (for debugging)
            Serial.println("--+ System timeout, requesting resend");
        sendIRData("691148", "long", 50);                     // Request Bob to resend his last message
    }
    if (Time(500) && Commu)                                   // If time out occur when collecting characters from Serial Monitor
    {                                                         // This time out indicates the collection of String from Serial Monitor is now empty
        sendIRData("691149", "long", 50);                     // Inform Bob on the start of sending encrypted message
        Serial.print("Alice: ");
        Serial.println(msg);
        Serial.println("--+ End of Message");
        Loop = 0;                                             // Reset message collection Loop
        MsgChannel = true;                                    // Enable message channel to be opened
        Commu = false;                                        // Disable collection of characters from Serial Monitor
        RecordTimer();                                        // Reset IR transmission delay timer
    }
}

/*----- Input function -----*/
// Function to process characters received from Serial Monitor
void input(char IVal)
{
    String IVal2 = String(IVal);                              // Convert Characters to String
    IVal2.toUpperCase();                                      // Standardise Serial Monitor message input by capitalising it
    if(StartEn && EncryKey != "")                             // If Serial Monitor input collection is enabled and Encryption key is built
    {
        Commu = true;                                         // Enable the collection of String from Serial Monitor
        msg += String(IVal);                                  // Build the String of message from Serial Monitor character by character
        if (Printing)                                         // If printing is enabled (for debugging)
            Serial.print(IVal);
        RecordTimer();                                        // Reset IR Transmission timer 
    }
    else if (IVal2 == "S" && StartEn)                         // If Serial Monitor input collection is enabled and "S" (Start command) is received
    { 
        sendIRData("691144", "long", 300);                    // Send code to Bob to start building encryption key
        LastTask = "Sending";                                 // Set task to building encryption key
        Sending = true;                                       // Enable sending of key
        idle = false;                                         // Disable idle mode (Enable timeout)
        Serial.println("Start Command Received. Sending Data...");
        StartEn = false;                                      // Disable Serial Monitor
    }
}
/*----- Reset Key function -----*/
void resetKey()
{
    KeyBuilt    = false;              // CHECKER for key sent.
    idle        = true;               // Disable timeout (Controller forced to idle)
    EncryKey  = "";
    ValiKey   = "";
}
/*----- Reset IR Transmission function -----*/
void resetIR()
{
    LastVal     = "";                 // STORE last IR transmission value
    LastType    = "";                 // STORE last IR transmission type  
}
/*----- Reset Task function -----*/
void resetTask()
{
    LastTask = "";                    // Task Type
    msTask = 0;                       // Task delay
    Loop = 0;                         // Task Looping
}
/*----- Enable IR transmission function -----*/
void enableIR()                       // IR have to be re-enabled after receiving a byte
{
    if (Time(100) && EnReceiver)      // If delay timer is reached and IR receiver is to be enabled
    {
        myReceiver.enableIRIn();      // Start the receiver
        if (Printing)                 // If printing is enabled (for debugging)
        {
            Serial.print("--+ IR enabled after ");
            Serial.print(Time());
            Serial.println("ms");
        }
        EnReceiver = false;
        RecordTimer();                // Reset IR Transmission timer
    }
}
/*----- Send data through IR transmission function -----*/
void sendIRData()                     // This function sends data through IR
{
    if (Time(ms) && val != "")        // If delay have been reached 
    {
        type.toUpperCase();           // Standardise data type to be sent.
        myReceiver.disableIRIn();     // Disable receiving of IR transmission during receiving
        if (type == "LONG" || type == "INT")      // Check for data type
            mySender.send(NEC, val.toInt(), 38);
        else if (type == "CHAR")
            mySender.send(NEC, val.charAt(0), 38);
        if (Printing)                 // If printing is enabled (for debugging)
        {
            Serial.print("--+ Sent value ");
            Serial.print(val);
            Serial.print(" after ");
            Serial.print(Time());
            Serial.println("ms");
        }
        type = "";                    // Reset IR transmission after sending
        val = "";
        ms = 0;
        RecordTimer();
        EnReceiver = true;
    }  
}
/*----- Set data to be sent through IR transmission function -----*/
void sendIRData(String valIR, String typeIR, long msIR)
{
    val = valIR;                    // Function to declare all IR transmission requirement
    type = typeIR;
    ms = msIR;
    LastVal = val;
    LastType = type;
}
/*----- Reset timer function -----*/
void RecordTimer()
{
    TimeRec = millis();
}
/*----- Get lapse time function -----*/
long Time()
{
    return millis() - TimeRec;
}
/*----- Time comparison function -----*/
bool Time(long time)
{
    if (millis() - TimeRec >= time)
        return true;
    else
        return false;
}
/*----- Reset task timer function -----*/
void RecordTaskTimer()
{
    TaskTimeRec = millis();
}
/*----- Get lapse task time function -----*/
long TaskTime()
{
    return millis() - TaskTimeRec;
}
/*----- Task time comparison function -----*/
bool TaskTime(long time)
{
    if (millis() - TaskTimeRec >= time)
        return true;
    else
        return false;
}
/*----- XOR encryption function -----*/
long EncryptValue(char Val)
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
    return Val ^ EncryKeyInt;       // XOR individual characters
}
/*----- String to binary to long function -----*/
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
