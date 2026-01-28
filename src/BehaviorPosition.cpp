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
 * @file BehaviorPosition.cpp
 * @brief 位置行为实现 - WrightEagleBase 的站位动作执行核心
 * 
 * 本文件实现了 BehaviorPositionExecuter 和 BehaviorPositionPlanner 类，它们是 WrightEagleBase 系统的站位动作核心：
 * 1. 实现智能的站位动作执行
 * 2. 处理不同情况下的站位策略
 * 3. 优化站位位置和时机选择
 * 4. 提供灵活的站位规划支持
 * 
 * 位置行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在无球状态下实现有效的战术站位。
 * 
 * 主要功能：
 * - 智能站位：根据战术需要选择站位位置
 * - 阵型站位：保持战术阵型的位置
 * - 协作站位：与队友协调站位位置
 * - 动态调整：根据比赛状态调整站位
 * 
 * 技术特点：
 * - 战术导向：基于战术系统的站位决策
 * - 动态调整：根据球和对手位置调整站位
 * - 协作意识：考虑队友位置的站位策略
 * - 智能避让：避免与队友位置冲突
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorPosition.h"
#include "Agent.h"
#include "Strategy.h"
#include "InterceptInfo.h"
#include "Dasher.h"
#include "VisualSystem.h"
#include "BehaviorPass.h"
#include "Logger.h"
#include "TimeTest.h"
#include <sstream>
#include "Evaluation.h"
using namespace std;

/**
 * @brief 位置行为类型常量
 * 
 * 定义位置行为的行为类型为BT_Position，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorPositionExecuter::BEHAVIOR_TYPE = BT_Position;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册位置行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorPositionExecuter>();
}

/**
 * @brief BehaviorPositionExecuter 构造函数
 * 
 * 初始化位置行为执行器，设置与智能体的关联。
 * 位置行为执行器负责具体的站位动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorPositionExecuter::BehaviorPositionExecuter(Agent & agent) :
	BehaviorExecuterBase<BehaviorAttackData> (agent)
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorPositionExecuter 析构函数
 * 
 * 清理位置行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorPositionExecuter::~BehaviorPositionExecuter(void)
{
	// 析构函数体为空
}

/**
 * @brief 执行位置行为
 * 
 * 这是位置行为执行的核心函数，负责实现具体的站位动作。
 * 该函数调用跑动系统的GoToPoint方法来实现站位。
 * 
 * 执行过程：
 * 1. 记录站位日志信息
 * 2. 调用跑动系统执行站位
 * 3. 返回执行结果
 * 
 * @param beh 活跃的位置行为，包含目标位置和参数
 * @return bool 位置行为是否成功执行
 * 
 * @note 使用Dasher系统的GoToPoint方法实现站位
 * @note 站位动作会自动计算最优路径和时机
 * @note 参数说明：beh.mBuffer为缓冲区，beh.mPower为跑动力度
 */
bool BehaviorPositionExecuter::Execute(const ActiveBehavior & beh)
{
	// === 记录站位日志 ===
	// 记录当前位置和目标位置，便于调试和分析
	Logger::instance().LogGoToPoint(mSelfState.GetPos(), beh.mTarget, "@Position");

	// === 执行站位动作 ===
	// 调用跑动系统的GoToPoint方法来实现站位
	// 参数说明：
	// - mAgent: 智能体引用
	// - beh.mTarget: 目标位置
	// - beh.mBuffer: 缓冲区大小
	// - beh.mPower: 跑动力度
	// - false: 不允许倒跑
	// - true: 优先转身调整方向
	return Dasher::instance().GoToPoint(mAgent, beh.mTarget, beh.mBuffer, beh.mPower, false, true);
}

/**
 * @brief BehaviorPositionPlanner 析构函数
 * 
 * 清理位置行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorPositionPlanner::~BehaviorPositionPlanner()
{
	// 析构函数体为空
}

/**
 * @brief BehaviorPositionPlanner 构造函数
 * 
 * 初始化位置行为规划器，设置与智能体的关联。
 * 位置行为规划器负责站位决策的规划和评估。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 */
BehaviorPositionPlanner::BehaviorPositionPlanner(Agent &agent) :
	BehaviorPlannerBase<BehaviorAttackData> (agent)
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief 位置行为规划函数
 * 
 * 这是位置行为规划的核心函数，负责生成和选择最佳站位行为。
 * 该函数根据当前比赛状态和战术需要，生成站位决策。
 * 
 * 规划过程：
 * 1. 检查是否已有其他行为
 * 2. 检查是否适合站位（守门员、对手控制球等）
 * 3. 获取战术站位位置
 * 4. 处理队友持球情况
 * 5. 生成站位候选行为
 * 6. 选择最优站位行为
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 只有在没有其他行为时才考虑站位
 * @note 守门员和对手控制球时不进行站位
 * @note 特殊处理队友持球时的站位策略
 */
void BehaviorPositionPlanner::Plan(ActiveBehaviorList &behavior_list)
{
	// === 检查行为列表 ===
	// 如果已有其他行为，则不需要站位
	if (!behavior_list.empty()) return;

	// === 检查站位条件 ===
	// 守门员不进行站位规划
	if (mSelfState.IsGoalie()) return;
	
	// 对手控制球时不进行站位规划
	if (mStrategy.IsOppControl()) return;
	
	// 检查比赛模式，某些模式下不进行站位
	if (mWorldState.GetPlayMode() > PM_Opp_Mode) return;

	// === 创建位置行为 ===
	ActiveBehavior position(mAgent, BT_Position);
	Vector target;
	position.mBuffer = 1.0;
	target = mStrategy.GetTeammateSBSPPosition(mSelfState.GetUnum(), mBallState.GetPos());
	if(mPositionInfo.GetTeammateWithBall() && mPositionInfo.GetClosestPlayerToTeammate(mPositionInfo.GetTeammateWithBall()) == mSelfState.GetUnum()){
		if((mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToTeammate(mSelfState.GetUnum())).GetPos()-
				mSelfState.GetPos()).Mod() >= 1.0)
		position.mTarget = target;
		else {
			position.mTarget.SetX(target.X() + 1.0);
			position.mTarget.SetY(target.Y());
		}

	}
	else position.mTarget = target;

	if(mWorldState.GetPlayMode() == PM_Our_Goalie_Free_Kick  &&
			mAgent.GetFormation().GetMyRole().mLineType == LT_Forward){
		if(mAgent.GetFormation().GetMyRole().mPositionType ==PT_Left){
			position.mTarget.SetY(26);
		}
		if(mAgent.GetFormation().GetMyRole().mPositionType ==PT_Right){
			position.mTarget.SetY(-26);
		}
	}

	Unum opp =mPositionInfo.GetClosestOpponentToPoint(position.mTarget);
	if(mWorldState.GetOpponent(opp).GetPos().Dist(position.mTarget) < 1.5){
			int p =mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToPlayer(opp)).GetPos().Y() > mWorldState.GetOpponent(opp).GetPos().X() ? -1:1;
			position.mTarget.SetY(target.Y() + p * 2);
	}

	if(mPositionInfo.GetOpponentOffsideLine() > mBallState.GetPos().X()){
		position.mTarget.SetX(Min(target.X(),(mPositionInfo.GetOpponentOffsideLine())));
	}
	if(position.mTarget.X()> mSelfState.GetPos().X() && mAgent.GetFormation().GetMyRole().mLineType == LT_Forward){
	position.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower());
	}
	else 	if(position.mTarget.X()< mSelfState.GetPos().X() && mAgent.GetFormation().GetMyRole().mLineType == LT_Defender){
		position.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower());
	}
	else position.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower())/2;
	position.mEvaluation = Evaluation::instance().EvaluatePosition(position.mTarget, false);

	behavior_list.push_back(position);
}

