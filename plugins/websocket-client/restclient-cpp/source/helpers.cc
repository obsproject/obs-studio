/**
 * @file helpers.cpp
 * @brief implementation of the restclient helpers
 * @author Daniel Schauenberg <d@unwiredcouch.com>
 */

#include "restclient-cpp/helpers.h"

#include <cstring>

#include "restclient-cpp/restclient.h"

/**
 * @brief write callback function for libcurl
 *
 * @param data returned data of size (size*nmemb)
 * @param size size parameter
 * @param nmemb memblock parameter
 * @param userdata pointer to user data to save/work with return data
 *
 * @return (size * nmemb)
 */
size_t RestClient::Helpers::write_callback(void *data, size_t size,
                                           size_t nmemb, void *userdata) {
  RestClient::Response* r;
  r = reinterpret_cast<RestClient::Response*>(userdata);
  r->body.append(reinterpret_cast<char*>(data), size*nmemb);

  return (size * nmemb);
}

/**
 * @brief header callback for libcurl
 *
 * @param data returned (header line)
 * @param size of data
 * @param nmemb memblock
 * @param userdata pointer to user data object to save headr data
 * @return size * nmemb;
 */
size_t RestClient::Helpers::header_callback(void *data, size_t size,
                                            size_t nmemb, void *userdata) {
  RestClient::Response* r;
  r = reinterpret_cast<RestClient::Response*>(userdata);
  std::string header(reinterpret_cast<char*>(data), size*nmemb);
  size_t seperator = header.find_first_of(':');
  if ( std::string::npos == seperator ) {
    // roll with non seperated headers...
    trim(header);
    if (0 == header.length()) {
      return (size * nmemb);  // blank line;
    }
    r->headers[header] = "present";
  } else {
    std::string key = header.substr(0, seperator);
    trim(key);
    std::string value = header.substr(seperator + 1);
    trim(value);
    r->headers[key] = value;
  }

  return (size * nmemb);
}

/**
 * @brief read callback function for libcurl
 *
 * @param data pointer of max size (size*nmemb) to write data to
 * @param size size parameter
 * @param nmemb memblock parameter
 * @param userdata pointer to user data to read data from
 *
 * @return (size * nmemb)
 */
size_t RestClient::Helpers::read_callback(void *data, size_t size,
                                          size_t nmemb, void *userdata) {
  /** get upload struct */
  RestClient::Helpers::UploadObject* u;
  u = reinterpret_cast<RestClient::Helpers::UploadObject*>(userdata);
  /** set correct sizes */
  size_t curl_size = size * nmemb;
  size_t copy_size = (u->length < curl_size) ? u->length : curl_size;
  /** copy data to buffer */
  std::memcpy(data, u->data, copy_size);
  /** decrement length and increment data pointer */
  u->length -= copy_size;
  u->data += copy_size;
  /** return copied size */
  return copy_size;
}
