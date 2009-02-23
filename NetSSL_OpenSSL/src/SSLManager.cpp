//
// SSLManager.cpp
//
// $Id: //poco/Main/NetSSL_OpenSSL/src/SSLManager.cpp#14 $
//
// Library: NetSSL_OpenSSL
// Package: SSLCore
// Module:  SSLManager
//
// Copyright (c) 2006-2009, Applied Informatics Software Engineering GmbH.
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


#include "Poco/Net/SSLManager.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/Utility.h"
#include "Poco/Net/PrivateKeyPassphraseHandler.h"
#include "Poco/Net/SSLInitializer.h"
#include "Poco/Net/SSLException.h"
#include "Poco/SingletonHolder.h"
#include "Poco/Delegate.h"
#include "Poco/Util/Application.h"
#include "Poco/Util/OptionException.h"
#include "Poco/Util/LayeredConfiguration.h"


namespace Poco {
namespace Net {


const std::string SSLManager::CFG_PRIV_KEY_FILE("privateKeyFile");
const std::string SSLManager::CFG_CERTIFICATE_FILE("certificateFile");
const std::string SSLManager::CFG_CA_LOCATION("caConfig");
const std::string SSLManager::CFG_VER_MODE("verificationMode");
const Context::VerificationMode SSLManager::VAL_VER_MODE(Context::VERIFY_STRICT);
const std::string SSLManager::CFG_VER_DEPTH("verificationDepth");
const int         SSLManager::VAL_VER_DEPTH(9);
const std::string SSLManager::CFG_ENABLE_DEFAULT_CA("loadDefaultCAFile");
const bool        SSLManager::VAL_ENABLE_DEFAULT_CA(false);
const std::string SSLManager::CFG_CYPHER_LIST("cypherList");
const std::string SSLManager::VAL_CYPHER_LIST("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
const std::string SSLManager::CFG_DELEGATE_HANDLER("privateKeyPassphraseHandler.name");
const std::string SSLManager::VAL_DELEGATE_HANDLER("KeyConsoleHandler");
const std::string SSLManager::CFG_CERTIFICATE_HANDLER("invalidCertificateHandler.name");
const std::string SSLManager::VAL_CERTIFICATE_HANDLER("ConsoleCertificateHandler");
const std::string SSLManager::CFG_SERVER_PREFIX("openSSL.server.");
const std::string SSLManager::CFG_CLIENT_PREFIX("openSSL.client.");


SSLManager::SSLManager()
{
	SSLInitializer::initialize();
}


SSLManager::~SSLManager()
{
	PrivateKeyPassPhrase.clear();
	ClientVerificationError.clear();
	ServerVerificationError.clear();
	SSLInitializer::uninitialize();
}


SSLManager& SSLManager::instance()
{
	static Poco::SingletonHolder<SSLManager> singleton;
	return *singleton.get();
}


void SSLManager::initializeServer(PrivateKeyPassphraseHandlerPtr ptrPassPhraseHandler, InvalidCertificateHandlerPtr ptrHandler, Context::Ptr ptrContext)
{
	_ptrServerPassPhraseHandler  = ptrPassPhraseHandler;
	_ptrServerCertificateHandler = ptrHandler;
	_ptrDefaultServerContext     = ptrContext;
}


void SSLManager::initializeClient(PrivateKeyPassphraseHandlerPtr ptrPassPhraseHandler, InvalidCertificateHandlerPtr ptrHandler, Context::Ptr ptrContext)
{
	_ptrClientPassPhraseHandler  = ptrPassPhraseHandler;
	_ptrClientCertificateHandler = ptrHandler;
	_ptrDefaultClientContext     = ptrContext;
}


Context::Ptr SSLManager::defaultServerContext()
{
	if (!_ptrDefaultServerContext)
		initDefaultContext(true);

	return _ptrDefaultServerContext;
}


Context::Ptr SSLManager::defaultClientContext()
{
	if (!_ptrDefaultClientContext)
		initDefaultContext(false);

	return _ptrDefaultClientContext;
}


SSLManager::PrivateKeyPassphraseHandlerPtr SSLManager::serverPassPhraseHandler()
{
	if (!_ptrServerPassPhraseHandler)
		initPassPhraseHandler(true);

	return _ptrServerPassPhraseHandler;
}


SSLManager::PrivateKeyPassphraseHandlerPtr SSLManager::clientPassPhraseHandler()
{
	if (!_ptrClientPassPhraseHandler)
		initPassPhraseHandler(false);

	return _ptrClientPassPhraseHandler;
}


SSLManager::InvalidCertificateHandlerPtr SSLManager::serverCertificateHandler()
{
	if (!_ptrServerCertificateHandler)
		initCertificateHandler(true);

	return _ptrServerCertificateHandler;
}


SSLManager::InvalidCertificateHandlerPtr SSLManager::clientCertificateHandler()
{
	if (!_ptrClientCertificateHandler)
		initCertificateHandler(false);

	return _ptrClientCertificateHandler;
}


int SSLManager::verifyCallback(bool server, int ok, X509_STORE_CTX* pStore)
{
	if (!ok)
	{
		X509* pCert = X509_STORE_CTX_get_current_cert(pStore);
		X509Certificate x509(pCert);
		int depth = X509_STORE_CTX_get_error_depth(pStore);
		int err = X509_STORE_CTX_get_error(pStore);
		std::string error(X509_verify_cert_error_string(err));
		VerificationErrorArgs args(x509, depth, err, error);
		if (server)
			SSLManager::instance().ServerVerificationError.notify(&SSLManager::instance(), args);
		else
			SSLManager::instance().ClientVerificationError.notify(&SSLManager::instance(), args);
		ok = args.getIgnoreError() ? 1 : 0;
	}

	return ok;
}


int SSLManager::privateKeyPasswdCallback(char* pBuf, int size, int flag, void* userData)
{
	std::string pwd;
	SSLManager::instance().PrivateKeyPassPhrase.notify(&SSLManager::instance(), pwd);

	strncpy(pBuf, (char *)(pwd.c_str()), size);
	pBuf[size - 1] = '\0';
	if (size > pwd.length())
		size = (int) pwd.length();

	return size;
}


void SSLManager::initDefaultContext(bool server)
{
	if (server && _ptrDefaultServerContext) return;
	if (!server && _ptrDefaultClientContext) return;

	initEvents(server);

	Poco::Util::LayeredConfiguration& config = Poco::Util::Application::instance().config();
	std::string prefix = server ? CFG_SERVER_PREFIX : CFG_CLIENT_PREFIX;

	// mandatory options
	std::string privKeyFile = config.getString(prefix + CFG_PRIV_KEY_FILE, "");
	std::string certFile = config.getString(prefix + CFG_CERTIFICATE_FILE, privKeyFile);	
	std::string caLocation = config.getString(prefix + CFG_CA_LOCATION, "");

	if (certFile.empty() && privKeyFile.empty())
		throw SSLException("Configuration error: no certificate file has been specified.");

	// optional options for which we have defaults defined
	Context::VerificationMode verMode = VAL_VER_MODE;
	if (config.hasProperty(prefix + CFG_VER_MODE))
	{
		// either: none, relaxed, strict, once
		std::string mode = config.getString(prefix + CFG_VER_MODE);
		verMode = Utility::convertVerificationMode(mode);
	}

	int verDepth = config.getInt(prefix + CFG_VER_DEPTH, VAL_VER_DEPTH);
	bool loadDefCA = config.getBool(prefix + CFG_ENABLE_DEFAULT_CA, VAL_ENABLE_DEFAULT_CA);
	std::string cypherList = config.getString(prefix + CFG_CYPHER_LIST, VAL_CYPHER_LIST);
	if (server)
		_ptrDefaultServerContext = new Context(Context::SERVER_USE, privKeyFile, certFile, caLocation, verMode, verDepth, loadDefCA, cypherList);
	else
		_ptrDefaultClientContext = new Context(Context::CLIENT_USE, privKeyFile, certFile, caLocation, verMode, verDepth, loadDefCA, cypherList);
}


void SSLManager::initEvents(bool server)
{
	initPassPhraseHandler(server);
	initCertificateHandler(server);
}


void SSLManager::initPassPhraseHandler(bool server)
{
	if (server && _ptrServerPassPhraseHandler) return;
	if (!server && _ptrClientPassPhraseHandler) return;
	
	std::string prefix = server ? CFG_SERVER_PREFIX : CFG_CLIENT_PREFIX;
	Poco::Util::LayeredConfiguration& config = Poco::Util::Application::instance().config();

	std::string className(config.getString(prefix + CFG_DELEGATE_HANDLER, VAL_DELEGATE_HANDLER));

	const PrivateKeyFactory* pFactory = 0;
	if (privateKeyFactoryMgr().hasFactory(className))
	{
		pFactory = privateKeyFactoryMgr().getFactory(className);
	}

	if (pFactory)
	{
		if (server)
			_ptrServerPassPhraseHandler = pFactory->create(server);
		else
			_ptrClientPassPhraseHandler = pFactory->create(server);
	}
	else throw Poco::Util::UnknownOptionException(std::string("No PassPhrasehandler known with the name ") + className);
}
	

void SSLManager::initCertificateHandler(bool server)
{
	if (server && _ptrServerCertificateHandler) return;
	if (!server && _ptrClientCertificateHandler) return;

	std::string prefix = server ? CFG_SERVER_PREFIX : CFG_CLIENT_PREFIX;
	Poco::Util::LayeredConfiguration& config = Poco::Util::Application::instance().config();

	std::string className(config.getString(prefix+CFG_CERTIFICATE_HANDLER, VAL_CERTIFICATE_HANDLER));

	const CertificateHandlerFactory* pFactory = 0;
	if (certificateHandlerFactoryMgr().hasFactory(className))
	{
		pFactory = certificateHandlerFactoryMgr().getFactory(className);
	}

	if (pFactory)
	{
		if (server)
			_ptrServerCertificateHandler = pFactory->create(true);
		else
			_ptrClientCertificateHandler = pFactory->create(false);
	}
	else throw Poco::Util::UnknownOptionException(std::string("No InvalidCertificate handler known with the name ") + className);
}


} } // namespace Poco::Net
