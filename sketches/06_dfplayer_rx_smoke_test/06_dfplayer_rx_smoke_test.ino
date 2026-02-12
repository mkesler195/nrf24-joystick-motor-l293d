#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// DFPlayer wiring on RX Nano:
// DFPlayer TX -> Nano D7
// DFPlayer RX <- Nano D8 (through 1k series resistor)
//
// DFPlayer power:
// VCC -> 5V
// GND -> GND (common ground with Nano)
//
// Speaker:
// SPK1 -> speaker lead
// SPK2 -> speaker other lead
// (Do NOT connect speaker to GND)
//
// Decoupling (recommended, close to DFPlayer):
// 100–470uF electrolytic across VCC/GND
// 0.1uF (100nF, "104") ceramic across VCC/GND
//
// SD card layout:
// /MP3/0001.mp3

const uint8_t DF_RX_PIN = 7;  // Nano receives (connect to DF TX)
const uint8_t DF_TX_PIN = 8;  // Nano transmits (to DF RX via 1k resistor)

SoftwareSerial dfSerial(DF_RX_PIN, DF_TX_PIN);
DFRobotDFPlayerMini dfPlayer;

void setup() {
  Serial.begin(115200);
  Serial.println("DFPlayer RX smoke test starting...");

  dfSerial.begin(9600);

  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer init FAILED.");
    Serial.println("Check wiring, SD card (FAT32), folder /MP3, file 0001.mp3, and power caps.");
    while (true) { delay(1000); }
  }

  Serial.println("DFPlayer init OK.");

  dfPlayer.volume(22);              // 0..30
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);

  delay(800);

  Serial.println("Playing /MP3/0001.mp3 ...");
  dfPlayer.playMp3Folder(1);        // plays /MP3/0001.mp3
}

void loop() {
  delay(5000);
  Serial.println("Playing again...");
  dfPlayer.playMp3Folder(1);
}
