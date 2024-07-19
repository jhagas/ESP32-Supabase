#include "ESP-Supabase-Realtime.h"

// Internal functions
String getEventTable(JsonDocument result)
{
  String table = result["payload"]["data"]["table"];
  return table;
}

String getEventType(JsonDocument result)
{
  String type = result["payload"]["data"]["type"];
  return type;
}

int SupabaseRealtime::_login_process()
{
  HTTPClient Loginhttps;
  WiFiClientSecure *clientLogin = new WiFiClientSecure();

  clientLogin->setInsecure();

  int httpCode;
  JsonDocument doc;
  String url = "https://" + hostname + "/auth/v1/token?grant_type=password";
  Serial.println("Beginning to login to " + url);

  if (Loginhttps.begin(*clientLogin, url))
  {
    Loginhttps.addHeader("apikey", key);
    Loginhttps.addHeader("Content-Type", "application/json");

    String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
    httpCode = Loginhttps.POST(query);

    if (httpCode > 0)
    {
      String data = Loginhttps.getString();
      deserializeJson(doc, data);
      if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty())
      {
        USER_TOKEN = doc["access_token"].as<String>();
        authTimeout = doc["expires_in"].as<int>() * 1000;
        Serial.println("Login Success");
      }
      else
      {
        Serial.println("Login Failed: Invalid access token in response");
      }
    }
    else
    {
      Serial.print("Login Failed : ");
      Serial.println(httpCode);
    }

    Loginhttps.end();
    delete clientLogin;
    clientLogin = NULL;

    loginTime = millis();
  }
  else
  {
    return -100;
  }

  return httpCode;
}

void SupabaseRealtime::listen(String table, String event, String filter, void (*func)(String))
{
  String configJSON;
  JsonDocument jsonRealtimeConfig;
  deserializeJson(jsonRealtimeConfig, config);

  jsonRealtimeConfig["payload"]["config"]["postgres_changes"][0]["table"] = table;
  jsonRealtimeConfig["payload"]["config"]["postgres_changes"][0]["event"] = event;

  if (filter != "")
  {
    jsonRealtimeConfig["payload"]["config"]["postgres_changes"][0]["filter"] = filter;
  }

  serializeJson(jsonRealtimeConfig, configJSON);

  String slug = "/realtime/v1/websocket?apikey=" + String(key) + "&vsn=1.0.0";

  // Server address, port and URL
  // 1st param: Enter your project URL there
  // 2nd param: I have no idea, but it does not work with others than 443
  // Port number found here: https://github.com/esp8266/Arduino/issues/1442
  // 3rd param: url containing your anon key
  webSocket.beginSSL(
      hostname.c_str(),
      443,
      slug.c_str());

  // event handler
  webSocket.onEvent(std::bind(&SupabaseRealtime::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, configJSON, func));
}

void SupabaseRealtime::processMessage(uint8_t *payload, void (*func)(String))
{
  JsonDocument result;
  deserializeJson(result, payload);
  String table = getEventTable(result);
  if (table != "null")
  {
    String data = result["payload"]["data"]["record"];
    func(data);
  };
}

void SupabaseRealtime::webSocketEvent(WStype_t type, uint8_t *payload, size_t length, String configJSON, void (*func)(String))
{
  String configAUTH;
  JsonDocument authConfig;
  deserializeJson(authConfig, tokenConfig);
  authConfig["payload"]["access_token"] = USER_TOKEN;
  serializeJson(authConfig, configAUTH);

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!");
    break;
  case WStype_CONNECTED:
    Serial.println("[WSc] Connected!");
    webSocket.sendTXT(configJSON);
    webSocket.sendTXT(configAUTH);
    break;
  case WStype_TEXT:
    processMessage(payload, func);
    break;
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    break;
  case WStype_ERROR:
    Serial.printf("[WSc] Error: %s\n", payload);
    break;
  }
}

void SupabaseRealtime::loop()
{
  if (useAuth && millis() - loginTime > authTimeout / 2)
  {
    webSocket.disconnect();
    _login_process();
  }
  else
  {
    webSocket.loop();
  }

  if (millis() - last_ms > 30000)
  {
    last_ms = millis();
    webSocket.sendTXT(jsonRealtimeHeartbeat);

    String configJSON;
    JsonDocument authConfig;
    deserializeJson(authConfig, tokenConfig);

    authConfig["payload"]["access_token"] = USER_TOKEN;
    serializeJson(authConfig, configJSON);
    webSocket.sendTXT(configJSON);
  }
}

void SupabaseRealtime::begin(String hostname, String key)
{
  hostname.replace("https://", "");
  this->hostname = hostname;
  this->key = key;
}

int SupabaseRealtime::login_email(String email_a, String password_a)
{
  useAuth = true;
  loginMethod = "email";
  phone_or_email = email_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}

int SupabaseRealtime::login_phone(String phone_a, String password_a)
{
  useAuth = true;
  loginMethod = "phone";
  phone_or_email = phone_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}