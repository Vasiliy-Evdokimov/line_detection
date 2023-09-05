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
	//
	ret["01_CAM_ADDR_1"] = config.CAM_ADDR_1;
	ret["02_CAM_ADDR_2"] = config.CAM_ADDR_2;
	//
	ret["03_UDP_ADDR"] = config.UDP_ADDR;
	ret["04_UDP_PORT"] = config.UDP_PORT;
	//
	ret["05_NUM_ROI"] = config.NUM_ROI;
	ret["06_NUM_ROI_H"] = config.NUM_ROI_H;
	ret["07_NUM_ROI_V"] = config.NUM_ROI_V;
	//
	ret["08_SHOW_CAM"] = config.SHOW_CAM;
	ret["09_SHOW_GRAY"] = config.SHOW_GRAY;
	ret["10_DETAILED"] = config.DETAILED;
	ret["11_DRAW_GRID"] = config.DRAW_GRID;
	ret["12_DRAW"] = config.DRAW;
	//
	ret["13_MIN_CONT_LEN"] = config.MIN_CONT_LEN;
	ret["14_HOR_COLLAPSE"] = config.HOR_COLLAPSE;
	//
	ret["15_GAUSSIAN_BLUR_KERNEL"] = config.GAUSSIAN_BLUR_KERNEL;
	ret["16_MORPH_OPEN_KERNEL"] = config.MORPH_OPEN_KERNEL;
	ret["17_MORPH_CLOSE_KERNEL"] = config.MORPH_CLOSE_KERNEL;
	//
	ret["18_THRESHOLD_THRESH"] = config.THRESHOLD_THRESH;
	ret["19_THRESHOLD_MAXVAL"] = config.THRESHOLD_MAXVAL;
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

void apply_params(const HttpRequestPtr &request, Callback &&callback)
{
	std::cout << "apply_params request!" << std::endl;
	//
	ConfigData buf;
	//
	Json::Value v;
	std::string err;
	if(jsonParse(request->body(), v, err)) {
		 buf.CAM_ADDR_1 = v["CAM_ADDR_1"].asString();
		 buf.CAM_ADDR_2 = v["CAM_ADDR_2"].asString();
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
	drogon::app().registerHandler("/apply_params", &apply_params, { Get, Post, Options });
	drogon::app().run();
}
