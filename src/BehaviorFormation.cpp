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
 * @file BehaviorFormation.cpp
 * @brief 阵型行为实现 - WrightEagleBase 的阵型管理系统
 * 
 * 本文件实现了阵型行为的执行器和规划器：
 * 1. BehaviorFormationExecuter：阵型行为执行器
 * 2. BehaviorFormationPlanner：阵型行为规划器
 * 
 * 阵型行为是防守系统的基础，
 * 负责让球员回到自己的阵型位置。
 * 
 * 主要功能：
 * - 阵型位置：计算球员的阵型位置
 * - 位置调整：根据比赛情况调整阵型位置
 * - 特殊处理：处理防守球员的特殊位置要求
 * - 体力管理：根据体力状况调整跑动力度
 * - 位置评估：评估阵型位置的优劣
 * 
 * 技术特点：
 * - 智能定位：基于分析器的阵型位置进行定位
 * - 角色适配：根据球员角色调整位置策略
 * - 防守优化：防守球员有特殊的位置优化逻辑
 * - 体力考虑：前锋后退时减少跑动力度
 * - 位置评估：使用评估系统评估位置质量
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorFormation.h"
#include "VisualSystem.h"
#include "Formation.h"
#include "Dasher.h"
#include "BasicCommand.h"
#include "BehaviorPosition.h"
#include "Agent.h"
#include "PositionInfo.h"
#include "Logger.h"
#include <cstdlib>
#include "Evaluation.h"

// === 阵型行为类型定义 ===
const BehaviorType BehaviorFormationExecuter::BEHAVIOR_TYPE = BT_Formation;

// === 自动注册阵型行为执行器 ===
namespace
{
bool ret = BehaviorExecutable::AutoRegister<BehaviorFormationExecuter>();
}

/**
 * @brief BehaviorFormationExecuter 构造函数
 * 
 * 初始化阵型行为执行器。
 * 继承自BehaviorExecuterBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用防守数据作为数据基类
 */
BehaviorFormationExecuter::BehaviorFormationExecuter(Agent & agent) :
	BehaviorExecuterBase<BehaviorDefenseData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

/**
 * @brief BehaviorFormationExecuter 析构函数
 * 
 * 清理阵型行为执行器资源。
 */
BehaviorFormationExecuter::~BehaviorFormationExecuter(void)
{
}

/**
 * @brief 执行阵型行为
 * 
 * 执行具体的阵型动作，主要是移动到阵型位置。
 * 
 * 执行流程：
 * 1. 记录阵型日志
 * 2. 调用跑动系统移动到目标位置
 * 
 * @param beh 活跃行为对象，包含目标位置、缓冲区、力度等参数
 * @return bool 执行是否成功
 * 
 * @note 使用Dasher系统进行精确的位置移动
 * @note 最后一个参数表示使用逆跑
 */
bool BehaviorFormationExecuter::Execute(const ActiveBehavior & beh)
{
	// === 记录阵型日志 ===
	Logger::instance().LogGoToPoint(mSelfState.GetPos(), beh.mTarget, "@Formation");

	// === 执行跑动到目标位置 ===
	return Dasher::instance().GoToPoint(mAgent, beh.mTarget, beh.mBuffer, beh.mPower, false, true);
}

/**
 * @brief BehaviorFormationPlanner 构造函数
 * 
 * 初始化阵型行为规划器。
 * 继承自BehaviorPlannerBase，使用防守数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 使用防守数据作为数据基类
 */
BehaviorFormationPlanner::BehaviorFormationPlanner(Agent & agent):
	BehaviorPlannerBase<BehaviorDefenseData>( agent)
{
}

/**
 * @brief BehaviorFormationPlanner 析构函数
 * 
 * 清理阵型行为规划器资源。
 */
BehaviorFormationPlanner::~BehaviorFormationPlanner()
{
}

/**
 * @brief 规划阵型行为
 * 
 * 规划阵型行为，计算球员的阵型位置并生成相应的行为。
 * 这是阵型行为的核心决策函数。
 * 
 * 决策逻辑：
 * 1. 获取基本阵型位置
 * 2. 根据球员角色调整位置策略
 * 3. 处理防守球员的特殊位置要求
 * 4. 评估阵型位置的优劣
 * 5. 生成阵型行为
 * 
 * @param behavior_list 行为列表，用于添加生成的阵型行为
 * 
 * @note 前锋后退时会减少跑动力度
 * @note 防守球员有特殊的位置优化逻辑
 * @note 使用评估系统评估位置质量
 */
void BehaviorFormationPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 创建阵型行为 ===
	ActiveBehavior formation(mAgent, BT_Formation);

	// === 设置基本阵型参数 ===
	formation.mBuffer = 1.0;  // 缓冲区距离
	formation.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower());  // 根据体力调整力度
	formation.mTarget = mAnalyser.mHome[mSelfState.GetUnum()];  // 目标位置：分析器的阵型位置
	
	// === 前锋特殊处理 ===
	// 如果目标位置在当前位置后面且自己是前锋，减少跑动力度
	if(formation.mTarget.X() < mSelfState.GetPos().X() && mAgent.GetFormation().GetMyRole().mLineType == LT_Forward){
		formation.mPower = mSelfState.CorrectDashPowerForStamina(ServerParam::instance().maxDashPower())/2;
	}

	// === 防守球员特殊处理 ===
	if(mAgent.GetFormation().GetMyRole().mLineType == LT_Defender){
		// === 限制防守球员位置 ===
		formation.mTarget.SetX(Min(0.0,formation.mTarget.X()));  // 防守球员不超过中线
		
		// === 深度防守特殊处理 ===
		if(mBallState.GetPos().X() < -25){  // 球在己方深处
			// === 获取守门员信息 ===
			Unum goalie = mWorldState.GetTeammateGoalieUnum();
			Vector gpos = mWorldState.GetTeammate(goalie).GetPos();
			
			// === 计算防守位置 ===
			Vector left = (ServerParam::instance().ourLeftGoalPost() + gpos)/2;   // 左侧防守位置
			Vector right = (ServerParam::instance().ourRightGoalPost() + gpos)/2;  // 右侧防守位置
			Line l(gpos,mBallState.GetPos());  // 守门员到球的连线
			
			// === 判断球在左侧还是右侧 ===
			if(l.IsPointInSameSide(left,ServerParam::instance().ourGoal())){
				// === 球在左侧 ===
				const std::vector<Unum> & t2t = mPositionInfo.GetClosePlayerToPoint(left,goalie);
				if(t2t[0] == mSelfState.GetUnum()){
					formation.mTarget = left;  // 自己是最接近左侧位置的球员
				}
				else if(mPositionInfo.GetClosePlayerToPoint(right,t2t[0])[0] == mSelfState.GetUnum()){
					formation.mTarget = right;  // 自己是第二接近右侧位置的球员
				} else if(mPositionInfo.GetClosePlayerToPoint(right,t2t[0])[0] == goalie && mPositionInfo.GetClosePlayerToPoint(right,t2t[0])[1] == mSelfState.GetUnum()){
					formation.mTarget = right;  // 守门员最接近，自己是第二接近
				}
			}
			else {
				// === 球在右侧 ===
				const std::vector<Unum> & t2t = mPositionInfo.GetClosePlayerToPoint(right,goalie);
				if(t2t[0] == mSelfState.GetUnum()){
					formation.mTarget = right;  // 自己是最接近右侧位置的球员
				}
				else if(mPositionInfo.GetClosePlayerToPoint(left,t2t[0])[0] == mSelfState.GetUnum()){
					formation.mTarget = left;  // 自己是第二接近左侧位置的球员
				} else if(mPositionInfo.GetClosePlayerToPoint(left,t2t[0])[0] == goalie && mPositionInfo.GetClosePlayerToPoint(right,t2t[0])[1] == mSelfState.GetUnum()){
					formation.mTarget = left;  // 守门员最接近，自己是第二接近
				}
			}
		}
		
		// === 评估防守位置 ===
		formation.mEvaluation = Evaluation::instance().EvaluatePosition(formation.mTarget, true);
	}
	else {
		// === 评估普通位置 ===
		formation.mEvaluation = Evaluation::instance().EvaluatePosition(formation.mTarget, false);
	}

	// === 添加到行为列表 ===
	behavior_list.push_back(formation);
}

