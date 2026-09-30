/*
 * AI-PI Lite MiniTV firmware entry point.
 *
 * Initial scaffold intentionally performs no hardware or flash initialization.
 * In particular, it leaves GPIO10 and the existing MicroPython data
 * filesystem untouched until the device partition map has been verified.
 */

void setup() {
  Serial.begin(115200);
  Serial.println("AI-PI Lite MiniTV safe scaffold");
  Serial.println("Filesystem and hardware initialization are not enabled yet.");
}

void loop() {
  // Components are added in later implementation steps.
}
