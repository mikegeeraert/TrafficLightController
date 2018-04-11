#include <TimerOne.h>

enum SpeedLEDPins { Low = 8, Med = 9, High = 10 };

enum LightPins { Green = 5, Amber = 6, Red = 7 };
LightPins current_light = LightPins::Green;

enum LightDelay { GreenDelay = 5, AmberDelay = 1, RedDelay = 5 };
LightDelay current_delay = LightDelay::GreenDelay;
int delay_scalar = 0; //Multiplier to increase or decrease the light delay

enum Mode { Cycle=0, Party=1, GreenOnly=2, AmberOnly=3, RedOnly=4, All=5 };
Mode current_mode = Mode::Cycle;

const byte toggle_speed_button = 3;
const byte toggle_mode_button = 2;

// Used by the ISRs to communicate to the main loop that a button has been pressed and setting should change
volatile bool speed_changed = false;
volatile bool mode_changed = false;

void setup() {
  //  Setup I/O pins
  setupLightPins();
  setupButtonPins();
  setupSpeedIndicatorPins();
  
  Timer1.initialize(1000000); //Initialize timer to 1 second period
  startCycleLights();

  Serial.begin(9600);

}

void setupLightPins() {
  pinMode(LightPins::Green, OUTPUT);
  pinMode(LightPins::Amber, OUTPUT);
  pinMode(LightPins::Red, OUTPUT);
  digitalWrite(LightPins::Green, HIGH);
  digitalWrite(LightPins::Amber, HIGH);
  digitalWrite(LightPins::Red, HIGH);
}

void setupButtonPins() {
  //  Setup speed toggle button
  pinMode(toggle_speed_button, INPUT);
  attachInterrupt(0, toggleCycleSpeed, FALLING); // 0 is the Interrupt corresponding to pin 
  // Setup mode toggle button
  pinMode(toggle_mode_button, INPUT_PULLUP);
  attachInterrupt(1, toggleMode, FALLING);
}

void setupSpeedIndicatorPins() {
  pinMode(SpeedLEDPins::Low, OUTPUT);
  pinMode(SpeedLEDPins::Med, OUTPUT);
  pinMode(SpeedLEDPins::High, OUTPUT);
  speed_changed = true; //Tell main loop to update the lights on its first cycle
}

void loop() {
  if(speed_changed) {
    speed_changed = false;
    Serial.print("Toggling Cycle Speed\n");
    delay_scalar = (delay_scalar % 3) + 1;
    updateIndicatorLights();
  }
  if(mode_changed) {
    mode_changed = false;
    switch (current_mode) {
      case Cycle:
        startCycleLights();
        Serial.print("Cycle Mode\n");
        break;
      case Party:
        stopCycleLights();
        startPartyLights();
        Serial.print("Party Mode\n");
        break;
      case GreenOnly:
        stopPartyLights();
        startGreenLight();
        Serial.print("Green Mode\n");
        break;
      case AmberOnly:
        startAmberLight();
        Serial.print("Amber Mode\n");
        break;
      case RedOnly:
        startRedLight();
        Serial.print("Red Mode\n");
        break;
      case All:
        startAllLights();
        Serial.print("All Mode\n");
        break;
    }
  }
}

void updateIndicatorLights() {
  switch (delay_scalar) {
    case 1:
      digitalWrite(SpeedLEDPins::Low, HIGH);
      digitalWrite(SpeedLEDPins::Med, LOW);
      digitalWrite(SpeedLEDPins::High, LOW);
      break;
    case 2:
      digitalWrite(SpeedLEDPins::Low, HIGH);
      digitalWrite(SpeedLEDPins::Med, HIGH);
      digitalWrite(SpeedLEDPins::High, LOW);
      break;
    case 3:
      digitalWrite(SpeedLEDPins::Low, HIGH);
      digitalWrite(SpeedLEDPins::Med, HIGH);
      digitalWrite(SpeedLEDPins::High, HIGH);
      break;
    default:
      digitalWrite(SpeedLEDPins::Low, LOW);
      digitalWrite(SpeedLEDPins::Med, LOW);
      digitalWrite(SpeedLEDPins::High, LOW);
      break;
  }
}

void toggleMode() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500) { // Debounce the falling edge interrupt to half a second, since buttons don't quite give a clean signal
    current_mode = (current_mode + 1) % 6;
    mode_changed = true;
  }
  last_interrupt_time = interrupt_time;
}

void startPartyLights() {
  digitalWrite(LightPins::Amber, HIGH); //turn off lights
  digitalWrite(LightPins::Red, HIGH);
  digitalWrite(LightPins::Green, HIGH);
  Timer1.attachInterrupt(PartyISR, 100000); //set interrupt with 1 second period
}

void stopPartyLights() {
  Timer1.detachInterrupt();
}

void PartyISR() { //ISR for Party Mode
  static int party_tick = 0;
  party_tick = ++party_tick % (current_delay); //Advance or reset the tick
  if (!party_tick) { //If tick has reached the current_delay, advance the light and set new delay
    digitalWrite(current_light, HIGH); //turn off old light
    chooseRandomLightAndDelay();
    digitalWrite(current_light, LOW); //turn on new light
  }
}

void chooseRandomLightAndDelay() {
  current_light = random(5, 8);
  current_delay = random(2, 5);
}

// Scalar can be from 1-3
void toggleCycleSpeed() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500) { // Debounce the falling edge interrupt, since buttons don't quite give a clean signal
    speed_changed = true;
  }
  last_interrupt_time = interrupt_time;
}

void startCycleLights() {
  Timer1.attachInterrupt(CycleISR, 1000000); //set interrupt with 1 second period
  current_light = LightPins::Green;
  current_delay = LightDelay::GreenDelay;
  digitalWrite(LightPins::Amber, HIGH); //turn off amber and red lights
  digitalWrite(LightPins::Red, HIGH);
  digitalWrite(current_light, LOW); //turn on green light
  attachInterrupt(0, toggleCycleSpeed, FALLING); // turn on speed toggle button 
}

void stopCycleLights() {
  Timer1.detachInterrupt(); // disable the CycleISR that is updating the lights
  detachInterrupt(0); //disable speed toggle button 
  
}

void CycleISR() { //timer1 ISR. Interrupt cycle of 1hz (1 second) - ISR for Cycle Mode
  static int tick = 0;
  tick = ++ tick % (current_delay * delay_scalar); //Advance or reset the tick
  if (!tick) { //If tick has reached the current_delay, advance the light and set new delay
    digitalWrite(current_light, HIGH); //turn off old light
    advanceLight();
    digitalWrite(current_light, LOW); //turn on new light
  }
}

void advanceLight() {
  switch (current_light) {
    case Green:
      current_light = LightPins::Amber;
      current_delay = LightDelay::AmberDelay;
      break;
    case Amber:
      current_light = LightPins::Red;
      current_delay = LightDelay::RedDelay;
      break;
    case Red:
      current_light = LightPins::Green;
      current_delay = LightDelay::GreenDelay;
      break;
  }
}

void startGreenLight() {
  digitalWrite(LightPins::Amber, HIGH);
  digitalWrite(LightPins::Red, HIGH);
  digitalWrite(LightPins::Green, LOW);
}

void startAmberLight() {
  digitalWrite(LightPins::Green, HIGH);
  digitalWrite(LightPins::Red, HIGH);
  digitalWrite(LightPins::Amber, LOW);
}

void startRedLight() {
  digitalWrite(LightPins::Green, HIGH);
  digitalWrite(LightPins::Amber, HIGH);
  digitalWrite(LightPins::Red, LOW);
}

void startAllLights() {
  digitalWrite(LightPins::Green, LOW);
  digitalWrite(LightPins::Amber, LOW);
  digitalWrite(LightPins::Red, LOW);
}

