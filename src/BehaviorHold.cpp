/************************************************************************************
 * WrightEagle (Soccer Simulation League 2D)                                        *
 * BASE SOURCE CODE RELEASE 2016                                                    *
 * Copyright (C) 1998-2016 WrightEagle 2D Soccer Simulation Team,                   *
 *                         Multi-Agent Systems Lab.,                                *
 *                         School of Computer Science and Technology,               *
 *                         University of Science and Technology of China, China.    *
 *                                                                                  *
 * This program is free software; you can redistribute it and/or                    *
 * modify it under the terms of the GNU General Public License                      *
 * as published by the Free Software Foundation; either version 2                   *
 * of the License, or (at your option) any later version.                           *
 *                                                                                  *
 * This program is distributed in the hope that it will be useful,                  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 * GNU General Public License for more details.                                     *
 *                                                                                  *
 * You should have received a copy of the GNU General Public License                *
 * along with this program; if not, write to the Free Software                      *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.  *
 ************************************************************************************/

/**
 * @file BehaviorHold.cpp
 * @brief 持球行为实现 - WrightEagleBase 的持球动作执行核心
 * 
 * 本文件实现了 BehaviorHoldExecuter 和 BehaviorHoldPlanner 类，它们是 WrightEagleBase 系统的持球动作核心：
 * 1. 实现智能的持球动作执行
 * 2. 处理不同情况下的持球策略
 * 3. 优化持球位置和时机选择
 * 4. 提供灵活的持球规划支持
 * 
 * 持球行为系统是 WrightEagleBase 的高级执行模块，
 * 负责在持球状态下实现有效的球权控制。
 * 
 * 主要功能：
 * - 智能持球：根据对手威胁选择持球策略
 * - 转身持球：调整身体方向进行持球
 * - 近身持球：将球控制在身体附近
 * - 威胁应对：根据对手距离调整持球方式
 * 
 * 技术特点：
 * - 威胁感知：实时感知对手威胁程度
 * - 动态调整：根据对手位置调整持球策略
 * - 精确控制：精确的球控制技术
 * - 自适应策略：根据情况选择最佳持球方式
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorHold.h"
#include "Agent.h"
#include "Kicker.h"
#include "WorldState.h"
#include "Strategy.h"
#include "Formation.h"
#include "Dasher.h"
#include "Types.h"
#include "Logger.h"
#include "VisualSystem.h"
#include "CommunicateSystem.h"
#include "Utilities.h"
#include <sstream>
#include <vector>
#include <utility>
#include "Evaluation.h"

using namespace std;

/**
 * @brief 持球行为类型常量
 * 
 * 定义持球行为的行为类型为BT_Hold，
 * 用于行为系统的分类和识别。
 */
const BehaviorType BehaviorHoldExecuter::BEHAVIOR_TYPE = BT_Hold;

/**
 * @brief 自动注册静态变量
 * 
 * 用于自动注册持球行为执行器到行为系统中。
 * 这种设计确保了模块的自动初始化。
 */
namespace {
bool ret = BehaviorExecutable::AutoRegister<BehaviorHoldExecuter>();
}

/**
 * @brief BehaviorHoldExecuter 构造函数
 * 
 * 初始化持球行为执行器，设置与智能体的关联。
 * 持球行为执行器负责具体的持球动作实现。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 * @note 自动注册机制确保执行器被正确注册
 */
BehaviorHoldExecuter::BehaviorHoldExecuter(Agent & agent) :
   BehaviorExecuterBase<BehaviorAttackData>( agent )
{
	// 确保自动注册成功
	Assert(ret);
}

/**
 * @brief BehaviorHoldExecuter 析构函数
 * 
 * 清理持球行为执行器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorHoldExecuter::~BehaviorHoldExecuter(void)
{
	// 析构函数体为空
}

/**
 * @brief 执行持球行为
 * 
 * 这是持球行为执行的核心函数，负责实现具体的持球动作。
 * 该函数根据持球类型和当前状态，生成相应的持球指令。
 * 
 * 执行过程：
 * 1. 记录持球日志信息
 * 2. 根据持球类型选择执行策略
 * 3. 调用相应的持球执行函数
 * 4. 返回执行结果
 * 
 * 支持的持球类型：
 * - BDT_Hold_Turn：转身持球
 * - 其他类型：近身持球
 * 
 * @param hold 活跃的持球行为，包含目标和类型信息
 * @return bool 持球行为是否成功执行
 * 
 * @note 转身持球用于调整身体方向
 * @note 近身持球将球控制在身体附近
 * @note 持球动作会根据对手威胁进行调整
 */
bool BehaviorHoldExecuter::Execute(const ActiveBehavior & hold)
{
	// === 记录持球日志 ===
	// 记录持球起始位置和目标位置，便于调试和分析
	Logger::instance().LogDribble(mBallState.GetPos(), hold.mTarget, "@Hold", true);
	
	// === 根据持球类型执行不同策略 ===
	if(hold.mDetailType == BDT_Hold_Turn){
		// === 转身持球模式 ===
		// 调用跑动系统执行转身动作
		return Dasher::instance().GetTurnBodyToAngleAction(mAgent, hold.mAngle).Execute(mAgent);
	}
	else {
		// === 近身持球模式 ===
		// 调用踢球系统将球控制在身体附近
		// 参数说明：
		// - mAgent: 智能体引用
		// - hold.mAngle: 持球角度
		// - 0.6: 持球距离参数
		return Kicker::instance().KickBallCloseToBody(mAgent ,hold.mAngle, 0.6);
	}
}

/**
 * @brief BehaviorHoldPlanner 构造函数
 * 
 * 初始化持球行为规划器，设置与智能体的关联。
 * 持球行为规划器负责持球决策的规划和评估。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会调用基类的构造函数进行初始化
 */
BehaviorHoldPlanner::BehaviorHoldPlanner(Agent & agent) :
    BehaviorPlannerBase <BehaviorAttackData>(agent)
{
	// 构造函数体为空，初始化由基类完成
}

/**
 * @brief BehaviorHoldPlanner 析构函数
 * 
 * 清理持球行为规划器资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
BehaviorHoldPlanner::~BehaviorHoldPlanner(void)
{
	// 析构函数体为空
}

/**
 * @brief 持球行为规划函数
 * 
 * 这是持球行为规划的核心函数，负责生成和选择最佳持球行为。
 * 该函数根据当前比赛状态和对手威胁，生成持球决策。
 * 
 * 规划过程：
 * 1. 检查是否可以踢球
 * 2. 检查是否适合持球（守门员等）
 * 3. 评估对手威胁程度
 * 4. 根据对手距离选择持球策略
 * 5. 生成持球候选行为
 * 6. 选择最优持球行为
 * 
 * @param behavior_list 活跃行为列表，用于存储规划结果
 * 
 * @note 只有在可以踢球时才考虑持球
 * @note 守门员不进行持球规划
 * @note 根据对手威胁程度动态调整持球策略
 */
void BehaviorHoldPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	// === 检查踢球条件 ===
	// 只有在可以踢球的情况下才考虑持球
	if (!mSelfState.IsKickable()) return;
	
	// === 检查持球条件 ===
	// 守门员不进行持球规划
	if (mSelfState.IsGoalie()) return;
	
	// === 检查对手威胁 ===
	// 当对手可能在2个周期内截球时，需要持球应对
	if(mStrategy.GetSureOppInterCycle() <= 2 &&
			mStrategy.GetSureOppInterCycle() != 0 ){

		// === 创建持球行为 ===
		ActiveBehavior hold(mAgent, BT_Hold);
		
		// === 获取对手信息 ===
		double        dDist;      // 球员与对手的距离
		Vector   		posAgent = mSelfState.GetPos();  // 球员位置
		PlayerState   objOpp = mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToBall());  // 最近对手状态
		Vector 	  	posOpp   = objOpp.GetPos();  // 对手位置
		dDist = (posOpp - posAgent).Mod();  // 计算距离
		AngleDeg      angOpp   = objOpp.GetBodyDir();  // 对手身体方向
		AngleDeg      ang      = 0.0;  // 持球角度

		// === 根据对手距离选择持球策略 ===
		if(dDist < 5 )
			ang = ( posAgent - posOpp ).Dir();
			int iSign = (GetNormalizeAngleDeg( angOpp - ang )) >0 ? -1:1;
			ang +=  iSign*45 - mSelfState.GetBodyDir();
			ang = GetNormalizeAngleDeg( ang );
		}
		// === 生成持球候选行为 ===
		if( mBallState.GetPos().Dist(posAgent + Polar2Vector(0.7,ang))< 0.3 )
		{

			Vector   posBallPred = mBallState.GetPredictedPos(1);
			Vector   posPred     = mSelfState.GetPredictedPos(1);
			if( posPred.Dist( posBallPred )< 0.85 * mSelfState.GetKickableArea() )
			{
				hold.mDetailType = BDT_Hold_Turn;
				hold.mEvaluation = 1.0 + FLOAT_EPS;
				hold.mAngle = (Vector(ServerParam::instance().PITCH_LENGTH / 2.0, 0.0) - mSelfState.GetPos()).Dir();
				mActiveBehaviorList.push_back(hold);
			}
		}
		else
		{
			hold.mDetailType = BDT_Hold_Kick;
			hold.mAngle = ang;
			hold.mEvaluation = 1.0 + FLOAT_EPS;
			mActiveBehaviorList.push_back(hold);
		}

		if (!mActiveBehaviorList.empty()) {
			mActiveBehaviorList.sort(std::greater<ActiveBehavior>());
			behavior_list.push_back(mActiveBehaviorList.front());
		}
	}

}
