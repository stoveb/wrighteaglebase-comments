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
 * @file Client.cpp
 * @brief 客户端核心实现 - WrightEagleBase 的客户端管理核心
 * 
 * 本文件实现了 Client 类，它是 WrightEagleBase 系统的客户端管理核心：
 * 1. 管理所有系统组件的初始化和销毁
 * 2. 协调各个模块之间的交互
 * 3. 管理线程的创建和销毁
 * 4. 提供系统运行的主循环
 * 
 * 客户端核心是 WrightEagleBase 的主控制器，
 * 负责整个系统的生命周期管理。
 * 
 * 主要功能：
 * - 系统初始化：初始化所有系统组件
 * - 实例管理：管理各种单例实例的创建
 * - 线程管理：管理解析器和命令发送线程
 * - 资源管理：管理系统资源的分配和释放
 * - 主循环：提供系统运行的主循环
 * 
 * 技术特点：
 * - 单例模式：大量使用单例模式管理全局实例
 * - 多线程：支持多线程并发处理
 * - 模块化：高度模块化的系统架构
 * - 资源管理：完善的资源管理机制
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "Client.h"
#include "DynamicDebug.h"
#include "Formation.h"
#include "Kicker.h"
#include "CommandSender.h"
#include "Parser.h"
#include "Thread.h"
#include "UDPSocket.h"
#include "WorldModel.h"
#include "Agent.h"
#include "Logger.h"
#include "CommunicateSystem.h"
#include "NetworkTest.h"
#include "Dasher.h"
#include "Tackler.h"
#include "TimeTest.h"
#include "VisualSystem.h"
#include "InterceptModel.h"
#include "Plotter.h"

/**
 * @brief Client 构造函数
 * 
 * 初始化客户端系统，创建所有必要的组件和实例。
 * 这是整个 WrightEagleBase 系统的初始化入口。
 * 
 * 初始化顺序：
 * 1. 随机数种子初始化
 * 2. 核心组件创建（观察者、世界模型）
 * 3. 辅助实例创建（调试、日志、绘图等）
 * 4. 参数系统初始化
 * 5. 基础模型初始化
 * 6. 智能动作系统初始化
 * 7. 其他实用实例初始化
 * 8. 线程系统初始化
 * 
 * @note 构造函数会创建大量单例实例，需要保证初始化顺序
 * @note 部分实例不能在构造函数中创建，需要时再创建
 */
Client::Client() {
	// === 随机数种子初始化 ===
	// 全局随机数种子只初始化一次
	srand(time(0)); //global srand once
	srand48(time(0));

	// === 核心组件初始化 ===
	/** Observer and World Model */
	mpAgent         = 0;                    // 智能体指针初始化
    mpObserver      = new Observer;         // 创建观察者实例
	mpWorldModel    = new WorldModel;       // 创建世界模型实例

    // === 单例实例初始化 ===
    // instance放在这里创建以节省后面的运行效率，部分instance不能放在这里，需要时创建

    /** Assistant instance */
    // 辅助实例：用于调试、测试、日志等
	TimeTest::instance();                    // 时间测试实例
    NetworkTest::instance();                 // 网络测试实例
    DynamicDebug::instance();                // 动态调试实例
    Logger::instance().Initial(mpObserver, &(mpWorldModel->World(false))); // 日志系统初始化
    Plotter::instance();                    // 绘图系统实例

    /** Param */
    // 参数系统：服务器参数和球员参数
    ServerParam::instance();                 // 服务器参数实例
    PlayerParam::instance();                 // 球员参数实例

	/** Base Model */
    // 基础模型：截球模型等
    InterceptModel::instance();             // 截球模型实例

    /** Smart Action */
    // 智能动作系统：跑动、铲球、踢球等
    Dasher::instance();                      // 跑动系统实例
    Tackler::instance();                     // 铲球系统实例
    Kicker::instance();                     // 踢球系统实例

    /** Other Useful instance */
    // 其他实用实例：行为工厂、网络、视觉、通信等
    BehaviorFactory::instance();            // 行为工厂实例
    UDPSocket::instance();                   // UDP套接字实例
    VisualSystem::instance();                // 视觉系统实例
    CommunicateSystem::instance();           // 通信系统实例

	// === 线程系统初始化 ===
	/** Parser thread and CommandSend thread */
    mpCommandSender = new CommandSender(mpObserver); // 命令发送器

	// === 解析器初始化 ===
	//ServerPlayModeMap::instance(); // 服务器比赛模式映射（注释掉）

	mpParser = new Parser(mpObserver);           // 消息解析器

}

/**
 * @brief Client 析构函数
 * 
 * 清理客户端系统资源，销毁所有创建的组件。
 * 这是整个 WrightEagleBase 系统的清理入口。
 * 
 * 清理顺序：
 * 1. 停止线程系统
 * 2. 销毁解析器和命令发送器
 * 3. 销毁智能体
 * 4. 销毁观察者和世界模型
 * 
 * @note 析构函数会按照与初始化相反的顺序清理资源
 * @note 需要确保所有线程都已经停止
 */
Client::~Client()
{  
	delete mpCommandSender;
	delete mpParser;

	delete mpObserver;
	delete mpWorldModel;
	delete mpAgent;
}

void Client::RunDynamicDebug()
{
	static char msg[MAX_MESSAGE];
	DynamicDebug::instance().Initial(mpObserver); // 动态调试的初始化，注意位置不能移动

	DynamicDebug::instance().Run(msg); // 初始化信息
	mpParser->ParseInitializeMsg(msg);

	ConstructAgent();

	MessageType msg_type;

	bool first_parse = true;

	while (true)
	{
		msg_type = DynamicDebug::instance().Run(msg);

		switch (msg_type)
		{
		case MT_Parse:
			if (first_parse) {
				mpObserver->Reset();
				first_parse = false;
			}
			mpParser->Parse(msg);
			break;
		case MT_Run:
			Run();
			Logger::instance().Flush(); //flush log
			mpObserver->SetPlanned();
			break;
		case MT_Send:
			mpCommandSender->Run();
			first_parse = true;
			break;
		default:
			return;
		}
	}
}

void Client::RunNormal()
{

	mpCommandSender->Start(); //发送命令线程，向server发送信息
	Logger::instance().Start(); //log线程
	mpParser->Start(); //分析线程，接受server发来的信息

	int past_cycle = 0;
	do
	{
		WaitFor(100); // wait for the parser thread to connect the server
		if (++past_cycle > 20)
		{
			std::cout << PlayerParam::instance().teamName() << ": Connect Server Error ..." << std::endl;
			return;
		}
	} while (mpParser->IsConnectServerOk() == false);

	ConstructAgent();

	SendOptionToServer();

	MainLoop();

	WaitFor(mpObserver->SelfUnum() * 100);
    if (mpObserver->SelfUnum() == 0)
    {
        std::cout << PlayerParam::instance().teamName() << " Coach: Bye ..." << std::endl;
    }
    else
    {
        std::cout << PlayerParam::instance().teamName() << " " << mpObserver->SelfUnum() << ": Bye ..." << std::endl;
    }
}

void Client::ConstructAgent()
{
	Assert(mpAgent == 0);
	if (mpObserver->SelfUnum() > 0 && mpObserver->SelfUnum() < TRAINER_UNUM) 
	{
		mpWorldModel->World(false).Teammate(mpObserver->SelfUnum()).SetIsAlive(true);
		std::cout << "WrightEagle 2012: constructing agent for player " << mpObserver->SelfUnum() << "..." << std::endl;
	}
	else if (mpObserver->SelfUnum() == TRAINER_UNUM) 
	{
        	std::cout << "WrightEagle 2012: constructing agent for trainer... " <<  std::endl;
	}
    	else 
	{
        	std::cout << "WrightEagle 2012: constructing agent for coach..." << std::endl;
   	 }
	mpAgent = new Agent(mpObserver->SelfUnum(), mpWorldModel, false); //要知道号码才能初始化

	Formation::instance.AssignWith(mpAgent);
	mpCommandSender->RegisterAgent(mpAgent);
	CommunicateSystem::instance().Initial(mpObserver , mpAgent); //init communicate system
	VisualSystem::instance().Initial(mpAgent);
}

void Client::MainLoop()
{
	while (mpObserver->WaitForNewInfo()) // 等待新视觉
	{
        NetworkTest::instance().AddDecisionBegin();

        // 比赛结束，bye ...
        if (mpObserver->GetPlayMode() == PM_Time_Over)
        {
            mpAgent->CheckCommands(mpObserver);
            mpAgent->Bye();

            mpObserver->SetPlanned();
            mpObserver->SetCommandSend();
            Logger::instance().SetFlushCond();

            break;
        }

        DynamicDebug::instance().AddMessage("\0", MT_Run); // 动态调试记录Run信息
        Run();

		mpObserver->SetPlanned();
		mpObserver->SetCommandSend(); //唤醒发送命令的线程
		Logger::instance().SetFlushCond(); // set flush cond and let the logger thread flush the logs to file.

        NetworkTest::instance().AddDecisionEnd(mpObserver->CurrentTime());
	}
}
