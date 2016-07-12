

#include <Arduino.h>

// include the library code:
#include <Button.h>
#include <LiquidCrystal.h>
#include <RotaryEncoderAcelleration.h>
#include <TicksPerSecond.h>
#include <ValueMenu.h>
#include <Wire.h>
#include <lcd_menu.h>

#define LCD_D7 9
#define LCD_D6 10
#define LCD_D5 4
#define LCD_D4 5
#define LCD_EN 11
#define LCD_RS 12

#define LCD_COLS 20
#define LCD_ROWS 4

#define encoderPinA 2
#define encoderPinB 3

int print_centered(int, char *);

double gwireSize;
double gturnsTotal;
double gspoolLength;
static long glastRotor = 0; // The laet rotor position
struct stackup gstackup;
static boolean gnewJobSetup;

// initialize the lcd library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;

static const int buttonPin = A0; // the number of the pushbutton pin
static const int rotorPinA = 2;  // One quadrature pin
static const int rotorPinB = 3;  // the other quadrature pin

RotaryEncoderAcelleration rotor;
Button pushButton;

void UpdateRotor() { rotor.update(); }

mainMenuMode gmenuMode = setupJob;

void setup() {
  // setup LCD
  lcd.begin(LCD_COLS, LCD_ROWS);

  // setup rotary encoder
  rotor.initialize(rotorPinA, rotorPinB);

  // setup push button on encode
  pushButton.initialize(buttonPin);

  // Setup Wire
  Wire.begin();

  // setup interupt for encoder
  attachInterrupt(0, UpdateRotor, CHANGE);

  // Print Splash screen to the LCD.
  lcd.setCursor(8, 1);
  lcd.print("Coil");
  lcd.setCursor(7, 2);
  lcd.print("Winder");
  delay(2000);
  lcd.clear();

  // Data Stream Test returns number of failed recieve events
  /*
  int fail = DataStreamTest();
  if (fail) {
    lcd.print("I2C test failed on  ");
    lcd.setCursor(0,1);
    lcd.print(fail);
    lcd.print(" events");
    lcd.setCursor(0,3);
    lcd.print("Rebooting...");
    delay(2000);
    setup();
  }
*/
  gnewJobSetup = false;
  lcdMainMenu();
}

void newJob() {
  menuResult wireResult;
  menuResult turnsResult;
  menuResult spoolResult;

  wireResult = showMenu(2.00, 0.01, 0.5, 2, "mm", "Set wire size");

  if (!wireResult.status) {
    return;
  } else {
    turnsResult =
        showMenu(20000.0, 1.0, 500.0, 1, "turns", "Set number of turns");
    if (!turnsResult.status) {
      return;
    } else {
      spoolResult = showMenu(500.0, 1.0, 10.0, 1, "mm", "Set spool length");
      if (!spoolResult.status) {
        return;
      } else {
        // If we get here all data has been OK'd

        gwireSize = wireResult.value;
        gturnsTotal = turnsResult.value;
        gspoolLength = spoolResult.value;
      }
    }
  }

  gstackup = calculateStackup(gwireSize, gspoolLength, gturnsTotal);

  return;
}

stackup calculateStackup(double wireSize, double bobbinLength, double turns) {

  struct stackup newStack;

  int turnsPerLayer;

  double fractional, wholeLayers;

  printf("Wire size = %g, bobbin = %g, turns = %g ", wireSize, bobbinLength,
         turns);

  turnsPerLayer = (int)(bobbinLength / wireSize);

  double layers = turns / (double)turnsPerLayer;

  fractional = modf(layers, &wholeLayers);

  newStack.numberWholeLayers = wholeLayers;
  newStack.turnsWholeLayer = turnsPerLayer;
  newStack.turnsLastLayer = fractional * (double)turnsPerLayer;

  return newStack;
}

void loop() {}

void lcdReview(stackup stack) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(gturnsTotal, 1);
  lcd.print(" W:");
  lcd.print(gwireSize, 2);

  lcd.setCursor(0, 1);
  lcd.print("Spool:");
  lcd.print(gspoolLength, 1);

  lcd.setCursor(0, 2);
  lcd.print("Stk:");
  lcd.print(stack.numberWholeLayers, 1);
  lcd.print("@");
  lcd.print(stack.turnsWholeLayer, 1);
  lcd.print(" 1@");
  lcd.print(stack.turnsLastLayer, 1);
  lcd.setCursor(0, 3);
  lcd.print("    >OK");

  do {
    pushButton.update();
  } while (!pushButton.isPressed());

  return;
}

void printMainMenu() {
  rotor.setMinMax(0, startJob);
  rotor.setPosition(0);

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Setup new job");
  lcd.setCursor(1, 1);
  lcd.print("Review job");
  lcd.setCursor(1, 2);
  lcd.print("Start");

  // Print out the cursor
  lcdPrintCursor();

  rotor.setMinMax(0, 2);
  rotor.setPosition(0);

  return;
}

void lcdMainMenu() {
  printMainMenu();

  while (1) {
    pushButton.update();
    if (pushButton.isPressed()) {
      if (gmenuMode == setupJob) {
        newJob();
        printMainMenu();
      }
      if (gmenuMode == reviewJob) {
        lcdReview(gstackup);
        printMainMenu();
      }
    }

    long pos = rotor.getPosition();

    if (glastRotor != pos) {
      switch (pos) {
      case 0: {
        gmenuMode = setupJob;
        break;
      };
      case 1: {
        gmenuMode = reviewJob;
        break;
      };
      case 2: {
        gmenuMode = startJob;
        break;
      };
      }
      lcdPrintCursor();
    }
    glastRotor = pos;
  };
}

// Decide which cursor position to clear based on the current menu
void lcdPrintCursor() {

  // Clear all >
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(0, i);
    lcd.print(" ");
  }

  // Display the > at the correct line
  switch (gmenuMode) {
  case setupJob: {
    lcd.setCursor(0, 0);
    break;
  }
  case reviewJob: {
    lcd.setCursor(0, 1);
    break;
  }
  case startJob: {
    lcd.setCursor(0, 2);
    break;
  }
  }
  lcd.print(">");
}

int DataStreamTest() {
  // Command Test

  Wire.beginTransmission(8);
  Wire.write(0x01);

  // Send Data
  Wire.write(0xAA);
  Wire.write(0x55);
  Wire.write(0xFF);

  Wire.endTransmission();
  delay(100);
  // Listen for data from other side
  Wire.requestFrom(8, 3);

  int i = 0;
  int completed = 0;
  while (Wire.available()) {
    byte c = Wire.read();
    if (c == 0xAA && i == 0) {
      completed++;
    } else if (c == 0x55 && i == 1) {
      completed++;
    } else if (c == 0xFF && i == 2) {
      completed++;
    }
    i++;
  }
  return 3 - completed;
}
