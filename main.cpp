#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "http_server.h"
#include "sendgrid/CJsonObject.hpp"

int main()
{
  std::ifstream fin("config.json");
  if (!fin.good())
  {
    std::cout << "config.json not found" << std::endl;
    return 0;
  }

  neb::CJsonObject json;
  std::stringstream ssContent;
  ssContent << fin.rdbuf();
  if (json.Parse(ssContent.str()))
  {
    auto http_server = std::shared_ptr<HttpServer>(new HttpServer);

    std::string https_addr = "https://0.0.0.0:8443";
    json.Get("https_addr", https_addr);
    http_server->Init(https_addr, json);

    std::string log_verbose = "3";
    json.Get("log_verbose", log_verbose);
    http_server->Start(log_verbose);
  }
  else
  {
    std::cout << "parse config.json error" << std::endl;
  }
  fin.close();
  return 0;
}
