#pragma once

#include <string.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "mongoose.h"

class HttpServer {
 public:
  HttpServer() {
  }
  ~HttpServer() {
  }

  void Init(const std::string &port);
  bool Start(const std::string &log_level);
  bool Close();

  static void HandleHttpEvent(struct mg_connection *c, int ev, void *ev_data,
                              void *fn_data);
  static void SendHttpRsp200(mg_connection *connection, std::string rsp);

  static void SendHttpRsp301(mg_connection *connection, std::string location);

  static void TwitterHandlerAuth(struct mg_connection *c2, int ev,
                                 void *ev_data, void *fn_data);

  static void TwitterHandlerFinal(struct mg_connection *c3, int ev,
                                  void *ev_data, void *fn_data);

 private:
  std::string m_addr;
  mg_mgr m_mgr;
  struct mg_http_serve_opts opts;
};
