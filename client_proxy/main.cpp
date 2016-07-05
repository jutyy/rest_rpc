#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <kapok/Kapok.hpp>
#include "client_proxy.hpp"
#include "client_base.hpp"
#include "base64.hpp"
#include "../common.h"

struct person
{
	int age;
	std::string name;

	META(age, name);
};

struct configure
{
	std::string hostname;
	std::string port;

	META(hostname, port);
};

configure get_config()
{
	//congfigure cfg1 = { "127.0.0.1", 9000 };
	//Serializer sr;
	//sr.Serialize(cfg1);

	//const char* buf = sr.GetString();
	//std::ofstream myfile("server.cfg");
	//myfile << buf;
	//myfile.close();

	std::ifstream in("client.cfg");
	std::stringstream ss;
	ss << in.rdbuf();

	configure cfg = {};
	DeSerializer dr;
	try
	{
		dr.Parse(ss.str());
		dr.Deserialize(cfg);
	}
	catch (const std::exception& e)
	{
		SPD_LOG_ERROR(e.what());
	}

	return cfg;
}

namespace client
{
	TIMAX_DEFINE_PROTOCOL(translate, std::string(std::string const&));
	TIMAX_DEFINE_PROTOCOL(add, int(int, int));
	TIMAX_DEFINE_PROTOCOL(binary_func, std::string(const char*, int));
}

void test_translate(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);

		std::string result = client.call(with_tag(client::translate, 1), "test");
		
		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_add(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);

		auto result = client.call(client::add, 1, 2);

		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_sub(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);

		auto result = client.sub(client::add);

		while (true)
		{
			size_t len = client.recieve();
			auto result = client::add.parse_json(std::string(client.data(), len));
			std::cout << result << std::endl;
		}
		
		//client.pub(client::add, 1, 2);
		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_pub(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);

		std::string str;
		cin >> str;
		while (str!="stop")
		{
			client.pub(client::add, 1, 2);
			cin >> str;
		}

		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_binary(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);

		char buf[40] = {};
		std::string str = "it is test";
		strcpy(buf, str.c_str());
		auto result = client.call_binary(client::binary_func, buf, sizeof(buf));

		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}


void test_performance(const configure& cfg)
{
	try
	{
		boost::asio::io_service io_service;
		timax::client_proxy client{ io_service };
		client.connect(cfg.hostname, cfg.port);
		std::thread thd([&io_service] {io_service.run(); });

		while (true)
		{
			//client.call(str);
			client.call("translate", "test");
		}

		getchar();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

int main(void)
{
	log::get().init("rest_rpc_client.lg");
	configure cfg = get_config();
	if (cfg.hostname.empty())
	{
		cfg = { "127.0.0.1", "9000" };
	}

	//if (cfg.is_sub)
	//	test_sub(cfg);
	//else
	//	test_pub(cfg);
	test_performance(cfg);
	test_translate(cfg);
	test_add(cfg);
	test_binary(cfg);
	getchar();
}

/*template<typename T>
void handle_result(const char* result)
{
	DeSerializer dr;
	dr.Parse(result);
	Document& doc = dr.GetDocument();

	if (doc[CODE].GetInt() == result_code::OK)
	{
		response_msg<T> response = {};
		dr.Deserialize(response);
		std::cout << response.result << std::endl;
	}
	else
	{
		//maybe exception, output the exception message.
		std::cout << doc[RESULT].GetString() << std::endl;
	}
}

void test_async_client()
{
	try
	{
		boost::asio::io_service io_service;
		client_proxy client(io_service);
		client.async_connect("baidu.com", "9000", 5*1000, [&client] (boost::system::error_code& ec)
		{
			if (ec)
			{
				std::cout << "connect error." << std::endl;
				return;
			}

			client.async_call("add", [&client](boost::system::error_code ec, std::string result)
			{
				if (ec)
				{
					std::cout << "call error." << std::endl;
					return;
				}

				handle_result<int>(result.c_str());
			},1, 2);

		});

		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_spawn_client()
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::spawn(io_service, [&io_service] (boost::asio::yield_context yield)
		{
			client_proxy client(io_service);
			client.async_connect("127.0.0.1", "9000", yield);

			std::string result = client.async_call("add", yield, 1,2);
			handle_result<int>(result.c_str());
		});
		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

int main()
{
	log::get().init("rest_rpc_client.lg");
	//test_performance();
	test_translate();

	return 0;
}*/