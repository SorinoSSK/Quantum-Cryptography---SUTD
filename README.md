# Quantum Cryptography
As networking technology advances, there has been an upsurge in demand for secure communication channels. Many important data have been transmitted across the network, and they must be encrypted to safeguard it. **Quantum Cryptography** is one of the techniques used to ensure that all of our communication channels are protected. This project uses the BB84 protocol to mimic Quantum Cryptography with an 8-bit encryption key.
## Requirements notes
### Software
The structure is based on the use of three computers, notably Alice, Bob, and Eve, each of which controls a modular set. Arduino Mega is used to operate in each modules thus each computer requires the installation of Arduino IDE, https://www.arduino.cc/en/software. The project is tested and operated on Arduino IDE version 1.8.5.
### Hardware
Ensure that the laser's intensity is within the OPT101's operating range. In this project, the focused laser's maximum OPT101 readings in a closed room lab setting range from 570 to 650.
## Operation
### Step 1:
Link the modules together, either 
1) Alice and Bob
2) Alice, Eve and Bob where Eve is in between Alice and Bob
### Step 2:
Insert *charged* batteries into the battery case.
### Step 3:
Connect the Arduino Mega to a computer, 1 Arduino Mega is to be connected to a computer. All linear polariser should be rotated to stepper motor's 0 position upon the connection or start up of the Arduino Mega (Alice, Bob and Eve's program should have been uploaded into the Arduino Mega).
### Step 4:
Align the laser on Alice to the OPT101 on Bob. If Eve is connected, align the laser on Alice to the OPT101 on Eve and align the laser on Eve to the OPT101 on Bob. The focused laser should be fully within the black portion of OPT101 sensor.
### Step 5:
Open Arduino IDE on each computer controlling, Alice, Bob and Eve. To run the program, open Serial Monitor in the following order.
1) Alice or Eve
2) Bob
Bob's module is programmed to initiate the alignment of linear polariser thus it has to be reset last. (Opening Serial Monitor restarts the Arduino). The Serial Monitor button is a magnifying glass like button at the top right of the Arduino IDE.
### Step 6:
The project is split into 3 section:
1) Align Polariser
2) Bob initiate sending of encryption key 
3) 1. If the key Alice has matches with Bob, Alice and Bob will proceed to send message to one another.
3) 2. If the key Alice has does not match with Bob, a prompt will appear in Bob's Serial Monitor to rebuild the key.

(Note: It is advisable to restart the whole process starting from polariser alignment. This is due to the difference in ambient light at different period of time. Ambient light can affect the light intensity reading when building the key). 
### Step 7:
Enjoy sending message over to each other! :)
