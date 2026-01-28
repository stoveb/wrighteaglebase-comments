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
 * @file Player.cpp
 * @brief 球员类实现 - WrightEagleBase 的核心决策执行单元
 * 
 * 本文件实现了 Player 类，它是 WrightEagleBase 系统的核心组件，负责：
 * 1. 球员的主循环逻辑（感知-决策-执行）
 * 2. 与服务器的通信和参数设置
 * 3. 协调各个子系统的工作
 * 
 * 主要功能：
 * - Run(): 主循环，包含感知、决策和执行的完整流程
 * - SendOptionToServer(): 向服务器发送初始化参数
 * - 时间同步和状态管理
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "Player.h"
#include "DecisionTree.h"
#include "DynamicDebug.h"
#include "Formation.h"
#include "CommandSender.h"
#include "Parser.h"
#include "Thread.h"
#include "UDPSocket.h"
#include "WorldModel.h"
#include "Agent.h"
#include "VisualSystem.h"
#include "Logger.h"
#include "CommunicateSystem.h"
#include "TimeTest.h"
#include "Dasher.h"

/**
 * @brief Player 类构造函数
 * 
 * 初始化球员对象，创建决策树实例。
 * Player 类是整个球员智能体系统的核心控制器。
 */
Player::Player():
	mpDecisionTree( new DecisionTree )
{
	// 决策树是球员的核心决策模块，负责选择最佳行为
}

/**
 * @brief Player 类析构函数
 * 
 * 清理资源，删除决策树实例。
 * 确保内存正确释放，避免内存泄漏。
 */
Player::~Player()
{
	delete mpDecisionTree;  // 释放决策树内存
}

/**
 * @brief 向服务器发送初始化参数
 * 
 * 在球员连接到服务器后，发送必要的初始化参数和设置：
 * 1. 设置教练语言版本 (clang 7,8)
 * 2. 设置同步视觉模式
 * 3. 关闭听觉（避免干扰）
 * 
 * 这些设置确保球员能够正确地与服务器通信并接收信息。
 * 每个设置后都等待服务器确认，确保参数设置成功。
 */
void Player::SendOptionToServer()
{
	// 第一步：设置教练语言版本
	// clang(7,8) 表示支持教练语言协议版本7和8
	while (!mpParser->IsClangOk())
	{
		mpAgent->CheckCommands(mpObserver);  // 检查并处理服务器命令
		mpAgent->Clang(7, 8);               // 发送教练语言版本设置
		mpObserver->SetCommandSend();       // 标记命令已发送
		WaitFor(200);                       // 等待200ms让服务器处理
	}

	// 第二步：设置同步视觉模式
	// 同步视觉模式确保视觉信息与模拟周期同步
	while (!mpParser->IsSyncOk())
	{
		mpAgent->CheckCommands(mpObserver);  // 检查并处理服务器命令
		mpAgent->SynchSee();                 // 发送同步视觉设置
		mpObserver->SetCommandSend();       // 标记命令已发送
		WaitFor(200);                       // 等待200ms让服务器处理
	}

	// 第三步：关闭听觉（设置为不关闭）
	// false表示不关闭听觉，保持接收hear信息的能力
	mpAgent->CheckCommands(mpObserver);  // 检查并处理服务器命令
	mpAgent->EarOff(false);              // 设置听觉状态
	mpObserver->SetCommandSend();       // 标记命令已发送
	WaitFor(200);                       // 等待200ms让服务器处理
}

/**
 * @brief 球员主循环 - WrightEagleBase 的核心执行逻辑
 * 
 * 这是整个球员智能体的主循环，实现了完整的"感知-决策-执行"流程：
 * 
 * 1. 感知阶段 (Perception):
 *    - 更新阵型信息
 *    - 解析通信信息
 *    - 更新世界状态
 *    - 时间同步处理
 * 
 * 2. 决策阶段 (Decision):
 *    - 重置视觉请求
 *    - 通过决策树选择最佳行为
 *    - 视觉系统决策
 *    - 通信系统决策
 * 
 * 3. 执行阶段 (Execution):
 *    - 同步模式下发送done命令
 *    - 记录历史行为
 *    - 记录日志
 * 
 * @note 更新顺序非常重要，不能随意改变，否则可能导致状态不一致
 * @note 时间同步逻辑确保决策数据的正确更新
 */
void Player::Run()
{
    //TIMETEST("Run");  // 性能测试宏，可用于性能分析

	// 记录上次执行时间，用于时间同步和异常检测
	static Time last_time = Time(-100, 0);

	// === 感知阶段开始 ===
	// 锁定观察者，确保状态更新的原子性
	mpObserver->Lock();

	/** 下面几个更新顺序不能变 - 这是系统正确运行的关键 **/
	
	// 1. 更新队友阵型信息
	// 阵型系统需要首先更新，为后续决策提供基础信息
	Formation::instance.SetTeammateFormations();
	
	// 2. 更新通信系统并解析hear信息
	// 必须在更新世界状态之前解析通信信息，因为hear信息可能影响世界状态
	CommunicateSystem::instance().Update();
	
	// 3. 检查并处理服务器命令
	// 处理来自服务器的各种命令和确认
	mpAgent->CheckCommands(mpObserver);
	
	// 4. 更新世界模型
	// 基于观察者信息更新完整的世界状态
	mpWorldModel->Update(mpObserver);

	// 解锁观察者，允许其他线程访问
	mpObserver->UnLock();

	// === 时间同步处理 ===
    // 获取当前世界时间
    const Time & time = mpAgent->GetWorldState().CurrentTime();

	// 检测时间异常，确保决策数据更新的正确性
	// 时间同步是系统正确运行的关键，必须保证时间正常递进
	if (last_time.T() >= 0) {
		// 检查时间是否正常递增（周期+1 或 秒数+1）
		// 正常情况下，时间应该按周期或秒数递增
		if (time != Time(last_time.T() + 1, 0) && time != Time(last_time.T(), last_time.S() + 1)) {
			// 如果时间没有正常递进，可能是服务器延迟或网络问题
			if (time == last_time) {
				// 强制推进时间，确保决策数据能够正确更新
				// 这种情况通常发生在服务器响应延迟时
				mpAgent->World().SetCurrentTime(Time(last_time.T(), last_time.S() + 1));
			}
		}
	}

	// 更新上次时间记录
	last_time = time;

	// 更新对手角色信息
	// TODO: 暂时放在这里，理想情况下应该由教练发送对手阵型信息
	// 当教练未发来对手阵型信息时，自己先计算对手角色
	Formation::instance.UpdateOpponentRole();

	// === 决策阶段开始 ===
	
	// 重置视觉请求，为新的决策周期做准备
	VisualSystem::instance().ResetVisualRequest();
	
	// 核心决策：通过决策树选择最佳行为
	// 这是整个系统的决策核心，基于当前世界状态选择最优动作
	mpDecisionTree->Decision(*mpAgent);

	// 视觉系统决策：确定下一周期的视觉需求
	VisualSystem::instance().Decision();
	
	// 通信系统决策：确定是否需要发送通信信息
	CommunicateSystem::instance().Decision();

	// === 执行阶段开始 ===
	
	// 同步模式下发送done命令
	// 告诉服务器本周期决策完成，可以进入下一周期
	// 只有在同步模式下才需要发送done命令
	if (ServerParam::instance().synchMode()) {
		mpAgent->Done();
	}

	// 记录历史行为
	// 保存当前周期的行为信息，供下一周期决策参考
	mpAgent->SetHistoryActiveBehaviors();

	// 记录视觉日志
	// 保存当前周期的视觉信息，用于调试和分析
	Logger::instance().LogSight();
}
