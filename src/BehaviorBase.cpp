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
 * @file BehaviorBase.cpp
 * @brief 行为基类实现 - WrightEagleBase 的行为系统基础架构
 * 
 * 本文件实现了 WrightEagleBase 系统的行为基类，包括：
 * 1. BehaviorAttackData：进攻行为数据类
 * 2. BehaviorDefenseData：防守行为数据类
 * 3. ActiveBehavior：活跃行为类
 * 4. 行为工厂模式的相关实现
 * 
 * 行为基类系统是 WrightEagleBase 的行为系统基础架构，
 * 为所有具体行为类提供统一的接口和数据结构。
 * 
 * 主要功能：
 * - 数据管理：管理行为相关的数据和状态
 * - 行为执行：提供行为的执行接口
 * - 视觉请求：支持视觉调试请求
 * - 工厂模式：使用工厂模式创建行为实例
 * - 阵型管理：管理进攻和防守阵型
 * 
 * 技术特点：
 * - 继承体系：基于继承的行为类体系
 * - 工厂模式：使用工厂模式创建行为实例
 * - 阵型切换：支持进攻和防守阵型的动态切换
 * - 视觉调试：支持视觉调试和日志记录
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "BehaviorBase.h"
#include "Agent.h"
#include "WorldState.h"
#include "InfoState.h"
#include "Strategy.h"
#include "Analyser.h"
#include "Logger.h"

/**
 * @brief BehaviorAttackData 构造函数
 * 
 * 初始化进攻行为数据，设置进攻阵型。
 * 该类继承自BehaviorAttackData，专门用于进攻行为的数据管理。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会初始化所有相关的状态引用
 * @note 会自动切换到进攻阵型
 * @note 包含世界状态、球状态、自身状态等关键数据
 */
BehaviorAttackData::BehaviorAttackData(Agent & agent):
	mAgent ( agent ),
    mWorldState ( agent.GetWorldState() ),
    mBallState ( agent.GetWorldState().GetBall() ),
	mSelfState ( agent.Self() ),
	mPositionInfo ( agent.Info().GetPositionInfo()),
	mInterceptInfo ( agent.Info().GetInterceptInfo()),
	mStrategy (agent.GetStrategy()),
    mFormation ( agent.GetFormation() )
{
	// === 切换到进攻阵型 ===
	mFormation.Update(Formation::Offensive, "Offensive");
}

/**
 * @brief BehaviorAttackData 析构函数
 * 
 * 清理进攻行为数据资源。
 * 在析构时会回滚阵型状态。
 * 
 * @note 会将阵型回滚到之前的状态
 * @note 确保阵型状态的一致性
 */
BehaviorAttackData::~BehaviorAttackData()
{
	// === 回滚阵型状态 ===
	mFormation.Rollback("Offensive");
}

/**
 * @brief BehaviorDefenseData 构造函数
 * 
 * 初始化防守行为数据，设置防守阵型和分析器。
 * 该类继承自BehaviorAttackData，专门用于防守行为的数据管理。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会初始化所有相关的状态引用
 * @note 会自动切换到防守阵型
 * @note 包含分析器用于防守分析
 */
BehaviorDefenseData::BehaviorDefenseData(Agent & agent):
	BehaviorAttackData (agent),
	mAnalyser (agent.GetAnalyser())
{
	// === 切换到防守阵型 ===
	mFormation.Update(Formation::Defensive, "Defensive");
}

/**
 * @brief BehaviorDefenseData 析构函数
 * 
 * 清理防守行为数据资源。
 * 在析构时会回滚阵型状态。
 * 
 * @note 会将阵型回滚到之前的状态
 * @note 确保阵型状态的一致性
 */
BehaviorDefenseData::~BehaviorData()
{
	// === 回滚阵型状态 ===
	mFormation.Rollback("Defensive");
}

/**
 * @brief 执行活跃行为
 * 
 * 这是活跃行为的执行函数，负责：
 * 1. 创建对应的行为执行器
 * 2. 提交视觉请求
 * 3. 执行行为
 * 4. 清理资源
 * 
 * @return bool 行为是否成功执行
 * 
 * @note 使用工厂模式创建行为执行器
 * @note 执行前会提交视觉请求用于调试
 * @note 执行后会自动清理行为执行器资源
 */
bool ActiveBehavior::Execute()
{
	// === 创建行为执行器 ===
	BehaviorExecutable * behavior = BehaviorFactory::instance().CreateBehavior(GetAgent(), GetType());

	if (behavior){
		// === 记录执行日志 ===
		Logger::instance().GetTextLogger("executing") << GetAgent().GetWorldState().CurrentTime() << " " << BehaviorFactory::instance().GetBehaviorName(GetType()) << " executing" << std::endl;

		// === 提交视觉请求 ===
		behavior->SubmitVisualRequest(*this);
		
		// === 执行行为 ===
		bool ret = behavior->Execute(*this);

		// === 清理资源 ===
		delete behavior;
		return ret;
	}
	else {
		return false;  // 创建失败，返回false
	}
}

/**
 * @brief 提交视觉请求
 * 
 * 为活跃行为提交视觉请求，用于调试和可视化。
 * 该函数会创建行为执行器并提交视觉请求。
 * 
 * @param plus 视觉请求的额外参数
 * 
 * @note 使用工厂模式创建行为执行器
 * @note 视觉请求用于调试和可视化分析
 * @note 执行后会自动清理行为执行器资源
 */
void ActiveBehavior::SubmitVisualRequest(double plus)
{
	// === 创建行为执行器 ===
	BehaviorExecutable * behavior = BehaviorFactory::instance().CreateBehavior(GetAgent(), GetType());

	if (behavior){
		// === 记录视觉请求日志 ===
		Logger::instance().GetTextLogger("executing") << GetAgent().GetWorldState().CurrentTime() << " " << BehaviorFactory::instance().GetBehaviorName(GetType()) << " visual plus: " << plus << std::endl;

		// === 提交视觉请求 ===
		behavior->SubmitVisualRequest(*this, plus);

		// === 清理资源 ===
		delete behavior;
	}
}

BehaviorFactory::BehaviorFactory()
{
}

BehaviorFactory::~BehaviorFactory()
{
}

BehaviorFactory & BehaviorFactory::instance()
{
	static BehaviorFactory factory;

	return factory;
}

BehaviorExecutable * BehaviorFactory::CreateBehavior(Agent & agent, BehaviorType type)
{
	if (type == BT_None) {
		return 0;
	}

	BehaviorCreator creator = mCreatorMap[type];

	if (creator){
		return creator( agent );
	}

	return 0;
}

bool BehaviorFactory::RegisterBehavior(BehaviorType type, BehaviorCreator creator, const char *behavior_name)
{
	if (type > BT_None && type < BT_Max) {
		if (mCreatorMap[type] == 0){
			mCreatorMap[type] = creator;
			mNameMap[type] = behavior_name;
			return true;
		}
	}

	return false;
}

