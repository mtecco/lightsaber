#include <EEPROM.h>    // For saving/loading color settings to non-volatile memory
#include <SD.h>        // For reading sound files from SD card
#include <SPI.h>       // For SPI communication with SD card
#include <Wire.h>      // For I2C communication with MPU6050 accelerometer
#include <TMRpcm.h>    // For audio playback functionality

// Assembly function declarations - these are implemented in WS2812B.S
// The 'extern "C"' tells the compiler these are C functions
extern "C" {
    void initWS2812B();                               // Initialize the LED control hardware
    void sendColor(uint8_t r, uint8_t g, uint8_t b);  // Send RGB color to one LED
    void clearLEDs();                                 // Turn off all LEDs
    void sendReset();                                 // Send reset signal to latch LED data
}

// Global variables for LED count - accessed by both C++ and assembly code
// These are split into high/low bytes for 16-bit handling in assembly
extern "C" {
    uint8_t NUM_LEDS_LOW = 0;   // Low byte of LED count (0-255)
    uint8_t NUM_LEDS_HIGH = 1;  // High byte of LED count (256+)
}

// Hardware pin definitions
#define BUTTON_PIN  7    // Main control button input
#define LED_PIN     3    // Data pin for WS2812B LED strip
#define BRIGHTNESS  100  // Maximum brightness level (0-255 scale)
#define NUM_LEDS    256  // Total number of LEDs in the strip

#define SD_CS_PIN   10     // Chip select pin for SD card module
#define AUDIO_PIN   9      // Audio output pin for speaker
#define MPU6050_ADDR 0x68  // I2C address of MPU6050 accelerometer

TMRpcm tmrpcm;  // Audio playback object

// --- State Variables ---
volatile byte saberState = 0;                        // 0=off, 1=on, 2=color-change
byte savedRed = 255, savedGreen = 0, savedBlue = 0;  // Current saber color

// --- Button Variables ---
volatile unsigned long buttonPressStartTime = 0;  // Timestamp when button was pressed
volatile unsigned long prevAnimationTime = 0;     // For timing color animations
volatile byte lastButtonState = HIGH;             // Previous button state for edge detection
volatile bool buttonPressed = false;              // Flag indicating button is currently pressed
volatile bool longPressDetected = false;          // Flag indicating long press was detected
volatile bool inColorChangeMode = false;          // Flag for color selection mode

// --- MPU Variables ---
int16_t ax, ay, az;            // Raw accelerometer readings (X, Y, Z axes)
bool sdCardAvailable = false;  // Flag indicating SD card initialization status

// --- Sound Management ---
volatile unsigned long lastSwingTime = 0;           // Last time a motion sound was played
const unsigned long SWING_COOLDOWN = 500;           // Minimum time between motion sounds (ms)
volatile unsigned long saberActivationTime = 0;     // When the saber was last turned on
const unsigned long ACTIVATION_GRACE_PERIOD = 1500; // No motion sounds right after activation

// Sound state machine states
#define SOUND_NONE 0                             // No sound currently playing
#define SOUND_PLAYING 1                          // Sound is currently playing
volatile byte soundState = SOUND_NONE;           // Current sound state
volatile unsigned long soundStartTime = 0;       // When current sound started playing
volatile unsigned long soundMinDuration = 0;     // Minimum time sound should play

// --- Memory Management ---
unsigned long lastMemoryCheck = 0;                 // Last time memory was checked
const unsigned long MEMORY_CHECK_INTERVAL = 2000;  // How often to check memory (ms)

/**
 * Calculates the amount of free RAM available
 * @return Free RAM in bytes
 */
int freeMemory() {
    char top;  // Variable at top of stack
    extern char *__brkval;  // Heap pointer
    return &top - __brkval; // Difference gives free memory
}

/**
 * System initialization - runs once at startup
 */
void setup() {
    Serial.begin(9600);  // Initialize serial communication for debugging
    delay(1000);         // Wait for serial to stabilize

    Serial.println(F("Lightsaber Starting..."));  // F() stores string in program memory to save RAM

    // Display initial free memory
    Serial.print(F("Free RAM: "));
    Serial.println(freeMemory());

    // Initialize LEDs - prepare the LED count for assembly code
    // Split 16-bit NUM_LEDS into two 8-bit values for assembly processing
    NUM_LEDS_LOW = NUM_LEDS & 0xFF;          // Extract low byte (bits 0-7)
    NUM_LEDS_HIGH = (NUM_LEDS >> 8) & 0xFF;  // Extract high byte (bits 8-15)
    initWS2812B();                           // Call assembly function to initialize LED hardware
    Serial.println(F("LEDs OK"));

    // Load saved color from EEPROM
    savedRed = EEPROM.read(0);    // Read red value from address 0
    savedGreen = EEPROM.read(1);  // Read green value from address 1
    savedBlue = EEPROM.read(2);   // Read blue value from address 2

    // If no valid color saved (all zeros), default to red
    if (savedRed == 0 && savedGreen == 0 && savedBlue == 0) {
        savedRed = 255; savedGreen = 0; savedBlue = 0;
        Serial.println(F("Default color: Red"));
    } else {
        Serial.print(F("Loaded color - R:"));
        Serial.print(savedRed);
        Serial.print(F(" G:"));
        Serial.print(savedGreen);
        Serial.print(F(" B:"));
        Serial.println(savedBlue);
    }

    // Initialize hardware pins
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Button with internal pull-up resistor
    pinMode(AUDIO_PIN, OUTPUT);         // Audio output pin
    digitalWrite(AUDIO_PIN, LOW);       // Start with audio off

    Serial.print(F("SD Card: "));

    // Initialize SD card with retry logic
    SPI.begin();  // Initialize SPI bus

    // Try to initialize SD card up to 3 times
    for (int attempt = 0; attempt < 3; attempt++) {
        if (SD.begin(SD_CS_PIN)) {
            sdCardAvailable = true;  // SD card successfully initialized
            Serial.println(F("OK"));
            break;
        }
        delay(100);
    }

    // Handle SD card failure
    if (!sdCardAvailable) {
        Serial.println(F("FAIL - Continuing without sound"));
    }

    // Initialize audio system if SD card is available
    if (sdCardAvailable) {
        tmrpcm.speakerPin = AUDIO_PIN;  // Set audio output pin
        tmrpcm.setVolume(4);            // Set volume level (0-7 scale)
    }

    Serial.print(F("MPU6050: "));

    // Initialize MPU6050 accelerometer
    Wire.begin();           // Initialize I2C communication
    Wire.setClock(400000);  // Set I2C clock speed to 400kHz (fast mode)

    // Reset MPU6050 to ensure clean startup
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B);  // PWR_MGMT_1 register
    Wire.write(0x80);  // Set reset bit
    Wire.endTransmission();
    delay(100);

    // Configure MPU6050 for normal operation
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B);  // PWR_MGMT_1 register
    Wire.write(0x00);  // Wake up and use internal oscillator
    Wire.endTransmission();

    // Configure accelerometer range (±8g)
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x1C);  // ACCEL_CONFIG register
    Wire.write(0x10);  // Set to ±8g range
    Wire.endTransmission();

    Serial.println(F("OK"));

    // Initialize state variables
    soundState = SOUND_NONE;      // Start with no sound playing
    inColorChangeMode = false;    // Not in color change mode initially

    Serial.println(F("System Ready"));  // Startup complete
}

/**
 * Safely plays a sound file with error checking and state management
 * @param soundName Name of the sound file to play
 * @param minDuration Minimum time the sound should play (ms)
 * @return true if sound started playing, false otherwise
 */
bool playSoundSafe(const char* soundName, unsigned long minDuration = 0) {
    // Check if SD card is available
    if (!sdCardAvailable) {
        return false;
    }

    // Don't play if another sound is already playing
    if (soundState == SOUND_PLAYING) {
        return false;
    }

    // Check if the sound file exists on SD card
    if (!SD.exists(soundName)) {
        Serial.print(F("Missing sound file: "));
        Serial.println(soundName);
        return false;
    }

    // Stop any currently playing sound to ensure clean start
    if (tmrpcm.isPlaying()) {
        tmrpcm.disable();  // Stop audio playback
        delay(30);         // Brief pause for cleanup
    }

    // Play the requested sound file
    tmrpcm.play(soundName);
    soundState = SOUND_PLAYING;  // Update sound state

    // Record timing information for sound management
    soundStartTime = millis();       // Record when sound started
    soundMinDuration = minDuration;  // Set minimum play time

    Serial.print(F("Playing sound: "));
    Serial.println(soundName);

    return true;  // Sound successfully started
}

/**
 * Handles motion detection and triggers appropriate sound effects
 * Uses accelerometer data to detect saber movements
 */
void handleMotionSounds() {
    static unsigned long lastMotionCheck = 0;  // Last time motion was checked
    static int16_t lastAccel = 0;              // Previous acceleration magnitude

    unsigned long currentTime = millis();  // Current time

    // Don't detect motion during color change mode
    if (inColorChangeMode) return;

    // Check motion less frequently for stability (10Hz maximum)
    if (currentTime - lastMotionCheck < 100) return;
    lastMotionCheck = currentTime;

    // Skip motion detection during grace period after activation
    if (currentTime - saberActivationTime < ACTIVATION_GRACE_PERIOD) return;

    // Don't trigger new sounds if one is already playing
    if (soundState == SOUND_PLAYING) return;

    // Read accelerometer data from MPU6050
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B);  // Starting register for accelerometer data (ACCEL_XOUT_H)

    if (Wire.endTransmission(false) != 0) return;  // Send repeated start, check for error

    // Request 6 bytes of accelerometer data (X, Y, Z - each 16-bit)
    if (Wire.requestFrom(MPU6050_ADDR, 6) != 6) return;

    // Read accelerometer values (16-bit signed, big-endian format)
    ax = Wire.read() << 8 | Wire.read();  // X-axis acceleration
    ay = Wire.read() << 8 | Wire.read();  // Y-axis acceleration
    az = Wire.read() << 8 | Wire.read();  // Z-axis acceleration

    // Calculate acceleration magnitude (simplified - sum of squares)
    int32_t accelMag = (int32_t)ax * ax + (int32_t)ay * ay + (int32_t)az * az;
    int16_t currentAccel = accelMag >> 16;  // Approximate magnitude (scaled down)

    // Calculate change in acceleration (movement intensity)
    int16_t delta = abs(currentAccel - lastAccel);
    lastAccel = currentAccel;  // Store for next comparison

    // Trigger sounds based on motion intensity and cooldown
    if (delta > 30 && currentTime - lastSwingTime > SWING_COOLDOWN) {
        if (delta > 80) {
            // Strong motion - play combat sounds
            if (random(2) == 0) {
                playSoundSafe("hit.wav", 400);  // Hit sound
            } else {
                playSoundSafe("cls.wav", 400);  // Clash sound
            }
        } else {
            // Gentle motion - play swing sounds
            if (random(2) == 0) {
                playSoundSafe("sw1.wav", 300);  // Swing sound 1
            } else {
                playSoundSafe("sw2.wav", 300);  // Swing sound 2
            }
        }
        lastSwingTime = currentTime;  // Record time of last swing sound
    }
}

/**
 * Manages sound system state and handles sound completion
 */
void updateSoundSystem() {
    unsigned long currentTime = millis();

    // Don't update sounds during color change mode
    if (inColorChangeMode) return;

    // Handle sound completion
    if (soundState == SOUND_PLAYING) {
        // Check if sound has finished playing
        if (!tmrpcm.isPlaying()) {
            soundState = SOUND_NONE;  // Mark sound as completed
        }
    }
}

/**
 * Performs memory management and cleanup tasks
 * Monitors free RAM and performs cleanup if necessary
 */
void performCleanup() {
    unsigned long currentTime = millis();

    // Periodically check and display free memory
    if (currentTime - lastMemoryCheck > MEMORY_CHECK_INTERVAL) {
        lastMemoryCheck = currentTime;
        Serial.print(F("Free RAM: "));
        Serial.print(freeMemory());
        Serial.println(F(" bytes"));
    }

    // Perform emergency cleanup if memory is critically low
    if (freeMemory() < 300) {
        Serial.println(F("LOW MEMORY - Performing cleanup"));

        // Stop any playing sounds to free memory
        if (tmrpcm.isPlaying()) {
            tmrpcm.disable();  // Stop audio playback
            delay(50);         // Wait for cleanup
        }

        // Clear serial input buffer to free memory
        while (Serial.available()) Serial.read();
    }
}

/**
 * Main program loop - runs continuously
 * Handles all major system functions in sequence
 */
void loop() {
    unsigned long currentMillis = millis();  // Get current time

    // 1. Memory management first (highest priority)
    performCleanup();

    // 2. Button handling - detect presses and state changes
    int buttonState = digitalRead(BUTTON_PIN);

    // Detect button press (falling edge - HIGH to LOW transition)
    if (buttonState == LOW && lastButtonState == HIGH) {
        // Button was just pressed
        buttonPressStartTime = currentMillis;  // Record press time
        buttonPressed = true;                  // Set pressed flag
        longPressDetected = false;             // Reset long press flag
        inColorChangeMode = false;             // Reset color change mode flag
    }

    // Check for long press to enter color change mode (while button is still held)
    if (buttonPressed && !longPressDetected && !inColorChangeMode &&
        (currentMillis - buttonPressStartTime > 3000)) {
        longPressDetected = true;  // Mark long press detected
        if (saberState == 1) {
            inColorChangeMode = true;  // Enter color change mode
            saberState = 2;            // Update system state
            Serial.println(F("Color change mode - HOLD to cycle colors, RELEASE to select"));
        }
        }

        // Handle button release (rising edge - LOW to HIGH transition)
        if (buttonState == HIGH && lastButtonState == LOW) {
            if (buttonPressed) {
                if (inColorChangeMode) {
                    // Button released while in color change mode = select current color
                    Serial.println(F("Color SELECTED"));
                    saveCurrentColor();         // Save color to EEPROM
                    inColorChangeMode = false;  // Exit color change mode
                    saberState = 1;             // Return to ON state
                    // Update LEDs immediately with selected color
                    lightEntireStrip(savedRed, savedGreen, savedBlue);
                }
                else if (!longPressDetected) {
                    // Short press = toggle saber on/off
                    Serial.println(F("Button RELEASED - Short press"));
                    if (saberState == 0) {
                        // Turn saber ON
                        playSoundSafe("ign.wav", 1200);  // Play ignition sound
                        activateSaber();                 // Activate saber
                    } else if (saberState == 1) {
                        // Turn saber OFF
                        // Force stop any currently playing sound and play power-off sound
                        if (sdCardAvailable) {
                            if (tmrpcm.isPlaying()) {
                                tmrpcm.disable();  // Stop current sound
                            }
                            soundState = SOUND_NONE;         // Reset sound state
                            playSoundSafe("pou.wav", 1200);  // Play power-off sound
                        }
                        deactivateSaber();  // Deactivate saber
                    }
                }
            }
            // Reset button state flags
            buttonPressed = false;
            longPressDetected = false;
        }

        lastButtonState = buttonState;  // Store current state for next comparison

        // 3. Sound system update (skip during color change mode)
        if (sdCardAvailable && !inColorChangeMode) {
            updateSoundSystem();
        }

        // 4. Saber state handling - different behaviors for each state
        switch (saberState) {
            case 0: // OFF state - do nothing
                break;

            case 1: // ON state - handle motion-activated sounds
                if (sdCardAvailable) {
                    handleMotionSounds();
                }
                break;

            case 2: // COLOR CHANGE state - handle color cycling
                if (inColorChangeMode) {
                    handleColorChange(currentMillis);
                }
                break;
        }

        // 5. Small delay for system stability and power management
        delay(20);
}

/**
 * Handles color cycling in color change mode
 * Cycles through rainbow colors while button is held
 * @param currentTime Current time in milliseconds
 */
void handleColorChange(unsigned long currentTime) {
    static uint8_t hue = 0;  // Color wheel position (0-255)

    // Update color every 100ms for smooth animation
    if (currentTime - prevAnimationTime > 100) {
        prevAnimationTime = currentTime;
        hue += 4;  // Move through color wheel

        uint8_t r, g, b;  // RGB color components

        // Generate rainbow colors based on hue position
        if (hue < 85) {
            // Red to Green transition (hue 0-84)
            r = hue * 3;        // Red increases
            g = 255 - hue * 3;  // Green decreases
            b = 0;              // Blue off
        } else if (hue < 170) {
            // Green to Blue transition (hue 85-169)
            r = 255 - (hue - 85) * 3;  // Red decreases
            g = 0;                     // Green off
            b = (hue - 85) * 3;        // Blue increases
        } else {
            // Blue to Red transition (hue 170-255)
            r = 0;                      // Red off
            g = (hue - 170) * 3;        // Green increases
            b = 255 - (hue - 170) * 3;  // Blue decreases
        }

        // Scale down brightness for preview (save power)
        r = r / 4;
        g = g / 4;
        b = b / 4;

        // Update LEDs with new preview color
        lightEntireStrip(r, g, b);

        // Store the selected color
        savedRed = r;
        savedGreen = g;
        savedBlue = b;

        // Debug output for color preview
        Serial.print(F("Color Preview - R:"));
        Serial.print(r);
        Serial.print(F(" G:"));
        Serial.print(g);
        Serial.print(F(" B:"));
        Serial.println(b);
    }
}

/**
 * Activates the lightsaber with smooth animation and sound
 */
void activateSaber() {
    Serial.println(F("Activating lightsaber"));

    // Ensure clean start for LEDs - disable interrupts for precise timing
    noInterrupts();
    sendReset();  // Assembly function - send reset signal to LEDs
    interrupts();

    // Wait a bit for the activation sound to start playing
    delay(100);

    // Smooth activation animation synchronized with sound
    // Gradually increase brightness from 0 to maximum
    for (int i = 0; i <= BRIGHTNESS; i += 2) {
        // Calculate current color based on brightness level
        uint8_t r = map(i, 0, BRIGHTNESS, 0, savedRed);
        uint8_t g = map(i, 0, BRIGHTNESS, 0, savedGreen);
        uint8_t b = map(i, 0, BRIGHTNESS, 0, savedBlue);

        // Update all LEDs with current brightness level
        lightEntireStrip(r, g, b);
        delay(15);  // Control animation speed
    }

    saberState = 1;  // Update system state to ON
    saberActivationTime = millis();  // Record activation time for grace period
}

/**
 * Deactivates the lightsaber with smooth animation and sound
 */
void deactivateSaber() {
    Serial.println(F("Deactivating lightsaber"));

    // Smooth deactivation animation synchronized with sound
    // Gradually decrease brightness from maximum to 0
    for (int i = BRIGHTNESS; i >= 0; i -= 2) {
        // Calculate current color based on brightness level
        uint8_t r = map(i, 0, BRIGHTNESS, 0, savedRed);
        uint8_t g = map(i, 0, BRIGHTNESS, 0, savedGreen);
        uint8_t b = map(i, 0, BRIGHTNESS, 0, savedBlue);

        // Update all LEDs with current brightness level
        lightEntireStrip(r, g, b);
        delay(15);  // Control animation speed
    }

    // Ensure LEDs are properly reset and turned off
    noInterrupts();
    for (int i = 0; i < 3; i++) sendReset();  // Multiple resets for reliability
    interrupts();

    // Update system state
    saberState = 0;              // Set state to OFF
    soundState = SOUND_NONE;     // Reset sound state
    inColorChangeMode = false;   // Reset color change mode
}

/**
 * Lights all LEDs in the strip with the specified color
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 */
void lightEntireStrip(uint8_t r, uint8_t g, uint8_t b) {
    // Critical section - disable interrupts for precise LED timing
    noInterrupts();

    // Send the same color to every LED in the strip
    for (int i = 0; i < NUM_LEDS; i++) {
        sendColor(r, g, b);  // Assembly function - send color data
    }

    interrupts();  // Re-enable interrupts
    sendReset();   // Assembly function - latch the data to update LEDs
}

/**
 * Saves the current color to EEPROM for persistence
 * Color will be remembered after power cycle
 */
void saveCurrentColor() {
    EEPROM.write(0, savedRed);    // Save red component to address 0
    EEPROM.write(1, savedGreen);  // Save green component to address 1
    EEPROM.write(2, savedBlue);   // Save blue component to address 2
    Serial.println(F("Color saved to EEPROM"));

    // Display saved color for confirmation
    Serial.print(F("Saved color - R:"));
    Serial.print(savedRed);
    Serial.print(F(" G:"));
    Serial.print(savedGreen);
    Serial.print(F(" B:"));
    Serial.println(savedBlue);
}
