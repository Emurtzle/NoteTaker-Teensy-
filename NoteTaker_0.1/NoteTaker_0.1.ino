/****************************************************************************************
NOTE Taker v0.1

****************************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BBQ10Keyboard.h>
#include <SdFat.h>
#include <sdios.h>

/*************************************************************
                  DECLARATIONS AND INITIALIZATIONS
*************************************************************/

// SD CARD

#define SD_FAT_TYPE 3   // 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SPI_SPEED SD_SCK_MHZ(4)   // Change SPI_SPEED to SD_SCK_MHZ(50) for best performance.
#define CHIP_SELECT_PIN 10    // Chip Select (CS) Pin

//------------------------------------------------------------------------------
#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE
//------------------------------------------------------------------------------



// KEYBOARD

BBQ10Keyboard keyboard;


// DISPLAY

#define TEXT_SCALE 2
#define CHARS_PER_ROW 10
#define NUM_ROWS 2

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WORD PROCESSING

#define FULL_STRING_LENGTH 1000
bool IgnoreFirst = true;
int MinIndex = 0;
int MaxIndex = CHARS_PER_ROW * NUM_ROWS;
char FullString[FULL_STRING_LENGTH];
char WorkingString[CHARS_PER_ROW * NUM_ROWS];
int CursorIndex = 0;


/*************************************************************
                  Functions
*************************************************************/

void setup() {
  Serial.begin(9600);

  // SD CARD

  if (!sd.begin(CHIP_SELECT_PIN, SPI_SPEED)) {
    if (sd.card() -> errorCode()) {
      Serial.println(F("SD CARD INITIALIZATION: Failed"));
      for(;;); // Don't proceed, loop forever
    }
  }
  Serial.println("SD CARD INITIALIZATION: Successful");

  // Keyboard
  
  Wire1.begin();
  keyboard.begin(0x1F, &Wire1);
  keyboard.setBacklight(0.5f);

  Serial.println("KEYBOARD INITIALIZATION: Successful");

  // Display

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  Serial.println("DISPLAY INITIALIZATION: Successful");

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  const int keycount = keyboard.keyCount();

  if (keycount == 0)
    return;

  const BBQ10Keyboard::KeyEvent key = keyboard.keyEvent();
  
  if (key.state == BBQ10Keyboard::StateRelease || key.state == BBQ10Keyboard::StateLongPress)
    return;

  if (IgnoreFirst == true) {
    IgnoreFirst = false;
    return;
  }

  // Backspace Button
  if (key.key == 8 && key.state == BBQ10Keyboard::StatePress) {
    if (CursorIndex >= 0) {
      backspace();
      return;
    }
  }

  // TopB0(dec 6, hex 06) ->> Clear
  if (key.key == 6 && key.state == BBQ10Keyboard::StatePress) {
    clear();
    return;
  }

  // TopB1(dec 17, hex 11) ->> File Browser
  if (key.key == 17 && key.state == BBQ10Keyboard::StatePress) {
    startFileBrowser();
    return;
  }

  // TOPB2(dec 7, hex 07) ->> Save File 
  if (key.key == 7 && key.state == BBQ10Keyboard::StatePress) {
    saveFile();
    return;
  }

  // TopB3(dec 18, hex 12) ->> Save File and Start New
  if (key.key == 18 && key.state == BBQ10Keyboard::StatePress) {
    saveFileAndNew();
    return;
  }

  if (key.state == BBQ10Keyboard::StatePress) {
    addChar(key.key);
  }






  String state = "pressed";
  Serial.printf("key: '%c' (dec %d, hex %02x) %s\r\n", key.key, key.key, key.key, state.c_str());
}

void printStatus() {
  Serial.printf("Cursor: %d | FS: %s | WS: %s | MinIdex: %d | MaxIndex: %d\r\n", CursorIndex, FullString, WorkingString, MinIndex, MaxIndex);
}

void addChar(char c) {
  FullString[CursorIndex] = c;
  setCursor(CursorIndex + 1);
  displayText();
  printStatus();
}

void backspace() {
  setCursor(CursorIndex - 1);
  FullString[CursorIndex] = 0;   // Set character to "empty"
  displayText();
  printStatus();
}

void saveFileAndNew() {

}

void saveFile() {

}

void startFileBrowser() {

}

void clear() {
  memset(FullString, 0, sizeof(FullString));
  memset(WorkingString, 0, sizeof(WorkingString));

  setCursor(0);

  displayText();
  printStatus();
}

void displayText() {
  setWorkingString();
  display.clearDisplay();
  display.setTextSize(TEXT_SCALE);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F(WorkingString));  // Display working String
  display.display();
}

void setCursor(int index) {
  if (index <= 0) {   // Cursor goes to or below 0
    CursorIndex = 0;
    MinIndex = 0;
    MaxIndex = CHARS_PER_ROW * NUM_ROWS;
  } else if (index < MinIndex) {    // Cursor hits min bound
    CursorIndex = index;
    MinIndex = CursorIndex;
    MaxIndex = MinIndex + (CHARS_PER_ROW * NUM_ROWS);
  } else if (index >= MaxIndex) {
    CursorIndex = index;
    MaxIndex = CursorIndex;
    MinIndex = MaxIndex - (CHARS_PER_ROW * NUM_ROWS);
  }


  // CursorIndex += move;
  // if (CursorIndex < 0) {
  //   CursorIndex = 0;
  //   MinIndex = 0;
  //   MaxIndex = CHARS_PER_ROW * NUM_ROWS;

  // } else if (CursorIndex > MaxIndex) {
  //   MaxIndex = CursorIndex + 10;
  //   MinIndex = MaxIndex - (CHARS_PER_ROW * NUM_ROWS) + 10;
  
  // } else if (CursorIndex < MinIndex) {
  //   MinIndex = CursorIndex - 10;
  //   MaxIndex = MaxIndex - 10;
  //   if (MinIndex < 0) {
  //     MinIndex = 0;
  //     MaxIndex = CHARS_PER_ROW * NUM_ROWS;
  //   }
  //   MaxIndex = MinIndex + (CHARS_PER_ROW * NUM_ROWS);
  // }
}

// First max characters behind cursor
void setWorkingString() {
  for (int i = MinIndex; i < MaxIndex; i++) {
    WorkingString[i - MinIndex] = FullString[i];
  }
}