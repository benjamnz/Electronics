// Arduino micro for stepper motor using Sparksfun EasyDrive
// Carlos Kometter
// 8/8/2020

int dirPin=7;
int stepPin=6;
int ms1 = 3;
int ms2 = 4;
int stepsPerRev = 200;
int remoteSelectPin = A4;
int dirSelectPin1 = A5;
int dirSelectPin2 = A3;
const int Noperations = 3;
String operations[Noperations] = {"NOP", "STEP", "STEP_SIZE"};
String comm[20];

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
  pinMode(stepPin,OUTPUT);
  digitalWrite(stepPin, LOW);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);
  pinMode(ms1,OUTPUT);
  digitalWrite(ms1, LOW);
  pinMode(ms2,OUTPUT);
  digitalWrite(ms2, LOW);
  pinMode(remoteSelectPin, INPUT_PULLUP);
  pinMode(dirSelectPin1, INPUT_PULLUP);
  pinMode(dirSelectPin2, INPUT_PULLUP);
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


void step(int dir, int numSteps, int delayT){
  digitalWrite(dirPin, dir);
  for (int i = 0; i <= numSteps; i++){
    digitalWrite(stepPin, HIGH);
    delay(delayT);
    digitalWrite(stepPin, LOW);
    delay(delayT);
  }
}

void setStepSize(int index){
  switch (index){
    case 0:
    digitalWrite(ms1, HIGH);
    digitalWrite(ms2, HIGH);
    break;

    case 1:
    digitalWrite(ms1, LOW);
    digitalWrite(ms2, HIGH);
    break;

    case 2:
    digitalWrite(ms1, HIGH);
    digitalWrite(ms2, LOW);
    break;

    case 3:
    digitalWrite(ms1, LOW);
    digitalWrite(ms2, LOW);
    break;

    default:
    break;
  }
}

void router()
{
  int result;
  int operation = indexOfOperation(comm[0]);
  switch ( operation )
  {
    case 0:
    Serial.println("NOP");
    break;
    
    case 1:
    step(comm[1].toInt(), comm[2].toInt(), comm[3].toInt());
    Serial.println(0);
    break;

    case 2:
    setStepSize(comm[1].toInt());
    Serial.println(0);
    break;

    default:
    break;
  }
}

void loop() 
{
  while(digitalRead(remoteSelectPin)){  
    if(Serial.available())
    {
      query_serial();
      router();
      while(Serial.available()){
        Serial.read();  
      }
    }
  } 

  while(!digitalRead(remoteSelectPin)){
    setStepSize(0);
    if (!digitalRead(dirSelectPin1)){
      step(0, 10, 10);
    } else if (!digitalRead(dirSelectPin2)){
      step(1, 10, 10);
    }
  }
}
