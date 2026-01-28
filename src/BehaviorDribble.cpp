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
 * @file BehaviorDribble.cpp
 * @brief 带球行为实现 - WrightEagleBase 的带球动作执行核心
 * 
 * 本文件实现了 BehaviorDribbleExecuter 类，它是 WrightEagleBase 系统的带球动作执行核心：
 * 1. 实现智能的带球动作执行
 * 2. 处理带球过程中的对手规避
 * 3. 优化带球路径和速度控制
 * 4. 提供灵活的带球策略支持
 * 
 * 带球行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在持球时实现有效的带球推进。
 * 
 * 主要功能：
 * - 智能带球：根据对手位置调整带球策略
 * - 速度控制：根据体力状况调整带球速度
 * - 路径优化：选择最优的带球路径
 * - 对手规避：智能避开防守球员
 * 
 * 技术特点：
 * - 动态调整：实时调整带球参数
 * - 体力管理：根据体力状况调整策略
 * - 预测模型：使用位置预测提高精度
 * - 多种模式：支持正常和快速带球模式
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorDribble.h"
#include "Agent.h"
#include "Kicker.h"
#include "WorldState.h"
#include "Strategy.h"
#include "Formation.h"
#include "Dasher.h"
#include "Types.h"
#include "Logger.h"
#include "VisualSystem.h"
#include "CommunicateSystem.h"
#include "Utilities.h"
#include <sstream>
#include <vector>
#include <utility>
#include "Evaluation.h"
#include <cmath>

using namespace std;

/**
 * @brief 带球行为类型常量
 * 
 * 定义带球行为的行为类型为BT_Dribble，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorDribbleExecuter::BEHAVIOR_TYPE = BT_Dribble;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册带球行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorDribbleExecuter>();
}

/**
 * @brief BehaviorDribbleExecuter 构造函数
 * 
 * 初始化带球行为执行器，设置与智能体的关联。
 * 带球行为执行器负责具体的带球动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorDribbleExecuter::BehaviorDribbleExecuter(Agent & agent) :
    BehaviorExecuterBase<BehaviorAttackData>( agent )
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorDribbleExecuter 析构函数
 * 
 * 清理带球行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorDribbleExecuter::~BehaviorDribbleExecuter(void)
{
	// 析构函数体为空
}

/**
 * @brief 执行带球行为
 * 
 * 这是带球行为执行的核心函数，负责实现具体的带球动作。
 * 该函数根据带球类型和当前状态，生成相应的动作指令。
 * 
 * 执行过程：
 * 1. 记录带球日志信息
 * 2. 根据带球类型选择执行策略
 * 3. 计算对手位置和威胁评估
 * 4. 根据体力状况调整带球速度
 * 5. 生成具体的跑动动作
 * 6. 执行带球动作并返回结果
 * 
 * @param dribble 活跃的带球行为，包含目标和类型信息
 * @return bool 带球行为是否成功执行
 * 
 * @note 支持正常带球和快速带球两种模式
 * @note 根据体力状况动态调整带球策略
 * @note 使用预测模型提高带球精度
 */
bool BehaviorDribbleExecuter::Execute(const ActiveBehavior & dribble)
{
	// === 记录带球日志 ===
	// 记录带球起始位置和目标位置，便于调试和分析
	Logger::instance().LogDribble(mBallState.GetPos(), dribble.mTarget, "@Dribble", true);

	// === 根据带球类型执行不同策略 ===
	if(dribble.mDetailType == BDT_Dribble_Normal) {
		// === 正常带球模式 ===
		// 获取当前球和对手的位置信息
		Vector ballpos = mBallState.GetPos();
		PlayerState   oppState = mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToBall());
		Vector  	 	posOpp   = oppState.GetPos();
		ballpos = mBallState.GetPredictedPos(1);
		Vector agentpos = mSelfState.GetPos();
		Vector agent_v = agentpos - mSelfState.GetPos();
		AngleDeg agentang = mSelfState.GetBodyDir();

		AtomicAction act;

		// === 根据体力状况调整带球策略 ===
		// 体力不足时使用较低的带球速度，体力充足时使用较高速度
		if ( mSelfState.GetStamina() < 2700){
			// 体力不足，使用较低的跑动力度（30）
			Dasher::instance().GoToPoint(mAgent,act,dribble.mTarget,0.01,30);
		}
		else {
			// 体力充足，使用较高的跑动力度（100）
			Dasher::instance().GoToPoint(mAgent,act,dribble.mTarget, 0.01 ,100 );
		}

		// === 限制跑动力度在合理范围内 ===
		// 确保跑动力度不超过服务器限制
		act.mDashPower = MinMax(-ServerParam::instance().maxDashPower(), act.mDashPower, ServerParam::instance().maxDashPower());

		// === 计算预测位置 ===
		// 根据动作类型计算下一周期的位置
		agent_v = mSelfState.GetVel() * mSelfState.GetDecay();
		if(act.mType != CT_Dash){
			// 非跑动动作，使用基本位置预测
			agentpos = mSelfState.GetPredictedPos(1);
		}
		else
			// 跑动动作，使用带跑动的位置预测
			agentpos = mSelfState.GetPredictedPosWithDash(1,act.mDashPower,act.mDashDir);

		bool collide = mSelfState.GetCollideWithPlayer();
		if ( ballpos.Dist(agentpos) > 0.95 * ( mSelfState.GetKickableArea())
				|| collide )//will not be kickable or will make a collision ??
		{
			int p = ( ( mBallState.GetPos() - mSelfState.GetPos() ).Dir() - mSelfState.GetBodyDir()) > 0 ? 1:-1 ;
			double outSpeed = mSelfState.GetVel().Mod();
			if ( act.mType == CT_Dash && fabs( act.mDashDir ) < FLOAT_EPS )
				outSpeed += mSelfState.GetAccelerationFront(act.mDashPower);
			if((agentpos + Polar2Vector(mSelfState.GetKickableArea(),agentang + p * 45) - mBallState.GetPos()).Mod() <
					(	agentpos + Polar2Vector(mSelfState.GetKickableArea(),agentang + p * 45) -mSelfState.GetPos()).Mod()) {
				if ( mSelfState.GetStamina() < 2700)
				{
					return Dasher::instance().GoToPoint( mAgent , dribble.mTarget,0.01,30 ); // dash slow
				}
				else return Dasher::instance().GoToPoint( mAgent , dribble.mTarget,0.01,100 );

			}
			return Kicker::instance().KickBall(mAgent ,agentpos + Polar2Vector(mSelfState.GetKickableArea(),agentang + p * 45) , outSpeed,KM_Hard);
		}
		else {
			if ( mSelfState.GetStamina() < 2700)
			{
				return Dasher::instance().GoToPoint( mAgent , dribble.mTarget,0.01,30 ); // dash slow
			}
			else return Dasher::instance().GoToPoint( mAgent , dribble.mTarget,0.01,100 );
		}
	}
	else /*if(dribble.mDetailType == BDT_Dribble_Fast)*/{
		return Kicker::instance().KickBall(mAgent, dribble.mAngle, dribble.mKickSpeed, KM_Quick);
	}

}
BehaviorDribblePlanner::BehaviorDribblePlanner(Agent & agent) :
	BehaviorPlannerBase <BehaviorAttackData>(agent)
{
}


BehaviorDribblePlanner::~BehaviorDribblePlanner(void)
{
}


void BehaviorDribblePlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	if (!mSelfState.IsKickable()) return;
	if (mStrategy.IsForbidenDribble()) return;
	if (mSelfState.IsGoalie()) return;

	for (AngleDeg dir = -90.0; dir < 90.0; dir += 2.5) {
		ActiveBehavior dribble(mAgent, BT_Dribble, BDT_Dribble_Normal);

		dribble.mAngle = dir;

		const std::vector<Unum> & opp2ball = mPositionInfo.GetCloseOpponentToBall();
		AngleDeg min_differ = HUGE_VALUE;

		for (uint j = 0; j < opp2ball.size(); ++j) {
			Vector rel_pos = mWorldState.GetOpponent(opp2ball[j]).GetPos() - mBallState.GetPos();
			if (rel_pos.Mod() > 15.0) continue;

			AngleDeg differ = GetAngleDegDiffer(dir, rel_pos.Dir());
			if (differ < min_differ) {
				min_differ = differ;
			}
		}

		if (min_differ < 10.0) continue;

		dribble.mTarget= mSelfState.GetPos() + Polar2Vector( mSelfState.GetEffectiveSpeedMax(), dir);

		dribble.mEvaluation = Evaluation::instance().EvaluatePosition(dribble.mTarget, true);

		mActiveBehaviorList.push_back(dribble);
	}
	double speed = mSelfState.GetEffectiveSpeedMax();

	for (AngleDeg dir = -90.0; dir < 90.0; dir += 2.5) {
		ActiveBehavior dribble(mAgent, BT_Dribble, BDT_Dribble_Fast);
		dribble.mKickSpeed = speed;
		dribble.mAngle = dir;

		const std::vector<Unum> & opp2ball = mPositionInfo.GetCloseOpponentToBall();
		Vector target = mBallState.GetPos() + Polar2Vector(dribble.mKickSpeed * 10, dribble.mAngle);
		if(!ServerParam::instance().pitchRectanglar().IsWithin(target)){
			continue;
		}
		bool ok = true;
		for (uint j = 0; j < opp2ball.size(); ++j) {
			Vector rel_pos = mWorldState.GetOpponent(opp2ball[j]).GetPos() - target;
			if (rel_pos.Mod() < dribble.mKickSpeed * 12 ||
					mWorldState.GetOpponent(opp2ball[j]).GetPosConf() < PlayerParam::instance().minValidConf()){
				ok = false;
				break;
			}
		}
		if(!ok){
			continue;
		}
		dribble.mEvaluation = 0;
		for (int i = 1; i <= 8; ++i) {
			dribble.mEvaluation += Evaluation::instance().EvaluatePosition(mBallState.GetPos() + Polar2Vector(dribble.mKickSpeed * i, dribble.mAngle), true);
		}
		dribble.mEvaluation /= 8;
		dribble.mTarget = target;

		mActiveBehaviorList.push_back(dribble);
	}

	if (!mActiveBehaviorList.empty()) {
		mActiveBehaviorList.sort(std::greater<ActiveBehavior>());
		behavior_list.push_back(mActiveBehaviorList.front());
	}
}
