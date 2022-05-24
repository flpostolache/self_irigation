#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS_1 A1
#define trigPin A2
#define echoPin A3
#define sensorPin A0
#define buzzer_pin 6
#define buttonPin_back 2
#define buttonPin_forward 3

// Variabile pentru relee
#define pinRel_1 A4
#define pinRel_2 A5
OneWire ourWire1(ONE_WIRE_BUS_1);
DallasTemperature sensor1(&ourWire1);

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);
// Variabila pentru masurare temperatura
double old_temperature;
double temperature;
// Variabile pentru masurare senzor ultrasonic
double old_distanta;
double distanta;
// Variabile pentru masurare senzor de uniditate a solului
int sensorValue;
// Variabile pentru buzzer
bool buzzer_is_on = false;
// Variabile pentru butoanele de afisare
int buttonState_forward = LOW;
long lastDebounceTime_forward = 0;
int buttonState_back = LOW;
long lastDebounceTime_back = 0;
long debounceDelay = 200;
// Variabile pentru afisare
short curr_to_print = 0;
// Variabila auxiliara pe care o folosesc pentru calcule
double aux;
bool este_gol_rezervorul = false;
byte Caracter_Grade[] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

void setup()
{
    lcd.begin(16, 2);
    randomSeed(5);
    lcd.createChar(0, Caracter_Grade);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(buzzer_pin, OUTPUT);
    pinMode(pinRel_1, OUTPUT);
    pinMode(pinRel_2, OUTPUT);
    sensor1.begin();
    sensor1.setResolution(11);
    pinMode(buttonPin_forward, INPUT_PULLUP);
    pinMode(buttonPin_back, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin_forward), check_button, RISING);
    attachInterrupt(digitalPinToInterrupt(buttonPin_back), check_button_2, RISING);
}

void loop()
{
    //Prelucrare senzor umiditate
    humidity_analysis();
    //Prelucrare tempreratura
    get_temp();
    //Calculez distanta cu senzorul ultrasonic pentru a vedea
    //nivelul de apa din rezervor
    get_distance();
    //Pentru valorile senzorului ultrasonic si senzorului de temperatura,
    //verific daca trebuie sa pornesc buzzer-ul
    trigger_buzzer();
    //Afisez datete citite pe ecranul LCD
    afiseaza_date();
}

void afiseaza_date(){
    lcd.clear();
    lcd.setCursor(0, 0);
    if(curr_to_print == 0){
      lcd.print("Temperatura: ");
      lcd.setCursor(0, 1);
      lcd.print(temperature);
      lcd.write((uint8_t) 0);
      lcd.print("C");
    }
    else if(curr_to_print == 1){
      lcd.print("Umiditate sol: ");
      lcd.setCursor(0, 1);
      if ( sensorValue >= 800 )
        lcd.print("Uscat");
      else if ( sensorValue >= 600 )
        lcd.print("Mediu");
      else if ( sensorValue >= 400 )
        lcd.print("Umed");
      else
        lcd.print("Foarte umed");
    }
    else if(curr_to_print == 2){
      lcd.print("Nivel apa: ");
      lcd.setCursor(0, 1);
      lcd.print(distanta);
    }
}

void get_distance(){
  //Calculez distanta  
  long durata;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  durata = pulseIn(echoPin, HIGH);
  aux = (durata / 2) / 29.1;
  if ( abs( aux - old_distanta ) >= 0.3 ) {
    old_distanta = distanta;
    distanta = aux;
  }
}

void get_temp(){
  //Citesc temperatura si o prelucrezvb
  sensor1.requestTemperatures();
  double RawValue = sensor1.getTempCByIndex(0);
  aux = (((RawValue - 11.0) * 24.5) / 16.87) + 2.5;
  if ( abs( old_temperature - aux ) >= 0.15 ) {
    old_temperature = temperature;
    temperature = aux;
  }
}

void trigger_buzzer(){
  //Verificam valorile
  if(!buzzer_is_on){
    if (temperature > 40.0 || temperature < 15.0){
      analogWrite(buzzer_pin, 50);
      buzzer_is_on = true;
    }
    else if (distanta > 7.0) {
      analogWrite(buzzer_pin, 100);
      buzzer_is_on = true;
      este_gol_rezervorul = true;
    }
  }else{
    if(temperature >= 20.0 && temperature <= 40.0){
      if(distanta > 7.0){
        analogWrite(buzzer_pin, 100);
        este_gol_rezervorul = true;
      }
      else{
        analogWrite(buzzer_pin, 0);
        buzzer_is_on = false;
        este_gol_rezervorul = false;
      }
    }
    else if (distanta <= 7.0){
      if (temperature > 40.0 || temperature < 15.0){
        analogWrite(buzzer_pin, 50);
        este_gol_rezervorul = false;
      }
      else {
        analogWrite(buzzer_pin, 0);
        buzzer_is_on = false;
        este_gol_rezervorul = false;
      }
    }
  }
}

void check_button(){
  buttonState_forward = digitalRead(buttonPin_forward);
  //filter out any noise by setting a time buffer
  if ( (millis() - lastDebounceTime_forward) > debounceDelay) {
    //if the button has been pressed, lets toggle the LED from "off to on" or "on to off"
    if (buttonState_forward == HIGH) {
      curr_to_print = (curr_to_print + 1) % 3;
      lastDebounceTime_forward = millis(); //set the current time
    }
  }//close if(time buffer)  
}


void check_button_2(){
  buttonState_back = digitalRead(buttonPin_back);
  //filter out any noise by setting a time buffer
  if ( (millis() - lastDebounceTime_back) > debounceDelay ) {
    //if the button has been pressed, lets toggle the LED from "off to on" or "on to off"
    if ( buttonState_back == HIGH ) {
      curr_to_print = curr_to_print - 1;
      if ( curr_to_print <= -1 )
        curr_to_print = 2;
      lastDebounceTime_back = millis(); //set the current time
    }
  }//close if(time buffer)  
}

void humidity_analysis(){
  //Citesc valoarea pentru senzorul de umiditate a pamantului
  //si o prelucrez
  sensorValue = analogRead(sensorPin);
  if (sensorValue >= 800 && !este_gol_rezervorul){
    digitalWrite(pinRel_1, LOW);
    digitalWrite(pinRel_2, HIGH);
  }
  else{
    digitalWrite(pinRel_1, HIGH);
    digitalWrite(pinRel_2, LOW);
  }
}
