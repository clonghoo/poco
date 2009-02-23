//
// SecureServerSocketImpl.cpp
//
// $Id: //poco/Main/NetSSL_OpenSSL/src/SecureServerSocketImpl.cpp#9 $
//
// Library: NetSSL_OpenSSL
// Package: SSLSockets
// Module:  SecureServerSocketImpl
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


#include "Poco/Net/SecureServerSocketImpl.h"


namespace Poco {
namespace Net {


SecureServerSocketImpl::SecureServerSocketImpl(Context::Ptr pContext):
	_impl(new ServerSocketImpl, pContext)
{
}


SecureServerSocketImpl::~SecureServerSocketImpl()
{
}


SocketImpl* SecureServerSocketImpl::acceptConnection(SocketAddress& clientAddr)
{
	return _impl.acceptConnection(clientAddr);
}


void SecureServerSocketImpl::connect(const SocketAddress& address)
{
	throw Poco::InvalidAccessException("Cannot connect() a SecureServerSocket");
}


void SecureServerSocketImpl::connect(const SocketAddress& address, const Poco::Timespan& timeout)
{
	throw Poco::InvalidAccessException("Cannot connect() a SecureServerSocket");
}
	

void SecureServerSocketImpl::connectNB(const SocketAddress& address)
{
	throw Poco::InvalidAccessException("Cannot connect() a SecureServerSocket");
}
	

void SecureServerSocketImpl::bind(const SocketAddress& address, bool reuseAddress)
{
	_impl.bind(address, reuseAddress);
	reset(_impl.sockfd());
}

	
void SecureServerSocketImpl::listen(int backlog)
{
	_impl.listen(backlog);
	reset(_impl.sockfd());
}
	

void SecureServerSocketImpl::close()
{
	reset();
	_impl.close();
}
	

int SecureServerSocketImpl::sendBytes(const void* buffer, int length, int flags)
{
	throw Poco::InvalidAccessException("Cannot sendBytes() on a SecureServerSocket");
}


int SecureServerSocketImpl::receiveBytes(void* buffer, int length, int flags)
{
	throw Poco::InvalidAccessException("Cannot receiveBytes() on a SecureServerSocket");
}


int SecureServerSocketImpl::sendTo(const void* buffer, int length, const SocketAddress& address, int flags)
{
	throw Poco::InvalidAccessException("Cannot sendTo() on a SecureServerSocket");
}


int SecureServerSocketImpl::receiveFrom(void* buffer, int length, SocketAddress& address, int flags)
{
	throw Poco::InvalidAccessException("Cannot receiveFrom() on a SecureServerSocket");
}


void SecureServerSocketImpl::sendUrgent(unsigned char data)
{
	throw Poco::InvalidAccessException("Cannot sendUrgent() on a SecureServerSocket");
}
	

} } // namespace Poco::Net
