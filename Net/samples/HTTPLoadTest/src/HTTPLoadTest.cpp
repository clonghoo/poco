//
// HTTPLoadTest.cpp
//
// $Id: //poco/1.3/Net/samples/HTTPLoadTest/src/HTTPLoadTest.cpp#3 $
//
// This sample demonstrates the HTTPClientSession class.
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/NameValueCollection.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/AutoPtr.h"
#include "Poco/Thread.h"
#include "Poco/Mutex.h"
#include "Poco/Runnable.h"
#include "Poco/Stopwatch.h"
#include "Poco/NumberParser.h"
#include "Poco/StreamCopier.h"
#include "Poco/NullStream.h"
#include "Poco/Exception.h"
#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include <iostream>


using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPCookie;
using Poco::Net::NameValueCollection;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::AutoPtr;
using Poco::Thread;
using Poco::FastMutex;
using Poco::Runnable;
using Poco::Stopwatch;
using Poco::NumberParser;
using Poco::Path;
using Poco::URI;
using Poco::Exception;
using Poco::StreamCopier;
using Poco::NullOutputStream;


class HTTPClient : public Runnable
{
public:
	HTTPClient(const URI& uri, int repetitions, bool cookies=false, bool verbose=false):
	  _uri(uri), 
	  _verbose(verbose), 
	  _cookies(cookies), 
	  _repetitions(repetitions), 
	  _usec(0), 
	  _success(0)
	{
		_gRepetitions += _repetitions;
	}

	~HTTPClient()
	{
	}

	void run()
	{
		Stopwatch sw;
		std::vector<HTTPCookie> cookies;

		for (int i = 0; i < _repetitions; ++i)
		{
			try
			{
				int usec = 0;
				std::string path(_uri.getPathAndQuery());
				if (path.empty()) path = "/";

				HTTPClientSession session(_uri.getHost(), _uri.getPort());
				HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				
				if (_cookies)
				{
					NameValueCollection nvc;
					std::vector<HTTPCookie>::iterator it = cookies.begin();
					for(; it != cookies.end(); ++it)
						nvc.add((*it).getName(), (*it).getValue());

					req.setCookies(nvc);
				}

				HTTPResponse res;
				sw.restart();
				session.sendRequest(req);
				std::istream& rs = session.receiveResponse(res);
				NullOutputStream nos;
				StreamCopier::copyStream(rs, nos);
				sw.stop();
				_success += HTTPResponse::HTTP_OK == res.getStatus() ? 1 : 0;
				if (_cookies) res.getCookies(cookies);
				usec = int(sw.elapsed());

				if (_verbose)
				{
					FastMutex::ScopedLock lock(_mutex);
					std::cout 
					<< _uri.toString() << ' ' << res.getStatus() << ' ' << res.getReason() 
					<< ' ' << usec/1000.0 << "ms" << std::endl;
				}

				_usec += usec;
			}
			catch (Exception& exc)
			{
				FastMutex::ScopedLock lock(_mutex);
				std::cerr << exc.displayText() << std::endl;
			}
		}
		
		{
			FastMutex::ScopedLock lock(_mutex);
			_gSuccess += _success;
			_gUsec += _usec;
		}
		if (_verbose)
			printStats(_uri.toString(), _repetitions, _success, _usec);
	}

	static void printStats(std::string uri, int repetitions, int success, Poco::UInt64 usec);
	static int totalAttempts();
	static Poco::UInt64 totalMicroseconds();
	static int totalSuccessCount();

private:
	HTTPClient();

	URI _uri;
	bool _verbose;
	bool _cookies;
	int _repetitions;
	Poco::UInt64 _usec;
	int _success;
	static int _gRepetitions;
	static Poco::UInt64 _gUsec;
	static int _gSuccess;
	static FastMutex _mutex;
};

FastMutex HTTPClient::_mutex;
int HTTPClient::_gRepetitions;
Poco::UInt64 HTTPClient::_gUsec;
int HTTPClient::_gSuccess;

int HTTPClient::totalAttempts()
{
	return _gRepetitions;
}

Poco::UInt64 HTTPClient::totalMicroseconds()
{
	return _gUsec;
}

int HTTPClient::totalSuccessCount()
{
	return _gSuccess;
}

void HTTPClient::printStats(std::string uri, int repetitions, int success, Poco::UInt64 usec)
{
	FastMutex::ScopedLock lock(_mutex);

	std::cout << std::endl << "--------------" << std::endl 
		<< "Statistics for " << uri << std::endl << "--------------" 
		<< std::endl
		<< repetitions << " attempts, " << success << " succesful (" 
		<<  ((float) success / (float) repetitions) * 100.0 << "%)" << std::endl
		<< "Avg response time: " << ((float) usec / (float) repetitions) / 1000.0 << "ms, " << std::endl
		<< "Avg requests/second handled: " << ((float) success  /((float) usec / 1000000.0)) << std::endl
		<< "Total time: " << (float) usec / 1000000.0 << std::endl;
}

class HTTPLoadTest: public Application
	/// This sample demonstrates some of the features of the Poco::Util::Application class,
	/// such as configuration file handling and command line arguments processing.
	///
	/// Try HTTPLoadTest --help (on Unix platforms) or HTTPLoadTest /help (elsewhere) for
	/// more information.
{
public:
	HTTPLoadTest(): 
		_helpRequested(false), 
		_verbose(false), 
		_cookies(false),
		_repetitions(1), 
		_threads(1)
	{
	}

protected:	
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		Application::initialize(self);
		// add your own initialization code here
	}
	
	void uninitialize()
	{
		// add your own uninitialization code here
		Application::uninitialize();
	}
	
	void reinitialize(Application& self)
	{
		Application::reinitialize(self);
		// add your own reinitialization code here
	}
	
	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));

		options.addOption(
			Option("verbose", "v", "display messages on stdout")
				.required(false)
				.repeatable(false));

		options.addOption(
			Option("cookies", "c", "resend cookies")
				.required(false)
				.repeatable(false));

		options.addOption(
			Option("uri", "u", "HTTP URI")
				.required(true)
				.repeatable(false)
				.argument("uri"));
				
		options.addOption(
			Option("repetitions", "r", "fetch repetitions")
				.required(false)
				.repeatable(false)
				.argument("repetitions"));

		options.addOption(
			Option("threads", "t", "thread count")
				.required(false)
				.repeatable(false)
				.argument("threads"));
	}
	
	void handleOption(const std::string& name, const std::string& value)
	{
		Application::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
		else if (name == "verbose")
			_verbose = true;
		else if (name == "cookies")
			_cookies = true;
		else if (name == "uri")
			_uri = value;
		else if (name == "repetitions")
			_repetitions = NumberParser::parse(value);
		else if (name == "threads")
			_threads = NumberParser::parse(value);
	}
	
	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A sample application that demonstrates some of the features of the Poco::Util::Application class.");
		helpFormatter.format(std::cout);
	}
	
	void defineProperty(const std::string& def)
	{
		std::string name;
		std::string value;
		std::string::size_type pos = def.find('=');
		if (pos != std::string::npos)
		{
			name.assign(def, 0, pos);
			value.assign(def, pos + 1, def.length() - pos);
		}
		else name = def;
		config().setString(name, value);
	}

	int main(const std::vector<std::string>& args)
	{
		if (_helpRequested)
		{
			displayHelp();
		}
		else
		{
			URI uri(_uri);
			std::vector<Thread*> threads;

			Stopwatch sw;
			sw.start();
			for (int i = 0; i < _threads; ++i)
			{
				Thread* pt = new Thread(_uri);
				poco_check_ptr(pt);
				threads.push_back(pt);
				HTTPClient* pHTTPClient = new HTTPClient(uri, _repetitions, _cookies, _verbose);
				poco_check_ptr(pHTTPClient);
				threads.back()->start(*pHTTPClient);
			}

			std::vector<Thread*>::iterator it = threads.begin();
			for(; it != threads.end(); ++it)
			{
				(*it)->join();
				delete *it;
			}
			sw.stop();

			HTTPClient::printStats(_uri, HTTPClient::totalAttempts(), HTTPClient::totalSuccessCount(), sw.elapsed());
		}
		
		return Application::EXIT_OK;
	}
	
private:
	bool _helpRequested;
	bool _verbose;
	bool _cookies;
	std::string _uri;
	int _repetitions;
	int _threads;
};


int main(int argc, char** argv)
{
	AutoPtr<HTTPLoadTest> pApp = new HTTPLoadTest;
	try
	{
		pApp->init(argc, argv);
	}
	catch (Poco::Exception& exc)
	{
		pApp->logger().log(exc);
		return Application::EXIT_CONFIG;
	}
	return pApp->run();
}

