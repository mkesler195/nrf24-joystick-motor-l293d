#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

const uint8_t DF_RX_PIN = 7;  // DF TX -> D7
const uint8_t DF_TX_PIN = 8;  // D8 -> 1k -> DF RX

SoftwareSerial dfSerial(DF_RX_PIN, DF_TX_PIN);
DFRobotDFPlayerMini dfPlayer;

void setup() {
  Serial.begin(115200);
  Serial.println("DFPlayer slow-repeat test starting...");

  dfSerial.begin(9600);

  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer init FAILED.");
    while (true) { delay(1000); }
  }

  Serial.println("DFPlayer init OK.");

  dfPlayer.volume(16);                 // 0..30
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);

  delay(500);

  Serial.println("Play 1");
  dfPlayer.playMp3Folder(1);           // /MP3/0001.mp3
}

void loop() {
  delay(12000);                        // long gap (try 12–20s)

  Serial.println("Stop + Play 1");
  dfPlayer.stop();
  delay(300);                          // settle time
  dfPlayer.playMp3Folder(1);
}