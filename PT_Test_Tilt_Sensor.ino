/* Tilt sensor *************/
#define TILTSENSORPIN 14  // D5 on D1 Mini
#define TILTSENSORPINPOWER 12 // D6 on D1 Mini
bool currentDirection = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  
  pinMode(TILTSENSORPIN,INPUT_PULLUP); // Turn on pullup resistor so that it goes high when NC
  
  pinMode(TILTSENSORPINPOWER,OUTPUT); // Set nearby pin to low to get as a close by ground for
  digitalWrite(TILTSENSORPINPOWER,LOW); // tilt sensor

}

void loop() {
  currentDirection = digitalRead(TILTSENSORPIN); // Read digitial I/O pin

  Serial.print(currentDirection);Serial.print(" ");

  if(currentDirection) 
  {Serial.println("down");}
  else
  {Serial.println("up");}
  
  delay(1000);
}
