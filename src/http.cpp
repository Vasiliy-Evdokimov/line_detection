/*
 * http.cpp
 *
 *  Created on: 23 авг. 2023 г.
 *      Author: vevdokimov
 */

#include <iostream>

#include <drogon/drogon.h>

#include "config.hpp"
#include "http.hpp"

using namespace std;

using namespace drogon;

typedef std::function<void(const HttpResponsePtr &)> Callback;

bool jsonParse(std::string_view str, Json::Value& val, std::string& err)
{
  Json::CharReaderBuilder builder;
  Json::CharReader* reader = builder.newCharReader();
  val.clear();
  bool parsed = reader->parse(
    str.data(),
    str.data() + str.length(),
    &val,
    &err
  );
  delete reader;
  return parsed;
}

void get_params(const HttpRequestPtr &request, Callback &&callback)
{
	//std::cout << "get_params request!" << std::endl;
	//
	Json::Value ret;
	ret["cam_addr_1"] = config.cam_addr_1;
	ret["cam_addr_2"] = config.cam_addr_2;
	ret["udp_addr"] = config.udp_addr;
	ret["udp_port"] = config.udp_port;
	//
	auto resp = HttpResponse::newHttpResponse();
	Json::FastWriter fastWriter;
	std::string json_str = fastWriter.write(ret);
	//
	resp->addHeader("Content-Type", "application/json");
	resp->addHeader("Access-Control-Allow-Origin", "*");
	resp->addHeader("Access-Control-Request-Method", "*");
	//
	resp->setBody(json_str);
	//
	callback(resp);
}

void set_params(const HttpRequestPtr &request, Callback &&callback)
{
	std::cout << "set_params request!" << std::endl;
	//
	Json::Value v;
	std::string err;
	if(jsonParse(request->body(), v, err)) {
		std::cout << v["test_field"].asString() << std::endl;
		std::cout << v["test_field_2"].asString() << std::endl;
	}
	//
	Json::Value ret;
	ret["message"] = "Hello, World";
	//
	auto resp = HttpResponse::newHttpResponse();
	Json::FastWriter fastWriter;
	std::string json_str = fastWriter.write(ret);
	//
	resp->addHeader("Content-Type", "application/json");
	resp->addHeader("Access-Control-Allow-Origin", "*");
	resp->addHeader("Access-Control-Request-Method", "*");
	//
	resp->setBody(json_str);
	//
	callback(resp);
}

void http_init()
{
	drogon::app().addListener("127.0.0.1", 8000);
	drogon::app().registerHandler("/get_params", &get_params, { Get, Post, Options });
	drogon::app().registerHandler("/set_params", &set_params, { Get, Post, Options });
	drogon::app().run();
}
