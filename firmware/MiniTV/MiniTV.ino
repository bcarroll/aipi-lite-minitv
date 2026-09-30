#include "button.h"
#include "display.h"

namespace {

Display display;
Button button;

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("AI-PI Lite MiniTV starting");
  if (!display.begin()) {
    Serial.println("display: initialization failed");
  }
  if (!button.begin()) {
    Serial.println("button: initialization failed");
  }
  Serial.println("storage: disabled pending partition compatibility evidence");
}

void loop() {
  const ButtonEvent event = button.update(millis());
  switch (event) {
    case ButtonEvent::ShortPress:
      display.showStatus("Controls", "Next channel");
      break;
    case ButtonEvent::DoublePress:
      display.showStatus("Controls", "Previous channel");
      break;
    case ButtonEvent::LongPress:
      display.showStatus("Controls", "Mute toggle");
      break;
    case ButtonEvent::None:
      break;
  }
}
