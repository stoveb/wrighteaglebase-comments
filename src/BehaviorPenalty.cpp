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
 * @file BehaviorPenalty.cpp
 * @brief 点球行为实现 - WrightEagleBase 的点球系统
 * 
 * 本文件实现了点球行为的执行器和规划器：
 * 1. BehaviorPenaltyExecuter：点球行为执行器
 * 2. BehaviorPenaltyPlanner：点球行为规划器
 * 
 * 点球行为是特殊比赛情况的重要组成部分，
 * 负责处理点球进攻和防守的所有相关动作。
 * 
 * 主要功能：
 * - 点球进攻：执行点球射门和准备动作
 * - 点球防守：执行点球防守和拦截动作
 * - 位置安排：安排球员在点球时的位置
 * - 时机控制：精确控制点球的执行时机
 * - 视觉管理：管理点球时的视觉请求
 * 
 * 技术特点：
 * - 状态机：基于比赛模式的状态机设计
 * - 角色区分：守门员和普通球员的不同处理
 * - 时机精确：精确的时间控制和等待逻辑
 * - 位置计算：智能的点球位置计算
 * - 视觉优化：针对点球情况的视觉优化
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorPenalty.h"
#include "WorldState.h"
#include "Agent.h"
#include "BehaviorGoalie.h"
#include "BehaviorShoot.h"
#include "BehaviorDribble.h"
#include "BehaviorIntercept.h"
#include "Kicker.h"
#include "Dasher.h"
#include "VisualSystem.h"

// === 点球行为类型定义 ===
const BehaviorType BehaviorPenaltyExecuter::BEHAVIOR_TYPE = BT_Penalty;

// === 自动注册点球行为执行器 ===
namespace
{
	bool ret = BehaviorExecutable::AutoRegister<BehaviorPenaltyExecuter>();
}

//==============================================================================
/**
 * @brief BehaviorPenaltyExecuter 构造函数
 * 
 * 初始化点球行为执行器。
 * 继承自BehaviorExecuterBase，使用进攻数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会验证自动注册结果
 * @note 使用进攻数据作为数据基类
 */
BehaviorPenaltyExecuter::BehaviorPenaltyExecuter(Agent & agent): BehaviorExecuterBase<BehaviorAttackData>(agent)
{
	Assert(ret);  // 验证自动注册是否成功
}

//==============================================================================
/**
 * @brief 执行点球行为
 * 
 * 执行具体的点球动作，根据详细类型选择不同的执行方式。
 * 这是点球行为的核心执行函数。
 * 
 * 执行逻辑：
 * 1. 获取对方守门员状态
 * 2. 根据行为详细类型执行相应动作
 * 3. 处理扫描、移动和取球三种情况
 * 
 * @param beh 活跃行为对象，包含行为类型和相关参数
 * @return bool 执行是否成功
 * 
 * @note 支持三种点球行为：扫描、移动和取球
 * @note 守门员和普通球员有不同的处理逻辑
 */
bool BehaviorPenaltyExecuter::Execute(const ActiveBehavior &beh)
{
    // === 获取对方守门员状态 ===
    const PlayerState & opp_goalie_state = mWorldState.GetOpponent(mWorldState.GetOpponentGoalieUnum());

    // === 根据行为详细类型执行相应动作 ===
    switch (beh.mDetailType)
    {
    case BDT_Setplay_Scan:
		// === 执行扫描动作 ===
		mAgent.Turn(beh.mAngle);  // 转身扫描
        VisualSystem::instance().SetCanTurn(mAgent, false);  // 禁止转身
        break;
        
    case BDT_Setplay_Move:
        // === 执行移动动作 ===
        if (mSelfState.IsGoalie())
        {
            // === 守门员移动 ===
            Dasher::instance().GoToPoint(mAgent, beh.mTarget, 0.26);  // 移动到目标位置
            VisualSystem::instance().RaiseBall(mAgent, 1.0);  // 提升球的视觉优先级
        }
        else
        {
            // === 普通球员移动 ===
        	Dasher::instance().GoToPoint(mAgent, beh.mTarget, 0.26);  // 移动到目标位置
        }
        break;
        
    case BDT_Setplay_GetBall:
		// === 执行取球动作 ===
		if (mAgent.GetSelfUnum() == mStrategy.GetPenaltyTaker())
        {
            // === 点球主罚者处理 ===
			if (mWorldState.GetPlayMode() == PM_Our_Penalty_Ready)
            {
                // === 点球准备阶段 ===
                if (mWorldState.CurrentTime() - mWorldState.GetPlayModeTime() < 
                    ServerParam::instance().penReadyWait() - 6) // 等待时机
                {
                    // === 等待时机，调整身体方向 ===
                    Dasher::instance().GetTurnBodyToAngleAction(mAgent, 0.0).Execute(mAgent);
                    VisualSystem::instance().RaisePlayer(mAgent, opp_goalie_state.GetUnum(), 1.0);  // 观察守门员
                    VisualSystem::instance().SetCanTurn(mAgent, false);  // 禁止转身
                }
				else if (mSelfState.IsKickable() == true)
                {
                    // === 可踢球，执行点球 ===
                    Kicker::instance().KickBall(mAgent, beh.mAngle, beh.mKickSpeed);
                }
                else 
                {
                    // === 不可踢球，移动到球附近 ===
					Dasher::instance().GoToPoint(mAgent, beh.mTarget, 0.26);
                }
            }
			else if (mWorldState.GetPlayMode() == PM_Our_Penalty_Taken)
            {
                // === 点球已踢，移动到目标位置 ===
				Dasher::instance().GoToPoint(mAgent,beh.mTarget, 0.26);
            }
            else
            {
                // === 异常情况 ===
                std::cerr << "what penalty_taker will do ???" << std::endl;
            }
        }
        else
        {
            // === 非点球主罚者 ===
            Dasher::instance().GetTurnBodyToAngleAction(mAgent, 0.0).Execute(mAgent);
            VisualSystem::instance().RaisePlayer(mAgent, opp_goalie_state.GetUnum(), 1.0);  // 观察守门员
            VisualSystem::instance().SetCanTurn(mAgent, false);  // 禁止转身
        }
        break;
        
    default:
        // === 未知行为类型 ===
        break;
	}
	return true;
}

/**
 * @brief BehaviorPenaltyPlanner 构造函数
 * 
 * 初始化点球行为规划器。
 * 继承自BehaviorPlannerBase，使用进攻数据。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 使用进攻数据作为数据基类
 */
BehaviorPenaltyPlanner::BehaviorPenaltyPlanner(Agent & agent): BehaviorPlannerBase<BehaviorAttackData>( agent )
{
}

BehaviorPenaltyPlanner::~BehaviorPenaltyPlanner()
{
}

//==============================================================================
/**
 * @brief 规划点球行为
 * 
 * 规划点球行为，根据比赛模式和球员角色生成相应的点球行为。
 * 这是点球行为的核心决策函数。
 * 
 * 决策逻辑：
 * 1. 根据球员角色（守门员/普通球员）选择处理方式
 * 2. 根据比赛模式确定行为类型
 * 3. 为不同行为类型设置相应参数
 * 4. 生成并添加到行为列表
 * 
 * 行为类型：
 * - BDT_Setplay_Scan：扫描观察
 * - BDT_Setplay_Move：移动到位置
 * - BDT_Setplay_GetBall：取球/射门
 * 
 * @param behaviorlist 行为列表，用于添加生成的点球行为
 * 
 * @note 守门员和普通球员有不同的处理逻辑
 * @note 点球主罚者有特殊的处理逻辑
 * @note 包含精确的时间控制和位置计算
 */
void BehaviorPenaltyPlanner::Plan(std::list<ActiveBehavior> &behaviorlist)
{
	// === 创建点球行为 ===
	ActiveBehavior penaltyKO(mAgent, BT_Penalty);

    // === 守门员处理 ===
    if (mSelfState.IsGoalie())
    {
        // === 根据比赛模式确定守门员行为 ===
        switch (mWorldState.GetPlayMode())
        {
            // === 点球结束后或准备阶段，原地休息 ===
            case PM_Penalty_On_Our_Field:
		    case PM_Penalty_On_Opp_Field:
            case PM_Our_Penalty_Score:
            case PM_Our_Penalty_Miss:
            case PM_Opp_Penalty_Score:
            case PM_Opp_Penalty_Miss:
            case PM_Our_Penalty_Setup:
            case PM_Our_Penalty_Ready:
	        case PM_Our_Penalty_Taken:
                penaltyKO.mDetailType = BDT_Setplay_Scan;  // 扫描观察
                break;
                
            // === 对方点球设置阶段，移动到防守位置 ===
            case PM_Opp_Penalty_Setup:
                penaltyKO.mDetailType = BDT_Setplay_Move;  // 移动到位置
                break;
                
            // === 对方点球准备或执行阶段，准备防守 ===
            case PM_Opp_Penalty_Ready:
            case PM_Opp_Penalty_Taken:
		    penaltyKO.mDetailType = BDT_Setplay_GetBall;  // 准备接球
		    break;
            default:
                return;  // 其他模式不处理
        }
    }
    else
    {
        // === 普通球员处理 ===
	    switch (mWorldState.GetPlayMode())
        {
            // === 点球结束后，原地休息 ===
		    case PM_Penalty_On_Our_Field:
		    case PM_Penalty_On_Opp_Field:
            case PM_Our_Penalty_Score:
            case PM_Our_Penalty_Miss:
            case PM_Opp_Penalty_Score:
            case PM_Opp_Penalty_Miss:
		    penaltyKO.mDetailType = BDT_Setplay_Scan;  // 原地休息
		    break;
            
            // === 点球设置阶段，移动到目标位置 ===
            case PM_Our_Penalty_Setup:
            case PM_Opp_Penalty_Setup:
		    penaltyKO.mDetailType = BDT_Setplay_Move;  // 走向目标位置
		    break;
            
            // === 点球准备或执行阶段，准备踢球 ===
            case PM_Our_Penalty_Ready:
		    case PM_Our_Penalty_Taken:
            case PM_Opp_Penalty_Ready:
            case PM_Opp_Penalty_Taken:
		    penaltyKO.mDetailType = BDT_Setplay_GetBall;  // 正在罚点球
		    break;
		    default:
		    return;  // 其他模式不处理
	    }
    }

    // === 根据行为类型设置参数 ===
    if (penaltyKO.mDetailType == BDT_Setplay_Scan)
    {
        // === 扫描行为参数 ===
		penaltyKO.mAngle = 60.0;  // 扫描角度
        behaviorlist.push_back(penaltyKO);
    }
	else if (penaltyKO.mDetailType == BDT_Setplay_Move)
    {
        // === 移动行为参数 ===
        if (mSelfState.IsGoalie())
        {
            // === 守门员移动目标 ===
            // 移动到禁区内合适位置
            penaltyKO.mTarget = Vector(-ServerParam::instance().PITCH_LENGTH / 2.0 + ServerParam::instance().penMaxGoalieDistX() - 1.0, 0.0);
        }
		else if (mAgent.GetSelfUnum() == mStrategy.GetPenaltyTaker())
        {
            // === 点球主罚者移动逻辑 ===
			if (mWorldState.CurrentTime() - mWorldState.GetPlayModeTime() < 5 && mStrategy.IsPenaltyFirstStep() != false)
            {
                // === 第一步：设置第一步标志 ===
				mStrategy.SetPenaltyFirstStep(false);
            }

            // === 检查是否到达第一步位置 ===
			if (mSelfState.GetPos().Dist(Vector(6.6, 0.0)) < 1.0 && mStrategy.IsPenaltyFirstStep() != true)
            {
                // === 到达第一步位置，设置标志 ===
                mStrategy.SetPenaltyFirstStep(true);
            }

            if (mStrategy.IsPenaltyFirstStep() == false)
            {
                // === 第一步：移动到中间位置 ===
				penaltyKO.mTarget = Vector(6.6, 0.0);
            }
            else
            {
                // === 第二步：移动到球附近 ===
            	const Vector setup_ball_pos = Vector(ServerParam::instance().PITCH_LENGTH / 2.0 - ServerParam::instance().penDistX(), 0.0);
				penaltyKO.mTarget = (mBallState.GetPos().Dist(setup_ball_pos) < 1.0) ? setup_ball_pos : mBallState.GetPos();
            }
        }
		else
        {
            // === 其他球员移动到观察位置 ===
			penaltyKO.mAngle = (mAgent.GetSelfUnum() - 2) * 36.0 + 18.0;  // 2 ~ 11
			penaltyKO.mTarget = Polar2Vector(8.26, penaltyKO.mAngle);  // 极坐标转换
        }

		penaltyKO.mEvaluation = 1.0;  // 评估值
		behaviorlist.push_back(penaltyKO);
    }
	else if (penaltyKO.mDetailType == BDT_Setplay_GetBall)
    {
        // === 取球行为参数 ===
		if (mSelfState.IsGoalie())
        {
            // === 守门员防守处理 ===
			BehaviorInterceptPlanner(mAgent).Plan(behaviorlist);  // 截球规划
			if(behaviorlist.empty() || mSelfState.IsBallCatchable())
                BehaviorGoaliePlanner(mAgent).Plan(behaviorlist);  // 守门员规划
        }
        else if (mStrategy.IsMyPenaltyTaken() == true)
        {
            // === 点球主罚者处理 ===
            // 先算带球，射门中根据带球的情况来决策
            BehaviorDribblePlanner(mAgent).Plan(behaviorlist);  // 带球规划
            BehaviorShootPlanner(mAgent).Plan(behaviorlist);    // 射门规划
            BehaviorInterceptPlanner(mAgent).Plan(behaviorlist); // 截球规划
        }

        // === 如果行为列表为空，添加默认行为 ===
        if (behaviorlist.empty())
        {
			penaltyKO.mTarget = mBallState.GetPos();  // 目标：球的位置
			penaltyKO.mAngle = ((mBallState.GetPos() - mSelfState.GetPos()).Dir() > 0.0) ? 36.0 : -36.0;  // 踢球角度
			penaltyKO.mKickSpeed = Min(1.0, Kicker::instance().GetMaxSpeed(mAgent,penaltyKO.mAngle, 1));  // 踢球速度
			penaltyKO.mEvaluation = 1.0;  // 评估值
			behaviorlist.push_back(penaltyKO);
        }
    }
}
