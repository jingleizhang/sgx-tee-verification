#include <iostream>
#include <memory>
#include "http_server.h"

std::string https_addr = "https://0.0.0.0:8443";
std::string log_verbose = "2";

int main() {
  auto http_server = std::shared_ptr<HttpServer>(new HttpServer);
  http_server->Init(https_addr);
  http_server->Start(log_verbose);

  return 0;
}
