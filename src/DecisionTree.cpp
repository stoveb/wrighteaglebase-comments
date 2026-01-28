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
 * @file DecisionTree.cpp
 * @brief 决策树实现 - WrightEagleBase 的核心决策引擎
 * 
 * 本文件实现了 DecisionTree 类，它是 WrightEagleBase 系统的决策核心：
 * 1. 实现基于优先级的行为选择机制
 * 2. 管理不同类型行为的决策流程
 * 3. 提供统一的决策接口
 * 
 * 决策树结构（按优先级排序）：
 * - Penalty: 点球相关行为（最高优先级）
 * - Setplay: 定位球行为
 * - Attack: 进攻行为
 * - Defense: 防守行为（普通球员）/ Goalie: 守门员行为（守门员）
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "DecisionTree.h"
#include "BehaviorPenalty.h"
#include "BehaviorSetplay.h"
#include "BehaviorAttack.h"
#include "BehaviorGoalie.h"
#include "BehaviorDefense.h"
#include "BehaviorIntercept.h"
#include "Agent.h"
#include "Strategy.h"
#include "TimeTest.h"

/**
 * @brief 决策树主决策函数
 * 
 * 这是决策树的核心决策函数，负责为智能体选择最佳行为：
 * 1. 验证智能体状态（必须存活）
 * 2. 搜索最佳行为
 * 3. 设置并执行选中的行为
 * 
 * @param agent 参与决策的智能体引用
 * @return bool 是否成功执行了行为，true表示成功，false表示无合适行为
 * 
 * @note 这是整个决策系统的入口点，每个周期都会调用
 * @note 如果没有找到合适的行为，返回false，智能体将保持当前状态
 */
bool DecisionTree::Decision(Agent & agent)
{
	// 断言：智能体必须处于存活状态
	// 死亡状态的智能体不能进行决策
	Assert(agent.GetSelf().IsAlive());

	// 搜索最佳行为（step=1表示从第一层开始搜索）
	ActiveBehavior beh = Search(agent, 1);

	// 检查是否找到了有效行为
	if (beh.GetType() != BT_None) {
		// 设置智能体的当前活跃行为类型
		agent.SetActiveBehaviorInAct(beh.GetType());
		
		// 断言：行为必须属于当前智能体
		Assert(&beh.GetAgent() == &agent);
		
		// 执行选中的行为
		return beh.Execute();
	}
	
	// 没有找到合适的行为
	return false;
}

/**
 * @brief 决策树搜索函数
 * 
 * 在决策树中搜索最佳行为，采用分层决策机制：
 * 1. 检查智能体状态（空闲状态直接返回）
 * 2. 根据智能体类型（守门员/普通球员）选择不同的行为序列
 * 3. 按优先级顺序评估各种行为
 * 4. 返回评估最高的行为
 * 
 * 决策优先级（从高到低）：
 * 守门员：Penalty > Setplay > Attack > Goalie
 * 普通球员：Penalty > Setplay > Attack > Defense
 * 
 * @param agent 参与决策的智能体引用
 * @param step 决策树的搜索深度（当前只支持step=1）
 * @return ActiveBehavior 搜索到的最佳行为，可能是BT_None（无行为）
 * 
 * @note MutexPlan使用短路或运算，一旦找到有效行为就停止后续搜索
 * @note 这种设计确保了高优先级行为的优先执行
 */
ActiveBehavior DecisionTree::Search(Agent & agent, int step)
{
	// 当前只支持第一层决策（step=1）
	// 决策树目前只支持单层决策，不支持多层递归决策
	if (step == 1) {
		// 检查智能体是否处于空闲状态
		// 空闲状态的智能体不能执行任何行为
		if (agent.GetSelf().IsIdling()) {
			return ActiveBehavior(agent, BT_None); // 返回无行为
		}

		// 创建活跃行为列表，用于存储所有候选行为
		std::list<ActiveBehavior> active_behavior_list;

		// 根据智能体类型选择不同的行为决策序列
		// 守门员和普通球员有不同的行为优先级
		if (agent.GetSelf().IsGoalie()) {
			// 守门员的决策序列：点球 > 定位球 > 进攻 > 守门员专用行为
			// 使用短路或运算，一旦某个行为规划器产生有效行为就停止
			// 这种设计确保了高优先级行为的优先执行
			MutexPlan<BehaviorPenaltyPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorSetplayPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorAttackPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorGoaliePlanner>(agent, active_behavior_list);
		}
		else {
			// 普通球员的决策序列：点球 > 定位球 > 进攻 > 防守
			// 同样使用短路或运算，按优先级顺序尝试各种行为
			MutexPlan<BehaviorPenaltyPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorSetplayPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorAttackPlanner>(agent, active_behavior_list) ||
			MutexPlan<BehaviorDefensePlanner>(agent, active_behavior_list);
		}

		// 检查是否找到了候选行为
		if (!active_behavior_list.empty()){
			// 从候选行为中选择最佳的一个
			// GetBestActiveBehavior会根据评估分数选择最优行为
			return GetBestActiveBehavior(agent, active_behavior_list);
		}
		else {
			// 没有找到任何合适的行为
			// 返回无行为，智能体将保持当前状态
			return ActiveBehavior(agent, BT_None);
		}
	}
	else {
		// 不支持多层决策（当前版本只支持单层决策）
		// 如果step不为1，直接返回无行为
		return ActiveBehavior(agent, BT_None);
	}
}

/**
 * @brief 从候选行为列表中选择最佳行为
 * 
 * 从所有候选行为中选择评估分数最高的行为：
 * 1. 保存行为列表到智能体，供下一周期决策参考
 * 2. 按评估分数降序排序
 * 3. 返回分数最高的行为
 * 
 * @param agent 参与决策的智能体引用
 * @param behavior_list 候选行为列表的引用
 * @return ActiveBehavior 评估分数最高的行为
 * 
 * @note ActiveBehavior重载了>运算符，基于mEvaluation进行比较
 * @note 保存行为列表的目的是为了下一周期的决策优化
 */
ActiveBehavior DecisionTree::GetBestActiveBehavior(Agent & agent, std::list<ActiveBehavior> & behavior_list)
{
	// 保存活跃行为列表到智能体
	// behavior_list里面存储了本周期所有behavior决策出的最优activebehavior
	// 这里统一保存一下，供特定behavior下周期plan时使用
	// 这样下一周期就可以参考本周期的决策结果进行优化
	agent.SaveActiveBehaviorList(behavior_list);

	// 按评估分数降序排序（使用ActiveBehavior重载的>运算符）
	// 分数越高的行为排在越前面
	// ActiveBehavior重载了>运算符，基于mEvaluation进行比较
	behavior_list.sort(std::greater<ActiveBehavior>());

	// 返回分数最高的行为（列表的第一个元素）
	// 这是本轮决策的最终结果
	return behavior_list.front();
}
