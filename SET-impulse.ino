#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

char ssid[] = "xxxxxxxxxxxxxxxx";  //  your network SSID (name)
char pass[] = "xxxxxxxxxxxxxxxx";  // your network password
int hour, minute, second, refreshTime = 1, outputPulse, advanceMins = 0, holdMins = 0;
String netChoices, sendBuffer = "";

WiFiUDP packet;
Ticker callGetTime, oneMinute;
ESP8266WebServer server(80);

void handleRoot() {
  sendBuffer = "<!DOCTYPE HTML>\r\n<html>";
  sendBuffer += "<form method='get' action='setoptions'>";
  sendBuffer += "<input type='number' name='adv' style='width: 3.5em'>";
  sendBuffer += "&nbsp;&nbsp;<input type='submit' value='Advance X minutes'></form><p>";
  sendBuffer += "<form method='get' action='setoptions'>";
  sendBuffer += "<input type='number' name='hold' style='width: 3.5em'>";
  sendBuffer += "&nbsp;&nbsp;<input type='submit' value='Hold for X minutes'></form><p>";
  sendBuffer += "</html>";
  server.send(200, "text/html", sendBuffer);
}

void setOptions()
{
  String serverArg;
  sendBuffer = "<!DOCTYPE HTML>\r\n<html>";
  if ((serverArg = server.arg("adv")) != "" )
  {
    advanceMins = serverArg.toInt();
    sendBuffer += "<p>Advancing minutes by " + String(advanceMins) + "</p>";
  }
  else if ((serverArg = server.arg("hold")) != "" )
  {
    holdMins = serverArg.toInt();
    sendBuffer += "<p>Holding for " + String(holdMins) + " minutes</p>";
  }
  sendBuffer += "</html>";
  server.send(200, "text/html", sendBuffer);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup()
{
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  digitalWrite(3, LOW);
  pinMode(3, FUNCTION_3);
  pinMode(3, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/setoptions", setOptions);
  server.onNotFound(handleNotFound);
  server.begin();

  packet.begin(2390);
  getTime();
}

void pulseClock()
{
  if ( holdMins > 0)
    holdMins--;
  else
    outputPulse = 1;
}
void triggerRefresh()
{
  refreshTime = 1;
}

void loop()
{
  if(advanceMins > 0)
  {
    advanceMins--;
    digitalWrite(3, HIGH);
    delay(250);
    digitalWrite(3, LOW);
    delay(1000);
  }
  if (outputPulse)
  {
    outputPulse = 0;
    oneMinute.attach(60, pulseClock);
    Serial.println("Click");
    digitalWrite(3, HIGH);
    delay(250);
    digitalWrite(3, LOW);
  }
  if (refreshTime)
    getTime();
  server.handleClient();
}

void getTime() {
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[ NTP_PACKET_SIZE];
  IPAddress timeServerIP;

  refreshTime = 0;
  //IPAddress timeServerIP(172, 17, 2, 21);
  WiFi.hostByName("pool.ntp.org", timeServerIP);
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  packet.beginPacket(timeServerIP, 123);
  packet.write(packetBuffer, NTP_PACKET_SIZE);
  packet.endPacket();
  // Wait for reply
  while (packet.parsePacket() < 1)
  {
    Serial.print(".");
    delay(50);
  }
  packet.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch = secsSince1900 - seventyYears;
  epoch -= 7 * 3600; //Adjust time zone
  hour = (epoch % 86400L) / 3600;
  Serial.println(second);
  minute = (epoch % 3600) / 60;
  second = epoch % 60;

  printTime(hour, minute, second);

  callGetTime.attach((3600 - second), triggerRefresh);
  oneMinute.attach((59 - second), pulseClock);

}

void printTime(int hour, int minute, int second)
{
  if (hour < 10)
    Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10)
    Serial.print("0");
  Serial.print(minute);
  Serial.print(":");
  if (second < 10)
    Serial.print("0");
  Serial.println(second);
}
