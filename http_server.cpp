#include "http_server.h"
#include <utility>
#include "liboauth/liboauthcpp.h"

// twitter verification
const std::string consumer_key =
    "PnnNWAreKdAaC2IWceAV6CtMD";  // Key from Twitter
const std::string consumer_secret =
    "I3gk364fwbf6uCGCqfvUlfImjFJCiBnxOR62TomxTgTpHyL8an";  // Secret from
                                                           // Twitter
const std::string request_token_url =
    "https://api.twitter.com/oauth/request_token";
const std::string request_token_query_args = "oauth_callback=oob";
const std::string authorize_url = "https://api.twitter.com/oauth/authorize";
const std::string access_token_url =
    "https://api.twitter.com/oauth/access_token";

std::string oAuthQuery;
std::string authRedirect;
std::string authRedirect2;  // for oauth token

std::string requestTokenKey;
std::string requestTokenSecret;

static OAuth::Consumer *authConsumer;
static OAuth::Client *oauthClient;

static const char *oAuthPostData = NULL;  // POST data

static const uint64_t s_timeout_ms = 1500;  // Connect timeout in milliseconds

void HttpServer::Init(const std::string &addr) {
  m_addr = addr;

  opts = {0, 0, 0, 0, 0, 0};
  opts.root_dir = ".";

  OAuth::Consumer *consumer =
      new OAuth::Consumer(consumer_key, consumer_secret);
  authConsumer = consumer;
  OAuth::Client *oauth = new OAuth::Client(consumer);
  oauthClient = oauth;
}

bool HttpServer::Start(const std::string &log_level) {
  mg_log_set(log_level.c_str());
  mg_mgr_init(&m_mgr);
  // for https
  mg_connection *connection = mg_http_listen(
      &m_mgr, m_addr.c_str(), HttpServer::HandleHttpEvent, (void *) 1);
  if (connection == NULL) {
    exit(EXIT_FAILURE);
  }

  for (;;) mg_mgr_poll(&m_mgr, 500);  // Infinite event loop
  mg_mgr_free(&m_mgr);
  return 0;
}

void HttpServer::SendHttpRsp200(mg_connection *connection, std::string rsp) {
  // send header first, not support HTTP/2.0
  mg_printf(connection, "%s",
            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
  mg_http_printf_chunk(connection, "{ \"result\": %s }", rsp.c_str());
  // close
  mg_http_printf_chunk(connection, "");
}

void HttpServer::SendHttpRsp301(mg_connection *connection,
                                std::string location) {
  // send header first, not support HTTP/2.0
  mg_printf(connection,
            "HTTP/1.1 301 Moved\r\n"
            "Location: %s/\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            location.c_str());

  // close
  mg_http_printf_chunk(connection, "");
}

void HttpServer::HandleHttpEvent(struct mg_connection *connection, int ev,
                                 void *ev_data, void *fn_data) {
  if (ev == MG_EV_ACCEPT && fn_data != NULL) {
    struct mg_tls_opts opts = {
        //.ca = "ca.pem",           // Uncomment to enable two-way SSL
        .cert = "server.pem",     // Certificate PEM file
        .certkey = "server.pem",  // This pem contains both cert and key
    };
    mg_tls_init(connection, &opts);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (mg_http_match_uri(hm, "/api/checkalive")) {
      SendHttpRsp200(connection, "\"alive\"");
    } else if (mg_http_match_uri(hm, "/api/twitter/auth")) {
      printf("start twitter verification\n");

      OAuth::Consumer consumer(consumer_key, consumer_secret);
      OAuth::Client oauth(&consumer);

      authConsumer = &consumer;
      oauthClient = &oauth;

      std::string base_request_token_url =
          request_token_url +
          (request_token_query_args.empty()
               ? std::string("")
               : (std::string("?") + request_token_query_args));
      std::string oAuthQueryParams = oauthClient->getURLQueryString(
          OAuth::Http::Get, base_request_token_url);
      oAuthQuery = request_token_url;
      oAuthQuery = oAuthQuery.append("?").append(oAuthQueryParams);
      printf("oAuthQuery:%s\n", oAuthQuery.c_str());

      bool done = false;
      struct mg_mgr t_mgr;
      mg_mgr_init(&t_mgr);
      struct mg_connection *c2 = mg_http_connect(&t_mgr, oAuthQuery.c_str(),
                                                 TwitterHandlerAuth, &done);
      if (c2 == NULL) {
        mg_error(connection, "Cannot create client connection");
      }
      while (!done) {
        mg_mgr_poll(&t_mgr, 5000);  // Event manager loops until 'done'
      }
      mg_mgr_free(&t_mgr);

      std::string msg = "\"";
      msg = msg.append(authRedirect).append("\"");
      SendHttpRsp200(connection, msg);
    } else if (mg_http_match_uri(hm, "/api/twitter/pin")) {
      if (authConsumer == NULL) {
        SendHttpRsp200(connection, "authConsumer is NULL");
      }
      if (oauthClient == NULL) {
        SendHttpRsp200(connection, "oauthClient is NULL");
      }

      std::string pin(hm->body.ptr, hm->body.len);

      OAuth::Token requestToken(requestTokenKey, requestTokenSecret, pin);
      printf(
          "requestToken key:%s, requestToken secret:%s, requestToken pin: %s\n",
          requestToken.key().c_str(), requestToken.secret().c_str(),
          requestToken.pin().c_str());

      OAuth::Consumer consumer(consumer_key, consumer_secret);
      OAuth::Client oauth = OAuth::Client(&consumer, &requestToken);
      oauthClient = &oauth;
      std::string oAuthQueryString = oauthClient->getURLQueryString(
          OAuth::Http::Get, access_token_url, std::string(""), true);

      authRedirect2 = access_token_url;
      authRedirect2 = authRedirect2.append("?").append(oAuthQueryString);
      printf("authRedirect2: %s\n", authRedirect2.c_str());

      std::string msg = "\"";
      msg = msg.append(authRedirect2).append("\"");
      SendHttpRsp200(connection, msg);
    } else if (mg_http_match_uri(hm, "/api/twitter/final")) {
      bool done = false;
      struct mg_mgr t_mgr;
      mg_mgr_init(&t_mgr);
      struct mg_connection *c3 = mg_http_connect(&t_mgr, oAuthQuery.c_str(),
                                                 TwitterHandlerFinal, &done);
      if (c3 == NULL) {
        mg_error(connection, "Cannot create client connection");
      }
      while (!done) {
        mg_mgr_poll(&t_mgr, 5000);  // Event manager loops until 'done'
      }
      mg_mgr_free(&t_mgr);

      std::string msg = "\"";
      SendHttpRsp200(connection, msg.append("\""));
    } else {
      struct mg_http_serve_opts opts = {.root_dir = "."};
      mg_http_serve_dir(connection, hm, &opts);
    }
  }
  (void) fn_data;
}

void HttpServer::TwitterHandlerAuth(struct mg_connection *c2, int ev,
                                    void *ev_data, void *fn_data) {
  if (ev == MG_EV_OPEN) {
    *(uint64_t *) c2->label = mg_millis() + s_timeout_ms;
  } else if (ev == MG_EV_POLL) {
    if (mg_millis() > *(uint64_t *) c2->label &&
        (c2->is_connecting || c2->is_resolving)) {
      mg_error(c2, "Connect timeout");
    }
  } else if (ev == MG_EV_CONNECT) {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(oAuthQuery.c_str());

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(oAuthQuery.c_str())) {
      struct mg_tls_opts opts = {.ca = "ca.pem", .srvname = host};
      mg_tls_init(c2, &opts);
    }

    // Send request
    int content_length = oAuthPostData ? strlen(oAuthPostData) : 0;
    mg_printf(c2,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: octet-stream\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              oAuthPostData ? "POST" : "GET", mg_url_uri(oAuthQuery.c_str()),
              (int) host.len, host.ptr, content_length);
    mg_send(c2, oAuthPostData, content_length);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    // printf("got data:%.*s\n", (int) hm->message.len, hm->message.ptr);

    std::string request_token_resp(hm->body.ptr, hm->body.len);
    OAuth::Token request = OAuth::Token::extract(request_token_resp);

    requestTokenKey = request.key();
    requestTokenSecret = request.secret();
    authRedirect = authorize_url;
    authRedirect.append("?oauth_token=").append(requestTokenKey);
    printf("authorizeURL: %s\n", authRedirect.c_str());
    c2->is_closing = 1;        // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  } else if (ev == MG_EV_CLOSE) {
    c2->is_closing = 1;        // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  } else if (ev == MG_EV_ERROR) {
    *(bool *) fn_data = true;  // Error, tell event loop to stop
  }
}

void HttpServer::TwitterHandlerFinal(struct mg_connection *c3, int ev,
                                     void *ev_data, void *fn_data) {
  if (ev == MG_EV_OPEN) {
    *(uint64_t *) c3->label = mg_millis() + s_timeout_ms;
  } else if (ev == MG_EV_POLL) {
    if (mg_millis() > *(uint64_t *) c3->label &&
        (c3->is_connecting || c3->is_resolving)) {
      mg_error(c3, "Connect timeout");
    }
  } else if (ev == MG_EV_CONNECT) {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(oAuthQuery.c_str());

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(oAuthQuery.c_str())) {
      struct mg_tls_opts opts = {.ca = "ca.pem", .srvname = host};
      mg_tls_init(c3, &opts);
    }

    // Send request
    int content_length = oAuthPostData ? strlen(oAuthPostData) : 0;
    mg_printf(c3,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: octet-stream\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              oAuthPostData ? "POST" : "GET", mg_url_uri(oAuthQuery.c_str()),
              (int) host.len, host.ptr, content_length);
    mg_send(c3, oAuthPostData, content_length);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    printf("got data:%.*s\n", (int) hm->message.len, hm->message.ptr);

    c3->is_closing = 1;        // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  } else if (ev == MG_EV_CLOSE) {
    c3->is_closing = 1;        // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  } else if (ev == MG_EV_ERROR) {
    *(bool *) fn_data = true;  // Error, tell event loop to stop
  }
}