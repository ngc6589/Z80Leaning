/*
     DDUAL-86 ROM HEXWRITER (Aki-80 Compatible board)
     Arduino MEGA2560
     Mashiro Kusunoki (JP3SRS)
     2021/08/16 Draft Version.
*/
#define BUSACK PH4  //  7
#define RESET  PH5  //  8
#define BUSREQ PH6  //  9
#define MREQ   PB4  // 10
#define WR     PB5  // 11
#define RD     PB6  // 12
#define LED    13

volatile boolean  processingHexWriteMode = false;
volatile uint16_t startAddress;
volatile uint16_t displayLength;
volatile uint16_t stopAddress;
char ihexBuf[520];
volatile int  ihexIdx;

void resetSignalOUTPUT()
{
  PORTH |= (1 << RESET);
  DDRH  |= (1 << RESET);
}

void resetSignalHIZ()
{
  DDRH  &= ~(1 << RESET);
  PORTH &= ~(1 << RESET);
}

void resetSignalHIGH()
{
  PORTH |= (1 << RESET);
}

void resetSignalLOW()
{
  PORTH &= ~(1 << RESET);
}

void memorySignalOUTPUT()
{
  PORTB |= ((1 << MREQ) | (1 << RD) | (1 << WR));
  DDRB  |= ((1 << MREQ) | (1 << RD) | (1 << WR));
}

void memorySignalHIZ()
{
  DDRB  &= ~((1 << MREQ) | (1 << RD) | (1 << WR));
  PORTB &= ~((1 << MREQ) | (1 << RD) | (1 << WR));
}

void mreqSignalHIGH()
{
  PORTB |= (1 << MREQ);
}

void mreqSignalLOW()
{
  PORTB &= ~(1 << MREQ);
}

void wrSignalHIGH()
{
  PORTB |= (1 << WR);
}

void wrSignalLOW()
{
  PORTB &= ~(1 << WR);
}

void rdSignalHIGH()
{
  PORTB |= (1 << RD);
}

void rdSignalLOW()
{
  PORTB &= ~(1 << RD);
}

void busreqSignalOUTPUT()
{
  PORTH |= (1 << BUSREQ);
  DDRH  |= (1 << BUSREQ);
}

void busreqSignalHIZ()
{
  DDRH  &= ~(1 << BUSREQ);
  PORTH &= ~(1 << BUSREQ);
}

void busreqSignalHIGH()
{
  PORTH |= (1 << BUSREQ);
}

void busreqSignalLOW()
{
  PORTH &= ~(1 << BUSREQ);
}

uint8_t charToByte(char c1, char c2)
{
  volatile uint8_t result;

  result = 0;
  if (c1 >= 'A' && c1 <= 'F' )
  {
    result += c1 - 'A' + 10;
  } else if ( c1 >= 'a' && c1 <= 'f' )
  {
    result += c1 - 'a' + 10;
  } else if ( c1 >= '0' && c1 <= '9' )
  {
    result += c1 - '0';
  }
  result = (result << 4);
  if (c2 >= 'A' && c2 <= 'F' )
  {
    result += c2 - 'A' + 10;
  } else if ( c2 >= 'a' && c2 <= 'f' )
  {
    result += c2 - 'a' + 10;
  } else if ( c2 >= '0' && c2 <= '9' )
  {
    result += c2 - '0';
  }
  return result;
}

uint16_t charToInt(char c1, char c2, char c3, char c4)
{
  volatile uint16_t result;

  result = 0;
  if (c1 >= 'A' && c1 <= 'F' )
  {
    result += c1 - 'A' + 10;
  } else if ( c1 >= 'a' && c1 <= 'f' )
  {
    result += c1 - 'a' + 10;
  } else if ( c1 >= '0' && c1 <= '9' )
  {
    result += c1 - '0';
  }
  result = (result << 4);
  if (c2 >= 'A' && c2 <= 'F' )
  {
    result += c2 - 'A' + 10;
  } else if ( c2 >= 'a' && c2 <= 'f' )
  {
    result += c2 - 'a' + 10;
  } else if ( c2 >= '0' && c2 <= '9' )
  {
    result += c2 - '0';
  }
  result = (result << 4);
  if (c3 >= 'A' && c3 <= 'F' )
  {
    result += c3 - 'A' + 10;
  } else if ( c3 >= 'a' && c3 <= 'f' )
  {
    result += c3 - 'a' + 10;
  } else if ( c3 >= '0' && c3 <= '9' )
  {
    result += c3 - '0';
  }
  result = (result << 4);
  if (c4 >= 'A' && c4 <= 'F' )
  {
    result += c4 - 'A' + 10;
  } else if ( c4 >= 'a' && c4 <= 'f' )
  {
    result += c4 - 'a' + 10;
  } else if ( c4 >= '0' && c4 <= '9' )
  {
    result += c4 - '0';
  }
  return result;
}

void byteToChar(char *str, uint8_t num)
{
  volatile uint8_t wk;

  wk = (num >> 4);
  if (wk >= 0 && wk <= 9)
  {
    *(str + 0) = wk + '0';
  } else if (wk >= 10 && wk <= 15)
  {
    *(str + 0) = wk - 10 + 'A';
  }
  wk = num & 0x0F;
  if (wk >= 0 && wk <= 9)
  {
    *(str + 1) = wk + '0';
  } else if (wk >= 10 && wk <= 15)
  {
    *(str + 1) = wk - 10 + 'A';
  }
}

void writeSRAM()
{
  volatile uint16_t address;
  volatile uint8_t addressH;
  volatile uint8_t addressL;
  volatile uint8_t data;
  volatile uint8_t len;
  volatile int datalen;
  volatile int j;

  Serial.println(ihexBuf);
  //Serial.write('.');
  len = charToByte(ihexBuf[1], ihexBuf[2]);
  datalen = len * 2;
  address = charToInt(ihexBuf[3], ihexBuf[4], ihexBuf[5], ihexBuf[6]);
  addressL = address;
  addressH = address >> 8;

  for (j = 0; j < datalen; j = j + 2)
  {
    data = charToByte(ihexBuf[9 + j], ihexBuf[10 + j]);
    PORTA = data;
    PORTC = addressL;
    PORTL = addressH;
    wrSignalLOW();
    asm("NOP\n");
    wrSignalHIGH();
    address++;
    addressL = address;
    addressH = address >> 8;
  }
}

uint16_t readuint16()
{
  char  buf[5];
  volatile char ch1;
  volatile int  bufidx;
  volatile int  cpos;

  for (int i = 0; i < 5; i++)
  {
    buf[i] = 0;
  }
  ch1 = 0;
  cpos = 0;
  bufidx = 0;
  do
  {
    if (Serial.available() > 0) {
      ch1 = Serial.read();
      if ( (ch1 >= '0' && ch1 <= '9') ||
           (ch1 >= 'a' && ch1 <= 'f') ||
           (ch1 >= 'A' && ch1 <= 'F') )
      {
        buf[bufidx] = ch1;
        bufidx++;
        Serial.write(ch1);
        cpos++;
      }
      if (ch1 == 0x08 || ch1 == 0x7f)
      {
        bufidx--;
        if (cpos > 0)
        {
          Serial.write(0x08);
          cpos--;
        }
      }
      if (bufidx >= 4)
      {
        bufidx = 3;
      }
      if (bufidx <= 0)
      {
        bufidx = 0;
      }
      if (cpos >= 4)
      {
        Serial.write(0x08);
        cpos--;
      }
    }
  } while (ch1 != 0x0d);

  return (charToInt(buf[0], buf[1], buf[2], buf[3]));
}

void displaySRAM(uint16_t saddr, uint16_t eaddr)
{
  uint16_t tmpaddr;
  uint8_t data;
  char dispmsg[80];
  int col;
  bool endFlag = false;

  tmpaddr = saddr;
  do
  {
    for (int i = 0; i < 80; i++)
    {
      dispmsg[i] = ' ';
    }
    dispmsg[79] = 0;
    for (col = 0; col < 16; col++)
    {
      if (col == 0)
      {
        byteToChar(&dispmsg[0], (uint8_t)(tmpaddr >> 8));
        byteToChar(&dispmsg[2], (uint8_t)(tmpaddr & 0x00ff));
      }
      PORTL = (tmpaddr >> 8);
      PORTC = tmpaddr & 0x00ff;
      mreqSignalLOW();
      rdSignalLOW();
      asm("NOP\n");
      asm("NOP\n");
      asm("NOP\n");
      asm("NOP\n");
      data = PINA;
      rdSignalHIGH();
      mreqSignalHIGH();
      byteToChar(&dispmsg[3 * col + 6], data);

      if (data >= 0x20 && data < 0x7f)
      {
        dispmsg[col + 55] = data;
      } else {
        dispmsg[col + 55] = '.';
      }
      if (tmpaddr == eaddr)
      {
        endFlag = true;
        break;
      } else {
        tmpaddr++;
      }
    }
    Serial.println(dispmsg);
  } while (endFlag == false);
}

void printPrompt()
{
  Serial.println();
  Serial.println("r or R : RESET Z80");
  Serial.println("d or D : DUMP MEMORY");
  Serial.println("       : COPY PASTE IHEX FILE. AUTO START HEXLOADER");
  Serial.println();
  Serial.print("* ");
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.begin(115200);
  Serial.println("-- Arduino Mega 2560 hex writer --");
  printPrompt();
}

void loop() {
  volatile char  ch;

  if (Serial.available() > 0) {
    if (processingHexWriteMode == false) {
      ch = Serial.read();
      if (ch == 0x0d)
      {
        Serial.println();
        Serial.print("* ");
      }
      if (ch == '?')
      {
        printPrompt();
      }
      if (ch == 'r' || ch == 'R')
      {
        Serial.print("Resetting Z80 ");
        resetSignalOUTPUT();
        resetSignalLOW();
        delay(1000);
        resetSignalHIGH();
        resetSignalHIZ();
        Serial.println("Done.");
        Serial.print("* ");
      }
      if (ch == 'd' || ch == 'D')
      {
        Serial.println("TEST");
        Serial.print("START ADDRESS          : 0000\b\b\b\b");
        startAddress = readuint16();
        Serial.println();
        Serial.print("Length(default 0x00FF) : 00FF\b\b\b\b");
        displayLength = readuint16();
        Serial.println();
        if (displayLength == 0)
        {
          displayLength = 0x00FF;
        }
        stopAddress = startAddress + displayLength;
        busreqSignalOUTPUT();
        busreqSignalLOW();
        while (PINH & (1 << BUSACK)) ;
        memorySignalOUTPUT();
        DDRA = 0x00;
        DDRC = 0xFF;
        DDRL = 0xFF;
        displaySRAM(startAddress, stopAddress);
        DDRA  = 0x00; // D7-D0
        DDRC  = 0x00; // A7-A0
        DDRL  = 0x00; // A15-A8
        memorySignalHIZ();
        busreqSignalHIZ();

        //Serial.println(data, HEX);
        Serial.println("TEST END");
      }
      if (ch == ':')
      {
        Serial.println("--Enter Hex load mode--");
        processingHexWriteMode = true;
        ihexIdx = 0;
        ihexBuf[ihexIdx++] = ch;
        // DATA/ADDRESS BUS OUTPUT
        busreqSignalOUTPUT();
        busreqSignalLOW();
        while (PINH & (1 << BUSACK)) ;
        memorySignalOUTPUT();
        DDRA  = 0xFF; // D7-D0
        DDRC  = 0xFF; // A7-A0
        DDRL  = 0xFF; // A15-A8
        //resetSignalOUTPUT();
        //resetSignalLOW();
        mreqSignalLOW();
      }
    } else {
      ch = Serial.read();
      ihexBuf[ihexIdx++] = ch;
      // End Of File 検出
      if ((ihexIdx - 1) == 8 && ihexBuf[8] == '1')
      {
        processingHexWriteMode = false;
        Serial.println();
        Serial.println("--Exit Hex load mode--");
        DDRA  = 0x00; // D7-D0
        DDRC  = 0x00; // A7-A0
        DDRL  = 0x00; // A15-A8
        memorySignalHIZ();
        busreqSignalHIZ();
        printPrompt();
        return;
      }
      // NEXT HEX LINE
      if (ch == ':') {
        ihexBuf[ihexIdx++] = 0;
        writeSRAM();
        ihexIdx = 0;
        ihexBuf[ihexIdx++] = ch;
      }
    }
  }
}
