/************************************************************************************
 * WrightEagle (Soccer Simulation League 2D)                                        *
 * BASE SOURCE CODE RELEASE 2016                                                    *
 * Copyright (c) 1998-2016 WrightEagle 2D Soccer Simulation Team,                   *
 *                         Multi-Agent Systems Lab.,                                *
 *                         School of Computer Science and Technology,               *
 *                         University of Science and Technology of China            *
 * All rights reserved.                                                             *
 *                                                                                  *
 * Redistribution and use in source and binary forms, with or without               *
 * modification, are permitted provided that the following conditions are met:      *
 *     * Redistributions of source code must retain the above copyright             *
 *       notice, this list of conditions and the following disclaimer.              *
 *     * Redistributions in binary form must reproduce the above copyright          *
 *       notice, this list of conditions and the following disclaimer in the        *
 *       documentation and/or other materials provided with the distribution.       *
 *     * Neither the name of the WrightEagle 2D Soccer Simulation Team nor the      *
 *       names of its contributors may be used to endorse or promote products       *
 *       derived from this software without specific prior written permission.      *
 *                                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
 * DISCLAIMED. IN NO EVENT SHALL WrightEagle 2D Soccer Simulation Team BE LIABLE    *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL       *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR       *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER       *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,    *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                *
 ************************************************************************************/

/**
 * @file UDPSocket.cpp
 * @brief UDP 套接字（UDPSocket）实现
 *
 * UDPSocket 封装跨平台的 UDP 通信，用于与 rcssserver 交换消息。
 * 功能包括：
 * - 初始化：解析主机名、创建 UDP 套接字、绑定本地任意端口、设置目标地址；
 * - 接收：阻塞式接收服务器消息，并自动更新目标端口（用于响应 server 的端口变化）；
 * - 发送：向目标地址发送字符串消息（若未初始化则仅打印到标准输出）。
 *
 * 设计要点：
 * - 单例模式，全局唯一套接字实例；
 * - Windows 使用 WSAStartup/WSACleanup 与 SOCKET 类型；
 * - Receive 返回实际字节数，并保证字符串以 '\0' 结尾。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "UDPSocket.h"

/**
 * @brief 构造函数（私有）
 */
UDPSocket::UDPSocket()
{
	mIsInitialOK = false;
}

/**
 * @brief 析构函数
 */
UDPSocket::~UDPSocket()
{
}

/**
 * @brief 获取全局单例
 *
 * @return UDPSocket& 全局 UDP 套接字实例
 */
UDPSocket & UDPSocket::instance()
{
    static UDPSocket udp_socket;
    return udp_socket;
}


//==============================================================================
/**
 * @brief 初始化 UDP 套接字
 *
 * 解析主机名、创建 UDP 套接字、绑定本地任意端口、设置目标地址。
 * 若主机名解析失败，则尝试直接当作 IP 地址处理。
 *
 * @param host 服务器主机名或 IP 地址
 * @param port 服务器端口
 */
void UDPSocket::Initial(const char *host, int port)
{
	struct hostent *host_ent ;
	struct in_addr *addr_ptr ;

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		PRINT_ERROR("WSAStartup failed");
	}
	if (LOBYTE( wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		PRINT_ERROR("WSASTartup version mismatch");
	}
#endif

	if ((host_ent = (struct hostent *)gethostbyname(host)) == 0)
	{
		if (inet_addr(host) == INADDR_NONE)
			PRINT_ERROR("Invalid host name");
	}
	else
	{
		addr_ptr = (struct in_addr *) *host_ent->h_addr_list ;
		host = inet_ntoa(*addr_ptr) ;
	}

	if( (mSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		PRINT_ERROR("Can't create socket") ;
	}

	memset(&mAddress, 0, sizeof(mAddress)) ;
	mAddress.sin_family         = AF_INET ;
	mAddress.sin_addr.s_addr    = htonl(INADDR_ANY) ;
	mAddress.sin_port           = htons(0) ;

	if(bind(mSockfd, (sockaddr *)&mAddress, sizeof(mAddress)) < 0)
	{
#ifdef WIN32
		closesocket(mSockfd);
#else
		close(mSockfd);
#endif
		PRINT_ERROR("Can't bind client to any port") ;
	}

	memset(&mAddress, 0, sizeof(mAddress)) ;
	mAddress.sin_family         = AF_INET ;
	mAddress.sin_addr.s_addr    = inet_addr( host ) ;
	mAddress.sin_port           = htons( port ) ;
	mIsInitialOK                = true;
}


//==============================================================================
/**
 * @brief 接收服务器消息
 *
 * 阻塞式接收一条 UDP 消息，自动更新目标端口（用于响应 server 的端口变化），
 * 并保证消息以 '\0' 结尾。
 *
 * @param msg 输出缓冲区（至少 MAX_MESSAGE+1 字节）
 * @return int 实际接收的字节数（不含结尾 '\0'）
 */
int UDPSocket::Receive(char *msg)
{
#ifdef WIN32
	int servlen;
#else
	socklen_t servlen ;
#endif

	sockaddr_in serv_addr;
	servlen = sizeof(serv_addr);
	int n = recvfrom(mSockfd, msg, MAX_MESSAGE, 0, (sockaddr *)&serv_addr, &servlen);
	if (n > 0)
	{
		msg[n] = '\0' ; // rccparser will crash if msg has no end
		mAddress.sin_port = serv_addr.sin_port ;
	}

	return n ;
}


//==============================================================================
/**
 * @brief 发送消息到服务器
 *
 * 若套接字已初始化，则通过 UDP 发送；否则仅打印到标准输出（用于离线模式）。
 *
 * @param msg 要发送的 C 字符串
 * @return int 实际发送的字节数（含结尾 '\0'）
 */
int UDPSocket::Send(const char *msg)
{
	if (mIsInitialOK == true)
	{
		int n = std::strlen(msg) ;
		n = sendto(mSockfd, msg, n+1, 0, (sockaddr *)&mAddress, sizeof(mAddress));
		return n;
	}

	std::cout << msg << std::endl;
	return std::strlen(msg);
}


//end of UDPSocket.cpp

