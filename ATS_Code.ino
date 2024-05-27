#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define RELAY_MAIN 2
#define RELAY_GEN 3
#define MAIN_VOLTAGE A1
#define GEN_VOLTAGE A2
#define GREEN_LED 6
#define BLUE_LED 7
#define RED_LED 8
#define RESTART_BUTTON 9

// Voltage Threshold
#define MAIN_LOW 145
#define MAIN_MAX 245
#define GEN_LOW 145
#define GEN_HIGH 245

// Delay Constants
#define BOOT_DURATION 3000         // 3 Seconds
#define EMERG_SHUTDOWN_DELAY 2000  // 2 Seconds
#define AUTO_RECOVERY_DELAY 5000  // 5 Seconds
#define RESTART_DELAY 5000        // 5 Seconds

// Screen Initialization & Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Emergency Shutdown Variable
bool restartButtonPressed = false;

// Voltage Monitoring Variables
bool mainPowerAvailable = false;
bool genPowerAvailable = false;

// System State Variables
bool powerSourceSwitched = false;
bool mainPowerSelected = false;
unsigned long bootStartTime = 0;
unsigned long displayInitStartTime = 0;
unsigned long switchStartTime = 0;

// Function Prototypes
void setup();
void loop();
void displayBootMessage();
void RestartButtonMessage();
void drawProgressBar(unsigned long currentTime, unsigned long duration);
void displayVoltageGraphs(int mainVoltage, int genVoltage);
void displayPowerSourceIndicator();
void displayPowerSourceSwitched(const String &powerSource);
void checkVoltageErrors(int mainVoltage, int genVoltage);
void flashErrorLED();
//void monitorPowerSupplyStatus();
void normalOperation(int mainVoltage, int genVoltage);
void switchToMain();
void switchToGen();
void emergencyShutdown();
void autoRecovery();
void restartSystem();
void displayVoltage(int mainVoltage, int genVoltage);

void setup() {
  // Pin Modes Setup
  pinMode(RELAY_MAIN, OUTPUT);
  pinMode(RELAY_GEN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(RESTART_BUTTON, INPUT_PULLUP);

  // Initial RELAY State
  digitalWrite(RELAY_MAIN, LOW);
  digitalWrite(RELAY_GEN, LOW);

  // Serial and Display Setup
  Serial.begin(9600);
  displayInitStartTime = millis();  // Record display initialization start time
  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // Retry display initialization for up to 10 seconds
    if (millis() - displayInitStartTime > 10000) {
      // If initialization fails after 10 seconds .......
      Serial.println(F("Display Communication Error"));
      break;
    }
    delay(500); // Wait for 0.5 seconds before retrying
  }

  bootStartTime = millis();  // Record boot start time
  powerSourceSwitched = false; // Initialize power source switched flag
  displayBootMessage();
}

void loop() {
  // Read voltages of both the incoming lines from sensors
  int mainVoltage = analogRead(MAIN_VOLTAGE) * (300.0 / 1023.0);  // in volts
  int genVoltage = analogRead(GEN_VOLTAGE) * (300.0 / 1023.0);    // in volts

  // Check if either of the power lines is available
  bool mainPowerAvailable = (mainVoltage > 50); // Assuming 50 volts as the minimum voltage to indicate power availability
  bool genPowerAvailable = (genVoltage > 50);
  
  // Check if restart button is pressed
  if (digitalRead(RESTART_BUTTON) == LOW && !restartButtonPressed) {
    // Restart the system only if the restart button is pressed and not already being pressed
    restartButtonPressed = true;
    RestartButtonMessage();

  } else {
    restartButtonPressed = false;
  }

  // Handle voltage errors
  if (mainVoltage < MAIN_LOW || mainVoltage > MAIN_MAX || genVoltage < GEN_LOW || genVoltage > GEN_HIGH) {
    checkVoltageErrors(mainVoltage, genVoltage);
  } //else {
    //normalOperation(mainVoltage, genVoltage); //DOUGHT !!!
  //}
    
  // Display voltage bar graphs, power source indicator and Voltage
  displayVoltageGraphs(mainVoltage, genVoltage);
  displayPowerSourceIndicator();
  displayVoltage(mainVoltage, genVoltage);

  //Check if the power source has switched and update the display in real-time
  if (powerSourceSwitched) {
    if (millis() - switchStartTime <= 2000) { // Display the message for 2 seconds
      if (mainPowerSelected) {
        displayPowerSourceSwitched("Gen");
      } else {
        displayPowerSourceSwitched("Main");
      }
    } else {
      //display.clearDisplay(); // Clear the display after 2 seconds
      //display.display();
      powerSourceSwitched = false; // Reset the flag
    }
  }
}

void displayBootMessage() {
  unsigned long currentTime = millis();

  // Booting progress bar
  unsigned long bootTimeElapsed = currentTime - bootStartTime;
  if (currentTime <= BOOT_DURATION) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Namaste");
    display.println("System Booting...");
    drawProgressBar(currentTime, BOOT_DURATION);
    display.display();
    return;
  }
}

void drawProgressBar(unsigned long currentTime, unsigned long duration) {
  int progress = map(currentTime, 0, duration, 0, SCREEN_WIDTH - 2);
  display.fillRect(1, 32, progress, 8, SSD1306_WHITE);
}

void RestartButtonMessage() {
  // Display restart button pressed message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Restart button pressed,");
  display.println("Restarting the system");
  display.println("in 5 Seconds..." );
  display.display();

  // Delay before restart
  delay(RESTART_DELAY);

  // Restart the system
  restartSystem();
}

void displayVoltageGraphs(int mainVoltage, int genVoltage) {
  display.clearDisplay();

  // Display Main Voltage Graph
  display.setCursor(0, 0);
  display.print("Main Voltage: ");
  display.println(mainVoltage);
  display.fillRect(0, 16, map(mainVoltage, 0, 220, 0, SCREEN_WIDTH - 2), 8, SSD1306_WHITE);
  // Display Gen Voltage Graph
  display.print("Gen Voltage: ");
  display.println(genVoltage);
  display.fillRect(0, 32, map(genVoltage, 0, 220, 0, SCREEN_WIDTH - 2), 8, SSD1306_WHITE);

  display.display();
}

void displayPowerSourceIndicator() {
  // Display power source indicator in the top left corner
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(mainPowerSelected ? "Main" : "Gen");
  display.display();
}

void checkVoltageErrors(int mainVoltage, int genVoltage) {
  // Check for threshold errors
  if ((mainVoltage < MAIN_LOW || mainVoltage > MAIN_MAX) && (genVoltage < GEN_LOW || genVoltage > GEN_HIGH)) {
  //if (mainVoltage < MAIN_LOW || mainVoltage > MAIN_MAX || genVoltage < GEN_LOW || genVoltage > GEN_HIGH) {  
    // Both main and gen voltages are outside their thresholds
    flashErrorLED();
  } else {
    // Handle individual voltage errors
    if (mainVoltage < MAIN_LOW || mainVoltage > MAIN_MAX) {
      flashErrorLED();
    }
    if (genVoltage < GEN_LOW || genVoltage > GEN_HIGH) {
      flashErrorLED();
    }
    
    // Check if both voltages are zero (power cut)
    if (!mainPowerAvailable && !genPowerAvailable) {
      digitalWrite(RELAY_MAIN, LOW);
      digitalWrite(RELAY_GEN, LOW);
      return; // Continue monitoring voltages
    // DOUGHT !!! CHECK FOR POWER RESTORE
    }
  }
}

unsigned long lastFlashTime = 0;
const unsigned long flashInterval = 500; // Flash interval in milliseconds
bool isLEDon = false;

void flashErrorLED() {
  unsigned long currentTime = millis();

  if (currentTime - lastFlashTime >= flashInterval) {
    lastFlashTime = currentTime;

    // Toggle the LED state
    if (isLEDon) {
      digitalWrite(RED_LED, LOW);
      isLEDon = false;
    } else {
      digitalWrite(RED_LED, HIGH);
      isLEDon = true;
    }
  }
}

//void monitorPowerSupplyStatus() {
    // Check if either of the power lines is available
  //mainPowerAvailable = (mainVoltage > 5); //Set the min Voltage to indicate Power Cut
  //genPowerAvailable = (genVoltage > 5);
//}

void normalOperation(int mainVoltage, int genVoltage) {
  // If both lines are within threshold
  if ((mainVoltage >= MAIN_LOW && mainVoltage <= MAIN_MAX) && (genVoltage >= GEN_LOW && genVoltage <= GEN_HIGH)) {
    if (mainVoltage == genVoltage && mainVoltage != 0) {
      // If both lines have equal voltages, prioritize switching to the main line
      switchToMain();
    } else {
      if (mainVoltage > genVoltage) {
       switchToMain();
      } else {
       switchToGen();
      }
    }
  }
}

void switchToMain() {
  digitalWrite(RELAY_GEN, LOW);
  digitalWrite(RELAY_MAIN, HIGH);  // Activate Main Line
  digitalWrite(GREEN_LED, HIGH);  // Green LED ON
  digitalWrite(BLUE_LED, LOW);
  mainPowerSelected = true; // Update power source selection
  displayPowerSourceSwitched("Main");
}

void switchToGen() {
  digitalWrite(RELAY_MAIN, LOW);
  digitalWrite(RELAY_GEN, HIGH);  // Activate Generator Line
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);  // Blue LED ON
  mainPowerSelected = false; // Update power source selection
  displayPowerSourceSwitched("Gen");
}

void emergencyShutdown() {
  // Turn off RELAYS
  digitalWrite(RELAY_MAIN, LOW);
  digitalWrite(RELAY_GEN, LOW);
  flashErrorLED();

  // Display emergency shutdown message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("! WARNING !,");
  display.println("Emergency Shutdown initialized...");
  display.display();

  // Trigger emergency shutdown after delay
  delay(EMERG_SHUTDOWN_DELAY);

  // Auto-recovery process
  autoRecovery();
}

void autoRecovery() {
  // Display auto-recovery message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Auto-Recovery...");
  display.println("initialized");
  display.display();

  // Delay for auto-recovery
  delay(AUTO_RECOVERY_DELAY);

  // Restart the system
  restartSystem();
}

void restartSystem() {
  // Display restart message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Restarting...");
  display.display();

  // Delay before restart
  delay(RESTART_DELAY);

  // Restart the system
  setup();
}

void displayVoltage(int mainVoltage, int genVoltage) {
  // Display voltage updates on OLED display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Main Voltage: ");
  display.println(mainVoltage);
  display.print("V");
  display.print("Gen Voltage: ");
  display.println(genVoltage);
  display.print("V");
  display.display();
}

void displayPowerSourceSwitched(const String &powerSource) {
  // Display power source switched message for 2 seconds
  unsigned long switchStartTime = millis();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Switched to " + powerSource + " Line");
  display.display();

  while (millis() - switchStartTime <= 2000) {
    // Wait for 2 seconds
    // Check if the power source has switched again during the 2-second interval
    if (powerSourceSwitched) {
      // Update the display immediately if the power source switches again
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Switched to " + String(mainPowerSelected ? "Gen" : "Main") + " Line");
      display.display();
      // Update the switch start time to continue the 2-second display duration
      switchStartTime = millis();
      // Reset the power source switched flag
      powerSourceSwitched = false;
    }
  }

  // Clear the display after 2 seconds
  display.clearDisplay();
  display.display();
}


