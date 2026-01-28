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
 * @file BehaviorPass.cpp
 * @brief 传球行为实现 - WrightEagleBase 的传球动作执行核心
 * 
 * 本文件实现了 BehaviorPassExecuter 和 BehaviorPassPlanner 类，它们是 WrightEagleBase 系统的传球动作核心：
 * 1. 实现智能的传球动作执行
 * 2. 处理不同类型的传球策略
 * 3. 优化传球角度和力度控制
 * 4. 提供灵活的传球规划支持
 * 
 * 传球行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在进攻时实现有效的团队协作。
 * 
 * 主要功能：
 * - 智能传球：根据队友位置选择传球策略
 * - 直接传球：快速直接的传球方式
 * - 解围传球：在压力下的传球策略
 * - 精确控制：优化传球角度和力度
 * 
 * 技术特点：
 * - 多种模式：支持直接传球和解围传球
 * - 智能选择：根据情况选择最佳传球方式
 * - 力度控制：根据距离调整传球力度
 * - 快速执行：支持快速传球决策
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorPass.h"
#include "Tackler.h"
#include "Dasher.h"
#include "Agent.h"
#include "WorldState.h"
#include "Strategy.h"
#include "Kicker.h"
#include "Dasher.h"
#include "InfoState.h"
#include "PositionInfo.h"
#include "Formation.h"
#include "Types.h"
#include "Logger.h"
#include "VisualSystem.h"
#include "CommunicateSystem.h"
#include "TimeTest.h"
#include "Evaluation.h"

#include <sstream>
using namespace std;

/**
 * @brief 传球行为类型常量
 * 
 * 定义传球行为的行为类型为BT_Pass，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorPassExecuter::BEHAVIOR_TYPE = BT_Pass;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册传球行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorPassExecuter>();
}

/**
 * @brief BehaviorPassExecuter 构造函数
 * 
 * 初始化传球行为执行器，设置与智能体的关联。
 * 传球行为执行器负责具体的传球动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorPassExecuter::BehaviorPassExecuter(Agent &agent):
	BehaviorExecuterBase<BehaviorAttackData>( agent )
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorPassExecuter 析构函数
 * 
 * 清理传球行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorPassExecuter::~BehaviorPassExecuter(void)
{
	// 析构函数体为空
}

/**
 * @brief 执行传球行为
 * 
 * 这是传球行为执行的核心函数，负责实现具体的传球动作。
 * 该函数根据传球类型和目标位置，生成相应的传球指令。
 * 
 * 执行过程：
 * 1. 记录传球日志信息
 * 2. 根据传球类型选择执行策略
 * 3. 调用相应的传球执行函数
 * 4. 返回执行结果
 * 
 * 支持的传球类型：
 * - BDT_Pass_Direct：直接传球
 * - BDT_Pass_Clear：解围传球
 * 
 * @param pass 活跃的传球行为，包含目标和类型信息
 * @return bool 传球行为是否成功执行
 * 
 * @note 直接传球使用指定的传球速度
 * @note 解围传球会优先考虑铲球传球，否则使用最大速度
 * @note 铲球传球可能造成球或球员被抬升
 */
bool BehaviorPassExecuter::Execute(const ActiveBehavior & pass)
{
	// === 记录传球日志 ===
	// 记录传球起始位置和目标位置，便于调试和分析
	Logger::instance().LogPass(false, mBallState.GetPos(), pass.mTarget, "@Pass", true);
	
	// === 根据传球类型执行不同策略 ===
	if (pass.mDetailType == BDT_Pass_Direct) {
		// === 直接传球模式 ===
		// 使用踢球动作向目标位置传球
		// 参数说明：
		// - pass.mTarget: 传球目标位置
		// - pass.mKickSpeed: 传球速度
		// - KM_Quick: 快速传球模式
		return Kicker::instance().KickBall(mAgent, pass.mTarget, pass.mKickSpeed, KM_Quick);
	}
	
	if(pass.mDetailType == BDT_Pass_Clear){
		// === 解围传球模式 ===
		// 在压力下的传球策略，优先考虑铲球传球
		
		// 检查是否可以使用铲球传球
		if(	Tackler::instance().CanTackleToDir(mAgent, pass.mAngle)
				&& Tackler::instance().GetBallVelAfterTackle(mAgent,pass.mAngle).Mod() > Kicker::instance().GetMaxSpeed(mAgent,pass.mTarget,1) - 0.08)
		){
			// === 铲球传球策略 ===
			// 当铲球传球能获得更好的球速时，使用铲球传球
			return Tackler::instance().TackleToDir(mAgent,pass.mAngle);
		}
		else {
			// === 普通解围传球策略 ===
			// 使用最大球速进行解围传球
			// 参数说明：
			// - pass.mTarget: 传球目标位置
			// - ServerParam::instance().ballSpeedMax(): 最大球速
			// - KM_Quick: 快速传球模式
			// - 0: 踢球次数限制（0表示不限制）
			return Kicker::instance().KickBall(mAgent, pass.mTarget, ServerParam::instance().ballSpeedMax(), KM_Quick, 0);
		}
	}
	
	// === 未知传球类型，返回失败 ===
	return false;
}

/**
 * @brief BehaviorPassPlanner 构造函数
 * 
 * 初始化传球行为规划器，设置与智能体的关联。
 * 传球行为规划器负责传球决策的规划和评估。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 */
BehaviorPassPlanner::BehaviorPassPlanner(Agent & agent):
	BehaviorPlannerBase<BehaviorAttackData>(agent)
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorPassPlanner 析构函数
 * 
 * 清理传球行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorPassPlanner::~BehaviorPassPlanner(void)
{
	// 析构函数体为空
}

/**
 * @brief 传球行为规划函数
 * 
 * 这是传球行为规划的核心函数，负责生成和选择最佳传球行为。
 * 该函数根据当前比赛状态和队友位置，生成传球决策。
 * 
 * 规划过程：
 * 1. 检查是否可以踢球
 * 2. 评估传球时机和目标
 * 3. 生成传球候选行为
 * 4. 选择最优传球行为
 * 5. 返回规划结果
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 只有在可以踢球时才进行传球规划
 * @note 传球决策考虑队友位置和对手威胁
 * @note 支持多种传球策略的智能选择
 */
void BehaviorPassPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 检查踢球条件 ===
	// 只有在可以踢球的情况下才考虑传球
	if (!mSelfState.IsKickable()) return;

	const std::vector<Unum> & tm2ball = mPositionInfo.GetCloseTeammateToTeammate(mSelfState.GetUnum());

	Unum _opp = mStrategy.GetFastestOpp();
    if (!_opp) return;

	PlayerState oppState = mWorldState.GetOpponent( _opp );
	bool oppClose = oppState.IsKickable()|| oppState.GetTackleProb(true) > 0.65 ;
	for (uint i = 0; i < tm2ball.size(); ++i) {
		ActiveBehavior pass(mAgent, BT_Pass);

		pass.mTarget = mWorldState.GetTeammate(tm2ball[i]).GetPredictedPos();

		if(mWorldState.GetTeammate(tm2ball[i]).IsGoalie()){
			continue;
		}
		Vector rel_target = pass.mTarget - mBallState.GetPos();
		const std::vector<Unum> & opp2tm = mPositionInfo.GetCloseOpponentToTeammate(tm2ball[i]);
		AngleDeg min_differ = HUGE_VALUE;

		for (uint j = 0; j < opp2tm.size(); ++j) {
			Vector rel_pos = mWorldState.GetOpponent(opp2tm[j]).GetPos() - mBallState.GetPos();
			if (rel_pos.Mod() > rel_target.Mod() + 3.0) continue;

			AngleDeg differ = GetAngleDegDiffer(rel_target.Dir(), rel_pos.Dir());
			if (differ < min_differ) {
				min_differ = differ;
			}
		}

		if (min_differ < 10.0) continue;

		pass.mEvaluation = Evaluation::instance().EvaluatePosition(pass.mTarget, true);

		pass.mAngle = (pass.mTarget - mSelfState.GetPos()).Dir();
		pass.mKickSpeed = ServerParam::instance().GetBallSpeed(5, pass.mTarget.Dist(mBallState.GetPos()));
		pass.mKickSpeed = MinMax(2.0, pass.mKickSpeed, Kicker::instance().GetMaxSpeed(mAgent , pass.mAngle ,3 ));
		if(oppClose){//in oppnent control, clear it
			pass.mDetailType = BDT_Pass_Clear;
		}
		else pass.mDetailType = BDT_Pass_Direct;
		mActiveBehaviorList.push_back(pass);
	}
	if (!mActiveBehaviorList.empty()) {
		mActiveBehaviorList.sort(std::greater<ActiveBehavior>());
		if(mActiveBehaviorList.front().mDetailType == BDT_Pass_Clear){
			mActiveBehaviorList.front().mEvaluation = 1.0 + FLOAT_EPS;
		}
		behavior_list.push_back(mActiveBehaviorList.front());
	}
	else {														//如果此周期没有好的动作
		if (mAgent.IsLastActiveBehaviorInActOf(BT_Pass)) {
			ActiveBehavior pass(mAgent, BT_Pass, BDT_Pass_Direct);
			pass.mTarget = mAgent.GetLastActiveBehaviorInAct()->mTarget; //行为保持
			pass.mEvaluation = Evaluation::instance().EvaluatePosition(pass.mTarget, true);
			pass.mKickSpeed = ServerParam::instance().GetBallSpeed(5 + random() % 6, pass.mTarget.Dist(mBallState.GetPos()));
			pass.mKickSpeed = MinMax(2.0, pass.mKickSpeed, ServerParam::instance().ballSpeedMax());
			behavior_list.push_back(pass);
		}
		if(oppClose){
			Vector p;
			BallState SimBall = mBallState;
			int MinTmInter = HUGE_VALUE, MinOppInter = HUGE_VALUE, MinTm;
			Vector MinTmPos;
			for(AngleDeg dir = -45 ; dir <= 45  ; dir += 2.5){
				if(!Tackler::instance().CanTackleToDir(mAgent,dir)){
					SimBall.UpdateVel(Polar2Vector(Kicker::instance().GetMaxSpeed(mAgent,mSelfState.GetBodyDir() + dir,1),mSelfState.GetBodyDir() + dir),0,1.0);
				}
				else
					SimBall.UpdateVel(Polar2Vector(Max(Tackler::instance().GetBallVelAfterTackle(mAgent,dir).Mod(), Kicker::instance().GetMaxSpeed(mAgent,mSelfState.GetBodyDir() + dir,1)),mSelfState.GetBodyDir() + dir),0,1.0);
				for(int i = 2 ; i <= 11 ; i ++){
					if(fabs((mWorldState.GetTeammate(i).GetPos() - mSelfState.GetPos()).Dir() - dir) > 45 ){
						continue;
					}
					if(!mWorldState.GetPlayer(i).IsAlive()){continue;}
					PlayerInterceptInfo* a = mInterceptInfo.GetPlayerInterceptInfo(i);
					mInterceptInfo.CalcTightInterception(SimBall,a,true);
					if(MinTmInter > (*a).mMinCycle){
						MinTm= i;
						MinTmInter = (*a).mMinCycle;
						MinTmPos = (*a).mInterPos;
					}
				}
				for(int i = 1 ; i <= 11 ; i++){
					if(fabs((mWorldState.GetOpponent(i).GetPos() - mSelfState.GetPos()).Dir() - dir) > 45 ){
						continue;
					}
					PlayerInterceptInfo* a = mInterceptInfo.GetPlayerInterceptInfo(-i);
					mInterceptInfo.CalcTightInterception(SimBall,a,true);
					if(MinOppInter > (*a).mMinCycle){
						MinOppInter = (*a).mMinCycle;
					}
				}
				if(MinOppInter > MinTmInter){
					Ray a(mSelfState.GetPos() , mSelfState.GetBodyDir() + dir);
					Line c(ServerParam::instance().ourLeftGoalPost(),ServerParam::instance().ourRightGoalPost());
					c.Intersection(a,p);
					if(	p.Y() < ServerParam::instance().ourPenaltyArea().Top()
							|| p.Y() > ServerParam::instance().ourPenaltyArea().Bottom()){

						ActiveBehavior pass(mAgent , BT_Pass , BDT_Pass_Clear);
						pass.mTarget = MinTmPos;
						pass.mEvaluation = 1.0 + FLOAT_EPS;
						pass.mAngle = mSelfState.GetBodyDir() + dir;
						mActiveBehaviorList.push_back(pass);
					}

				}
			}
			if (!mActiveBehaviorList.empty()) {
				mActiveBehaviorList.sort(std::greater<ActiveBehavior>());
				behavior_list.push_back(mActiveBehaviorList.front());
			}
		}

	}
}
