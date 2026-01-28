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
 * @file BehaviorMark.cpp
 * @brief 标记行为实现 - WrightEagleBase 的防守标记系统
 * 
 * 本文件实现了标记行为的执行器和规划器：
 * 1. BehaviorMarkExecuter：标记行为执行器
 * 2. BehaviorMarkPlanner：标记行为规划器
 * 
 * 标记行为是防守系统的重要组成部分，
 * 负责对对手球员进行人盯人防守。
 * 
 * 主要功能：
 * - 人盯人防守：对特定对手进行紧密防守
 * - 位置计算：计算最佳的标记位置
 * - 距离控制：保持适当的防守距离
 * - 角色适配：根据球员角色调整防守策略
 * - 位置评估：评估标记位置的优劣
 * 
 * 技术特点：
 * - 智能选择：自动选择需要标记的对手
 * - 几何计算：基于球和对手位置计算标记点
 * - 距离优化：使用可踢球区域作为防守距离
 * - 角色区分：防守球员有特殊的评估方式
 * - 动态调整：根据比赛情况动态调整标记策略
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorMark.h"
#include "VisualSystem.h"
#include "Formation.h"
#include "Dasher.h"
#include "BasicCommand.h"
#include "BehaviorPosition.h"
#include "Agent.h"
#include "PositionInfo.h"
#include "Logger.h"
#include "Evaluation.h"

// === 标记行为类型定义 ===
const BehaviorType BehaviorMarkExecuter::BEHAVIOR_TYPE = BT_Mark;

// === 自动注册标记行为执行器 ===
namespace
{
bool ret = BehaviorExecutable::AutoRegister<BehaviorMarkExecuter>();
}

/**
 * @brief BehaviorMarkExecuter 构造函数
 * 
 * 初始化标记行为执行器。
 * 继承自BehaviorExecuterBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用防守数据作为数据基类
 */
BehaviorMarkExecuter::BehaviorMarkExecuter(Agent & agent) :
	BehaviorExecuterBase<BehaviorDefenseData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

/**
 * @brief BehaviorMarkExecuter 析构函数
 * 
 * 清理标记行为执行器资源。
 */
BehaviorMarkExecuter::~BehaviorMarkExecuter(void)
{
}

/**
 * @brief 执行标记行为
 * 
 * 执行具体的标记动作，主要是移动到标记位置。
 * 
 * 执行流程：
 * 1. 记录标记日志
 * 2. 调用跑动系统移动到目标位置
 * 
 * @param beh 活跃行为对象，包含目标位置、缓冲区、力度等参数
 * @return bool 执行是否成功
 * 
 * @note 使用Dasher系统进行精确的位置移动
 * @note 最后一个参数表示不使用逆跑
 */
bool BehaviorMarkExecuter::Execute(const ActiveBehavior & beh)
{
	// === 记录标记日志 ===
	Logger::instance().LogGoToPoint(mSelfState.GetPos(), beh.mTarget, "@Mark");

	// === 执行跑动到目标位置 ===
	return Dasher::instance().GoToPoint(mAgent, beh.mTarget, beh.mBuffer, beh.mPower, false, false);
}

/**
 * @brief BehaviorMarkPlanner 构造函数
 * 
 * 初始化标记行为规划器。
 * 继承自BehaviorPlannerBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 使用防守数据作为数据基类
 */
BehaviorMarkPlanner::BehaviorMarkPlanner(Agent & agent):
	BehaviorPlannerBase<BehaviorDefenseData>( agent)
{
}

/**
 * @brief BehaviorMarkPlanner 析构函数
 * 
 * 清理标记行为规划器资源。
 */
BehaviorMarkPlanner::~BehaviorMarkPlanner()
{
}

/**
 * @brief 规划标记行为
 * 
 * 规划标记行为，选择需要标记的对手并计算标记位置。
 * 这是标记行为的核心决策函数。
 * 
 * 决策逻辑：
 * 1. 找到离自己最近的对手
 * 2. 找到离该对手最近的队友
 * 3. 如果自己就是最近的队友，则执行标记
 * 4. 计算最佳标记位置
 * 5. 根据球员角色评估位置
 * 
 * 位置计算逻辑：
 * 1. 获取球的位置
 * 2. 计算球到对手的方向
 * 3. 在对手位置基础上，沿球到对手方向偏移一定距离
 * 4. 偏移距离等于自己的可踢球区域
 * 
 * @param behavior_list 行为列表，用于添加生成的标记行为
 * 
 * @note 只有在自己是对手最近的队友时才执行标记
 * @note 使用可踢球区域作为防守距离，确保能有效拦截
 * @note 防守球员有特殊的评估方式
 */
void BehaviorMarkPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 找到需要标记的对手 ===
	Unum closest_opp = mPositionInfo.GetClosestOpponentToTeammate(mSelfState.GetUnum());  // 离自己最近的对手
	Unum closest_tm = mPositionInfo.GetClosestTeammateToOpponent(closest_opp);           // 离该对手最近的队友

	// === 判断是否应该执行标记 ===
	// 条件：存在对手且自己是对手最近的队友
	if (closest_opp && closest_tm && closest_tm == mSelfState.GetUnum()) {
		// === 创建标记行为 ===
		ActiveBehavior mark(mAgent, BT_Mark);

		// === 计算标记位置 ===
		Vector ballPos = mBallState.GetPos();  // 球的位置
		AngleDeg b2o = (mBallState.GetPos()- mWorldState.GetOpponent(closest_opp).GetPos()).Dir();  // 球到对手的方向
		
		// === 设置标记参数 ===
		mark.mBuffer = mSelfState.GetKickableArea();  // 缓冲区距离：使用可踢球区域
		mark.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower());  // 根据体力调整力度
		
		// === 计算目标位置 ===
		// 在对手位置基础上，沿球到对手方向偏移可踢球区域距离
		mark.mTarget = mWorldState.GetOpponent(closest_opp).GetPos()  + Polar2Vector(mark.mBuffer , b2o);
		
		// === 评估标记位置 ===
		if( mAgent.GetFormation().GetMyRole().mLineType == LT_Defender){
			// === 防守球员特殊评估 ===
			mark.mEvaluation = Evaluation::instance().EvaluatePosition(mark.mTarget, true);
		}
		else {
			// === 普通球员评估 ===
			mark.mEvaluation = Evaluation::instance().EvaluatePosition(mark.mTarget, false);
		}

		// === 添加到行为列表 ===
		behavior_list.push_back(mark);
	}
}

