///////////////////////////////////////////////////////////////////////////////////
/// @file remotePanel.ino
/// @brief Sketch for the Compressed Air Rocket Remote Panel.
/// This sketch monitors the switches and buttons in the Remote Panel and
/// communicates with the Master Panel using Zigbee
/// @see LaunchPadPacket.h
///////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <XBee.h>
#include <LaunchDataPacket.h>

///////////////////////////////////////////////////////////////////////////////////
/// @def _ArmLedBitMode_
/// @brief Define the Digital Pin # for the Arm Switch LED.
/// Master controller has an Arm Toggle switch with build-in LED.
/// This output does not control the LED.  Instead it supplies power to the LED
/// and switch combo.  LED will not turn on without power.  LED will turn on when
/// it has power and the toggle switch is on.
///
/// @def _ArmLedPort_
/// @brief Define the Digital Port the Arm Switch LED is connected to.
///
/// @def _ArmLedBit_
/// @brief Define the Port Bit # for the Arm Switch LED.
/// In Port D, the Port Bit # is equal to the Digital Pin #.
/// In Port B, the Port Bit # is equal to the Digital Pin # minus 8.
///
/// @def _ArmSwitchBitMode_
/// @brief Define the Digital Input # for the Arm Switch.
/// This input monitors the Master controller Arm Toggle switch (DPST).
/// 
/// @def _ArmSwitchPort_
/// @brief Define the Digital Port the Arm Switch is connected to.
// 
/// @def _ArmSwitchBit_
/// @brief Define the Port Bit # for the Arm Switch
/// In Port D, the Port Bit # is equal to the Digital Pin #.
/// In Port B, the Port Bit # is equal to the Digital Pin # minus 8.
// 
/// @def _LaunchLedBitMode_
/// @brief Define the Digital Output # for the Launch Button LED.
/// Master controller has a Launch Button with build in LED.  This pin turns
/// the built in LED on and off.  The button does not control the LED.
/// 
/// @def _LaunchLedPort_
/// @brief Define the Digital Port the Launch Button LED is connected to.
/// 
/// @def _LaunchLedBit_
/// @brief Define the Port Bit # for the Launch Button LED.
/// In Port D, the Port Bit # is equal to the Digital Pin #.
/// In Port B, the Port Bit # is equal to the Digital Pin # minus 8.
///
/// @def _LaunchButtonBitMode_
/// @breif Define the Digital Input # for the Launch Button.
///
/// @def _LaunchButtonBitMode_
/// @breif Define the Digital Port the Launch Button is connected to.
///
/// @def _LaunchButtonBit_
/// @breif Define the Port Bit # for the Launch Button.
/// In Port D, the Port Bit # is equal to the Digital Pin #.
/// In Port B, the Port Bit # is equal to the Digital Pin # minus 8.
///
/// @def _ErrorLedBitMode_
/// @brief Define the Digital Input # for the Error LED.
///
/// @def _ErrorLedPort_
/// @brief Define the Digital Port the Error LED is connected to.
///
/// @def _ErrorLedBit_
/// @brief Define the Port Bit # for the Error LED.
/// In Port D, the Port Bit # is equal to the Digital Pin #.
/// In Port B, the Port Bit # is equal to the Digital Pin # minus 8.
///////////////////////////////////////////////////////////////////////////////////

#define _ArmLedBitMode_        2
#define _ArmLedPort_           PORTD
#define _ArmLedBit_            2

#define _ArmSwitchBitMode_     4
#define _ArmSwitchPort_        PORTD
#define _ArmSwitchBit_         4

#define _LaunchButtonBitMode_  5
#define _LaunchButtonPort_     PORTD
#define _LaunchButtonBit_      5

#define _LaunchLedBitMode_     3
#define _LaunchLedPort_        PORTD
#define _LaunchLedBit_         3

#define _ErrorLedBitMode_      6
#define _ErrorLedPort_         PORTD
#define _ErrorLedBit_          6

///////////////////////////////////////////////////////////////////////////////////
/// @def _pulse_halfwidth_
/// @brief Define the 1/2 period of the pulse used to flash the LED in the launch
/// button.
///
/// @def _zigbee_timeout_
/// @brief Define the timeout for the zigbee communications.  Error occurs if
/// the time since the last communicaitons exceeds the timeout.
///////////////////////////////////////////////////////////////////////////////////
#define _pulse_halfwidth_       100
#define _zigbee_timeout_        1000

///////////////////////////////////////////////////////////////////////////////////
/// @brief Track the on/off state of the launch button LED when the LED is
/// flashing.
///////////////////////////////////////////////////////////////////////////////////
boolean pulse = false;                // Is the launch button LED on or off.

///////////////////////////////////////////////////////////////////////////////////
/// @brief Time since the last toggle of the LED on/off status.  Used to control
/// the flashing of the LED.
///////////////////////////////////////////////////////////////////////////////////
long timeSinceLastPulse = 0;

///////////////////////////////////////////////////////////////////////////////////
/// @brief Time since the last good Zigbee communication with the master panel.
/// Used to monitor a timeout error.
///////////////////////////////////////////////////////////////////////////////////
long timeSinceLastSignal = 0;

///////////////////////////////////////////////////////////////////////////////////
/// @brief Instance of the Zigbee Class.  Only one instance created.  It is used
/// to control/perform handshakes and other communications with the other Zigbee
/// devices in the Zigbee wireless network.
///////////////////////////////////////////////////////////////////////////////////
XBee xbee = XBee();

///////////////////////////////////////////////////////////////////////////////////
/// @brief Instance of the LaunchDataPacket Class.  
/// Create an instance of the LaunchDataPacket Class.  Only one instance created.
/// This class decyphers all data being passed between the Master Panel, the
/// Remote Panel, and the Launch Pad.  The single instance of the Zigbee class
/// is passed to the LaunchDataPacket so the Master Panel can begin
/// communicate with the Remote Panel and the Launch Pad.
///////////////////////////////////////////////////////////////////////////////////
LaunchDataPacket data = LaunchDataPacket(xbee);

///////////////////////////////////////////////////////////////////////////////////
/// Returns the On/Off state of the arm switch.
///
/// @return boolean - TRUE - Arm switch is off, FALSE  - Arm switch is on.
///////////////////////////////////////////////////////////////////////////////////
boolean isArmSwitchOff(){
  return bitRead(_ArmSwitchPort_, _ArmSwitchBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Returns the On/Off state of the launch button.
///
/// @return boolean - TRUE - Launch button is off, FALSE - Launch button is on.
///////////////////////////////////////////////////////////////////////////////////
boolean isLaunchButtonOff(){
  return bitRead(_LaunchButtonPort_, _LaunchButtonBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Supplies power to the Arm Switch and therefore supplies power to both the 
/// switch and the LED.  The LED dues not light up unless it has both power and 
/// the switch is in the on position.
///
/// @param state - (int) 0 = turn off. all other values = turn on.
///////////////////////////////////////////////////////////////////////////////////
void setArmSwitchLedState(boolean state){
  if (state) {
    bitSet(_ArmLedPort_, _ArmLedBit_);
    return;
  }
  bitClear(_ArmLedPort_, _ArmLedBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Turns the Launch button LED on or off.
/// 
/// @param state - (int) 0 = turn off. all other values = turn on.
///////////////////////////////////////////////////////////////////////////////////
void setLaunchButtonLedState(boolean state){
  if (state) {
    bitSet(_LaunchLedPort_, _LaunchLedBit_);
    return;
  }
  bitClear(_LaunchLedPort_, _LaunchLedBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Checks if there is data waiting in the Zigbee.  If data is present, it reads 
/// the data and stores it in class variables and updates the LCD display acording 
/// to the data content.
///////////////////////////////////////////////////////////////////////////////////
void readZigbee() {
  data.readDataFromXbee();
}

///////////////////////////////////////////////////////////////////////////////////
/// Checks the current state of the remote controller swiches, updates the data
/// appropriate for the Remote Controller and sends a data packet to the Master
/// Controller via Zigbee.  All variables are stored in the class instance.
///////////////////////////////////////////////////////////////////////////////////
void writeZigbeeMaster() {
  data.setPressure1(0);
  data.setPressure2(0);
  data.sendDataToMaster();
}

///////////////////////////////////////////////////////////////////////////////////
/// Toggles the on/off state of the error LED to alert the user of an error.
///////////////////////////////////////////////////////////////////////////////////
void reportError() {
  if (pulse)
    bitSet(_ErrorLedPort_, _ErrorLedBit_);
  else
    bitClear(_ErrorLedPort_, _ErrorLedBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Toggles the on/off state of the Launch LED bit.
///////////////////////////////////////////////////////////////////////////////////
void blinkLaunchLed() {
  if (pulse)
    setLaunchButtonLedState(true);
  else
    setLaunchButtonLedState(false);
}

///////////////////////////////////////////////////////////////////////////////////
/// Sets the error LED to on to alert the user of an error.
///////////////////////////////////////////////////////////////////////////////////
void reportNoError() {
  bitClear(_ErrorLedPort_, _ErrorLedBit_);
}

///////////////////////////////////////////////////////////////////////////////////
/// Reads all switch states from the Remote Panel, updates the data
/// (LaunchDataPacket instance), sends the data over Zigbee, and checks for any 
/// data waiting on the Zigbee and reads the data into the LaunchDataPacket
/// instance.
///////////////////////////////////////////////////////////////////////////////////
void readPanel() {
  // In order to blink an LED, we need to track when to turn the LED on and off.
  // pulseTime is the time since we last changed the state of the LEDs from on
  // to off or off to on.  pulse is the current state of the LEDs (on or off).
  long pulseTime = millis() - timeSinceLastPulse;
  if (pulseTime > _pulse_halfwidth_) {
    timeSinceLastPulse = millis();
    pulse = pulse ^ true;
    if (pulse) {
      // on the rising edge of pulse, send a data packet over Zigbee.
      writeZigbeeMaster();
    }
  }
  // Check for data waiting on the Zigbee
  readZigbee();

  // Check for a Zigbee communication timeout.
  long timeOut = millis() - timeSinceLastSignal;
  // Error if time since last Zigbee receive is more than 1 second.
  if (timeOut > _zigbee_timeout_) { // Error
    reportError();
  } else {  // timeout has not occured (yet).
    reportNoError();                // do not report error via LED
    // Check if the last communication from the Master Panel indicates that
    // the Remote Panel is activated.
    if (data.isRemoteActivated()) {
      setArmSwitchLedState(true);       // Light up the Arm Switch LED to indicate
                                        // the Remote Panel is activated.
      if (!isArmSwitchOff()) {            // If the remote panel arm switch is on
        data.remoteArmIsOn();           // record the arm switch as on.
        blinkLaunchLed();               // blink the Launch Button LED
        if (!isLaunchButtonOff()) {       // If the Launch Button is pressed
          data.remoteLaunchSet();       // Launch
        } else {                        // The Launch Button is not pressed
          data.remoteLaunchClear();     // do not Launch
        }
      } else {                          // The remote panel arm switch is not on
        data.remoteArmIsOff();          // record the arm switch as off.
        data.remoteLaunchClear();       // do not launch.
        setLaunchButtonLedState(false); // Turn off the Launch Button LED.
      }
    }
  }
}
  
///////////////////////////////////////////////////////////////////////////////////
/// Arduino 'setup' function.  The 'setup' function is excecuted only once each
/// time the Atmel microcontroller is powered up and it executes after the arduino
/// boot loader finishes executing. \n
/// It will initialize the digital I/O pins, the LCD screen, the serial port, and
/// the Zigbee.
///////////////////////////////////////////////////////////////////////////////////
void setup() {
  // Setup the input and output pins
  pinMode(_ArmSwitchBitMode_, INPUT);
  pinMode(_LaunchButtonBitMode_, INPUT);
  pinMode(_ArmLedBitMode_, OUTPUT);
  pinMode(_LaunchLedBitMode_, OUTPUT);
  pinMode(_ErrorLedBitMode_, OUTPUT);

  // Turn on the pull-up resistor in the _ArmSwitchBit_ and _LaunchButtonBit_
  // Input Pins.
  bitSet(_ArmSwitchPort_, _ArmSwitchBit_);
  bitSet(_LaunchButtonPort_, _LaunchButtonBit_);

  // Turn on the Error LED to indicate the Remote Panel is funtional
  bitSet(_ErrorLedPort_, _ErrorLedBit_);

  // Initialize the pulse variable which keeps track if a flashing LED is on/off.
  pulse = false;
  
  // Initialize the serial port.
  Serial.begin(9600);

  // Initialize the Zigbee and define the Zigbee communication port as Serial
  xbee.setSerial(Serial);
}

///////////////////////////////////////////////////////////////////////////////////
/// Arduino 'loop' function.  The 'loop' function is excecuted over and over after
/// the 'setup' function finishes executing. \n
/// It will call the readPanel function in an infinite loop.
///////////////////////////////////////////////////////////////////////////////////
void loop() {
  readPanel();
}

