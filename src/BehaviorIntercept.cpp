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
 * @file BehaviorIntercept.cpp
 * @brief 截球行为实现 - WrightEagleBase 的截球动作执行核心
 * 
 * 本文件实现了 BehaviorInterceptExecuter 和 BehaviorInterceptPlanner 类，它们是 WrightEagleBase 系统的截球动作核心：
 * 1. 实现智能的截球动作执行
 * 2. 处理不同情况下的截球策略
 * 3. 优化截球路径和时机选择
 * 4. 提供灵活的截球规划支持
 * 
 * 截球行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在防守和进攻时实现有效的球权争夺。
 * 
 * 主要功能：
 * - 智能截球：根据球的位置和速度选择截球策略
 * - 守门员截球：特殊的守门员截球逻辑
 * - 路径优化：选择最优的截球路径
 * - 时机把握：精确的截球时机判断
 * 
 * 技术特点：
 * - 多种模式：支持普通截球和守门员截球
 * - 智能选择：根据情况选择最佳截球方式
 * - 几何计算：使用几何算法计算截球点
 * - 快速执行：支持快速截球决策
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include <sstream>
#include "BehaviorIntercept.h"
#include "Agent.h"
#include "Strategy.h"
#include "InterceptInfo.h"
#include "Formation.h"
#include "Dasher.h"
#include "Logger.h"
#include "VisualSystem.h"
#include "BehaviorDribble.h"
#include "Evaluation.h"
#include "BehaviorBase.h"

using namespace std;

/**
 * @brief 截球行为类型常量
 * 
 * 定义截球行为的行为类型为BT_Intercept，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorInterceptExecuter::BEHAVIOR_TYPE = BT_Intercept;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册截球行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorInterceptExecuter>();
}

/**
 * @brief BehaviorInterceptExecuter 构造函数
 * 
 * 初始化截球行为执行器，设置与智能体的关联。
 * 截球行为执行器负责具体的截球动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorInterceptExecuter::BehaviorInterceptExecuter(Agent & agent): BehaviorExecuterBase<BehaviorAttackData>(agent)
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorInterceptExecuter 析构函数
 * 
 * 清理截球行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorInterceptExecuter::~BehaviorInterceptExecuter() {
	// 析构函数体为空
}

/**
 * @brief 执行截球行为
 * 
 * 这是截球行为执行的核心函数，负责实现具体的截球动作。
 * 该函数调用跑动系统的GetBall方法来实现截球。
 * 
 * 执行过程：
 * 1. 记录截球日志信息
 * 2. 调用跑动系统执行截球
 * 3. 返回执行结果
 * 
 * @param intercept 活跃的截球行为，包含目标信息
 * @return bool 截球行为是否成功执行
 * 
 * @note 使用Dasher系统的GetBall方法实现截球
 * @note 截球动作会自动计算最优路径和时机
 */
bool BehaviorInterceptExecuter::Execute(const ActiveBehavior & intercept)
{
	// === 记录截球日志 ===
	// 记录截球目标位置，便于调试和分析
	Logger::instance().LogIntercept(intercept.mTarget, "@Intercept");

	// === 执行截球动作 ===
	// 调用跑动系统的GetBall方法来实现截球
	// 该方法会自动计算最优的截球路径和时机
	return Dasher::instance().GetBall(mAgent);
}

/**
 * @brief BehaviorInterceptPlanner 构造函数
 * 
 * 初始化截球行为规划器，设置与智能体的关联。
 * 截球行为规划器负责截球决策的规划和评估。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 */
BehaviorInterceptPlanner::BehaviorInterceptPlanner(Agent & agent): BehaviorPlannerBase<BehaviorAttackData>(agent)
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorInterceptPlanner 析构函数
 * 
 * 清理截球行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorInterceptPlanner::~BehaviorInterceptPlanner() {
	// 析构函数体为空
}

/**
 * @brief 截球行为规划函数
 * 
 * 这是截球行为规划的核心函数，负责生成和选择最佳截球行为。
 * 该函数根据当前比赛状态和球的位置，生成截球决策。
 * 
 * 规划过程：
 * 1. 检查是否可以踢球
 * 2. 检查比赛模式是否允许截球
 * 3. 评估截球可行性
 * 4. 处理守门员特殊情况
 * 5. 生成截球候选行为
 * 6. 选择最优截球行为
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 只有在无法踢球时才考虑截球
 * @note 特殊处理守门员的截球逻辑
 * @note 使用几何计算确定守门员截球点
 */
void BehaviorInterceptPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 检查踢球条件 ===
	// 如果已经可以踢球，则不需要截球
	if (mSelfState.IsKickable()) return;
	
	// === 检查比赛模式 ===
	// 只有在特定的比赛模式下才允许截球
	PlayMode play_mode = mWorldState.GetPlayMode();
	if ( play_mode != PM_Play_On &&
	     play_mode != PM_Our_Penalty_Ready &&
	     play_mode != PM_Our_Penalty_Taken &&
	     play_mode != PM_Opp_Penalty_Taken)
	{
		return;
	}
	
	// === 检查截球可行性 ===
	// 检查当前球员是否能够截球
	if (!mInterceptInfo.IsPlayerBallInterceptable(mSelfState.GetUnum()) ){
		// === 守门员特殊处理 ===
		// 如果无法截球且是守门员，使用特殊的守门员截球逻辑
		if(mSelfState.IsGoalie()){
			// 创建守门员截球行为
			ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);
			
			// === 计算守门员截球点 ===
			// 使用几何计算确定球运动轨迹与球门线的交点
			Ray a (mBallState.GetPos(),mBallState.GetVel().Dir());  // 球的运动射线
			Line c(ServerParam::instance().ourLeftGoalPost(),ServerParam::instance().ourRightGoalPost());  // 球门线
			c.Intersection(a,intercept.mTarget);  // 计算交点
			
			// === 检查截球点是否在球门范围内 ===
			if(intercept.mTarget.Y() < ServerParam::instance().ourLeftGoalPost().Y()
				|| intercept.mTarget.Y() > ServerParam::instance().ourRightGoalPost().Y()){
				// 截球点不在球门范围内，放弃截球
				return;
			}
			else {
				// 截球点在球门范围内，评估位置价值
				intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);
				behavior_list.push_back(intercept);
			}
		}
		else {
		return;
		}
	}

	Rectangular GoalieInterRec = ServerParam::instance().ourGoalArea();
	GoalieInterRec.SetRight(ServerParam::instance().ourPenaltyArea().Right());

	if (mStrategy.GetMyInterCycle() <= mStrategy.GetMinTmInterCycle() && mStrategy.GetMyInterCycle() <= mStrategy.GetSureOppInterCycle() + 1 && (!mSelfState.IsGoalie())) {
		ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);

		intercept.mTarget = mStrategy.GetMyInterPos();
		intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);

		behavior_list.push_back(intercept);
	}
	else if (mPositionInfo.GetClosestOpponentToBall() == 0 ||
  		((mWorldState.GetTeammate(mPositionInfo.GetClosestTeammateToBall()).GetPos()-mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToBall()).GetPos()).Mod() <= 1 &&
			mSelfState.GetUnum() == mPositionInfo.GetCloseTeammateToBall().at(1)))//0.4受PlayerSize影响，这里直接使用0.3
	{
		ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);
		intercept.mTarget = mStrategy.GetMyInterPos();
		intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);
		behavior_list.push_back(intercept);

	}
	else if (mSelfState.IsGoalie() && GoalieInterRec.IsWithin(mStrategy.GetMyInterPos()))
	{
		ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);
		intercept.mTarget = mStrategy.GetMyInterPos();
		intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);
		if(mWorldState.GetPlayMode() == PM_Opp_Penalty_Taken){
			intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, false);
		}
		behavior_list.push_back(intercept);
	}
	else if (mSelfState.GetUnum() == mPositionInfo.GetClosestTeammateToBall()
			&& !mSelfState.IsGoalie()){
		ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);
		intercept.mTarget = mStrategy.GetMyInterPos();
		intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);
		behavior_list.push_back(intercept);
	}
	else if (mPositionInfo.GetClosestTeammateToBall() == mWorldState.GetTeammateGoalieUnum()
			&& mSelfState.GetUnum() == mPositionInfo.GetCloseTeammateToBall()[1]){

		ActiveBehavior intercept(mAgent, BT_Intercept, BDT_Intercept_Normal);
		intercept.mTarget = mStrategy.GetMyInterPos();
		intercept.mEvaluation = Evaluation::instance().EvaluatePosition(intercept.mTarget, true);
		behavior_list.push_back(intercept);
	}
}
