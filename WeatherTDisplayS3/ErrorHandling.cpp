#include "ErrorHandling.h"

void ErrorHandler::displayError(TFT_eSprite &sprite, const String &message) {
    sprite.fillSprite(TFT_RED);
    sprite.setTextColor(TFT_WHITE, TFT_RED);
    sprite.drawString(message, 10, 10);
    sprite.pushSprite(0, 0);
}

void ErrorHandler::logError(const String &errorMessage) {
    Serial.println("ERROR: " + errorMessage);
}
