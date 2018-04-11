
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
  if (interrupt_time - last_interrupt_time > 500) { // Debounce the falling edge interrupt, since buttons don't quite give a clean signal
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

  cli();//stop interrupts

  TCCR0A = 0;// set entire TCCR2A register to 0
  TCCR0B = 0;// same for TCCR2B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR0A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS21 bit for 8 prescaler
  TCCR0B |= (1 << CS01);
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);

  sei();//allow interrupts
}

void stopPartyLights() {
  cli();//stop interrupts
  
  TCCR0A = 0;// set entire TCCR2A register to 0
  TCCR0B = 0;// same for TCCR2B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR0A = 0;// = (16*10^6) / (8000*8) - 1 (must be <256)
  TIMSK0 |= (0 << OCIE0A);
  sei();//allow interrupts
}

int party_tick = 0;
ISR(TIMER0_COMPA_vect) { //timer2 ISR. Interrupt cycle of 8hz (1/8 second)
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

  cli();//stop interrupts

  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  //set compare match register
  OCR1A = 15624; //// = (16*10^6hz clock speed) / (1hz*1024 prescaler) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  current_light = LightPins::Green;
  current_delay = LightDelay::GreenDelay;
  digitalWrite(LightPins::Amber, HIGH); //turn off amber and red lights
  digitalWrite(LightPins::Red, HIGH);
  digitalWrite(current_light, LOW); //turn on green light

  sei();//allow interrupts

}

void stopCycleLights() {
  cli();//stop interrupts
  TIMSK1 |= (0 << OCIE1A);
  sei();//allow interrupts
}

int tick = 0;

ISR(TIMER1_COMPA_vect) { //timer1 ISR. Interrupt cycle of 1hz (1 second)

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

