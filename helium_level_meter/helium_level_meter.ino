// Arduino MKRZERO code for reading the helium level meter AMI
// The length of the level probe is prop the voltage output of the He level meter
//Carlos Kometter
//7/25/2020

int outletSwitchPin=6;
int sensorPin=A4;
float sensorValue=0;
const int Noperations = 7;
String operations[Noperations] = {"NOP", "GET_V", "GET_LENGTH", "GET_PERCENT", "TURN_ON", "TURN_OFF", "*IDN?"};
String comm[20];
int analogResolution=12;
int analogRangeHigh=pow(2, analogResolution)-1;
float maxInputVoltage = 3.3;

float offset = 0;
float scale = 1;

void query_serial()
{
  char received='0';
  int count = 0;
  String inByte = "";
  while (received != '\r')
  {
    if(Serial.available())
    {
      received = Serial.read();
      if (received == '\n' || received == ' ')
      {}
      else if (received == ',' || received == '\r')
      {
        comm[count]=inByte;
        inByte = "";
        count++;
      }
      else
      {
        inByte += received;
      }        
    }
  }
}

void setup()
{
  analogReadResolution(analogResolution);
  pinMode(sensorPin,INPUT);
  pinMode(outletSwitchPin, OUTPUT);
  digitalWrite(outletSwitchPin, LOW);
  Serial.begin(115200);
  while(Serial.available()){
    Serial.read();  
  }
}

int indexOfOperation(String operation)
{
  for(int index = 0; index < Noperations; index++)
  {
    if(operations[index] == operation)
    {
      return index;
    }
  }
  return 0;
}

float map2(float x, long in_min, long in_max, float out_min, float out_max) //float
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


float get_v(){
  sensorValue = analogRead(sensorPin);
  sensorValue = map2(sensorValue, 0, analogRangeHigh, 0, maxInputVoltage);
  return(sensorValue);
}

float get_length(){
  sensorValue = analogRead(sensorPin);
  sensorValue = map2(sensorValue, 0, analogRangeHigh, 0, 35);
  return(sensorValue);
}

float get_percent(){
  sensorValue = analogRead(sensorPin);
  sensorValue = map2(sensorValue, 0, analogRangeHigh, 0, 100);
  return(sensorValue);
}

void turn_outlet_on(){
  digitalWrite(outletSwitchPin, HIGH);
}

void turn_outlet_off(){
  digitalWrite(outletSwitchPin, LOW);
}

void router()
{
  int operation = indexOfOperation(comm[0]);
  switch ( operation )
  {
    case 0:
    Serial.println("NOP");
    break;
    
    case 1:
    sensorValue = get_v();
    Serial.println(sensorValue);
    break;

    case 2:
    sensorValue = get_length();
    Serial.println(sensorValue);
    break;

    case 3:
    sensorValue = get_percent();
    Serial.println(sensorValue);
    break;

    case 4:
    turn_outlet_on();
    break;

    case 5:
    turn_outlet_off();
    break;

    case 6:
    Serial.println("Arduino MKRZERO He Level Meter");
    break;

    default:
    break;
  }
}

void loop() 
{
  if(Serial.available())
  {
    query_serial();
    router();
    while(Serial.available()){
      Serial.read();  
    }
  }
}
