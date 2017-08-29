/*
  ACE-GPIO sample for use with Ace IO Shield.
  uses opto in, opto out, and GPIO's (configurable as both triggers and outputs).
  NOTE:
  NOT ALL CAMERAS HAVE GPIO.
  USB CAMERAS HAVE TWO GPIO'S.
  GIGE CAMERAS TYPICALLY HAVE NO GPIO. (SOME HAVE ONLY ONE GPIO).
  PLEASE READ THE USER MANUAL TO FIND OUT WHICH CAMERA YOU HAVE, AND ADJUST CODE ACCORDINGLY.
  PS: I admit, this code may be overly complex, but it is so that a user can configure via the menu below and not have to worry about anything else. :)
*/
enum CameraType {GIGE_CAMERA, USB_CAMERA, GIGE_CAMERA_GPIO};
enum CameraTriggerSource {Input_Line1, Input_Line3, Input_Line4};
enum CameraFrameTriggerWaitSource {Output_Line2, Output_Line3, Output_Line4};
enum CameraLineMode {Camera_Input, Camera_Output};

// ****************************************************************************
// CONFIGURATION:

CameraType MyCamera                                   = GIGE_CAMERA_GPIO;             // Type of Camera
CameraLineMode Line3_Mode                             = Camera_Input;                 // GPIO 1 mode on camera
CameraLineMode Line4_Mode                             = Camera_Input;                 // GPIO 2 mode on camera
CameraTriggerSource myTriggerSource                   = Input_Line1;                  // Trigger Source
bool useFrameTriggerWait                              = false;                        // Shall we use FrameTriggerWait?
CameraFrameTriggerWaitSource myFrameTriggerWaitSource = Output_Line2;                 // If yes, where is FrameTriggerWait?
long frameRate                                        = 30;                           // If no, trigger at this frame rate
long triggerWidth                                     = 2;                            // width of trigger pulse in microseconds (if using trigger-width exposure. remember to set ExposureOverlapTimeMax in camera.)

// ****************************************************************************

// I/O pins for camera
int Line1 = 8; // "Line 1" on the ace camera (opto isolated input)
int Line2Power = 7;
int Line3Power = 6;
int Line4Power = 5;
int Line2 = 2; // "Line 2" on the ace camera (opto isolated output)
int Line3 = 3; // "Line 3" on the ace camera (directly coupled first gpio)
int Line4 = 4; // "Line 4" on the ace camera (directly coupled second gpio)
// optional
int testLed1 = 9;
int testLed2 = 10;

// stuff for the interrupt routine
int onBoardled = 13; // arduino on-board led
long debounceTime = 1; // debounce time in microseconds
volatile unsigned long last_micros; // used in debouncer code
volatile int ledState = 0; // initial state of on-board led is off

// function for setting the GPIO mode on the Ardunio, given the mode set in the camera.
void SetGPIOMode(int GPIONumber, CameraLineMode modeInCamera)
{
  // GPIO is camera trigger. Start high, and Sink GPIO later to trigger camera (close circuit to ground)
  switch (modeInCamera)
  {
    case (Camera_Input):
      pinMode(GPIONumber, OUTPUT);
      digitalWrite(GPIONumber, HIGH);
      break;
    case (Camera_Output):
      pinMode(GPIONumber, INPUT_PULLUP); // this way we can collect information on the camera's output signal
      // OUTPUT,HIGH could still be used for output monitoring if the line is set as output in camera, but only the led would light. no info.
      break;
    default:
      break;
  }
}

// function for polling FrameTrigger Wait
bool PollFrameTriggerWait(CameraFrameTriggerWaitSource source)
{
  switch (source)
  {
    case (Output_Line2):
      return digitalRead(Line2);
      break;
    case (Output_Line3):
      return digitalRead(Line3);
      break;
    case (Output_Line4):
      return digitalRead(Line4);
      break;
    default:
      return false;
      break;
  }
}

// function for sending the trigger, given the source, active time, and inactive time
int SendTrigger(CameraTriggerSource source, int activeTime)
{
  switch (source)
  {
    case (Input_Line1):
      digitalWrite(Line1, HIGH);  // source optoIn to trigger camera
      if (activeTime > 16383) // use delay() if activeTime => 16383 microseconds (arduino limitation)
        delay(activeTime / 1000);
      else
        delayMicroseconds(activeTime);
      digitalWrite(Line1, LOW);
      return 0;
      break;
    case (Input_Line3):
      if (MyCamera == USB_CAMERA || MyCamera == GIGE_CAMERA_GPIO)
      {
        digitalWrite(Line3, LOW);  // sink GPIO1 to gnd to trigger camera
        if (activeTime > 16383) // use delay() if activeTime => 16383 microseconds (arduino limitation)
          delay(activeTime / 1000);
        else
          delayMicroseconds(activeTime);
        digitalWrite(Line3, HIGH);
        return 0;
      }
      else
        return 1;
      break;
    case (Input_Line4):
      if (MyCamera == USB_CAMERA)
      {
        digitalWrite(Line4, LOW);  // sink GPIO1 to gnd to trigger camera
        if (activeTime > 16383) // use delay() if activeTime => 16383 microseconds (arduino limitation)
          delay(activeTime / 1000);
        else
          delayMicroseconds(activeTime);
        digitalWrite(Line4, HIGH);
        return 0;
      }
      else
        return 1;
    default:
      return 1;
      break;
  }
}

// the setup function runs once when you press reset or power the board
void setup()
{
  // All cameras support opto trigger on Line1 and opto output on Line2
  pinMode(Line1, OUTPUT); // opto trigger
  digitalWrite(Line1, LOW); // start with a low on the opto trigger
  pinMode(Line2Power, OUTPUT); // powersource for output 1
  digitalWrite(Line2Power, HIGH); // turn on opto output power
  pinMode(Line2, INPUT_PULLUP); // measure opto output

  // configure GPIO's appropriatley
  if (MyCamera == USB_CAMERA) // USB camera supports two gpio
  {
    pinMode(Line3Power, OUTPUT); // powersource for gpio1 (if set to output)/triggersource(if set to input)
    digitalWrite(Line3Power, HIGH); // turn on gpio 1 power
    pinMode(Line4Power, OUTPUT); // powersource for gpio2 (if set to output)/triggersource(if set to input)
    digitalWrite(Line4Power, HIGH); // turn on gpio 2 power
    SetGPIOMode(Line3, Line3_Mode); // setup the first GPIO
    SetGPIOMode(Line4, Line4_Mode); // setup the second GPIO (USB ONLY)
  }

  if (MyCamera == GIGE_CAMERA)// there are no GPIO on a normal GIGE Camera
  {
    // do nothing.
  }

  if (MyCamera == GIGE_CAMERA_GPIO) //Some GigE cameras support one GPIO, but on a different pin than USB.
  {
    Line3 = 4; // put Line3 on a different pin so that "Line3" term is still valid in this code.
    pinMode(Line4Power, OUTPUT); // powersource for the GPIO
    digitalWrite(Line4Power, HIGH); // turn on the gpio power
    SetGPIOMode(Line3, Line3_Mode); // setup the GPIO
  }

  // example 1: setup some leds to use for monitoring camera output signals.
  pinMode(testLed1, OUTPUT); // for test LED1
  digitalWrite(testLed1, LOW); // start with led off
  pinMode(testLed2, OUTPUT); // for test LED2
  digitalWrite(testLed2, LOW); // start with led off

  // example 2: use an interrupt to monitor a camera output signal.
  // reminder: you can only attach an interrupt to arduino pins 2 or 3!
  pinMode(onBoardled, OUTPUT); // arduino on-board led
  digitalWrite(onBoardled, LOW); // start with onboard led off

  //example of using interrupts (ie: trigger when a signal change is detected)
  //attachInterrupt(digitalPinToInterrupt(Line2), InterruptRoutine, FALLING);
}

// the loop function triggers the camera continuously
void loop()
{
  // Example 2: trigger by polling frame trigger wait
  if (useFrameTriggerWait == true)
  {
    if (PollFrameTriggerWait(myFrameTriggerWaitSource) == false) // wait for frame trigger wait
      SendTrigger(myTriggerSource, triggerWidth);
  }
  if (useFrameTriggerWait == false)
  {
    SendTrigger(myTriggerSource, triggerWidth);
    delay(1000/frameRate);
  }

  // example of monitoring the gpio outputs
  // if (Line3_Mode == Camera_Output)
  //  digitalWrite(testLed1, !digitalRead(Line3));
  // if (Line4_Mode == Camera_Output && MyCamera == USB_CAMERA)
  //  digitalWrite(testLed2, !digitalRead(Line4));
}

// what to do when the loop is interrupted by something
// whatever you do, do it quickly. The loop is not running while this is...
void InterruptRoutine()
{
  //SendTrigger(myTriggerSource, triggerWidth);
}


