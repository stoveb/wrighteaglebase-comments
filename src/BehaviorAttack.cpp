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
 * @file BehaviorAttack.cpp
 * @brief 进攻行为实现 - WrightEagleBase 的进攻决策核心
 * 
 * 本文件实现了 BehaviorAttackPlanner 类，它是 WrightEagleBase 系统的进攻决策核心：
 * 1. 协调各种进攻行为的规划和选择
 * 2. 管理进攻行为的优先级和评估
 * 3. 实现智能的视觉请求优化
 * 4. 提供统一的进攻决策接口
 * 
 * 进攻行为系统是 WrightEagleBase 的高级决策模块，
 * 负责在获得球权时选择最佳的进攻策略。
 * 
 * 主要功能：
 * - 截球行为：抢夺球权
 * - 射门行为：尝试得分
 * - 传球行为：配合进攻
 * - 带球行为：推进位置
 * - 位置行为：战术站位
 * - 持球行为：控制球权
 * 
 * 决策优先级（从高到低）：
 * Intercept > Shoot > Pass > Dribble > Position > Hold
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorAttack.h"
#include "BehaviorShoot.h"
#include "BehaviorPass.h"
#include "BehaviorDribble.h"
#include "BehaviorIntercept.h"
#include "BehaviorPosition.h"
#include "WorldState.h"
#include "BehaviorHold.h"

/**
 * @brief BehaviorAttackPlanner 构造函数
 * 
 * 初始化进攻行为规划器，设置与智能体的关联。
 * 进攻行为规划器负责协调所有进攻相关的行为决策。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 进攻规划器是行为系统的重要组成部分
 */
BehaviorAttackPlanner::BehaviorAttackPlanner(Agent & agent): BehaviorPlannerBase<BehaviorAttackData>( agent )
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorAttackPlanner 析构函数
 * 
 * 清理进攻行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorAttackPlanner::~BehaviorAttackPlanner()
{
	// 析构函数体为空
}

/**
 * @brief 进攻行为规划函数
 * 
 * 这是进攻行为规划的核心函数，负责生成和选择最佳进攻行为。
 * 该函数按照优先级顺序评估各种进攻行为，并选择最优的一个。
 * 
 * 规划过程：
 * 1. 检查特殊条件（守门员接球等）
 * 2. 按优先级顺序规划各种行为
 * 3. 对候选行为进行排序
 * 4. 选择最优行为并提交视觉请求
 * 5. 返回最终选择的进攻行为
 * 
 * 决策优先级：
 * - Intercept（截球）：最高优先级，抢夺球权
 * - Shoot（射门）：高优先级，尝试得分
 * - Pass（传球）：中高优先级，配合进攻
 * - Dribble（带球）：中优先级，推进位置
 * - Position（位置）：中低优先级，战术站位
 * - Hold（持球）：低优先级，控制球权
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 特殊条件检查：守门员在对手刚控制球时不执行进攻行为
 * @note 视觉请求优化：非最优行为也会提交视觉请求以支持决策
 * @note 视觉请求优先级按指数增长，确保重要信息优先获取
 */
void BehaviorAttackPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 特殊条件检查 ===
	// 如果自己可以接球，且对手刚控制球，且上一行为不是传球或带球，则不执行进攻行为
	// 这主要是为了避免守门员在对手刚获得球权时的不必要动作
	if (mSelfState.IsBallCatchable()  && mStrategy.IsLastOppControl() &&(!(mAgent.IsLastActiveBehaviorInActOf(BT_Pass)||mAgent.IsLastActiveBehaviorInActOf(BT_Dribble)))) return;

	// === 按优先级顺序规划各种进攻行为 ===
	// 每个规划器都会将生成的行为添加到mActiveBehaviorList中
	BehaviorInterceptPlanner(mAgent).Plan(mActiveBehaviorList);  // 截球行为规划
	BehaviorShootPlanner(mAgent).Plan(mActiveBehaviorList);      // 射门行为规划
	BehaviorPassPlanner(mAgent).Plan(mActiveBehaviorList);        // 传球行为规划
	BehaviorDribblePlanner(mAgent).Plan(mActiveBehaviorList);     // 带球行为规划
	BehaviorPositionPlanner(mAgent).Plan(mActiveBehaviorList);   // 位置行为规划
	BehaviorHoldPlanner(mAgent).Plan(mActiveBehaviorList);        // 持球行为规划

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
