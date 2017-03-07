/******************************************************************************
    Copyright (C) 2017 by Shamun <shamun.toha@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

typedef struct obs_custom {
  char        *id;
  string      url;
  obs_custom  *next;
} obs_custom;

//#define OBS_CUSTOM_MANIFEST_URL(REPLACE) "https://" REPLACE "/manifest.json";
static std::string manifest_url(const std::string& replacement) {
  return "https://" + replacement + "/manifest.json";
}

// TODO: rtmp-stream.c: opt_url_custom_manifest_value_set
static void obs_http_get(string url) {
  CURL *curl;
  CURLcode res;
  obs_custom *obs_custom_access;
  obs_custom_access = new obs_custom;
  curl = curl_easy_init();
  if (curl) {
    /*curl_easy_setopt(curl,
                     CURLOPT_URL,
                     manifest_url(obs_custom_access->url));*/

    curl_easy_setopt(curl,
                    CURLOPT_URL,
                    manifest_url(url));

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr,
              "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
  delete obs_custom_access;
}
