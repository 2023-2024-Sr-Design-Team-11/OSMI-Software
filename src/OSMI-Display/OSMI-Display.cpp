#include "OSMI-Display.h"
#include <Arduino.h>
#include <SPI.h>

PNG png; // PNG decoder instance

int16_t xpos = 50;
int16_t ypos = 150;

TFT_eSPI tft = TFT_eSPI();
bool enabled = false;

void DisplayTask(void *params)
{
    setupDisplay();
    QueueHandle_t *handle = (QueueHandle_t *)params;
    loopDisplay(handle);
}

void setupDisplay(void)
{

    tft.begin();

    tft.setRotation(0);

    if (!SPIFFS.begin())
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield(); // Stay here twiddling thumbs waiting
    }
    Serial.println("\r\nSPIFFS available!");

    // ESP32 will crash if any of the fonts are missing
    bool font_missing = false;
    if (SPIFFS.exists("/NotoSansBold15.vlw") == false)
        font_missing = true;
    if (SPIFFS.exists("/NotoSansBold36.vlw") == false)
        font_missing = true;

    if (font_missing)
    {
        Serial.println("\r\nFont missing in SPIFFS, did you upload it?");
        while (1)
            yield();
    }
    else
        Serial.println("\r\nFonts found OK.");

    tft.loadFont(AA_FONT_SMALL); // Must load the font first

    // Get ready for the next demo while we have this font loaded
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0); // Set cursor at top left of screen
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Device has been on for:");

    tft.unloadFont(); // Remove the font to recover memory used

    // IMAGE
    int16_t rc = png.openFLASH((uint8_t *)rose_logo, sizeof(rose_logo), pngDraw);
    String errorCode;
    switch (rc)
    {
    case PNG_INVALID_PARAMETER:
        errorCode = "Invalid Param";
        break;
    case PNG_DECODE_ERROR:
        errorCode = "Decode error.";
        break;
    case PNG_MEM_ERROR:
        errorCode = "Mem Error";
        break;
    case PNG_NO_BUFFER:
        errorCode = "No Buffer";
        break;
    default:
        errorCode = "Other Error " + String(rc);
    }
    Serial.println(errorCode);

    if (rc == PNG_SUCCESS)
    {
        Serial.println("Successfully opened png file");
        Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        tft.startWrite();
        uint32_t dt = millis();
        rc = png.decode(NULL, 0);
        Serial.print(millis() - dt);
        Serial.println("ms");
        tft.endWrite();
        png.close(); // not needed for memory->memory decode
        
    }

    Serial.print("Display() running on core ");
    Serial.println(xPortGetCoreID());
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[ROSE_LOGO_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);

    
}

void loopDisplay(QueueHandle_t *queue)
{

    while (true)
    {
        // Adding a parameter "true" to the setTextColor() function fills character background
        // This extra parameter is only for smooth fonts!
        tft.loadFont(AA_FONT_LARGE); // Load another different font
        tft.setTextColor(TFT_GREEN, TFT_BLACK, true);
        tft.setCursor(30, 30);
        tft.print(millis() / 1000.0, 1);
        tft.unloadFont();
        // delay (200);

        tft.loadFont(AA_FONT_SMALL);
        tft.setCursor(0, 80);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.println("Device Status:");
        tft.unloadFont();
        tft.loadFont(AA_FONT_SMALL);
        tft.setTextColor(TFT_GREEN, TFT_BLACK, true);
        tft.setCursor(30, 110);

        // Reads from Queue to toggle the stepper display.
        bool trash = false;
        if (xQueueReceive(*queue, &trash, 2) == pdTRUE) //if something in the queue, toggle.
        {
            enabled = !enabled;
        }

        String St1Stat = enabled ? "ON  " : "OFF"; // ON with spaces to clear out pixels.

        tft.println("STEPPER 1 " + St1Stat);
        delay(60);
    }
}