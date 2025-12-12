#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef void CURL;
    typedef struct curl_slist curl_slist;
    typedef int CURLcode;

#define CURLOPT_URL 10002
#define CURLOPT_HTTPHEADER 10023
#define CURLOPT_ERRORBUFFER 10010
#define CURLOPT_WRITEFUNCTION 20011
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_FAILONERROR 45
#define CURLOPT_NOSIGNAL 99
#define CURLOPT_ACCEPT_ENCODING 10102
#define CURLOPT_HEADERFUNCTION 20079
#define CURLOPT_HEADERDATA 10029
#define CURLINFO_RESPONSE_CODE 2097154

#define CURLE_OK 0
#define CURL_ERROR_SIZE 256

    CURL *curl_easy_init(void);
    CURLcode curl_easy_setopt(CURL *curl, int option, ...);
    CURLcode curl_easy_perform(CURL *curl);
    void curl_easy_cleanup(CURL *curl);
    CURLcode curl_easy_getinfo(CURL *curl, int info, ...);
    struct curl_slist *curl_slist_append(struct curl_slist *, const char *);
    void curl_slist_free_all(struct curl_slist *);

    const char *curl_easy_strerror(CURLcode);

#ifdef __cplusplus
}
#endif
