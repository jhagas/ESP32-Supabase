#include "ESP32_Supabase.h"

void Supabase::_check_last_string()
{
  unsigned int last = url_query.length() - 1;
  if (url_query[last] != '?')
  {
    url_query += "&";
  }
}

int Supabase::_login_process()
{
  int httpCode;
  StaticJsonDocument<1024> doc;
  Serial.println("Beginning to login..");

  if (https.begin(client, hostname + "/auth/v1/token?grant_type=password"))
  {
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");

    String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
    httpCode = https.POST(query);

  if (httpCode > 0)
  {
    String data = https.getString();
    deserializeJson(doc, data);
    if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty()) {
      USER_TOKEN = doc["access_token"].as<String>();
      authTimeout = doc["expires_in"].as<int>() * 1000;
      Serial.println("Login Success");
      Serial.println(USER_TOKEN);
      Serial.println(data);
    } else {
      Serial.println("Login Failed: Invalid access token in response");
    }
  }
  else
  {
    Serial.println(phone_or_email);
    Serial.println(password);

    Serial.print("Login Failed : ");
    Serial.println(httpCode);
  }

    https.end();
    loginTime = millis();
  }
  else
  {
    return -100;
  }

  return httpCode;
}

void Supabase::begin(String hostname_a, String key_a)
{
  client.setInsecure();
  hostname = hostname_a;
  key = key_a;
}
String Supabase::getQuery()
{
  String temp = url_query;
  urlQuery_reset();
  return hostname + "/rest/v1/" + temp;
}
// query reset
void Supabase::urlQuery_reset()
{
  url_query = "";
}
// membuat Query Builder
Supabase &Supabase::Supabase::from(String table)
{
  url_query += (table + "?");
  return *this;
}

int Supabase::insert(String table, String json, bool upsert)
{
  int httpCode;
  if (https.begin(client, hostname + "/rest/v1/" + table))
  {
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");

    String preferHeader = "return=representation";
    if (upsert)
    {
      preferHeader += ",resolution=merge-duplicates";
    }
    https.addHeader("Prefer", preferHeader);
    
    if (useAuth)
    {
      unsigned long t_now = millis();
      if (t_now - loginTime >= authTimeout)
      {
        _login_process();
      }
      https.addHeader("Authorization", "Bearer " + USER_TOKEN);
    }
    httpCode = https.POST(json);
    https.end();
  }
  else
  {
    return -100;
  }
  return httpCode;
}

Supabase &Supabase::select(String colls)
{
  url_query += ("select=" + colls);
  return *this;
}
Supabase &Supabase::update(String table)
{
  url_query += (table + "?");
  return *this;
}


int Supabase::upload(String bucket, String filename, String mime_type, uint8_t *buffer, uint32_t size)
{
  int httpCode = 0;

  // Gets protocol out of hostname.
  int index = hostname.indexOf("//");
  String hostname_no_prot = hostname.substring(index + 2, hostname.length());

  char hostname_char[hostname_no_prot.length() + 1];
  hostname_no_prot.toCharArray(hostname_char, hostname_no_prot.length() + 1);

  String finalPath = hostname + "/storage/v1/object/" + bucket + "/" + filename;

  // Make a HTTP request and add HTTP headers
  String boundary = "esp32-supabase-boundary";

  // Request Main Header
  String httpMainHeader = "POST " + finalPath + " HTTP/1.1\r\n";
  httpMainHeader += "Host: " + hostname_no_prot + "\r\n";
  httpMainHeader += "apikey: " + key + "\r\n";
  httpMainHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }

    httpMainHeader += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }

  httpMainHeader += "User-Agent: ESP32/Supabase\r\n";
  httpMainHeader += "Keep-Alive: 3600\r\n";
  httpMainHeader += "Connection: keep-alive\r\n";

  // request Content Header
  String contentHeader = "--" + boundary + "\r\n";
  contentHeader += "Content-Disposition: form-data; name=\"" + filename + "\"; filename=\"" + filename + "\"\r\n";
  contentHeader += "Content-Type: " + mime_type + "\r\n\r\n";

  // request Ending Header
  String endingHeader = "\r\n--" + boundary + "--\r\n\r\n";

  // content length
  int contentLength = contentHeader.length() + endingHeader.length();

  httpMainHeader += "Content-Length: " + String(contentLength + size) + "\n\n";

  Serial.printf("Hostname: %s\n\r", hostname_char);

  if (!client.connected())
  {
    // This needs further testing, might need to change to port 80.
    client.connect(hostname_char, 443);

    if (!client.connected())
    {
      return 0;
    }
  }

  // send post header
  size_t written = client.write((uint8_t *)httpMainHeader.c_str(), httpMainHeader.length());

  Serial.printf("HTTP Request:\n\r%s\n\r", httpMainHeader.c_str());
  Serial.printf("\n\r%s\n\r", contentHeader.c_str());
  Serial.printf("\n\r(file of %lu bytes)\n\r", size);
  Serial.printf("\n\r%s\n\r", endingHeader.c_str());

  written = client.write((uint8_t *)contentHeader.c_str(), contentHeader.length());

  const uint16_t chunkSize = 255;
  uint32_t remainingBytes = size;

  while (remainingBytes)
  {
    uint8_t bytesToRead = remainingBytes >= chunkSize ? chunkSize : remainingBytes;

    client.write(buffer + (size - remainingBytes), bytesToRead);

    remainingBytes -= bytesToRead;
  }

  written = client.write((uint8_t *)endingHeader.c_str(), endingHeader.length());

  bool firstLine = true;

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    line.trim(); // remove \r

    if (firstLine)
    {
      if (line == "")
        continue;

      int codePos = line.indexOf(' ') + 1;
      httpCode = line.substring(codePos, line.indexOf(' ', codePos)).toInt();
      firstLine = false;
      Serial.printf("Return Code: %d\n\r", httpCode);
    }

    Serial.printf("%s\n\r", line.c_str());

    if (line == "")
    {
      break;
    }
  }

  String response = client.readStringUntil('\n');

  Serial.printf("HTTP Response:\n\r");
  Serial.printf("==========\n\r");
  Serial.printf("%s\n\r", response.c_str());
  Serial.printf("==========\n\r");
  Serial.printf("HTTP End\n\r");

  Serial.printf("Return Code: %d\n\r", httpCode);

  return httpCode;
}

int Supabase::upload(String bucket, String filename, String mime_type, Stream *stream, uint64_t size)
{
  int httpCode = 0;

  // Gets protocol out of hostname.
  int index = hostname.indexOf("//");
  String hostname_no_prot = hostname.substring(index + 2, hostname.length());

  char hostname_char[hostname_no_prot.length() + 1];
  hostname_no_prot.toCharArray(hostname_char, hostname_no_prot.length() + 1);

  String finalPath = hostname + "/storage/v1/object/" + bucket + "/" + filename;

  // Make a HTTP request and add HTTP headers
  String boundary = "esp32-supabase-boundary";

  // Request Main Header
  String httpMainHeader = "POST " + finalPath + " HTTP/1.1\r\n";
  httpMainHeader += "Host: " + hostname_no_prot + "\r\n";
  httpMainHeader += "apikey: " + key + "\r\n";
  httpMainHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }

    httpMainHeader += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }

  httpMainHeader += "User-Agent: ESP32/Supabase\r\n";
  httpMainHeader += "Keep-Alive: 3600\r\n";
  httpMainHeader += "Connection: keep-alive\r\n";

  // request Content Header
  String contentHeader = "--" + boundary + "\r\n";
  contentHeader += "Content-Disposition: form-data; name=\"" + filename + "\"; filename=\"" + filename + "\"\r\n";
  contentHeader += "Content-Type: " + mime_type + "\r\n\r\n";

  // request Ending Header
  String endingHeader = "\r\n--" + boundary + "--\r\n\r\n";

  // content length
  int contentLength = contentHeader.length() + endingHeader.length();

  httpMainHeader += "Content-Length: " + String(contentLength + size) + "\n\n";

  Serial.printf("Hostname: %s\n\r", hostname_char);

  if (!client.connected())
  {
    // This needs further testing, might need to change to port 80.
    client.connect(hostname_char, 443);

    if (!client.connected())
    {
      return 0;
    }
  }

  // send post header
  size_t written = client.write((uint8_t *)httpMainHeader.c_str(), httpMainHeader.length());

  Serial.printf("HTTP Request:\n\r%s\n\r", httpMainHeader.c_str());
  Serial.printf("\n\r%s\n\r", contentHeader.c_str());
  Serial.printf("\n\r(file of %llu bytes)\n\r", size);
  Serial.printf("\n\r%s\n\r", endingHeader.c_str());

  written = client.write((uint8_t *)contentHeader.c_str(), contentHeader.length());

  const uint16_t chunkSize = 255;
  uint64_t availableBytes = stream->available();


  while (availableBytes)
  {
    uint8_t bytesToRead = availableBytes >= chunkSize ? chunkSize : availableBytes;

    uint8_t buffer[bytesToRead];

    uint8_t bytesRead = stream->readBytes(buffer, bytesToRead);

    client.write(buffer, bytesRead);

    availableBytes = stream->available();
  }


  written = client.write((uint8_t *)endingHeader.c_str(), endingHeader.length());

  bool firstLine = true;

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    line.trim(); // remove \r

    if (firstLine)
    {
      if(line == "") continue;

      int codePos = line.indexOf(' ') + 1;
      httpCode = line.substring(codePos, line.indexOf(' ', codePos)).toInt();
      firstLine = false;
      Serial.printf("Return Code: %d\n\r", httpCode);
    }

    Serial.printf("%s\n\r", line.c_str());

    if (line == "")
    {
      break;
    }
  }

  String response = client.readStringUntil('\n');

  Serial.printf("HTTP Response:\n\r");
  Serial.printf("==========\n\r");
  Serial.printf("%s\n\r", response.c_str());
  Serial.printf("==========\n\r");
  Serial.printf("HTTP End\n\r");

  Serial.printf("Return Code: %d\n\r", httpCode);

  return httpCode;
}

// Supabase& Supabase::drop(String table){
//   url_query += (table+"?");
//   return *this;
// }

// Comparison Operator
Supabase &Supabase::eq(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=eq." + conditions);
  return *this;
}
Supabase &Supabase::gt(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=gt." + conditions);
  return *this;
}
Supabase &Supabase::gte(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=gte." + conditions);
  return *this;
}
Supabase &Supabase::lt(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=lt." + conditions);
  return *this;
}
Supabase &Supabase::lte(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=lte." + conditions);
  return *this;
}
Supabase &Supabase::neq(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=neq." + conditions);
  return *this;
}
Supabase &Supabase::in(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=in.(" + conditions + ")");
  return *this;
}
Supabase &Supabase::is(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=is." + conditions);
  return *this;
}
Supabase &Supabase::cs(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=cs.{" + conditions + "}");
  return *this;
}
Supabase &Supabase::cd(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=cd.{" + conditions + "}");
  return *this;
}
Supabase &Supabase::ov(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=cd.{" + conditions + "}");
  return *this;
}
Supabase &Supabase::sl(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=sl.(" + conditions + ")");
  return *this;
}
Supabase &Supabase::sr(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=sr.(" + conditions + ")");
  return *this;
}
Supabase &Supabase::nxr(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=nxr.(" + conditions + ")");
  return *this;
}
Supabase &Supabase::nxl(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=nxl.(" + conditions + ")");
  return *this;
}
Supabase &Supabase::adj(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=adj.(" + conditions + ")");
  return *this;
}
// Supabase& Supabase::logic(String mylogic){
//   url_query += (mylogic);
// }

// Ordering
Supabase &Supabase::order(String coll, String by, bool nulls = 1)
{
  String subq[] = {"nullsfirst", "nullslast"};
  _check_last_string();
  url_query += ("order=" + coll + "." + by + "." + subq[(int)nulls]);
  return *this;
}
Supabase &Supabase::limit(unsigned int by)
{
  _check_last_string();
  url_query += ("limit=" + String(by));
  return *this;
}
Supabase &Supabase::offset(int by)
{
  _check_last_string();
  url_query += ("offset=" + String(by));
  return *this;
}
// do select. execute this after building your query
String Supabase::doSelect()
{
  https.begin(client, hostname + "/rest/v1/" + url_query);
  https.addHeader("apikey", key);
  https.addHeader("Content-Type", "application/json");

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }
    https.addHeader("Authorization", "Bearer " + USER_TOKEN);
  }

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = https.GET();
  }

  if (httpCode > 0)
  {
    data = https.getString();
  }
  https.end();
  urlQuery_reset();
  return data;
}
// do update. execute this after querying your update
int Supabase::doUpdate(String json)
{
  int httpCode;
  if (https.begin(client, hostname + "/rest/v1/" + url_query))
  {
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");
    if (useAuth)
    {
      unsigned long t_now = millis();
      if (t_now - loginTime >= authTimeout)
      {
        _login_process();
      }
      https.addHeader("Authorization", "Bearer " + USER_TOKEN);
    }
    httpCode = https.PATCH(json);
    https.end();
  }
  else
  {
    return -100;
  }
  urlQuery_reset();
  return httpCode;
}

int Supabase::login_email(String email_a, String password_a)
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

int Supabase::login_phone(String phone_a, String password_a)
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

String Supabase::rpc(String func_name, String json_param)
  {

    int httpCode;

    if( ! https.begin(client, hostname + "/rpc/" + func_name)){
      return String(-100);
    }
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");

    if (useAuth){
      unsigned long t_now = millis();
      if (t_now - loginTime >= authTimeout){
        _login_process();
      }
      https.addHeader("Authorization", "Bearer " + USER_TOKEN);
    }

    httpCode = https.POST(json_param);
    if (httpCode > 0){
      data = https.getString();
      https.end();
      return data;
    }

    https.end();
    return String(httpCode);
  }