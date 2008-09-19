//
// SocketAddress.cpp
//
// $Id: //poco/1.3/Net/src/SocketAddress.cpp#2 $
//
// Library: Net
// Package: NetCore
// Module:  SocketAddress
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


#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/DNS.h"
#include "Poco/RefCountedObject.h"
#include "Poco/NumberParser.h"
#include "Poco/NumberFormatter.h"
#include <algorithm>
#include <cstring>


using Poco::RefCountedObject;
using Poco::NumberParser;
using Poco::NumberFormatter;
using Poco::UInt16;
using Poco::InvalidArgumentException;


namespace Poco {
namespace Net {


//
// SocketAddressImpl
//


class SocketAddressImpl: public RefCountedObject
{
public:
	virtual IPAddress host() const = 0;
	virtual UInt16 port() const = 0;
	virtual poco_socklen_t length() const = 0;
	virtual const struct sockaddr* addr() const = 0;
	virtual int af() const = 0;
	
protected:
	SocketAddressImpl()
	{
	}
	
	virtual ~SocketAddressImpl()
	{
	}
	
private:
	SocketAddressImpl(const SocketAddressImpl&);
	SocketAddressImpl& operator = (const SocketAddressImpl&);
};


class IPv4SocketAddressImpl: public SocketAddressImpl
{
public:
	IPv4SocketAddressImpl()
	{
		std::memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family = AF_INET;
		poco_set_sin_len(&_addr);
	}
	
	IPv4SocketAddressImpl(const struct sockaddr_in* addr)
	{
		std::memcpy(&_addr, addr, sizeof(_addr));
	}
	
	IPv4SocketAddressImpl(const void* addr, UInt16 port)
	{
		std::memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family = AF_INET;
		std::memcpy(&_addr.sin_addr, addr, sizeof(_addr.sin_addr));
		_addr.sin_port = port;
	}

	IPAddress host() const
	{
		return IPAddress(&_addr.sin_addr, sizeof(_addr.sin_addr));
	}
	
	UInt16 port() const
	{
		return _addr.sin_port;
	}

	poco_socklen_t length() const
	{
		return sizeof(_addr);
	}
	
	const struct sockaddr* addr() const
	{
		return reinterpret_cast<const struct sockaddr*>(&_addr);
	}
	
	int af() const
	{
		return _addr.sin_family;
	}

private:
	struct sockaddr_in _addr;
};


#if defined(POCO_HAVE_IPv6)


class IPv6SocketAddressImpl: public SocketAddressImpl
{
public:
	IPv6SocketAddressImpl(const struct sockaddr_in6* addr)
	{
		std::memcpy(&_addr, addr, sizeof(_addr));
	}
	
	IPv6SocketAddressImpl(const void* addr, UInt16 port)
	{
		std::memset(&_addr, 0, sizeof(_addr));
		_addr.sin6_family = AF_INET6;
		poco_set_sin6_len(&_addr);
		std::memcpy(&_addr.sin6_addr, addr, sizeof(_addr.sin6_addr));
		_addr.sin6_port = port;
	}

	IPAddress host() const
	{
		return IPAddress(&_addr.sin6_addr, sizeof(_addr.sin6_addr));
	}
	
	UInt16 port() const
	{
		return _addr.sin6_port;
	}

	poco_socklen_t length() const
	{
		return sizeof(_addr);
	}
	
	const struct sockaddr* addr() const
	{
		return reinterpret_cast<const struct sockaddr*>(&_addr);
	}

	int af() const
	{
		return _addr.sin6_family;
	}

private:
	struct sockaddr_in6 _addr;
};


#endif // POCO_HAVE_IPv6


//
// SocketAddress
//


SocketAddress::SocketAddress()
{
	_pImpl = new IPv4SocketAddressImpl;
}

	
SocketAddress::SocketAddress(const IPAddress& addr, Poco::UInt16 port)
{
	init(addr, port);	
}

	
SocketAddress::SocketAddress(const std::string& addr, Poco::UInt16 port)
{
	init(addr, port);
}


SocketAddress::SocketAddress(const std::string& addr, const std::string& port)
{
	init(addr, resolveService(port));
}


SocketAddress::SocketAddress(const std::string& hostAndPort)
{
	poco_assert (!hostAndPort.empty());
	
	std::string host;
	std::string port;
	std::string::const_iterator it  = hostAndPort.begin();
	std::string::const_iterator end = hostAndPort.end();
	if (*it == '[')
	{
		++it;
		while (it != end && *it != ']') host += *it++;
		if (it == end) throw InvalidArgumentException("Malformed IPv6 address");
		++it;
	}
	else
	{
		while (it != end && *it != ':') host += *it++;
	}
	if (it != end && *it == ':')
	{
		++it;
		while (it != end) port += *it++;
	}
	else throw InvalidArgumentException("Missing port number");
	init(host, resolveService(port));
}


SocketAddress::SocketAddress(const SocketAddress& addr)
{
	_pImpl = addr._pImpl;
	_pImpl->duplicate();
}


SocketAddress::SocketAddress(const struct sockaddr* addr, poco_socklen_t length)
{
	if (length == sizeof(struct sockaddr_in))
		_pImpl = new IPv4SocketAddressImpl(reinterpret_cast<const struct sockaddr_in*>(addr));
#if defined(POCO_HAVE_IPv6)
	else if (length == sizeof(struct sockaddr_in6))
		_pImpl = new IPv6SocketAddressImpl(reinterpret_cast<const struct sockaddr_in6*>(addr));
#endif
	else throw Poco::InvalidArgumentException("Invalid address length passed to SocketAddress()");
}


SocketAddress::~SocketAddress()
{
	_pImpl->release();
}

	
SocketAddress& SocketAddress::operator = (const SocketAddress& addr)
{
	if (&addr != this)
	{
		_pImpl->release();
		_pImpl = addr._pImpl;
		_pImpl->duplicate();
	}
	return *this;
}


void SocketAddress::swap(SocketAddress& addr)
{
	std::swap(_pImpl, addr._pImpl);
}

	
IPAddress SocketAddress::host() const
{
	return _pImpl->host();
}

	
Poco::UInt16 SocketAddress::port() const
{
	return ntohs(_pImpl->port());
}

	
poco_socklen_t SocketAddress::length() const
{
	return _pImpl->length();
}

	
const struct sockaddr* SocketAddress::addr() const
{
	return _pImpl->addr();
}


int SocketAddress::af() const
{
	return _pImpl->af();
}


std::string SocketAddress::toString() const
{
	std::string result;
	if (host().family() == IPAddress::IPv6)
		result.append("[");
	result.append(host().toString());
	if (host().family() == IPAddress::IPv6)
		result.append("]");
	result.append(":");
	result.append(NumberFormatter::format(port()));
	return result;
}


void SocketAddress::init(const IPAddress& host, Poco::UInt16 port)
{
	if (host.family() == IPAddress::IPv4)
		_pImpl = new IPv4SocketAddressImpl(host.addr(), htons(port));
#if defined(POCO_HAVE_IPv6)
	else if (host.family() == IPAddress::IPv6)
		_pImpl = new IPv6SocketAddressImpl(host.addr(), htons(port));
#endif
	else throw Poco::NotImplementedException("unsupported IP address family");
}


void SocketAddress::init(const std::string& host, Poco::UInt16 port)
{
	IPAddress ip;
	if (IPAddress::tryParse(host, ip))
	{
		init(ip, port);
	}
	else
	{
		HostEntry he = DNS::hostByName(host);
		if (he.addresses().size() > 0)
			init(he.addresses()[0], port);
		else throw HostNotFoundException("No address found for host", host);
	}
}


Poco::UInt16 SocketAddress::resolveService(const std::string& service)
{
	unsigned port;
	if (NumberParser::tryParseUnsigned(service, port) && port <= 0xFFFF)
	{
		return (UInt16) port;
	}
	else
	{
		struct servent* se = getservbyname(service.c_str(), NULL);
		if (se)
			return ntohs(se->s_port);
		else
			throw ServiceNotFoundException(service);
	}
}


} } // namespace Poco::Net
