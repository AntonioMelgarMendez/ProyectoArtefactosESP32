#pragma once
#include <WiFi.h>
#include <HTTPClient.h>

extern String newsHeadline;
extern bool newsUpdated;

inline void fetch_first_rss_title(const char* rssUrl) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(rssUrl);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    int itemPos = payload.indexOf("<item");
    if (itemPos < 0) itemPos = 0;
    int titleStart = payload.indexOf("<title>", itemPos);
    int titleEnd = payload.indexOf("</title>", titleStart);
    if (titleStart >= 0 && titleEnd > titleStart) {
      String title = payload.substring(titleStart + 7, titleEnd);
      title.trim();
      if (title.startsWith("<![CDATA[")) {
        int cstart = title.indexOf("<![CDATA[");
        int cend = title.indexOf("]]>");
        if (cend > cstart) title = title.substring(cstart + 9, cend);
      }
      int lt = title.indexOf('<');
      if (lt >= 0) title = title.substring(0, lt);
      newsHeadline = title;
      newsUpdated = true;
    }
  }
  http.end();
}

inline void news_rss_loop(const char* rssUrl) {
  static unsigned long last = 0;
  const unsigned long interval = 15 * 60 * 1000UL; // 15 min
  if (millis() - last > interval) {
    last = millis();
    fetch_first_rss_title(rssUrl);
  }
}