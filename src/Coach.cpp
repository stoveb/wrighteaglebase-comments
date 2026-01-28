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
  * @file Coach.cpp
  * @brief 教练（coach）端逻辑实现
  *
  * 在 RoboCup 2D 仿真中，coach/trainer 是一种“场外视角”的客户端：
  * - 可以接收更完整的全局信息（fullstate / see_global 等，取决于 server 配置）。
  * - 可以在比赛开始前/特定规则下发送指令（如更换异构球员类型）。
  *
  * 本文件的核心职责：
  * - 启用教练端视觉（例如发送 (eye on)）。
  * - 根据预设策略（按位置线：前锋/后卫/中场/守门员）分配异构球员类型。
  * - 在每个周期更新阵型与世界模型，并在同步模式下发送 done。
  *
  * @note 本文件仅补充注释，不改动原有逻辑。
  */

#include <list>
#include <string>
#include <sstream>
#include "Coach.h"
#include "DynamicDebug.h"
#include "Formation.h"
#include "CommandSender.h"
#include "Parser.h"
#include "Thread.h"
#include "UDPSocket.h"
#include "WorldModel.h"
#include "Agent.h"
#include "Logger.h"
#include "TimeTest.h"
#include "PlayerParam.h"
#include <iostream>
using namespace std;

/**
 * @brief Coach 构造函数
 *
 * Coach 一般作为独立客户端运行，其依赖的 mpParser/mpObserver/mpAgent/mpWorldModel
 * 等对象通常在外部初始化并注入（见 Client/主控初始化逻辑）。
 */
Coach::Coach()
{
}

/**
 * @brief Coach 析构函数
 */
Coach::~Coach()
{

}

/**
 * @brief 向服务器发送教练端选项/初始化指令
 *
 * 目前主要做两件事：
 * 1. 反复发送 `(eye on)` 直到 parser 确认成功（开启教练端观察）。
 * 2. 根据异构球员参数（hetero player types）为队友分配 player type：
 *    - 先按 effectiveSpeedMax 排序得到候选类型集合；
 *    - 优先给前锋分配速度更高的类型；
 *    - 后卫再分配剩余中较快的类型；
 *    - 守门员单独挑选 dashPowerRate 最大的类型（追求加速度/爆发）；
 *    - 最后给中场分配剩余类型。
 *
 * @note 该函数依赖 coach 端能获取队友是否存活（fullstate）。
 * @note 过程中会插入 `WaitFor` 做节流，避免指令发送过快。
 */
void Coach::SendOptionToServer()
{
	while (!mpParser->IsEyeOnOk())
	{
		UDPSocket::instance().Send("(eye on)");
		WaitFor(200);
	}
	vector<pair<int , double> > a ;
	for (int i=0 ;i <= 17 ; ++i){  //这里的18不知道如何去引用PlayerParam::DEFAULT_PLAYER_TYPES
		a.push_back(pair<int , double>(i,PlayerParam::instance().HeteroPlayer(i).effectiveSpeedMax()));
	}
	sort(a.begin(),a.end(),PlayerCompare());
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
        	mpAgent->CheckCommands(mpObserver);
        	if(mpAgent->GetFormation().GetTeammateRoleType(i).mLineType==LT_Forward){
        		mpAgent->ChangePlayerType(i, a.back().first);
        		a.pop_back();
        	}
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
        	mpAgent->CheckCommands(mpObserver);
        	if(mpAgent->GetFormation().GetTeammateRoleType(i).mLineType==LT_Defender){
        		mpAgent->ChangePlayerType(i, a.back().first);
        	a.pop_back();
        	}
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}
	if(mpObserver->Teammate_Fullstate(PlayerParam::instance().ourGoalieUnum()).IsAlive()){
	double maxAcceleration = 0 ;
	vector<pair<int , double> > ::iterator goalie;
	for (vector<pair<int , double> > ::iterator it = a.begin(); it != a.end(); it++){
		if(PlayerParam::instance().HeteroPlayer((*it).first).dashPowerRate() > maxAcceleration){
			maxAcceleration = PlayerParam::instance().HeteroPlayer((*it).first).dashPowerRate();
			goalie = it;
		}
	}
	mpAgent->CheckCommands(mpObserver);
	mpAgent->ChangePlayerType(PlayerParam::instance().ourGoalieUnum(), (*goalie).first);
	a.erase(goalie);
	}

    mpObserver->SetCommandSend();
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
            mpAgent->CheckCommands(mpObserver);
        	if(mpAgent->GetFormation().GetPlayerRoleType(i).mLineType==LT_Midfielder){
        		mpAgent->ChangePlayerType(i, a.back().first);
        	a.pop_back();
    		}
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}

    /** check whether each player's type is changed OK */
	WaitFor(200);
/*	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
            while (!mpParser->IsChangePlayerTypeOk(i) && mpObserver->CurrentTime().T() < 1)
            {
			    mpAgent->CheckCommands(mpObserver);
			    mpAgent->ChangePlayerType(i, i);
			    mpObserver->SetCommandSend();
                WaitFor(200);
		    }
        }
	}
	*/
/*	a.clear();
	for (int i=1 ;i <= 18 ; ++i){  //这里的18不知道如何去引用PlayerParam::DEFAULT_PLAYER_TYPES
		a.push_back(pair<int , double>(i,PlayerParam::instance().HeteroPlayer(i).kickRand()));
	}
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
			mpAgent->CheckCommands(mpObserver);
			if(mpAgent->GetFormation().GetPlayerRoleType(i).mLineType==LT_Forward)
				mpAgent->ChangePlayerType(i, a.back().first);
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
			mpAgent->CheckCommands(mpObserver);
			if(mpAgent->GetFormation().GetPlayerRoleType(i).mLineType==LT_Defender)
				mpAgent->ChangePlayerType(i, a.back().first);
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}
	for (int i = 1; i <= TEAMSIZE; ++i)
    {
        if (i != PlayerParam::instance().ourGoalieUnum() && mpObserver->Teammate_Fullstate(i).IsAlive())
        {
			mpAgent->CheckCommands(mpObserver);
			if(mpAgent->GetFormation().GetPlayerRoleType(i).mLineType==LT_Midfielder)
				mpAgent->ChangePlayerType(i, a.back().first);
            mpObserver->SetCommandSend();
            WaitFor(5);
        }
	}*/
}

/**
 * @brief 教练端主循环
 *
 * 每个周期的更新步骤（顺序要求较严格）：
 * 1. 锁定 observer，避免解析线程/网络线程并发写入。
 * 2. 更新阵型（队友阵型）。
 * 3. 检查上一周期命令发送情况。
 * 4. 更新世界模型（基于 observer 最新数据）。
 * 5. 解锁 observer。
 * 6. 若为同步模式，则发送 done 告知 server 本周期处理结束。
 * 7. 记录日志。
 */
/*
 * The main loop for coach.
 */
void Coach::Run()
{
	mpObserver->Lock();

	/** 下面几个更新顺序不能变 */
	Formation::instance.SetTeammateFormations();
	mpAgent->CheckCommands(mpObserver); // 检查上周期发送命令情况
	mpWorldModel->Update(mpObserver);

	mpObserver->UnLock();

	//do planning...

	if (ServerParam::instance().synchMode()) {
		mpAgent->Done();
	}

	Logger::instance().LogSight();
}
