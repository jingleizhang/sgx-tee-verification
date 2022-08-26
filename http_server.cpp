#include "http_server.h"
#include <utility>
#include "liboauthcpp.h"
#include "sendgrid/sendgridmsg.h"

#define MAX_DATA_SIZE 10240

const std::string request_token_url = "https://api.twitter.com/oauth/request_token";
const std::string request_token_query_args = "oauth_callback=oob";
const std::string authorize_url = "https://api.twitter.com/oauth/authorize";
const std::string access_token_url = "https://api.twitter.com/oauth/access_token";
const std::string sendgrid_url = "https://api.sendgrid.com/v3/mail/send";

static std::string twitter_consumer_key;
static std::string twitter_consumer_secret;
static std::string sendgrid_apikey;

std::string oAuthQuery;
std::string authRedirect;
std::string authRedirect2; // for oauth token

std::string requestTokenKey;
std::string requestTokenSecret;

static OAuth::Consumer *authConsumer;
static OAuth::Client *oauthClient;

static char *oAuthPostData = NULL;
static char sendGridPostData[MAX_DATA_SIZE];

static const uint64_t s_timeout_ms = 1500; // Connect timeout in milliseconds

void HttpServer::Init(const std::string &addr, neb::CJsonObject &json)
{
  m_addr = addr;
  config = json;
  opts = {0, 0, 0, 0, 0, 0};
  opts.root_dir = ".";

  json.Get("twitter_consumer_key", twitter_consumer_key);
  json.Get("twitter_consumer_secret", twitter_consumer_secret);
  json.Get("sendgrid_apikey", sendgrid_apikey);

  OAuth::Consumer *consumer =
      new OAuth::Consumer(twitter_consumer_key, twitter_consumer_secret);
  authConsumer = consumer;
  OAuth::Client *oauth = new OAuth::Client(consumer);
  oauthClient = oauth;
}

bool HttpServer::Start(const std::string &log_level)
{
  mg_log_set(log_level.c_str());
  mg_mgr_init(&m_mgr);
  // for https
  mg_connection *connection = mg_http_listen(
      &m_mgr, m_addr.c_str(), HttpServer::HandleHttpEvent, (void *)1);
  if (connection == NULL)
  {
    exit(EXIT_FAILURE);
  }

  for (;;)
    mg_mgr_poll(&m_mgr, 500); // Infinite event loop
  mg_mgr_free(&m_mgr);
  return 0;
}

void HttpServer::SendHttpRsp200(mg_connection *connection, std::string rsp)
{
  // send header first, not support HTTP/2.0
  mg_printf(connection, "%s",
            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
  mg_http_printf_chunk(connection, "{ \"result\": %s }", rsp.c_str());
  // close
  mg_http_printf_chunk(connection, "");
}

void HttpServer::SendHttpRsp301(mg_connection *connection,
                                std::string location)
{
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

void HttpServer::SendHttpRsp400(mg_connection *connection, std::string rsp)
{
  // send header first, not support HTTP/2.0
  mg_printf(connection, "%s",
            "HTTP/1.1 400 Bad Request\r\nTransfer-Encoding: chunked\r\n\r\n");
  mg_http_printf_chunk(connection, "{ \"result\": %s }", rsp.c_str());
  // close
  mg_http_printf_chunk(connection, "");
}

void HttpServer::HandleHttpEvent(struct mg_connection *connection, int ev,
                                 void *ev_data, void *fn_data)
{
  if (ev == MG_EV_ACCEPT && fn_data != NULL)
  {
    struct mg_tls_opts opts = {
        //.ca = "ca.pem",           // Uncomment to enable two-way SSL
        .cert = "server.pem",    // Certificate PEM file
        .certkey = "server.pem", // This pem contains both cert and key
    };
    mg_tls_init(connection, &opts);
  }
  else if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;

    if (mg_http_match_uri(hm, "/api/checkalive"))
    {
      SendHttpRsp200(connection, "\"alive\"");
    }
    else if (mg_http_match_uri(hm, "/api/twitter/email"))
    {
      printf("send twitter email\n");
      struct mg_str json = hm->body;
      char *mail_code = mg_json_get_str(json, "$.mail_code");
      char *mail_address = mg_json_get_str(json, "$.mail_address");
      if (mail_code != NULL && mail_address != NULL)
      {
        printf("mail_code: %s, mail_address: %s\n", mail_code, mail_address);

        SendGrid::SendGridMessage msg;
        msg.setSubject("SendGrid Test");
        msg.setFrom(new SendGrid::EmailAddress{"eric@litentry.com", "no-reply"});
        msg.setTemplateId("d-c267e08c81ac4e6e9800be7df4026ed9");

        SendGrid::Personalization p;
        p.to.push_back(SendGrid::EmailAddress{mail_address});
        neb::CJsonObject templateData;
        string link("http://tee.microservice.site/api/twitter/auth?code=");
        link.append(mail_code);
        templateData.Add("verification_link", link);
        p.SetDynamicTemplateData(templateData);
        msg.addPersonalization(p);

        memset(sendGridPostData, 0, MAX_DATA_SIZE);
        memcpy(sendGridPostData, msg.toString().c_str(), msg.toString().length());

        bool done = false;
        struct mg_mgr t_mgr;
        mg_mgr_init(&t_mgr);
        struct mg_connection *c3 = mg_http_connect(&t_mgr, sendgrid_url.c_str(), TwitterHandlerEmail, &done);

        if (c3 == NULL)
        {
          mg_error(connection, "Cannot create client connection");
        }
        while (!done)
        {
          mg_mgr_poll(&t_mgr, 5000);
        }
        mg_mgr_free(&t_mgr);
        SendHttpRsp200(connection, "\"sent\"");
      }
      else
      {
        SendHttpRsp400(connection, "\"invalid inputs\"");
      }
    }
    else if (mg_http_match_uri(hm, "/api/twitter/auth"))
    {
      printf("start twitter verification\n");

      OAuth::Consumer consumer(twitter_consumer_key, twitter_consumer_secret);
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
      if (c2 == NULL)
      {
        mg_error(connection, "Cannot create client connection");
      }
      while (!done)
      {
        mg_mgr_poll(&t_mgr, 5000); // Event manager loops until 'done'
      }
      mg_mgr_free(&t_mgr);

      std::string msg = "\"";
      msg = msg.append(authRedirect).append("\"");
      SendHttpRsp200(connection, msg);
    }
    else if (mg_http_match_uri(hm, "/api/twitter/pin"))
    {
      if (authConsumer == NULL)
      {
        SendHttpRsp200(connection, "authConsumer is NULL");
      }
      if (oauthClient == NULL)
      {
        SendHttpRsp200(connection, "oauthClient is NULL");
      }

      std::string pin(hm->body.ptr, hm->body.len);

      OAuth::Token requestToken(requestTokenKey, requestTokenSecret, pin);
      printf(
          "requestToken key:%s, requestToken secret:%s, requestToken pin: %s\n",
          requestToken.key().c_str(), requestToken.secret().c_str(),
          requestToken.pin().c_str());

      OAuth::Consumer consumer(twitter_consumer_key, twitter_consumer_secret);
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
    }
    else if (mg_http_match_uri(hm, "/api/twitter/final"))
    {
      bool done = false;
      struct mg_mgr t_mgr;
      mg_mgr_init(&t_mgr);
      struct mg_connection *c3 = mg_http_connect(&t_mgr, oAuthQuery.c_str(),
                                                 TwitterHandlerFinal, &done);
      if (c3 == NULL)
      {
        mg_error(connection, "Cannot create client connection");
      }
      while (!done)
      {
        mg_mgr_poll(&t_mgr, 5000); // Event manager loops until 'done'
      }
      mg_mgr_free(&t_mgr);

      std::string msg = "\"";
      SendHttpRsp200(connection, msg.append("\""));
    }
    else
    {
      struct mg_http_serve_opts opts = {.root_dir = "."};
      mg_http_serve_dir(connection, hm, &opts);
    }
  }
  (void)fn_data;
}

void HttpServer::TwitterHandlerAuth(struct mg_connection *c2, int ev,
                                    void *ev_data, void *fn_data)
{
  if (ev == MG_EV_OPEN)
  {
    *(uint64_t *)c2->label = mg_millis() + s_timeout_ms;
  }
  else if (ev == MG_EV_POLL)
  {
    if (mg_millis() > *(uint64_t *)c2->label &&
        (c2->is_connecting || c2->is_resolving))
    {
      mg_error(c2, "Connect timeout");
    }
  }
  else if (ev == MG_EV_CONNECT)
  {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(oAuthQuery.c_str());

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(oAuthQuery.c_str()))
    {
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
              (int)host.len, host.ptr, content_length);
    mg_send(c2, oAuthPostData, content_length);
  }
  else if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    // printf("got data:%.*s\n", (int) hm->message.len, hm->message.ptr);

    std::string request_token_resp(hm->body.ptr, hm->body.len);
    OAuth::Token request = OAuth::Token::extract(request_token_resp);

    requestTokenKey = request.key();
    requestTokenSecret = request.secret();
    authRedirect = authorize_url;
    authRedirect.append("?oauth_token=").append(requestTokenKey);
    printf("authorizeURL: %s\n", authRedirect.c_str());
    c2->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_CLOSE)
  {
    c2->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_ERROR)
  {
    *(bool *)fn_data = true; // Error, tell event loop to stop
  }
}

void HttpServer::TwitterHandlerFinal(struct mg_connection *c3, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_OPEN)
  {
    *(uint64_t *)c3->label = mg_millis() + s_timeout_ms;
  }
  else if (ev == MG_EV_POLL)
  {
    if (mg_millis() > *(uint64_t *)c3->label &&
        (c3->is_connecting || c3->is_resolving))
    {
      mg_error(c3, "Connect timeout");
    }
  }
  else if (ev == MG_EV_CONNECT)
  {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(oAuthQuery.c_str());

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(oAuthQuery.c_str()))
    {
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
              (int)host.len, host.ptr, content_length);
    mg_send(c3, oAuthPostData, content_length);
  }
  else if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    printf("got data:%.*s\n", (int)hm->message.len, hm->message.ptr);

    c3->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_CLOSE)
  {
    c3->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_ERROR)
  {
    *(bool *)fn_data = true; // Error, tell event loop to stop
  }
}

void HttpServer::TwitterHandlerEmail(struct mg_connection *c4, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_OPEN)
  {
    *(uint64_t *)c4->label = mg_millis() + s_timeout_ms;
  }
  else if (ev == MG_EV_POLL)
  {
    if (mg_millis() > *(uint64_t *)c4->label && (c4->is_connecting || c4->is_resolving))
    {
      mg_error(c4, "Connect timeout");
    }
  }
  else if (ev == MG_EV_CONNECT)
  {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(sendgrid_url.c_str());

    // If s_url is https://, tell client connection to use TLS
    if (mg_url_is_ssl(sendgrid_url.c_str()))
    {
      struct mg_tls_opts opts = {.ca = "ca.pem", .srvname = host};
      mg_tls_init(c4, &opts);
    }

    // Send request
    int content_length = strlen(sendGridPostData);
    mg_printf(c4,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: application/json\r\n"
              "Authorization: %s\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              "POST", mg_url_uri(sendgrid_url.c_str()),
              (int)host.len, host.ptr, sendgrid_apikey.c_str(), content_length);

    printf("sendGridPostData: %s\n", sendGridPostData);

    mg_send(c4, sendGridPostData, content_length);
  }
  else if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    printf("got data:%.*s\n", (int)hm->message.len, hm->message.ptr);

    c4->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_CLOSE)
  {
    c4->is_closing = 1;      // Tell mongoose to close this connection
    *(bool *)fn_data = true; // Tell event loop to stop
  }
  else if (ev == MG_EV_ERROR)
  {
    *(bool *)fn_data = true; // Error, tell event loop to stop
  }
}