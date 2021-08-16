/*
     DDUAL-86 ROM HEXWRITER (Aki-80 Compatible board)
     Mashiro Kusunoki (JP3SRS)
     2021/08/16 Draft Version.
*/
#define RESET  11
#define MREQ    9
#define WR      8
#define LED    13


volatile boolean  processingHexWriteMode = false;
char ihexBuf[520];
volatile int  ihexIdx;
/*
     PA0-PA7(22-29) = D0-D7
     PC0-PC7(37-30) = A0-A7
     PL0-PL7(49-42) = A8-A15
     PB5(11) = ~RESET
     PH6(9) = ~MREQ
     PH5(8)  = ~WR
     PB4(10) = ~RD
*/

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
    result = result << 4;
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
    result = result << 4;
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
    result = result << 4;
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
    result = result << 4;
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
        digitalWrite(WR, LOW);
        digitalWrite(MREQ, LOW);
        digitalWrite(WR, HIGH);
        digitalWrite(MREQ, HIGH);
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
    Serial.println();
    for (int i = 0 ; i < 5; i++)
    {
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }

    return (charToInt(buf[0], buf[1], buf[2], buf[3]));
}

void printPrompt()
{
    Serial.println();
    Serial.println("r or R : RESET Z80");
    Serial.println("       : COPY PASTE IHEX FILE. AUTO START HEXLOADER");
    Serial.println();
    Serial.print("* ");
}

void setup() {

    DDRA  = 0x00; // D7-D0
    PORTA = 0x00;
    DDRC  = 0x00; // A7-A0
    PORTC = 0x00;
    DDRL  = 0x00; // A15-A8
    PORTL = 0x00;
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    digitalWrite(MREQ, HIGH);
    digitalWrite(WR, HIGH);
    Serial.begin(115200);
    Serial.println("-- Arduino Mega 2560 hex writer --");
    printPrompt();
}

void loop() {
    // put your main code here, to run repeatedly:
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
                pinMode(RESET, OUTPUT);
                digitalWrite(RESET, LOW);
                delay(1000);
                digitalWrite(RESET, HIGH);
                pinMode(RESET, INPUT);
                Serial.println("Done.");
                Serial.print("* ");
            }
            if (ch == ':')
            {
                Serial.println("--Enter Hex load mode--");
                processingHexWriteMode = true;
                ihexIdx = 0;
                ihexBuf[ihexIdx++] = ch;
                // DATA/ADDRESS BUS OUTPUT
                DDRA  = 0xFF; // D7-D0
                PORTA = 0x00;
                DDRC  = 0xFF; // A7-A0
                PORTC = 0x00;
                DDRL  = 0xFF; // A15-A8
                PORTL = 0x00;
                pinMode(MREQ, OUTPUT);
                digitalWrite(MREQ, HIGH);
                pinMode(WR, OUTPUT);
                digitalWrite(WR, HIGH);
                pinMode(RESET, OUTPUT);
                digitalWrite(RESET, LOW);
            }
        }
        else
        {
            ch = Serial.read();
            ihexBuf[ihexIdx++] = ch;
            // End Of File 検出
            if ((ihexIdx - 1) == 8 && ihexBuf[8] == '1')
            {
                processingHexWriteMode = false;
                Serial.println();
                Serial.println("--Exit Hex load mode--");
                // DATA/ADDRESS BUS HI-Z
                DDRA  = 0x00; // D7-D0
                PORTA = 0x00;
                DDRC  = 0x00; // A7-A0
                PORTC = 0x00;
                DDRL  = 0x00; // A15-A8
                PORTL = 0x00;
                pinMode(MREQ, INPUT);
                pinMode(WR, INPUT);
                delay(1000);
                digitalWrite(RESET, HIGH);
                pinMode(RESET, INPUT);
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
