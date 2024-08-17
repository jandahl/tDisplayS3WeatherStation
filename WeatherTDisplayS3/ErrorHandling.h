#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <TFT_eSPI.h>
#include "theme.h"

class ErrorHandler {
public:
    static void displayError(TFT_eSprite &sprite, const String &message);
    static void logError(const String &errorMessage);
};

#endif
