/*
  Based on the orginal code of Gutenbird by Adafruit Industries and modified by Federico Vanzati - Officine Arduino (tweets fetching and json parsing)
  Modified by Mirco Piccin aka pitusso (full working code for both WiFi and Ethernet Shield, thermal printing)
  Released with MIT license.
 
  Required hardware includes a WiFi Shield or an Ethernet Shield connected to the Arduino UNO board 

  Used Thermal Printer is a Citizen CBM-231
  Lot of resources here: http://microprinter.pbworks.com/w/page/20867146/FrontPage
 */

//if using a W5100 based Ethernet shield, comment out the following line; 
//leave untouched if using Arduino Wifi Shield
#define WIFI

#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

#ifdef WIFI
  #include <WiFi.h>
  #include <WiFiClient.h>
#else
  #include <Ethernet.h>
#endif

//printer commands (https://github.com/benosteen/microprinter/blob/master/microprinter.py)
const byte COMMAND = 0x1B;
const byte LF = 0x10;
const byte LINEFEED_RATE = 0x33;
const byte FULLCUT = 0x69; //[0x1B;0x69];
const byte PARTIALCUT = 0x6D; //[0x1B;0x6D];
const byte PRINT_MODE = 0x21;
const byte DOUBLEPRINT = 0x47;
const byte UNDERLINE = 0x2D;
const byte RESET = 0x40;
const byte COMMAND_IMAGE = 0x2A;
const byte COMMAND_FLIPCHARS = 0x7B;
const byte COMMAND_ROTATECHARS = 0x56;
const byte COMMAND_BARCODE = 0x1D;
const byte COMMAND_BARCODE_PRINT = 0x6B;
const byte COMMAND_BARCODE_WIDTH = 0x77;
const byte COMMAND_BARCODE_HEIGHT = 0x68;
const byte COMMAND_BARCODE_TEXTPOSITION = 0x48;
const byte COMMAND_BARCODE_FONT = 0x66;

const byte ON = 0x01;
const byte OFF = 0x00;

//CBMBARCODES
const byte BARCODE_WIDTH_NARROW = 0x02;
const byte BARCODE_WIDTH_MEDIUM = 0x03;
const byte BARCODE_WIDTH_WIDE = 0x04;
const byte BARCODE_TEXT_NONE = 0x00;
const byte BARCODE_TEXT_ABOVE = 0x01;
const byte BARCODE_TEXT_BELOW = 0x02;
const byte BARCODE_TEXT_BOTH = 0x03;
const byte BARCODE_MODE_UPCA = 0x00;
const byte BARCODE_MODE_UPCE = 0x01;
const byte BARCODE_MODE_JAN13AEN = 0x02;
const byte BARCODE_MODE_JAN8EAN = 0x03;
const byte BARCODE_MODE_CODE39 = 0x04;
const byte BARCODE_MODE_ITF = 0x05;
const byte BARCODE_MODE_CODEABAR = 0x06;
const byte BARCODE_MODE_CODE128 = 0x07;


//Software Serial
#define rxPin 2
#define txPin 3

SoftwareSerial printer =  SoftwareSerial(rxPin, txPin);

//network configuration, WIRED or WIFI
#ifdef WIFI
  // Enter your WiFi network settings
  char ssid[] = "your_ssid"; //  your network SSID (name) 
  char pass[] = "your_password";    // your network password (use for WPA, or use as key for WEP)

  int keyIndex = 0;            // your network key Index number (needed only for WEP)
  int status = WL_IDLE_STATUS; // status of the wifi connection
  WiFiClient client;
#else
  //if using WIRED
  byte mac[] = {
  0x90, 0xA2, 0xDA, 0x00, 0x69, 0xD5};

  // fill in an available IP address on your network here,
  // for auto configuration:
  IPAddress ip(192, 168, 0, 10);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dnss(8, 8, 8, 8);
  IPAddress gw(192, 168, 0, 1);

  EthernetClient client;
#endif

const int maxTweets = 1;  // Limit tweets printed; avoid runaway output

const unsigned long // Time limits, expressed in milliseconds:
pollingInterval = 10L * 1000L, // Note: Twitter server will allow 150/hr max
connectTimeout  = 15L * 1000L, // Max time to retry server link
responseTimeout = 15L * 1000L; // Max time to wait for data from server

byte resultsDepth; // Used in JSON parsing

//IPAddress serverName(199, 59, 148, 84);
char *serverName = "search.twitter.com";

// queryString can be any valid Twitter API search string, including
// boolean operators. See https://dev.twitter.com/docs/using-search
// for options and syntax. Funny characters do NOT need to be URL
// encoded here -- the sketch takes care of that.
char queryString[20] = "#hotprinter", 
lastId[21], // 18446744073709551615\0 (64-bit maxint as string)
timeStamp[32], // WWW, DD MMM YYYY HH:MM:SS +XXXX\0
fromUser[16], // Max username length (15) + \0
msgText[141], // Max tweet length (140) + \0
name[11], // Temp space for name:value parsing
value[141]; // Temp space for name:value parsing


// Function prototypes -------------------------------------------------------

boolean
jsonParse(int, byte),
readString(char *, int);
int
unidecode(byte),
timedRead(void);


// ---------------------------------------------------------------------------

void setup() {
  // Initialize the serial communication, used only for debuggging
  Serial.begin(9600);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  printer.begin(9600);

  //cut the paper
  printer.println("");
  printer.println(""); 
  printer.write(COMMAND);
  printer.write(FULLCUT);

  #ifdef WIFI
    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi shield not present"); 
      // don't continue:
      while(true);
    } 
  
    // attempt to connect to Wifi network:
    while ( status != WL_CONNECTED) { 
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. 
      status = WiFi.begin(ssid, pass);
  
      // wait 10 seconds for connection:
      delay(10000);
    } 
    Serial.println("Connected to wifi");
  #else
    if (!Ethernet.begin(mac)) {
      // if DHCP fails, start with a hard-coded address:
      Serial.println("Failed to get an IP address using DHCP, forcing manually");
      Ethernet.begin(mac, ip, dnss, gw, subnet);
    }
  #endif  

  printStatus();

  // Clear all string data
  memset(lastId , 0, sizeof(lastId));
  memset(timeStamp, 0, sizeof(timeStamp));
  memset(fromUser , 0, sizeof(fromUser));
  memset(msgText , 0, sizeof(msgText));
  memset(name , 0, sizeof(name));
  memset(value , 0, sizeof(value));
}

// ---------------------------------------------------------------------------

void loop() {
  unsigned long startTime, t;
  int i;
  char c;

  startTime = millis();

  // Attempt server connection, with timeout...
  Serial.print(F("Connecting to server..."));
  while((client.connect(serverName, 80) == false) &&
    ((millis() - startTime) < connectTimeout));

  if(client.connected()) { // Success!
    Serial.print(F("OK\r\nIssuing HTTP request..."));
    // URL-encode queryString to client stream:
    client.print(F("GET /search.json?q="));
    for(i=0; c=queryString[i]; i++) {
      if(((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        ((c >= '0') && (c <= '9')) ||
        (c == '-') || (c == '_') ||
        (c == '.') || (c == '~')) {
        // Unreserved char: output directly
        client.write(c);
      } 
      else {
        // Reserved/other: percent encode
        client.write('%');
        client.print(c, HEX);
      }
    }
    client.print(F("&result_type=recent&include_entities=false&rpp="));
    if(lastId[0]) {
      client.print(maxTweets); // Limit to avoid runaway printing
      client.print(F("&since_id=")); // Display tweets since prior query
      client.print(lastId);
    } 
    else {
      client.print('1'); // First run; show single latest tweet
    }
    client.print(F(" HTTP/1.1\r\nHost: "));
    client.println(serverName);
    client.println(F("Connection: close\r\n"));

    Serial.print(F("OK\r\nAwaiting results (if any)..."));
    t = millis();
    while((!client.available()) && ((millis() - t) < responseTimeout));
    if(client.available()) { // Response received?
      // Could add HTTP response header parsing here (400, etc.)
      if(client.find("\r\n\r\n")) { // Skip HTTP response header
        Serial.println(F("OK\r\nProcessing results..."));
        resultsDepth = 0;
        jsonParse(0, 0);
      } 
      else Serial.println(F("response not recognized."));
    } 
    else Serial.println(F("connection timed out."));
    client.stop();
  } 
  else { // Couldn't contact server
    Serial.println(F("failed"));
  }

  // Sometimes network access & printing occurrs so quickly, the steady-on
  // LED wouldn't even be apparent, instead resembling a discontinuity in
  // the otherwise smooth sleep throb. Keep it on at least 4 seconds.
  t = millis() - startTime;
  if(t < 4000L) delay(4000L - t);

  // Pause between queries, factoring in time already spent on network
  // access, parsing, printing and LED pause above.
  t = millis() - startTime;
  if(t < pollingInterval) {
    Serial.print(F("Pausing..."));
    delay(pollingInterval - t);
    Serial.println(F("done"));
  }
}

// ---------------------------------------------------------------------------

boolean jsonParse(int depth, byte endChar) {
  int c, i;
  boolean readName = true;

  for(;;) {
    while(isspace(c = timedRead())); // Scan past whitespace
    if(c < 0) return false; // Timeout
    if(c == endChar) return true; // EOD

    if(c == '{') { // Object follows
      if(!jsonParse(depth + 1, '}')) return false;
      if(!depth) return true; // End of file
      if(depth == resultsDepth) { // End of object in results list

        // Dump to serial console as well
        Serial.print(F("User: "));
        Serial.println(fromUser);
        Serial.print(F("Text: "));
        Serial.println(msgText);
        Serial.print(F("Time: "));
        Serial.println(timeStamp);
      
        printTweet();
        
        // Clear strings for next object
        timeStamp[0] = fromUser[0] = msgText[0] = 0;
      }
    } 
    else if(c == '[') { // Array follows
      if((!resultsDepth) && (!strcasecmp(name, "results")))
        resultsDepth = depth + 1;
      if(!jsonParse(depth + 1,']')) return false;
    } 
    else if(c == '"') { // String follows
      if(readName) { // Name-reading mode
        if(!readString(name, sizeof(name)-1)) return false;
      } 
      else { // Value-reading mode
        if(!readString(value, sizeof(value)-1)) return false;
        // Process name and value strings:
        if (!strcasecmp(name, "max_id_str")) {
          strncpy(lastId, value, sizeof(lastId)-1);
        } 
        else if(!strcasecmp(name, "created_at")) {
          strncpy(timeStamp, value, sizeof(timeStamp)-1);
        } 
        else if(!strcasecmp(name, "from_user")) {
          strncpy(fromUser, value, sizeof(fromUser)-1);
          delay(5000);

        }
        else if(!strcasecmp(name, "text")) {
          strncpy(msgText, value, sizeof(msgText)-1);
        }
      }
    } 
    else if(c == ':') { // Separator between name:value
      readName = false; // Now in value-reading mode
      value[0] = 0; // Clear existing value data
    } 
    else if(c == ',') {
      // Separator between name:value pairs.
      readName = true; // Now in name-reading mode
      name[0] = 0; // Clear existing name data
    } // Else true/false/null or a number follows. These values aren't
    // used or expected by this program, so just ignore...either a comma
    // or endChar will come along eventually, these are handled above.
  }
}

// ---------------------------------------------------------------------------

// Read string from client stream into destination buffer, up to a maximum
// requested length. Buffer should be at least 1 byte larger than this to
// accommodate NUL terminator. Opening quote is assumed already read,
// closing quote will be discarded, and stream will be positioned
// immediately following the closing quote (regardless whether max length
// is reached -- excess chars are discarded). Returns true on success
// (including zero-length string), false on timeout/read error.
boolean readString(char *dest, int maxLen) {
  int c, len = 0;

  while((c = timedRead()) != '\"') { // Read until closing quote
    if(c == '\\') { // Escaped char follows
      c = timedRead(); // Read it
      // Certain escaped values are for cursor control --
      // there might be more suitable printer codes for each.
      if (c == 'b') c = '\b'; // Backspace
      else if(c == 'f') c = '\f'; // Form feed
      else if(c == 'n') c = '\n'; // Newline
      else if(c == 'r') c = '\r'; // Carriage return
      else if(c == 't') c = '\t'; // Tab
      else if(c == 'u') c = unidecode(4);
      else if(c == 'U') c = unidecode(8);
      // else c is unaltered -- an escaped char such as \ or "
    } // else c is a normal unescaped char

    if(c < 0) return false; // Timeout

    // In order to properly position the client stream at the end of
    // the string, characters are read to the end quote, even if the max
    // string length is reached...the extra chars are simply discarded.
    if(len < maxLen) dest[len++] = c;
  }

  dest[len] = 0;
  return true; // Success (even if empty string)
}

// ---------------------------------------------------------------------------

// Read a given number of hexadecimal characters from client stream,
// representing a Unicode symbol. Return -1 on error, else return nearest
// equivalent glyph in printer's charset. (See notes below -- for now,
// always returns '-' or -1.)
int unidecode(byte len) {
  int c, v, result = 0;
  while(len--) {
    if((c = timedRead()) < 0) return -1; // Stream timeout
    if ((c >= '0') && (c <= '9')) v = c - '0';
    else if((c >= 'A') && (c <= 'F')) v = 10 + c - 'A';
    else if((c >= 'a') && (c <= 'f')) v = 10 + c - 'a';
    else return '-'; // garbage
    result = (result << 4) | v;
  }

  // To do: some Unicode symbols may have equivalents in the printer's
  // native character set. Remap any such result values to corresponding
  // printer codes. Until then, all Unicode symbols are returned as '-'.
  // (This function still serves an interim purpose in skipping a given
  // number of hex chars while watching for timeouts or malformed input.)

  return '-';
}

// ---------------------------------------------------------------------------

// Read from client stream with a 5 second timeout. Although an
// essentially identical method already exists in the Stream() class,
// it's declared private there...so this is a local copy.
int timedRead(void) {
  unsigned long start = millis();

  while((!client.available()) && ((millis() - start) < 5000L));

  return client.read(); // -1 on timeout
}

void printStatus() {
  #ifdef WIFI
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.print(" dBm");
  #else
    // print your local IP address:
    Serial.print("IP address: ");
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
      // print the value of each byte of the IP address:
      Serial.print(Ethernet.localIP()[thisByte], DEC);
      Serial.print("."); 
    }  
  #endif
  Serial.println();
}

void printTweet() {
        //BOLD  
        printer.write(COMMAND);
        printer.write(DOUBLEPRINT);
        printer.write(ON);
        printer.print(fromUser);
        printer.print(" ");
        printer.write(COMMAND);
        printer.write(DOUBLEPRINT);
        printer.write(OFF);

        //NORMAL
        printer.println(msgText);
        printer.println("");
        printer.println(""); 
}
