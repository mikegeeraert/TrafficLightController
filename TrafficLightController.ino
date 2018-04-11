#include <TimerOne.h>

enum LightPins { Green = 5, Amber = 6, Red = 7 };
LightPins current_light = LightPins::Green;

enum LightDelay { GreenDelay = 5, AmberDelay = 1, RedDelay = 5 };
LightDelay current_delay = LightDelay::GreenDelay;
int delay_scalar = 1; //Multiplier to increase or decrease the light delay

enum Mode { Cycle, Party, GreenOnly, AmberOnly, RedOnly, All };
Mode current_mode = Mode::Cycle;

const int green_light = 0;
const int amber_light = 1;
const int red_light = 2;

const byte toggle_speed_button = 1;
const byte toggle_mode_button = 2;

volatile bool speed_changed = false;
volatile bool mode_changed = false;

void setup() {
  //  Setup light pins
  pinMode(LightPins::Green, OUTPUT);
  pinMode(LightPins::Amber, OUTPUT);
  pinMode(LightPins::Red, OUTPUT);
  digitalWrite(LightPins::Green, HIGH);
  digitalWrite(LightPins::Amber, HIGH);
  digitalWrite(LightPins::Red, HIGH);
  //  Setup speed toggle button
  pinMode(toggle_speed_button, INPUT);
  attachInterrupt(0, toggleCycleSpeed, FALLING);
  // Setup mode toggle button
  pinMode(toggle_mode_button, INPUT_PULLUP);
  attachInterrupt(1, toggleMode, FALLING);

  Timer1.initialize(1000000); //Initialize timer to 1 second period
  startCycleLights();

  Serial.begin(9600);

}

void loop() {
  if(speed_changed) {
    speed_changed = false;
    Serial.print("Toggling Cycle Speed\n");
    delay_scalar = (delay_scalar % 3) + 1;
  }
  if(mode_changed) {
    mode_changed = false;
    switch (current_mode) {
      case Cycle:
        stopPartyLights();
        startCycleLights();
        Serial.print("Cycle Mode\n");
        break;
      case Party:
        stopCycleLights();
        startPartyLights();
        Serial.print("Party Mode\n");
        break;
    }
  }
}

void toggleMode() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500) { // Debounce the falling edge interrupt to half a second, since buttons don't quite give a clean signal
    switch (current_mode) {
      case Cycle:
        current_mode = Mode::Party;
        break;
      case Party:
        current_mode = Mode::Cycle;
        break;
    }
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

int party_tick = 0;

void PartyISR() { //ISR for Party Mode
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
}

void stopCycleLights() {
  Timer1.detachInterrupt();
}

int tick = 0;

void CycleISR() { //timer1 ISR. Interrupt cycle of 1hz (1 second) - ISR for Cycle Mode
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

