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
 ***********************************************************************************/

/**
 * @file DecisionData.cpp
 * @brief 决策数据基类实现
 *
 * DecisionData 是一类“面向决策/行为模块的共享上下文数据”。
 * 其核心作用是：
 * - 将 Agent 内部常用的子系统引用（WorldState/BallState/SelfState/InfoState/Formation）
 *   以成员引用的形式缓存下来，降低各模块获取依赖时的样板代码。
 * - 提供统一的 Update 接口，将“当前周期时间”传递给派生类的时间更新逻辑。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "DecisionData.h"
#include "Agent.h"

/**
 * @brief 构造函数
 *
 * 将 Agent 以及其内部各核心模块的引用缓存到本对象中。
 * 派生类（如 Strategy/各种 Planner/Executer）通常会继承 DecisionData
 * 来直接访问这些共享数据。
 *
 * @param agent 智能体引用
 */
DecisionData::DecisionData(Agent & agent):
	mAgent ( agent ),
	mWorldState ( agent.World() ),
	mBallState ( agent.World().Ball() ),
	mSelfState ( agent.Self() ),
	mInfoState ( agent.Info()),
	mFormation ( agent.GetFormation())
{
}

/**
 * @brief 析构函数
 */
DecisionData::~DecisionData() {
}

/**
 * @brief 更新决策数据
 *
 * 默认使用当前世界时间（WorldState::CurrentTime）作为时间基准，
 * 调用 `UpdateAtTime()`（通常由派生类实现/继承）完成本周期的内部缓存更新。
 */
void DecisionData::Update()
{
	UpdateAtTime(mAgent.GetWorldState().CurrentTime());
}
