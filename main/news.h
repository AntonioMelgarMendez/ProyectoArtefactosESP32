#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 

extern String newsHeadline;
extern bool newsUpdated;

inline void fetch_first_rss_title(const char* rssUrl) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure(); 

  HTTPClient http;
  if (http.begin(client, rssUrl)) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      int itemPos = payload.indexOf("<item>");
      if (itemPos > 0) {
        int titleStart = payload.indexOf("<title>", itemPos);
        int titleEnd = payload.indexOf("</title>", titleStart);
        
        if (titleStart > 0 && titleEnd > titleStart) {
          String title = payload.substring(titleStart + 7, titleEnd);
          if (title.indexOf("<![CDATA[") >= 0) {
             title.replace("<![CDATA[", "");
             title.replace("]]>", "");
          }
        
          int dashPos = title.lastIndexOf(" - ");
          if (dashPos > 0) title = title.substring(0, dashPos);

          title.trim();
          newsHeadline = title;
          newsUpdated = true;
        }
      }
    }
    http.end();
  }
}

inline void news_rss_loop(const char* rssUrl) {
  static unsigned long last = 0;
  const unsigned long interval = 15 * 60 * 1000UL; 
  if (last == 0 || millis() - last > interval) {
    last = millis();
    if (WiFi.status() == WL_CONNECTED) {
        fetch_first_rss_title(rssUrl);
    }
  }
}