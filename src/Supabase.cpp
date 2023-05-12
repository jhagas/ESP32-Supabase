#include "Supabase.h"

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
    if (upsert)
    {
      https.addHeader("Authorization", "resolution=merge-duplicates");
    }
    if (useAuth)
    {
      unsigned long t_now = millis();
      if (t_now - loginTime >= 3500000)
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
    if (t_now - loginTime >= 60 * 60 * 1000)
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
      if (t_now - loginTime >= 3500000)
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
