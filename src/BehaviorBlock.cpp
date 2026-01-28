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
 * @file BehaviorBlock.cpp
 * @brief 阻挡行为实现 - WrightEagleBase 的防守阻挡系统
 * 
 * 本文件实现了阻挡行为的执行器和规划器：
 * 1. BehaviorBlockExecuter：阻挡行为执行器
 * 2. BehaviorBlockPlanner：阻挡行为规划器
 * 
 * 阻挡行为是防守系统的重要组成部分，
 * 主要用于在防守时阻挡对手的进攻路线。
 * 
 * 主要功能：
 * - 路线阻挡：阻挡对手的传球和射门路线
 * - 位置选择：选择最佳的阻挡位置
 * - 时机判断：判断何时执行阻挡行为
 * - 动作执行：执行具体的阻挡动作
 * 
 * 技术特点：
 * - 智能定位：基于分析器的灯塔位置进行定位
 * - 体力管理：考虑体力状况调整跑动力度
 * - 策略集成：与整体防守策略紧密结合
 * - 条件判断：根据比赛情况决定是否执行阻挡
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorBlock.h"
#include "VisualSystem.h"
#include "Formation.h"
#include "Dasher.h"
#include "BasicCommand.h"
#include "BehaviorPosition.h"
#include "Agent.h"
#include "PositionInfo.h"
#include "Logger.h"
#include "Evaluation.h"

// === 阻挡行为类型定义 ===
const BehaviorType BehaviorBlockExecuter::BEHAVIOR_TYPE = BT_Block;

// === 自动注册阻挡行为执行器 ===
namespace
{
bool ret = BehaviorExecutable::AutoRegister<BehaviorBlockExecuter>();
}

/**
 * @brief BehaviorBlockExecuter 构造函数
 * 
 * 初始化阻挡行为执行器。
 * 继承自BehaviorExecuterBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用防守数据作为数据基类
 */
BehaviorBlockExecuter::BehaviorBlockExecuter(Agent & agent) :
	BehaviorExecuterBase<BehaviorDefenseData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

/**
 * @brief BehaviorBlockExecuter 析构函数
 * 
 * 清理阻挡行为执行器资源。
 */
BehaviorBlockExecuter::~BehaviorBlockExecuter(void)
{
}

/**
 * @brief 执行阻挡行为
 * 
 * 执行具体的阻挡动作，主要是移动到目标位置进行阻挡。
 * 
 * 执行流程：
 * 1. 记录阻挡日志
 * 2. 调用跑动系统移动到目标位置
 * 
 * @param beh 活跃行为对象，包含目标位置、缓冲区、力度等参数
 * @return bool 执行是否成功
 * 
 * @note 使用Dasher系统进行精确的位置移动
 * @note 最后一个参数表示不使用逆跑
 */
bool BehaviorBlockExecuter::Execute(const ActiveBehavior & beh)
{
	// === 记录阻挡日志 ===
	Logger::instance().LogGoToPoint(mSelfState.GetPos(), beh.mTarget, "@Block");

	// === 执行跑动到目标位置 ===
	return Dasher::instance().GoToPoint(mAgent, beh.mTarget, beh.mBuffer, beh.mPower, true, false);
}

/**
 * @brief BehaviorBlockPlanner 构造函数
 * 
 * 初始化阻挡行为规划器。
 * 继承自BehaviorPlannerBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 使用防守数据作为数据基类
 */
BehaviorBlockPlanner::BehaviorBlockPlanner(Agent & agent):
	BehaviorPlannerBase<BehaviorDefenseData>( agent)
{
}

/**
 * @brief BehaviorBlockPlanner 析构函数
 * 
 * 清理阻挡行为规划器资源。
 */
BehaviorBlockPlanner::~BehaviorBlockPlanner()
{
}

/**
 * @brief 规划阻挡行为
 * 
 * 规划阻挡行为，判断是否需要执行阻挡并生成相应的行为。
 * 这是阻挡行为的核心决策函数。
 * 
 * 决策逻辑：
 * 1. 检查比赛模式，某些模式下不执行阻挡
 * 2. 判断自己是否是最近球的队友
 * 3. 判断自己的截球周期是否足够短
 * 4. 如果满足条件，生成阻挡行为
 * 
 * @param behavior_list 行为列表，用于添加生成的阻挡行为
 * 
 * @note 只有在特定条件下才会执行阻挡行为
 * @note 使用分析器的灯塔位置作为目标位置
 * @note 会考虑体力状况调整跑动力度
 */
void BehaviorBlockPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 获取最近球的队友 ===
	Unum closest_tm = mPositionInfo.GetClosestTeammateToBall();
	
	// === 检查比赛模式 ===
	// 在对方定位球模式下不执行阻挡
	if(mWorldState.GetPlayMode() >= PM_Opp_Corner_Kick &&
			mWorldState.GetPlayMode() <=PM_Opp_Offside_Kick){
		return;  // 对方定位球，不执行阻挡
	}

	// === 判断是否执行阻挡 ===
	// 条件：自己是最接近球的队友 或 自己的截球周期足够短
	if (closest_tm == mSelfState.GetUnum() || mStrategy.GetMyInterCycle() <= mStrategy.GetSureTmInterCycle()) {
		// === 创建阻挡行为 ===
		ActiveBehavior block(mAgent, BT_Block);

		// === 设置阻挡参数 ===
		block.mBuffer = 0.5;  // 缓冲区距离
		block.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower());  // 根据体力调整力度
		block.mTarget = mAnalyser.mLightHouse;  // 目标位置：分析器的灯塔位置
		block.mEvaluation = 1.0 + FLOAT_EPS;  // 评估值

		// === 添加到行为列表 ===
		behavior_list.push_back(block);
	}
}

