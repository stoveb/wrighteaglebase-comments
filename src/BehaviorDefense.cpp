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
 * @file BehaviorDefense.cpp
 * @brief 防守行为实现 - WrightEagleBase 的防守决策核心
 * 
 * 本文件实现了 BehaviorDefensePlanner 类，它是 WrightEagleBase 系统的防守决策核心：
 * 1. 协调各种防守行为的规划和选择
 * 2. 管理防守行为的优先级和评估
 * 3. 实现智能的防守策略支持
 * 4. 提供统一的防守决策接口
 * 
 * 防守行为系统是 WrightEagleBase 的高级决策模块，
 * 负责在对手进攻时选择最佳的防守策略。
 * 
 * 主要功能：
 * - 阵型防守：保持防守阵型和位置
 * - 阻挡行为：阻挡对手的进攻路线
 * - 标记行为：盯防关键对手球员
 * - 协调防守：多球员间的防守协作
 * 
 * 决策优先级（从高到低）：
 * Formation > Block > Mark
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorDefense.h"
#include "BehaviorFormation.h"
#include "BehaviorBlock.h"
#include "BehaviorMark.h"
#include "WorldState.h"
#include "Agent.h"
#include "Formation.h"
#include "Dasher.h"
#include "Logger.h"
#include "BehaviorIntercept.h"

/**
 * @brief BehaviorDefensePlanner 构造函数
 * 
 * 初始化防守行为规划器，设置与智能体的关联。
 * 防守行为规划器负责协调所有防守相关的行为决策。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 防守规划器是行为系统的重要组成部分
 */
BehaviorDefensePlanner::BehaviorDefensePlanner(Agent & agent): BehaviorPlannerBase <BehaviorDefenseData>( agent )
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorDefensePlanner 析构函数
 * 
 * 清理防守行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorDefensePlanner::~BehaviorDefensePlanner()
{
	// 析构函数体为空
}

/**
 * @brief 防守行为规划函数
 * 
 * 这是防守行为规划的核心函数，负责生成和选择最佳防守行为。
 * 该函数按照优先级顺序评估各种防守行为，并选择最优的一个。
 * 
 * 规划过程：
 * 1. 按优先级顺序规划各种防守行为
 * 2. 对候选行为进行排序
 * 3. 选择最优行为并提交视觉请求
 * 4. 返回最终选择的防守行为
 * 
 * 决策优先级：
 * - Formation（阵型防守）：最高优先级，保持防守阵型
 * - Block（阻挡行为）：中高优先级，阻挡对手进攻路线
 * - Mark（标记行为）：中优先级，盯防关键对手
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 防守行为优先级确保阵型稳定性和防守效率
 * @note 视觉请求优化：非最优行为也会提交视觉请求以支持决策
 * @note 视觉请求优先级按指数增长，确保重要信息优先获取
 */
void BehaviorDefensePlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 按优先级顺序规划各种防守行为 ===
	// 每个规划器都会将生成的行为添加到mActiveBehaviorList中
	BehaviorFormationPlanner(mAgent).Plan(behavior_list);  // 阵型防守规划
	BehaviorBlockPlanner(mAgent).Plan(behavior_list);        // 阻挡行为规划
	BehaviorMarkPlanner(mAgent).Plan(behavior_list);          // 标记行为规划

	// === 处理规划结果 ===
	if (!mActiveBehaviorList.empty()) {
		// === 行为排序和选择 ===
		// 按评估分数从高到低排序
		mActiveBehaviorList.sort(std::greater<ActiveBehavior>());
		
		// 将最优行为添加到结果列表中
		behavior_list.push_back(mActiveBehaviorList.front());

		// === 视觉请求优化 ===
		// 如果有多个候选行为，允许非最优行为提交视觉请求
		// 这样可以为下一周期的决策提供更好的信息支持
		if (mActiveBehaviorList.size() > 1) {
			double plus = 1.0;  // 视觉请求优先级基数
			ActiveBehaviorPtr it = mActiveBehaviorList.begin();
			
			// 从第二个行为开始，为每个非最优行为提交视觉请求
			for (++it; it != mActiveBehaviorList.end(); ++it) {
				it->SubmitVisualRequest(plus);  // 提交视觉请求
				plus *= 2.0;  // 优先级按指数增长，确保重要信息优先获取
			}
		}
	}
}
