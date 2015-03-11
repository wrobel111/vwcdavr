#include "vwcdc.h"

#define CDC_PREFIX1 0x53
#define CDC_PREFIX2 0x2C

int cdc::getCommand(int cmd2)
{
  if (((cmd2>>24) & 0xFF) == CDC_PREFIX1 && ((cmd2>>16) & 0xFF) == CDC_PREFIX2)
    if (((cmd2>>8) & 0xFF) == (0xFF^((cmd2) & 0xFF)))
      return (cmd2>>8) & 0xFF;
  return 0;
}


cdc::cdc(int DataOut_pin){
  _DataOut_pin=DataOut_pin;
  //TODO: add independent code to work with all arduinos...
  cli();//stop interrupts
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0;
  TCCR1B |= (1 << CS11);  // Set CS11 bit for 8 => tick every 1us@8MHz, 0.5us@16MHz 
  sei();//allow interrupts

  pinMode(_DataOut_pin,INPUT); // signals from radio
  if (_DataOut_pin==2)
	attachInterrupt(0,read_Data_out,CHANGE);
  if (_DataOut_pin==3)
  	attachInterrupt(1,read_Data_out,CHANGE);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
  SPI.setClockDivider(SPI_CLOCK_DIV128); //62.5kHz@8Mhz
  //SPI.setClockDivider(SPI_CLOCK_DIV64);//125kHz@8Mhz 
};

void cdc::init(){
  send_package(0x74,0xBE,0xFE,0xFF,0xFF,0xFF,0x8F,0x7C); //idle
  delay(10);
  send_package(0x34,0xBE,0xFE,0xFF,0xFF,0xFF,0xFA,0x3C); //load disc
  delay(100);
  send_package(0x74,0xBE,0xFE,0xFF,0xFF,0xFF,0x8F,0x7C); //idle
  delay(10);
}

void cdc::send_package(int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7)
{
  SPI.transfer(c0);
  delayMicroseconds(874);
  SPI.transfer(c1);
  delayMicroseconds(874);
  SPI.transfer(c2);
  delayMicroseconds(874);
  SPI.transfer(c3);
  delayMicroseconds(874);
  SPI.transfer(c4);
  delayMicroseconds(874);
  SPI.transfer(c5);
  delayMicroseconds(874);
  SPI.transfer(c6);
  delayMicroseconds(874);
  SPI.transfer(c7);
}

static void cdc::read_Data_out() //remote signals
{
  if(digitalRead(_DataOut_pin))
  {
    if (capturingstart || capturingbytes)
    {
      captimelo = TCNT1;
    }
    else
    capturingstart = 1;
    TCNT1 = 0;

    //eval times
    //cca 9000us HIGH, 4500us LOW,; 18000tiks and 9000tics(0.5us tick@16MHz)
    if (captimehi > 16600 && captimelo > 8000)
    {
      capturingstart = 0;
      capturingbytes = 1;
      cmdbit=0;
      cmd=0;
      //Serial.println("startseq found");
    }//cca 1700us; 3400ticks (0.5us tick@16MHz)
    else if(capturingbytes && captimelo > 3300)
    {
      //Serial.println("bit 1");
      cmd = (cmd<<1) | 0x00000001;
      cmdbit++;
    }//cca 550us; 1100ticks (0.5us tick@16MHz)
    else if (capturingbytes && captimelo > 1000)
    {
      //Serial.println("bit 0");
      cmd = (cmd<<1);
      cmdbit++;
    }

    if(cmdbit == 32)
    {
      newcmd = 1;
      capturingbytes = 0;
    }
  }
  else
  {
      captimehi = TCNT1; 
      TCNT1 = 0;
  }
}
