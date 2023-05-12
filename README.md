# SupabaseESP

Simple library to connect ESP32/8266 to Supabase via REST API, including user authentication - forked from [jhavgas's original repo](https://github.com/jhagas/ESP32-Supabase) and trimmed down for simplicity.

## Installation

CLone this repo and copy the `src` directory - consisting of a single `Supabase.h` and `Supabase.cpp` - to your local arduino library directory.

In the same directory, create a `Secrets.h` file that contain your

- wifi ssid
- wifi password
- Supabase authenticated user name       (optional - when using an RLS table)
- Supabase authenticated user password   (optional - when using an RLS table)

`Secrets.h` should take the following form

```
#ifndef Secrets
#define Secrets

#define WIFI_SSID <your_wifi_ssid>
#define WIFI_PASSWORD <your_wifi_password>
#define SUPABASE_URL <your_supabase_url>
#define SUPABASE_ANON_KEY <your_supabase_anon_key>
#define SUPABASE_USER_EMAIL <your_supabase_authenticated_user_name>
#define SUPABASE_USER_PASSWORD <your_supabased_authenticated_user_password>

#endif

```

Remember to include `Secrets.h` in your `.gitignore` file - so that you do not accidently push secrets to a git repository!

## Example usage

An example `main.cpp` file using the library and `Secrets.h` is given below


```
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFi.h>
#include <Wifi.h>
#include <ArduinoJson.h>
#include <Supabase.h>
#include <Secrets.h>

Supabase db;

// define supabase credentials from constants stored in Secrets.h
const char *supabase_url = SUPABASE_URL;
const char *anon_key = SUPABASE_ANON_KEY;

const char *email = SUPABASE_USER_EMAIL;           // optional - only if using RLS table
const char *password = SUPABASE_USER_PASSWORD;     // optional - only if using RLS table

// define Wifi credentials from constants stored in Secrets.h
const char *ssid = WIFI_SSID;
const char *psswd = WIFI_PASSWORD;


void setup()
{
  
  Serial.begin(9600);

  // Connecting to Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, psswd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println("Connected!");

  // Logging in an authenticated user - for auth protected RLS tables
  db.login_email(email, password);
  
  // Create Supabase Connection
  db.begin(supabase_url, anon_key);

  // select from an non-RLS table "test"
  String read = db.from("test").select("*").doSelect();
  Serial.println(read);

  // insert into an RLS table "test_auth"
  DynamicJsonDocument doc(1024);
  doc["value"] = 6;
  String jsonStr;
  serializeJson(doc, jsonStr);
  bool upsert = false;
  String table = "test_auth";
  int status_code = db.insert(table, jsonStr, upsert);
  Serial.println(status_code);

  // Reset Your Query before doing everything else
  db.urlQuery_reset();

}

void loop()
{
  delay(10);
}

```


