#include "effects.h"

void bullseye(MD_MAX72XX *mx)
// Demonstrate the use of buffer based repeated patterns
// across all devices.
{
  Serial.println("Bullseye");
  mx->clear();
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  for (uint8_t n=0; n<3; n++)
  {
    byte  b = 0xff;
    int   i = 0;

    while (b != 0x00)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx->setRow(j, i, b);
        mx->setColumn(j, i, b);
        mx->setRow(j, ROW_SIZE-1-i, b);
        mx->setColumn(j, COL_SIZE-1-i, b);
      }
      mx->update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx->setRow(j, i, 0);
        mx->setColumn(j, i, 0);
        mx->setRow(j, ROW_SIZE-1-i, 0);
        mx->setColumn(j, COL_SIZE-1-i, 0);
      }

      bitClear(b, i);
      bitClear(b, 7-i);
      i++;
    }

    while (b != 0xff)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx->setRow(j, i, b);
        mx->setColumn(j, i, b);
        mx->setRow(j, ROW_SIZE-1-i, b);
        mx->setColumn(j, COL_SIZE-1-i, b);
      }
      mx->update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx->setRow(j, i, 0);
        mx->setColumn(j, i, 0);
        mx->setRow(j, ROW_SIZE-1-i, 0);
        mx->setColumn(j, COL_SIZE-1-i, 0);
      }

      i--;
      bitSet(b, i);
      bitSet(b, 7-i);
    }
  }

  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void stripe(MD_MAX72XX *mx)
// Demonstrates animation of a diagonal stripe moving across the display
// with points plotted outside the display region ignored.
{
  const uint16_t maxCol = MAX_DEVICES*ROW_SIZE;
  const uint8_t	stripeWidth = 10;

  Serial.println("Each individually by row then col");
  mx->clear();

  for (uint16_t col=0; col<maxCol + ROW_SIZE + stripeWidth; col++)
  {
    for (uint8_t row=0; row < ROW_SIZE; row++)
    {
      mx->setPoint(row, col-row, true);
      mx->setPoint(row, col-row - stripeWidth, false);
    }
    delay(DELAYTIME);
  }
}

void spiral(MD_MAX72XX *mx)
// setPoint() used to draw a spiral across the whole display
{
  Serial.println("Spiral in");
  int  rmin = 0, rmax = ROW_SIZE-1;
  int  cmin = 0, cmax = (COL_SIZE*MAX_DEVICES)-1;

  mx->clear();
  while ((rmax > rmin) && (cmax > cmin))
  {
    // do row
    for (int i=cmin; i<=cmax; i++)
    {
      mx->setPoint(rmin, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmin++;

    // do column
    for (uint8_t i=rmin; i<=rmax; i++)
    {
      mx->setPoint(i, cmax, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmax--;

    // do row
    for (int i=cmax; i>=cmin; i--)
    {
      mx->setPoint(rmax, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmax--;

    // do column
    for (uint8_t i=rmax; i>=rmin; i--)
    {
      mx->setPoint(i, cmin, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmin++;
  }
}

void bounce(MD_MAX72XX *mx)
// Animation of a bouncing ball
{
  const int minC = 0;
  const int maxC = mx->getColumnCount()-1;
  const int minR = 0;
  const int maxR = ROW_SIZE-1;

  int  nCounter = 0;

  int  r = 0, c = 2;
  int8_t dR = 1, dC = 1;	// delta row and column

  Serial.println("Bouncing ball");
  mx->clear();

  while (nCounter++ < 200)
  {
    mx->setPoint(r, c, false);
    r += dR;
    c += dC;
    mx->setPoint(r, c, true);
    delay(DELAYTIME/2);

    if ((r == minR) || (r == maxR))
      dR = -dR;
    if ((c == minC) || (c == maxC))
      dC = -dC;
  }
}

void rows(MD_MAX72XX *mx)
// Demonstrates the use of setRow()
{
  Serial.println("Rows 0->7");
  mx->clear();

  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
    mx->setRow(row, 0xff);
    delay(2*DELAYTIME);
    mx->setRow(row, 0x00);
  }
}

void checkboard(MD_MAX72XX *mx)
// nested rectangles spanning the entire display
{
  uint8_t chkCols[][2] = { { 0x55, 0xaa }, { 0x33, 0xcc }, { 0x0f, 0xf0 }, { 0xff, 0x00 } };

  Serial.println("Checkboard");
  mx->clear();

  for (uint8_t pattern = 0; pattern < sizeof(chkCols)/sizeof(chkCols[0]); pattern++)
  {
    uint8_t col = 0;
    uint8_t idx = 0;
    uint8_t rep = 1 << pattern;

    while (col < mx->getColumnCount())
    {
      for (uint8_t r = 0; r < rep; r++)
        mx->setColumn(col++, chkCols[pattern][idx]);   // use odd/even column masks
      idx++;
      if (idx > 1) idx = 0;
    }

    delay(10 * DELAYTIME);
  }
}

void columns(MD_MAX72XX *mx)
// Demonstrates the use of setColumn()
{
  Serial.println("Cols 0->max");
  mx->clear();

  for (uint8_t col=0; col<mx->getColumnCount(); col++)
  {
    mx->setColumn(col, 0xff);
    delay(DELAYTIME/MAX_DEVICES);
    mx->setColumn(col, 0x00);
  }
}

void cross(MD_MAX72XX *mx)
// Combination of setRow() and setColumn() with user controlled
// display updates to ensure concurrent changes.
{
  Serial.println("Moving cross");
  mx->clear();
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  // diagonally down the display R to L
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0xff);
      mx->setRow(j, i, 0xff);
    }
    mx->update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0x00);
      mx->setRow(j, i, 0x00);
    }
  }

  // moving up the display on the R
  for (int8_t i=ROW_SIZE-1; i>=0; i--)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0xff);
      mx->setRow(j, ROW_SIZE-1, 0xff);
    }
    mx->update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0x00);
      mx->setRow(j, ROW_SIZE-1, 0x00);
    }
  }

  // diagonally up the display L to R
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0xff);
      mx->setRow(j, ROW_SIZE-1-i, 0xff);
    }
    mx->update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx->setColumn(j, i, 0x00);
      mx->setRow(j, ROW_SIZE-1-i, 0x00);
    }
  }
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void showCharset(MD_MAX72XX *mx)
// Run through display of the the entire font characters set
{
  mx->clear();
  mx->update(MD_MAX72XX::OFF);

  for (uint16_t i=0; i<256; i++)
  {
    mx->clear(0);
    mx->setChar(COL_SIZE-1, i);

    if (MAX_DEVICES >= 3)
    {
      char hex[3];

      sprintf(hex, "%02X", i);

      mx->clear(1);
      mx->setChar((2*COL_SIZE)-1,hex[1]);
      mx->clear(2);
      mx->setChar((3*COL_SIZE)-1,hex[0]);
    }

    mx->update();
    delay(DELAYTIME*2);
  }
  mx->update(MD_MAX72XX::ON);
}

void transformation1(MD_MAX72XX *mx)
// Demonstrates the use of transform() to move bitmaps on the display
// In this case a user defined bitmap is created and animated.
{
  uint8_t arrow[COL_SIZE] =
  {
    0b00001000,
    0b00011100,
    0b00111110,
    0b01111111,
    0b00011100,
    0b00011100,
    0b00111110,
    0b00000000
  };

  uint8_t smiley[COL_SIZE] =
  {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
  };


  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TFLR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TRC,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TFUD,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV
  };

  Serial.println("Transformation1");
  mx->clear();

  // use the arrow bitmap
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t j=0; j<mx->getDeviceCount(); j=j+2)
    mx->setBuffer(((j+1)*COL_SIZE)-1, COL_SIZE, smiley);
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  delay(DELAYTIME);

  // run through the transformations
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx->transform(t[i]);
    delay(DELAYTIME*4);
  }
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);
}

void transformation2(MD_MAX72XX *mx)
// Demonstrates the use of transform() to move bitmaps on the display
// In this case font characters are loaded into the display for animation.
{
  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSD, MD_MAX72XX::TSU, MD_MAX72XX::TSD, MD_MAX72XX::TSU,
    MD_MAX72XX::TFLR, MD_MAX72XX::TFLR, MD_MAX72XX::TFUD, MD_MAX72XX::TFUD
  };

  Serial.println("Transformation2");
  mx->clear();
  mx->control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);

  // draw something that will show changes
  for (uint8_t j=0; j<mx->getDeviceCount(); j++)
  {
    mx->setChar(((j+1)*COL_SIZE)-1, '0'+j);
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx->transform(t[i]);
    delay(DELAYTIME*3);
  }
}

void intensity(MD_MAX72XX *mx)
// Demonstrates the control of display intensity (brightness) across
// the full range.
{
  uint8_t row;

  Serial.println("Vary intensity ");

  mx->clear();

  // Grow and get brighter
  row = 0;
  for (int8_t i=0; i<=MAX_INTENSITY; i++)
  {
    mx->control(MD_MAX72XX::INTENSITY, i);
    if (i%2 == 0)
      mx->setRow(row++, 0xff);
    delay(DELAYTIME*3);
  }

  mx->control(MD_MAX72XX::INTENSITY, 8);
}

void blinking(MD_MAX72XX *mx)
// Uses the test function of the MAX72xx to blink the display on and off.
{
  int  nDelay = 1000;

  Serial.println("Blinking");
  mx->clear();

  while (nDelay > 0)
  {
    mx->control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx->control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= DELAYTIME;
  }
}