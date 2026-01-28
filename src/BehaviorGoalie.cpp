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
 * @file BehaviorGoalie.cpp
 * @brief 守门员行为实现 - WrightEagleBase 的守门员系统
 * 
 * 本文件实现了守门员行为的执行器和规划器：
 * 1. BehaviorGoalieExecuter：守门员行为执行器
 * 2. BehaviorGoaliePlanner：守门员行为规划器
 * 
 * 守门员行为是防守系统的特殊组成部分，
 * 专门负责守门员的防守和扑救动作。
 * 
 * 主要功能：
 * - 扑救动作：执行接球动作
 * - 位置选择：选择最佳的守门位置
 * - 射线拦截：基于射线理论计算拦截位置
 * - 区域判断：判断球是否在可接球范围内
 * - 位置评估：评估守门位置的优劣
 * 
 * 技术特点：
 * - 智能定位：基于射线理论计算最佳守门位置
 * - 动态调整：根据球的位置动态调整守门位置
 * - 区域限制：守门员位置受禁区限制
 * - 时机判断：精确判断接球时机
 * - 位置优化：使用评估系统优化守门位置
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorGoalie.h"
#include "Logger.h"
#include "TimeTest.h"
#include "Utilities.h"
#include "Evaluation.h"

// === 守门员行为类型定义 ===
const BehaviorType BehaviorGoalieExecuter::BEHAVIOR_TYPE = BT_Goalie;

// === 自动注册守门员行为执行器 ===
namespace
{
bool ret = BehaviorExecutable::AutoRegister<BehaviorGoalieExecuter>();
}

/**
 * @brief 执行守门员行为
 * 
 * 执行具体的守门员动作，包括位置移动和接球。
 * 根据行为详细类型选择不同的执行方式。
 * 
 * 执行逻辑：
 * 1. 如果是位置行为，移动到目标位置
 * 2. 如果是接球行为，执行接球动作
 * 3. 其他情况返回失败
 * 
 * @param act_bhv 活跃行为对象，包含行为类型和目标位置等参数
 * @return bool 执行是否成功
 * 
 * @note 支持两种守门员行为：位置移动和接球
 * @note 接球时需要计算球相对于身体的方向
 */
bool BehaviorGoalieExecuter::Execute(const ActiveBehavior & act_bhv)
{
	if (act_bhv.mDetailType == BDT_Goalie_Position) {
		// === 执行位置移动 ===
		return Dasher::instance().GoToPoint(mAgent, act_bhv.mTarget);
	}
	else if (act_bhv.mDetailType == BDT_Goalie_Catch) {
		// === 执行接球动作 ===
		// 计算球相对于身体的方向，并执行接球
		return mAgent.Catch(GetNormalizeAngleDeg((mBallState.GetPos() - mSelfState.GetPos()).Dir() - mSelfState.GetBodyDir()));
	}
	else {
		// === 未知行为类型 ===
		return false;
	}
}

/**
 * @brief BehaviorGoaliePlanner 构造函数
 * 
 * 初始化守门员行为规划器。
 * 继承自BehaviorPlannerBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用防守数据作为数据基类
 */
BehaviorGoaliePlanner::BehaviorGoaliePlanner(Agent& agent): BehaviorPlannerBase<BehaviorDefenseData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

/**
 * @brief BehaviorGoaliePlanner 析构函数
 * 
 * 清理守门员行为规划器资源。
 */
BehaviorGoaliePlanner::~BehaviorGoaliePlanner(void)
{
}

/**
 * @brief BehaviorGoalieExecuter 构造函数
 * 
 * 初始化守门员行为执行器。
 * 继承自BehaviorExecuterBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用防守数据作为数据基类
 */
BehaviorGoalieExecuter::BehaviorGoalieExecuter(Agent& agent): BehaviorExecuterBase<BehaviorDefenseData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

/**
 * @brief BehaviorGoalieExecuter 析构函数
 * 
 * 清理守门员行为执行器资源。
 */
BehaviorGoalieExecuter::~BehaviorGoalieExecuter(void)
{
}

/**
 * @brief 规划守门员行为
 * 
 * 规划守门员行为，判断是否需要接球或移动到守门位置。
 * 这是守门员行为的核心决策函数。
 * 
 * 决策逻辑：
 * 1. 检查是否刚完成传球或带球动作
 * 2. 判断是否可以接球且对方刚控球
 * 3. 如果可以接球，生成接球行为
 * 4. 否则计算最佳守门位置并生成位置行为
 * 
 * 位置计算逻辑：
 * 1. 基于射线理论计算守门位置
 * 2. 优先选择禁区内位置
 * 3. 如果禁区外位置更好，选择禁区外位置
 * 4. 使用评估系统评估位置质量
 * 
 * @param behavior_list 行为列表，用于添加生成的守门员行为
 * 
 * @note 刚完成传球或带球时不执行守门员行为
 * @note 使用射线理论计算最佳守门位置
 * @note 位置选择受禁区限制
 */
void BehaviorGoaliePlanner::Plan(std::list<ActiveBehavior>& behavior_list)
{
	// === 检查行为冲突 ===
	// 如果刚完成传球或带球动作，不执行守门员行为
	if(mAgent.IsLastActiveBehaviorInActOf(BT_Pass) || mAgent.IsLastActiveBehaviorInActOf(BT_Dribble))
		return;

	// === 判断是否可以接球 ===
	// 条件：球可接球且对方刚控球
	if (mSelfState.IsBallCatchable() && mStrategy.IsLastOppControl())
	 {
		// === 创建接球行为 ===
		ActiveBehavior catchball(mAgent, BT_Goalie, BDT_Goalie_Catch);

		catchball.mEvaluation = 1.0 + FLOAT_EPS;  // 接球行为具有最高优先级

		// === 添加到行为列表 ===
		behavior_list.push_back(catchball);
	}
	else {
		// === 计算守门位置 ===
		Vector target;
		
		// === 基于射线理论计算守门位置 ===
		// 射线起点：球门中心
		// 射线方向：指向分析器的灯塔位置
		Ray ball_ray(ServerParam::instance().ourGoal(), (mAnalyser.mLightHouse - ServerParam::instance().ourGoal()).Dir());
		
		// === 计算射线与禁区的交点 ===
		ServerParam::instance().ourGoalArea().Intersection(ball_ray, target);

		// === 创建位置行为 ===
		ActiveBehavior position(mAgent, BT_Goalie, BDT_Goalie_Position);

		position.mTarget = target;  // 设置目标位置
		
		// === 检查位置是否在禁区内 ===
		if(ServerParam::instance().ourPenaltyArea().IsWithin(target)){
			// === 位置在禁区内，直接评估 ===
			position.mEvaluation = Evaluation::instance().EvaluatePosition(position.mTarget, false);
		}
		else
		{
			// === 位置在禁区外，重新计算 ===
			// 射线起点：球门中心
			// 射线方向：指向球的位置
			Ray ball_ray(ServerParam::instance().ourGoal(), (mBallState.GetPos() - ServerParam::instance().ourGoal()).Dir());
			
			// === 计算射线与禁区的交点 ===
			ServerParam::instance().ourGoalArea().Intersection(ball_ray, target);
			
			// === 评估禁区内的位置 ===
			position.mEvaluation = Evaluation::instance().EvaluatePosition(position.mTarget, false);
			position.mTarget = target;  // 更新目标位置为禁区内的位置
		}
		
		// === 添加到行为列表 ===
		behavior_list.push_back(position);
	}
}
