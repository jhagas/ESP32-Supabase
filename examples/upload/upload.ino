
#include <ESP32_Supabase.h>
#include "SPIFFS.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

Supabase db;

// Put your supabase URL and Anon key here...
// Because Login already implemented, there's no need to use secretrole key
String supabase_url = "";
String anon_key = "";

// put your WiFi credentials (SSID and Password) here
const char *ssid = "";
const char *psswd = "";

// Put Supabase account credentials here
const String email = "";
const String password = "";

// Supabase Storage Bucket name here
const String bucket = "";

void setup()
{
  const String fileToUpload = "/supabase_test.txt";

  Serial.begin(115200);
  Serial.println("");

  // Start SPIFFS module.
  if (!SPIFFS.begin(true))
  {
    Serial.println("Error occurred when starting SPIFFS module.");
    return;
  }

  if (!SPIFFS.exists(fileToUpload))
  {
    File file = SPIFFS.open(fileToUpload, "w+", true);

    if (!file)
    {
      Serial.println("Error when creating file.");
      return;
    }

    // Prints 10600 bytes to the file.
    for (size_t i = 1; i <= 255; i++)
    {
      file.printf("Line %d in message from ESP32_Supabase.\n\r", i);
    }

    file.close();
  }

  // Open the file in read mode.
  File file = SPIFFS.open(fileToUpload);

  if (!file)
  {
    Serial.println("Error when opening file.");
    return;
  }

  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, psswd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println("Connected!");

  db.begin(supabase_url, anon_key);

  int loginResponse = db.login_email(email, password);

  if (loginResponse != 200)
  {
    Serial.printf("Login failed with code: %d.\n\r", loginResponse);
    return;
  }

  int uploadResponse = db.upload(bucket, "supabase_example_" + String(millis()) + ".txt", "text/plain", &file, file.size());

  if (uploadResponse == 200)
  {
    Serial.println("File succesfully created!");
  }
  else
  {
    Serial.printf("File upload failed with code: %d.\n\r", uploadResponse);
  }

  file.close();
}

void loop()
{
  delay(10000);
}
