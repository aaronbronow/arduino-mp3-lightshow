/**
 * \file MP3Shield_Library_Demo.ino
 *
 * \brief Example sketch of using the MP3Shield Arduino driver
 * \remarks comments are implemented with Doxygen Markdown format
 *
 * \author Bill Porter
 * \author Michael P. Flaga
 *
 * This sketch listens for commands from a serial terminal (like the Serial
 * Monitor in the Arduino IDE). If it sees 1-9 it will try to play an MP3 file
 * named track00x.mp3 where x is a number from 1 to 9. For eaxmple, pressing
 * 2 will play 'track002.mp3'. A lowe case 's' will stop playing the mp3.
 * 'f' will play an MP3 by calling it by it's filename as opposed to a track
 * number.
 *
 * Sketch assumes you have MP3 files with filenames like "track001.mp3",
 * "track002.mp3", etc on an SD card loaded into the shield.
 */

#include <SPI.h>
#include <Event.h>
#include <Timer.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h>

//and the MP3 Shield Library
#include <SFEMP3Shield.h>

// Below is not needed if interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_Timer1
#include <TimerOne.h>
#elif defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer
#include <SimpleTimer.h>
#endif

/**
 * \brief Object instancing the SdFat library.
 *
 * principal object for handling all SdCard functions.
 */
SdFat sd;

/**
 * \brief Object instancing the SFEMP3Shield library.
 *
 * principal object for handling all the attributes, members and functions for the library.
 */
SFEMP3Shield MP3player;
Timer t;
int timerEvent;
int laserEvent;
int ledEvent;
int laser = 5;
int led = 9;
boolean isLightShowPlaying;
boolean isAutoStartEnabled = true;

//------------------------------------------------------------------------------
/**
 * \brief Setup the Arduino Chip's feature for our use.
 *
 * After Arduino's kernel has booted initialize basic features for this
 * application, such as Serial port and MP3player objects with .begin.
 * Along with displaying the Help Menu.
 *
 * \note returned Error codes are typically passed up from MP3player.
 * Whicn in turns creates and initializes the SdCard objects.
 *
 * \see
 * \ref Error_Codes
 */
void setup() {
  
  pinMode( 2, OUTPUT);
  pinMode( 3, OUTPUT);
  pinMode( 4, OUTPUT);
  pinMode(laser, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode( 6, OUTPUT);
  pinMode( 7, OUTPUT);
  pinMode( 8, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  isLightShowPlaying = false;
  
  uint8_t result; //result code from some function as to be tested at later time.

  Serial.begin(115200);

  Serial.print(F("Free RAM = ")); // available in Version 1.0 F() bases the string to into Flash, to use less SRAM.
  Serial.print(FreeRam(), DEC);  // FreeRam() is provided by SdFatUtil.h
  Serial.println(F(" Should be a base line of 1040, on ATmega328 when using INTx"));


  //Initialize the SdCard.
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
  if(!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to start MP3 player"));
    if( result == 6 ) {
      Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
      Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
    }
  }

#if (0)
  // Typically not used by most shields, hence commented out.
  Serial.println(F("Applying ADMixer patch."));
  if(MP3player.ADMixerLoad("admxster.053") == 0) {
    Serial.println(F("Setting ADMixer Volume."));
    MP3player.ADMixerVol(-3);
  }
#endif

}

//------------------------------------------------------------------------------
/**
 * \brief Main Loop the Arduino Chip
 *
 * This is called at the end of Arduino kernel's main loop before recycling.
 * And is where the user's serial input of bytes are read and analyzed by
 * parsed_menu.
 *
 * Additionally, if the means of refilling is not interrupt based then the
 * MP3player object is serviced with the availaible function.
 *
 * \note Actual examples of the libraries public functions are implemented in
 * the parse_menu() function.
 */
void loop() {

  // Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
  && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )

    MP3player.available();
#endif

  if(Serial.available()) {
    parse_menu(Serial.read()); // get command from serial input
  }

  t.update();
  
  if(isLightShowPlaying == false && isAutoStartEnabled == true) {
    lightShow_Start();
    isAutoStartEnabled = false;
  }

  //delay(100);
}

void lightShow_Start() {
  isLightShowPlaying = true;
  Serial.println("Starting light show sequence...");
  
  uint8_t result;
  uint32_t offset = 0;
  long stage1 = 7000; // lasers warm up
  long stage2 = 27500; // led on
  long stage3 = 40000; // lasers on
  long stage4 = 44000; // pause all
  long stage5 = 45000; // resume all
  long stage6 = 58000; // led off
  long stage7 = 110000; // lasers off
  long stage8 = 140000; // start second track
  
  //create a string with the filename
  char trackName[] = "track001.mp3";

  Serial.println("Playing audio");
  //tell the MP3 Shield to play that file
  result = MP3player.playMP3(trackName, offset);
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to play track"));
  }
  
  int stage1Event = t.after(stage1, lightShow_LasersWarmup);
  int stage2Event = t.after(stage2, lightShow_LEDOn);
  int stage3Event = t.after(stage3, lightShow_LasersOn);
  int stage4Event = t.after(stage4, lightShow_Pause);
  int stage5Event = t.after(stage5, lightShow_LEDOn);
  int stage5Event2 = t.after(stage5, lightShow_LasersOn);
  int stage6Event = t.after(stage6, lightShow_LEDOff);
  int stage7Event = t.after(stage7, lightShow_Stop);
  int stage8Event = t.after(stage8, lightShow_PlayTrack2);

  return;
}

void lightShow_LasersWarmup() {
  Serial.println("Warming up lasers");
  laserFlicker(3);
  delay(1000);
  laserFlicker(2);
  delay(500);
  laserFlicker(5);
  delay(200);
  laserFlicker(2);
  delay(500);
  laserFlicker(2);
  delay(50);
  digitalWrite(laser, HIGH);
  delay(1000);
  laserFlicker(3);
  delay(500);
  laserFlicker(3);
  digitalWrite(laser, HIGH);
  laserFlicker(3);
  digitalWrite(laser, HIGH);
}

void lightShow_LasersOn() {
  Serial.println("Lasers ON");
  digitalWrite(laser, HIGH);
}

void lightShow_LEDOn() {
  Serial.println("LED ON");
  digitalWrite(led, HIGH);
}

void lightShow_LEDOff() {
  Serial.println("LED OFF");
  digitalWrite(led, LOW);
}

void lightShow_Pause() {
  Serial.println("Pausing light show");
  digitalWrite(laser, LOW);
  digitalWrite(led, LOW);
}

void lightShow_Stop() {
  isLightShowPlaying = false;
  Serial.println("Stopping light show");
  digitalWrite(laser, LOW);
  digitalWrite(led, LOW);
}

void lightShow_PlayTrack2() {
  uint8_t result;
  uint32_t offset = 0;
  char trackName[] = "track002.mp3";
  Serial.println("Playing audio");
  //tell the MP3 Shield to play that file
  result = MP3player.playMP3(trackName, offset);
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to play track"));
  }
}


//------------------------------------------------------------------------------
/**
 * \brief Blink the laser pin in a random-looking way.
 *
 * Flash on and off [pulses] number of times.
 */
void laserFlicker(int pulses) {
  for(int i = 0; i < pulses; i++) {
    digitalWrite(laser, HIGH);
    delay(25);
    digitalWrite(laser, LOW);
    delay(50);
    digitalWrite(laser, HIGH);
    delay(25);
    digitalWrite(laser, LOW);
    delay(50);
    digitalWrite(laser, HIGH);
    delay(25);  
    digitalWrite(laser, LOW);  
    delay(150);
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Decode the Menu.
 *
 * Parses through the characters of the users input, executing corresponding
 * MP3player library functions and features then displaying a brief menu and
 * prompting for next input command.
 */
void parse_menu(byte key_command) {
  
  if(key_command == 'v') {
    lightShow_Start();
  }
  
}



