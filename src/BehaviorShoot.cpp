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
 * @file BehaviorShoot.cpp
 * @brief 射门行为实现 - WrightEagleBase 的射门动作执行核心
 * 
 * 本文件实现了 BehaviorShootExecuter 和 BehaviorShootPlanner 类，它们是 WrightEagleBase 系统的射门动作核心：
 * 1. 实现智能的射门动作执行
 * 2. 处理不同类型的射门策略
 * 3. 优化射门角度和力度控制
 * 4. 提供灵活的射门规划支持
 * 
 * 射门行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在进攻时实现有效的射门得分。
 * 
 * 主要功能：
 * - 智能射门：根据球门位置选择射门策略
 * - 铲球射门：使用铲球动作进行射门
 * - 精确控制：优化射门角度和力度
 * - 快速射门：支持快速射门模式
 * 
 * 技术特点：
 * - 多种模式：支持普通射门和铲球射门
 * - 精确计算：使用几何计算优化射门角度
 * - 力度控制：根据距离调整射门力度
 * - 快速执行：支持快速射门决策
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorShoot.h"
#include "ServerParam.h"
#include "PlayerParam.h"
#include "VisualSystem.h"
#include "Utilities.h"
#include "Dasher.h"
#include "Tackler.h"
#include "Kicker.h"
#include "InterceptModel.h"
#include "ActionEffector.h"
#include "TimeTest.h"
#include "Evaluation.h"
#include <list>
#include <cmath>
#include "PositionInfo.h"
#include "Geometry.h"
#include "BehaviorBase.h"

using namespace std;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册射门行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorShootExecuter>();
}

/**
 * @brief 射门行为类型常量
 * 
 * 定义射门行为的行为类型为BT_Shoot，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorShootExecuter::BEHAVIOR_TYPE = BT_Shoot;

/**
 * @brief BehaviorShootExecuter 构造函数
 * 
 * 初始化射门行为执行器，设置与智能体的关联。
 * 射门行为执行器负责具体的射门动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorShootExecuter::BehaviorShootExecuter(Agent & agent): BehaviorExecuterBase<BehaviorAttackData>(agent)
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorShootExecuter 析构函数
 * 
 * 清理射门行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorShootExecuter::~BehaviorShootExecuter()
{
	// 析构函数体为空
}

/**
 * @brief 执行射门行为
 * 
 * 这是射门行为执行的核心函数，负责实现具体的射门动作。
 * 该函数根据射门类型和目标位置，生成相应的射门指令。
 * 
 * 执行过程：
 * 1. 记录射门日志信息
 * 2. 根据射门类型选择执行策略
 * 3. 调用相应的射门执行函数
 * 4. 返回执行结果
 * 
 * 支持的射门类型：
 * - BDT_Shoot_Tackle：铲球射门
 * - 其他类型：普通射门
 * 
 * @param shoot 活跃的射门行为，包含目标和类型信息
 * @return bool 射门行为是否成功执行
 * 
 * @note 铲球射门可能造成球或球员被抬升
 * @note 普通射门使用最大球速和快速模式
 */
bool BehaviorShootExecuter::Execute(const ActiveBehavior & shoot)
{
	// === 记录射门日志 ===
	// 记录射门起始位置和目标位置，便于调试和分析
	Logger::instance().LogShoot(mBallState.GetPos(), shoot.mTarget, "@Shoot");
	
	// === 根据射门类型执行不同策略 ===
	if (shoot.mDetailType == BDT_Shoot_Tackle) {
		// === 铲球射门模式 ===
		// 使用铲球动作向指定方向射门
		return Tackler::instance().TackleToDir(mAgent,shoot.mAngle);
	}
	else {
		// === 普通射门模式 ===
		// 使用踢球动作向目标位置射门
		// 参数说明：
		// - shoot.mTarget: 射门目标位置
		// - ServerParam::instance().ballSpeedMax(): 最大球速
		// - KM_Quick: 快速射门模式
		// - 0: 踢球次数限制（0表示不限制）
		// - true: 是否强制执行
		return Kicker::instance().KickBall(mAgent, shoot.mTarget, ServerParam::instance().ballSpeedMax(), KM_Quick, 0, true);
	}
}

/**
 * @brief BehaviorShootPlanner 构造函数
 * 
 * 初始化射门行为规划器，设置与智能体的关联。
 * 射门行为规划器负责射门决策的规划和评估。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 */
BehaviorShootPlanner::BehaviorShootPlanner(Agent & agent):
	BehaviorPlannerBase<BehaviorAttackData>(agent)
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorShootPlanner 析构函数
 * 
 * 清理射门行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorShootPlanner::~BehaviorShootPlanner()
{
}

/**
 * Plan.
 * None or one ActiveBehavior will be push back to behavior_list.
 */
void BehaviorShootPlanner::Plan(list<ActiveBehavior> & behavior_list)
{
	if (!mSelfState.IsKickable()) return;

	if (mWorldState.GetPlayMode() == PM_Our_Foul_Charge_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Back_Pass_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Indirect_Free_Kick||
			(mWorldState.GetLastPlayMode()==PM_Our_Indirect_Free_Kick && mAgent.IsLastActiveBehaviorInActOf(BT_Pass)))   //Indircet后传球保持不射门，至少跑位后上个动作会改
	{
		return;
	}

	if (mSelfState.GetPos().X() > ServerParam::instance().pitchRectanglar().Right() - PlayerParam::instance().shootMaxDistance()) {
		AngleDeg left= (ServerParam::instance().oppLeftGoalPost()- mSelfState.GetPos()).Dir()  ;
		AngleDeg right = (ServerParam::instance().oppRightGoalPost()- mSelfState.GetPos()).Dir();
		Vector target ;
		AngleDeg interval;
		Line c(ServerParam::instance().oppLeftGoalPost(),ServerParam::instance().oppRightGoalPost());
		AngleDeg shootDir = mPositionInfo.GetShootAngle(left,right,mSelfState ,interval);
		if(interval < mSelfState.GetRandAngle(ServerParam::instance().maxPower(),ServerParam::instance().ballSpeedMax(),mBallState)*3){
			return;
		}

		Ray f(mSelfState.GetPos(),shootDir);
		c.Intersection(f,target);
		if(Tackler::instance().CanTackleToDir(mAgent, shootDir)
				&& Tackler::instance().GetBallVelAfterTackle(mAgent,shootDir).Mod() > ServerParam::instance().ballSpeedMax() -  0.05){
			ActiveBehavior shoot(mAgent, BT_Shoot,BDT_Shoot_Tackle);
			shoot.mTarget = target;
			shoot.mAngle = shootDir;
			shoot.mEvaluation = 2.0 + FLOAT_EPS;
			behavior_list.push_back(shoot);
		}

		else {
			ActiveBehavior shoot(mAgent, BT_Shoot);
			shoot.mTarget = target;
			shoot.mEvaluation = 2.0 + FLOAT_EPS;

			behavior_list.push_back(shoot);
		}
	}
}
