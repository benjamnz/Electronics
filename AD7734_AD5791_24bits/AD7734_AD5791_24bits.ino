// Ardunio *DUE*code for controlling EVAL-AD7734 ADC and EVAL-AD5791 DAC
// Andrea Young
// Carlos Kometter
// 3/23/2018
#include "SPI.h" // necessary library for SPI communication
#include <vector>
int adc=10; // The SPI pin for the ADC
int adc_sync=32;
const int Ndacs = 2;
int dac[Ndacs]={11,8};  // The SPI pin for the DAC
int spi=52;
int ldac=50; // Load DAC pin for DAC. Make it LOW if not in use. 
int reset=24; // Reset on ADC
int drdy=28; // Data is ready pin on ADC
int led=46;
int data=38; // Used for trouble shooting; connect an LED between pin 28 and GND
int err=30;
int eeprom=51;
const int Noperations = 22;
String operations[Noperations] = {"NOP", "INITIALIZE", "SET", "GET_DAC", "GET_ADC", "RAMP1", "RAMP2", "BUFFER_RAMP", "BUFFER_RAMP_DEC", "RESET", "TALK", "CONVERT_TIME", "*IDN?", "*RDY?", "GET_DUNIT","SET_DUNIT", "ADC_ZERO_SC_CAL", "ADC_CH_ZERO_SC_CAL", "ADC_CH_FULL_SC_CAL", "DAC_CH_CAL", "FULL_SCALE", "TEST"};
int initialized = 0;
int delayUnit=0; // 0=microseconds 1=miliseconds

SPISettings adcset(115200, MSBFIRST, SPI_MODE3);
SPISettings dacset(115200, MSBFIRST, SPI_MODE1);

float DAC_FULL_SCALE = 10.0;

// Calibration constants
float OS[4]={0,0,0,0}; // offset error
float GE[4]={1,1,1,1}; // gain error

// Looks for comma-separated values in input
std::vector<String> query_serial()
{
  char received;
  String inByte = "";
  std::vector<String> comm;
  while (received != '\r')
  {
    if(Serial.available())
    {
      received = Serial.read();
      if (received == '\n' || received == ' ')
      {}
      else if (received == ',' || received == '\r')
      {
        comm.push_back(inByte);
        inByte = "";
      }
      else
      {
        inByte += received;
      }        
    }
  }
  return comm;
}

// To avoid errors when using vectors
namespace std {
  void __throw_bad_alloc()
  {
    Serial.println("Unable to allocate memory");
  }

  void __throw_length_error( char const*e )
  {
    Serial.print("Length Error :");
    Serial.println(e);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(ldac,OUTPUT);   
  digitalWrite(ldac,HIGH); // Load DAC pin for DAC. Make it LOW if not in use. 
  pinMode(reset, OUTPUT);
  pinMode(drdy, INPUT); // Data ready pin for the ADC.  
  pinMode(led, OUTPUT); // Used for blinking indicator LED
  digitalWrite(led, HIGH);
  pinMode(data, OUTPUT);
  pinMode(eeprom, OUTPUT);
  digitalWrite(eeprom, HIGH);
  pinMode(adc_sync, OUTPUT);
  digitalWrite(adc_sync, HIGH);
  
  for(int i =0; i <= Ndacs-1; i++)
  {
    pinMode(dac[i],OUTPUT);
    digitalWrite(dac[i],HIGH);
  }

  
  //Resets ADC on startup.  	  
  digitalWrite(reset,HIGH);  digitalWrite(data,LOW); digitalWrite(reset,LOW);  digitalWrite(data,HIGH); 
  delay(5);
  digitalWrite(reset,HIGH);  digitalWrite(data,LOW); 
  
  SPI.begin(adc); // wake up the SPI bus for ADC
  SPI.begin(spi); // wake up the SPI bus for ADC
  
  SPI.setBitOrder(adc,MSBFIRST); // required for AD7734.
  SPI.setBitOrder(spi,MSBFIRST); // required for AD5764.
  SPI.setClockDivider(adc,84);  
  SPI.setClockDivider(spi,84);  
  SPI.setDataMode(adc,SPI_MODE3); // MODE3 for AD7734
  SPI.setDataMode(spi,SPI_MODE1); // MODE1 for AD5764

  // Disables DAC_SDO to avoid interference with ADC
//  SPI.transfer(dac,1,SPI_CONTINUE);
//  SPI.transfer(dac,0,SPI_CONTINUE);
//  SPI.transfer(dac,1);
}

void blinker(int s){digitalWrite(data,HIGH);delay(s);digitalWrite(data,LOW);delay(s);}
void sos(){blinker(50);blinker(50);blinker(50);blinker(500);blinker(500);blinker(500);blinker(50);blinker(50);blinker(50);}

void error()
{
  digitalWrite(err,HIGH);
  delay(3000);
  digitalWrite(err,LOW);
  delay(500);
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

int maxValueVectorInt(std::vector<int> DB, int size)
{
  int max_value = 0;
  for (int i = 0; i < size; i++)
  {
    if(DB[i] > max_value)
    {
      max_value=DB[i];
    }
  }
  return max_value;
}

void waitDRDY() {while (digitalRead(drdy)==HIGH){}}

void resetADC() // Resets the ADC, and sets the range to default +-10 V 
{
  SPI.transfer(adc,0);
  digitalWrite(data,HIGH);digitalWrite(reset,HIGH);digitalWrite(reset,LOW);digitalWrite(reset,HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x28);
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x2A);
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, HIGH);
}

void talkADC(std::vector<String> DB) // Init communication with ADC
{
  int comm;
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, LOW);
  comm=SPI.transfer(adc,DB[1].toInt());
  digitalWrite(adc_sync, HIGH);
  Serial.println(comm);
  Serial.flush();
}

void writeADCConversionTime(std::vector<String> DB)
{
  int adcChannel=DB[1].toInt();
  byte cr;

  byte fw = ((byte)(((DB[2].toFloat()*6.144-249)/128)+0.5))|128;
  
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x30+adcChannel);
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,fw);
  digitalWrite(adc_sync, HIGH);
  delayMicroseconds(100);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x70+adcChannel);
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  cr=SPI.transfer(adc,0); // Read back the CT register
  digitalWrite(adc_sync, HIGH);

  int convtime = ((int)(((cr&127)*128+249)/6.144)+0.5);
  Serial.println(convtime);
}

float map2(float x, long in_min, long in_max, float out_min, float out_max) //float
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int threeByteToIntAdc(byte DB1,byte DB2, byte DB3) // Returns a 24-bit integer (between 0 - 2^24)
{
  return ((int)(((DB1<<8)| DB2)<<8)|DB3);
}

int twoByteToInt(byte DB1,byte DB2) // Returns a 16-bit integer (between +/- 2^16)
{
  return ((int)((DB1<<8)| DB2));
}

void intToThreeByteAdc(int s, byte * DB1, byte * DB2, byte * DB3)
{
    *DB1 = ((byte)((s>>16)&0xFF));
    *DB2 = ((byte)((s>>8)&0xFF));
    *DB3 = ((byte)(s&0xFF));
}

void intToTwoByte(int s, byte * DB1, byte * DB2)
{
    *DB1 = ((byte)((s>>8)&0xFF));
    *DB2 = ((byte)(s&0xFF)); 
}


float twoByteToVoltage(byte DB1, byte DB2)
{
  int decimal;
  float voltage;

  decimal = twoByteToInt(DB1,DB2);

  if (decimal <= 32767)
  {
    voltage = decimal*10.0/32767;
  }
  else
  {
    voltage = -(65536-decimal)*10.0/32768;
  }
  return voltage;
}


void voltageToTwoByte(float voltage, byte * DB1, byte * DB2)
{
  int decimal;
  if (voltage > 10 || voltage < -10)
  {
    *DB1 = 128;
    *DB2 = 0;
    error();
  }  
  else if (voltage >= 0)
  {
    decimal = voltage*32767/10;
  }
  else
  {
    decimal = voltage*32768/10 + 65536;
  }
  intToTwoByte(decimal, DB1, DB2);
}

float getSingleReading(int adcchan)
{
  Serial.flush();
  int statusbyte=0;
  byte o1;
  byte o2;
  byte o3;
  int ovr;
  if(adcchan <= 3)
  {
    SPI.transfer(adc,0);
    digitalWrite(adc_sync, LOW);
    SPI.transfer(adc,0x38+adcchan);  // Indicates comm register to access mode register with channel
    digitalWrite(adc_sync, HIGH);
    digitalWrite(adc_sync, LOW);
    SPI.transfer(adc,0x4A);          // Indicates mode register to start single convertion in dump mode
    digitalWrite(adc_sync, HIGH);
    waitDRDY();                      // Waits until convertion finishes
    digitalWrite(adc_sync, LOW);
    SPI.transfer(adc,0x48+adcchan);  // Indicates comm register to read data channel data register
    digitalWrite(adc_sync, HIGH);
    digitalWrite(adc_sync, LOW);
    statusbyte=SPI.transfer(adc,0);  // Reads Channel 'ch' status
    digitalWrite(adc_sync, HIGH);
    digitalWrite(adc_sync, LOW);
    o1=SPI.transfer(adc,0);
    digitalWrite(adc_sync, HIGH);
    digitalWrite(adc_sync, LOW);
    o2=SPI.transfer(adc,0);          // Reads first byte
    digitalWrite(adc_sync, HIGH);
    digitalWrite(adc_sync, LOW);
    o3=SPI.transfer(adc,0);          // Reads second byte
    digitalWrite(adc_sync, HIGH);
    ovr=statusbyte&1;
    switch (ovr)
    {
      case 0:
      int decimal;
      decimal = threeByteToIntAdc(o1,o2,o3);
      float voltage;
      voltage = map2(decimal, 0, 16777216, -10.0, 10.0);
      return voltage;
      break;
      
      case 1:
      return 0.0;
      break;   
    }
  }
}

float readADC(byte DB)
{
  int adcChannel=DB;
  switch (adcChannel)
  {
    case 0:
    return getSingleReading(0);
    break;
    case 1:
    return getSingleReading(1);
    break;
    case 2:
    return getSingleReading(2);
    break;
    case 3:
    return getSingleReading(3);
    break;

    default:  
    break;
  }
}

int threeByteToInt(byte DB1,byte DB2, byte DB3) // Reeturns a 16 bit integer (between +/- 2^16)
{
  return ((int)(((((DB1&15)<<8)| DB2)<<8)|DB3));
}

void intToThreeBytes(int decimal, byte *DB1, byte *DB2, byte *DB3)
{
  *DB1 = (byte)((decimal >> 16) | 16);
  *DB2 = (byte)((decimal >> 8) & 255);
  *DB3 = (byte)(decimal & 255);
}

float threeByteToVoltage(byte DB1, byte DB2, byte DB3)
{
  int decimal;
  float voltage;

  decimal = threeByteToInt(DB1,DB2,DB3);

  if (decimal <= 524287)
  {
    voltage = decimal*DAC_FULL_SCALE/524287;
  }
  else
  {
    voltage = -(1048576-decimal)*DAC_FULL_SCALE/524288;
  }
  return voltage;
}

int voltageToDecimal(float voltage, byte *DB1, byte *DB2, byte *DB3)
{
  int decimal;
  if (voltage >= 0)
  {
    decimal = voltage*524287/DAC_FULL_SCALE;
  }
  else
  {
    decimal = voltage*524288/DAC_FULL_SCALE + 1048576;
  }
  intToThreeBytes(decimal, DB1, DB2, DB3);
}

float dacDataSend(int channelDAC, float voltage)
{
  byte b1;
  byte b2;
  byte b3;

  voltageToDecimal(voltage, &b1, &b2, &b3);
  SPI.transfer(spi,0);
  
  digitalWrite(data, HIGH);
  digitalWrite(channelDAC,LOW);
  SPI.transfer(spi,b1); // send command byte to DAC
  SPI.transfer(spi,b2); // MS data bits, DAC2
  SPI.transfer(spi,b3); //LS 8 data bits, DAC2
  digitalWrite(channelDAC,HIGH);

  digitalWrite(ldac,LOW);
  
  digitalWrite(ldac,HIGH);
  digitalWrite(data, LOW);

  return threeByteToVoltage(b1,b2,b3);
}

float writeDAC(int dacChannel, float voltage)
{
  switch(dacChannel)
  {
    case 0:
    return dacDataSend(dac[0],voltage/GE[0]-OS[0]);
    break;

    case 1:
    return dacDataSend(dac[1],voltage/GE[1]-OS[1]);
    break;

    case 2:
    return dacDataSend(dac[2],voltage/GE[2]-OS[2]);
    break;

    case 3:
    return dacDataSend(dac[3],voltage/GE[3]-OS[3]);
    break;

    default:
    break;
  }
}

void readingRampAvg(int adcchan, byte b1, byte b2, byte b3, byte * o1, byte * o2, byte * o3,int count,int nReadings)
{
  Serial.flush();
  int statusbyte=0;
  int ovr;
  byte db1;
  byte db2;
  byte db3;
  float sum=0;
  float avg;
  bool toSend = true;
  if(adcchan <= 3)
  {
    for(int i = 1; i<=nReadings; i++)
    {
      SPI.transfer(adc,0);
      digitalWrite(adc_sync, LOW);
      SPI.transfer(adc,0x38+adcchan);   // Indicates comm register to access mode register with channel
      digitalWrite(adc_sync, HIGH);
      digitalWrite(adc_sync, LOW);
      SPI.transfer(adc,0x4A);           // Indicates mode register to start single convertion in dump mode
      digitalWrite(adc_sync, HIGH);
      if (count>0 && toSend)
      {
        Serial.write(b1);                 // Sends previous reading while it is waiting for new reading
        Serial.write(b2);
        Serial.write(b3);
        toSend = false;
      }
      waitDRDY();                       // Waits until convertion finishes
      digitalWrite(adc_sync, LOW);
      SPI.transfer(adc,0x48+adcchan);   // Indcates comm register to read data channel data register
      digitalWrite(adc_sync, HIGH);
      digitalWrite(adc_sync, LOW);
      statusbyte=SPI.transfer(adc,0);   // Reads Channel 'ch' status
      digitalWrite(adc_sync, HIGH);
      digitalWrite(adc_sync, LOW);
      db1=SPI.transfer(adc,0);          // Reads first byte
      digitalWrite(adc_sync, HIGH);
      digitalWrite(adc_sync, LOW);
      db2=SPI.transfer(adc,0);          // Reads second byte
      digitalWrite(adc_sync, HIGH);
      digitalWrite(adc_sync, LOW);
      db3=SPI.transfer(adc,0);
      digitalWrite(adc_sync, HIGH);
      ovr=statusbyte&1;
      if (ovr){break;}
      int decimal = threeByteToIntAdc(db1,db2,db3);
      float voltage = map2(decimal, 0, 16777216, -10.0, 10.0);
      sum += voltage;
    }
    if(ovr)
    {
      *o1=128;
      *o2=0;
      *o3=0;
    }
    else
    {
      avg = sum/nReadings;
      int decimal = map2(avg, -10.0, 10.0, 0, 16777216);
      intToThreeByteAdc(decimal, &db1, &db2, &db3);
      *o1=db1;
      *o2=db2;
      *o3=db3;
    }
  }
}

void rampRead(byte DB,byte b1, byte b2, byte b3, byte * o1, byte * o2, byte * o3, int count,int nReadings )
{
  int adcChannel=DB;
  switch (adcChannel)
  {
    case 0:
    readingRampAvg(0, b1 , b2, b3, o1, o2, o3, count, nReadings);
    break;
    
    case 1:
    readingRampAvg(1, b1 , b2, b3, o1, o2, o3, count, nReadings);
    break;
    
    case 2:
    readingRampAvg(2, b1 , b2, b3, o1, o2, o3, count, nReadings);
    break;
    
    case 3:
    readingRampAvg(3, b1 , b2, b3, o1, o2, o3, count, nReadings);
    break;
  
    default:  
    break;
  }
}

float dacDataSend_buffer(int channelDAC, float voltage)
{
  byte b1;
  byte b2;
  byte b3;

  voltageToDecimal(voltage, &b1, &b2, &b3);
  SPI.transfer(spi,0);
  
  digitalWrite(data, HIGH);
  digitalWrite(channelDAC,LOW);
  SPI.transfer(spi,b1); // send command byte to DAC
  SPI.transfer(spi,b2); // MS data bits, DAC2
  SPI.transfer(spi,b3); //LS 8 data bits, DAC2
  digitalWrite(channelDAC,HIGH);
  digitalWrite(data, LOW);

  return threeByteToVoltage(b1,b2,b3);
}

float writeDAC_buffer(int dacChannel, float voltage)
{
  switch(dacChannel)
  {
    case 0:
    return dacDataSend_buffer(dac[0],voltage/GE[0]-OS[0]);
    break;

    case 1:
    return dacDataSend_buffer(dac[1],voltage/GE[1]-OS[1]);
    break;

    case 2:
    return dacDataSend_buffer(dac[2],voltage/GE[2]-OS[2]);
    break;

    case 3:
    return dacDataSend_buffer(dac[3],voltage/GE[3]-OS[3]);
    break;

    default:
    break;
  }
}

void bufferRamp(std::vector<String> DB)
{
  String channelsDAC = DB[1];
  int NchannelsDAC = channelsDAC.length();
  String channelsADC = DB[2];
  int NchannelsADC = channelsADC.length();
  std::vector<float> vi;
  std::vector<float> vf;
  float v_min = -1*DAC_FULL_SCALE;
  float v_max = DAC_FULL_SCALE;
  for(int i = 3; i < NchannelsDAC+3; i++)
  {
    vi.push_back(DB[i].toFloat());
    vf.push_back(DB[i+NchannelsDAC].toFloat());
  }
  int nSteps=(DB[NchannelsDAC*2+3].toInt());
  byte b1;
  byte b2;
  byte b3;
  int count =0;
  for (int j=0; j<nSteps;j++)
  {
    digitalWrite(data,HIGH);
    for(int i = 0; i < NchannelsDAC; i++)
    {
      float v;
      v = vi[i]+(vf[i]-vi[i])*j/(nSteps-1);
      if(v<v_min)
      {
        v=v_min;
      }
      else if(v>v_max)
      {
        v=v_max;
      }
      writeDAC_buffer(channelsDAC[i]-'0',v);
    }
    digitalWrite(ldac,LOW);
    digitalWrite(ldac,HIGH);
    if (delayUnit)
    {
      delay(DB[NchannelsDAC*2+4].toInt());
    }
    else
    {
      delayMicroseconds(DB[NchannelsDAC*2+4].toInt());
    }
    for(int i = 0; i < NchannelsADC; i++)
    {
      rampRead(channelsADC[i]-'0', b1, b2, b3, &b1, &b2, &b3, count,DB[NchannelsDAC*2+5].toInt());
      count+=1;
    }
    if(Serial.available())
    {
      std::vector<String> comm;
      comm = query_serial();
      if(comm[0] == "STOP")
      {
        break;
      }
    }
  }
  Serial.write(b1);
  Serial.write(b2);
  Serial.write(b3);
  digitalWrite(data,LOW);
}

int bufferRampDec(std::vector<String> DB)
{
  String channelsDAC = DB[1];
  int NchannelsDAC = channelsDAC.length();
  String channelsADC = DB[2];
  int NchannelsADC = channelsADC.length();
  int nSteps=(DB[NchannelsDAC*4+3].toInt());
  
  std::vector<float> vi;
  std::vector<float> vf;
  std::vector<int> min_nSteps;
  std::vector<int> min_delay;
  float v_min = -1*DAC_FULL_SCALE;
  float v_max = DAC_FULL_SCALE;
  for(int i = 3; i < NchannelsDAC+3; i++)
  {
    float vii = DB[i].toFloat();
    float vif = DB[i+NchannelsDAC].toFloat();
    float max_step_size = DB[i+2*NchannelsDAC].toFloat();
    float max_rate = DB[i+3*NchannelsDAC].toFloat();
    vi.push_back(vii);
    vf.push_back(vif);
    float min_nStepi=abs((int)((vif-vii)/(nSteps*max_step_size)));
    min_nSteps.push_back(min_nStepi);
    min_delay.push_back(abs((int)((vif-vii)/(nSteps*min_nStepi*max_rate*1000))));
  }
  int largest_min_nSteps = maxValueVectorInt(min_nSteps, NchannelsDAC);
  int largest_min_delay = maxValueVectorInt(min_delay, NchannelsDAC);
  
  byte b1;
  byte b2;
  byte b3;
  int count =0;
  for (int j=0; j<nSteps;j++)
  {
    digitalWrite(data,HIGH);
    for(int k = 0; k < largest_min_nSteps; k++)
    {
      for(int i = 0; i < NchannelsDAC; i++)
      {
        float v_k;
        float v_target;
        float v_previous;
        v_target = vi[i]+(vf[i]-vi[i])*j/(nSteps-1);
        v_previous = vi[i]+(vf[i]-vi[i])*(j-1)/(nSteps-1);

        if(j==0)
        {
          writeDAC_buffer(channelsDAC[i]-'0',vi[i]);
          continue;
        }

        if(largest_min_nSteps<2)
        {
          v_k = v_target;
        }
        else
        {
          v_k = v_previous + (v_target-v_previous)*k/(largest_min_nSteps-1);
        }
	
        if(v_k<v_min)
        {
          v_k=v_min;
        }
        else if(v_k>v_max)
        {
          v_k=v_max;
        }
        writeDAC_buffer(channelsDAC[i]-'0',v_k);
      }
      digitalWrite(ldac,LOW);
      digitalWrite(ldac,HIGH);
      if (largest_min_delay < 1)
      {
        delay(1);
      }
      else
      {
        delay(largest_min_delay);
      }
    }
    if (delayUnit)
    {
      delay(DB[NchannelsDAC*4+4].toInt());
    }
    else
    {
      delayMicroseconds(DB[NchannelsDAC*4+4].toInt());
    }
    for(int i = 0; i < NchannelsADC; i++)
    {
      rampRead(channelsADC[i]-'0', b1, b2, b3, &b1, &b2, &b3, count,DB[NchannelsDAC*4+5].toInt());
      count+=1;
    }
    if(Serial.available())
    {
      std::vector<String> comm;
      comm = query_serial();
      if(comm[0] == "STOP")
      {
        break;
      }
    }
  }
  Serial.write(b1);
  Serial.write(b2);
  Serial.write(b3);
  digitalWrite(data,LOW);
  return 0;
}


void autoRamp1(std::vector<String> DB)
{
  float v1 = DB[2].toFloat();
  float v2 = DB[3].toFloat();
  int nSteps = DB[4].toInt();
  int dacChannel=DB[1].toInt();

  for (int j=0; j<nSteps;j++)
  {
    int timer = micros();
    digitalWrite(data,HIGH);
    writeDAC(dacChannel, v1+(v2-v1)*j/(nSteps-1));
    digitalWrite(data,LOW);
    while(micros() <= timer + DB[5].toInt());
  }
}

void autoRamp2(std::vector<String> DB)
{
  float vi1 = DB[3].toFloat();
  float vi2 = DB[4].toFloat();
  float vf1 = DB[5].toFloat();
  float vf2 = DB[6].toFloat();
  int nSteps = DB[7].toInt();
  byte b1;
  byte b2;
  int dacChannel1=DB[1].toInt();
  int dacChannel2=DB[2].toInt();

  for (int j=0; j<nSteps;j++)
  {
    int timer = micros();
    digitalWrite(data,HIGH);
    writeDAC(dacChannel1, vi1+(vf1-vi1)*j/(nSteps-1));
    writeDAC(dacChannel2, vi2+(vf2-vi2)*j/(nSteps-1));
    while(micros() <= timer + DB[8].toInt());
    digitalWrite(data,LOW);
  }
}

void readDAC(int channelDAC)
{
  
  int o1;
  int o2;
  int o3;
  float voltage;
  SPI.transfer(spi,0);
  digitalWrite(channelDAC,LOW);
  SPI.transfer(spi,144); // send command byte to DAC
  SPI.transfer(spi,0); // MS data bits, DAC2
  SPI.transfer(spi,0);//LS 8 data bits, DAC2
  digitalWrite(channelDAC,HIGH);
  delayMicroseconds(1);
  digitalWrite(channelDAC,LOW);
  o1 = SPI.transfer(spi,0);
  o2 = SPI.transfer(spi,0);
  o3 = SPI.transfer(spi,0);
  digitalWrite(channelDAC,HIGH);
  voltage = threeByteToVoltage(o1, o2, o3);
  Serial.println(voltage,5);
}

void normalMode()
{
  SPI.transfer(spi,0); // The first spi transfer is nonsense sometimes
  delayMicroseconds(30);
  digitalWrite(data,HIGH);
  int attemps = 0;
  for( int i = 0; i <= Ndacs-1; i++)
  {
    int o;
    digitalWrite(dac[i],LOW);
    SPI.transfer(spi,160);
    SPI.transfer(spi,0);
    SPI.transfer(spi,0);
    digitalWrite(dac[i],HIGH);
    delayMicroseconds(1);
    digitalWrite(dac[i],LOW);
    SPI.transfer(spi,0);
    SPI.transfer(spi,0);
    o = SPI.transfer(spi,0);
    digitalWrite(dac[i],HIGH);

    if (o!=2)
    {
      for(int j=0; j<= Ndacs-1; j++)
      {
        dacDataSend(dac[i],0);
        digitalWrite(dac[i],LOW);
        SPI.transfer(spi,32);
        SPI.transfer(spi,0);
        SPI.transfer(spi,2);
        digitalWrite(dac[i],HIGH);
    
        digitalWrite(dac[i],LOW);
        SPI.transfer(spi,160);
        SPI.transfer(spi,0);
        SPI.transfer(spi,0);
        digitalWrite(dac[i],HIGH);
        delayMicroseconds(1);
        digitalWrite(dac[i],LOW);
        SPI.transfer(spi,0);
        SPI.transfer(spi,0);
        o = SPI.transfer(spi,0);
        digitalWrite(dac[i],HIGH);
      }
      
      if (attemps>=5)
      {
        Serial.print("ERROR INITIALIZING DAC");
        Serial.println(i);
      }
      else if (o!=2)
      {
        i=i-1;
        attemps++;
      }
    }
  }
  digitalWrite(data,LOW);
}

void ID()
{
  Serial.println("DAC-ADC_AD7734-AD5791");
}

void RDY()
{
  Serial.println("READY");
}

void setUnit(int unit)
{
  if (unit == 0)
  {
    delayUnit = 0;
    Serial.println("Delay unit set to microseconds");
  }
  else if (unit == 1)
  {
    delayUnit = 1;
    Serial.println("Delay unit set to miliseconds");
  }
  else
  {
    Serial.println("Unit should be 0 (microseconds) or 1 (miliseconds)");
  }
}

void adc_zero_scale_cal()
{
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x80);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();
}

void adc_ch_zero_scale_cal()
{
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xC0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+1);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xC0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+2);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xC0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+3);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xC0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();
}

void adc_ch_full_scale_cal()
{
  SPI.transfer(adc,0);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xE0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+1);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xE0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+2);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xE0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();

  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0x38+3);   // Indicates comm register to access mode register
  digitalWrite(adc_sync, HIGH);
  digitalWrite(adc_sync, LOW);
  SPI.transfer(adc,0xE0);
  digitalWrite(adc_sync, HIGH);
  waitDRDY();
}

void dac_ch_cal()
{
  for(int i=0; i<=Ndacs-1; i++)
  {
    OS[i]=0; // offset error
    GE[i]=1; // gain error
  }
  //set dacs to zero volts
  for(int i=0; i<=Ndacs-1; i++)
  {
    dacDataSend(dac[i],0);
  }
  delay(1);
  //reads the offset of each channel
  for(int i=0; i<=Ndacs-1; i++)
  {
    OS[i]=readADC(i);
  }
  
  //set dacs to 9 volts
  float ifs = 9;
  for(int i=0; i<=Ndacs-1; i++)
  {
    dacDataSend(dac[i],ifs);
  }
  delay(1);
  //reads each channel and calculates the gain error
  for(int i=0; i<=Ndacs-1; i++)
  {
    float nfs;
    nfs=readADC(i);
    GE[i]=(nfs-OS[i])/ifs;
  }
}

void debug()
{
  digitalWrite(data,HIGH);
  delay(3000);
  digitalWrite(data,LOW);
  delay(3000);
}

void test_debug()
{
  int max_value;
  std::vector<int> a;
  a.push_back(3);
  a.push_back(34);
  a.push_back(21);
  a.push_back(1);
  max_value = maxValueVectorInt(a, 4);
  Serial.println(max_value);
}

void router(std::vector<String> DB)
{
  float v;
  int operation = indexOfOperation(DB[0]);
  switch ( operation )
  {
    case 0:
    Serial.println("NOP");
    break;
    
    case 1:
//    debug();
    normalMode();
    Serial.println("INITIALIZATION COMPLETE");
    break;

    case 2:
    if(DB[2].toFloat() < -1*DAC_FULL_SCALE || DB[2].toFloat() > DAC_FULL_SCALE)
    {
      Serial.println("VOLTAGE_OVERRANGE");
      break;
    }
    v = writeDAC(DB[1].toInt(),DB[2].toFloat());
    Serial.print("DAC ");
    Serial.print(DB[1]);
    Serial.print(" UPDATED TO ");
    Serial.print(v,6);
    Serial.println("V");
    break;
    
    case 3:
    int channel;
    channel = dac[DB[1].toInt()];
    readDAC(channel);
    break;
    
    case 4: // Read ADC 
    v=readADC(DB[1].toInt());
    Serial.println(v,7);
    break;

    case 5:
    autoRamp1(DB);
    Serial.println("RAMP_FINISHED");
    break;

    case 6:
    autoRamp2(DB);
    Serial.println("RAMP_FINISHED");
    break;
    
    case 7: // Autoramp
    bufferRamp(DB);
    Serial.println("BUFFER_RAMP_FINISHED");
    break;

    case 8:
    bufferRampDec(DB);
    Serial.println("BUFFER_RAMP_FINISHED");
    break;

    case 9:
    resetADC();
    break;
    
    case 10:
    talkADC(DB);
    break;
    
    case 11: // Write conversion time registers
    writeADCConversionTime(DB);
    break;

    case 12: // ID
    ID();
    break;

    case 13:
    RDY();
    break;

    case 14:
    Serial.println(delayUnit);
    break;

    case 15:
    setUnit(DB[1].toInt()); // 0 = microseconds 1 = miliseconds
    break;

    case 16:
    adc_zero_scale_cal();
    Serial.println("CALIBRATION_FINISHED");
    break;

    case 17:
    adc_ch_zero_scale_cal();
    Serial.println("CALIBRATION_FINISHED");
    break;

    case 18:
    adc_ch_full_scale_cal();
    Serial.println("CALIBRATION_FINISHED");
    break;

    case 19:
    dac_ch_cal();
    Serial.println("CALIBRATION_FINISHED");

    case 20:
    DAC_FULL_SCALE = DB[1].toFloat();
    Serial.println("FULL_SCALE_UPDATED");
    break;

    case 21:
    test_debug();
    break;
    
    default:
    break;
  }
}

void loop() 
{
  Serial.flush();
  std::vector<String> comm;
  
  if(initialized == 0)
  {
    normalMode();
    initialized = 1;
  }
  
  if(Serial.available())
  {
    comm = query_serial();
    router(comm);
  }
}
