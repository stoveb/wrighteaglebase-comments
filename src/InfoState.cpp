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
 * @file InfoState.cpp
 * @brief 信息状态（InfoState）实现
 *
 * InfoState 是基于 WorldState 的“二次加工信息层”，用于缓存/管理一些
 * 计算量较大、但在决策中频繁使用的派生信息，例如：
 * - PositionInfo：球员/球与各对象之间的距离矩阵、最近球员、越位线等。
 * - InterceptInfo：双方对球的截球周期估计、最快截球者等。
 *
 * 设计特点：
 * - InfoState 内部以指针持有多个 Info 子模块，并在 GetXXX() 时触发 Update。
 * - 这种“按需更新”可以在保持接口简单的同时，避免每周期无条件计算所有信息。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "InfoState.h"
#include "InterceptInfo.h"
#include "PositionInfo.h"


/**
 * @brief InfoState 构造函数
 *
 * 创建并持有各个信息子模块（PositionInfo / InterceptInfo）。
 *
 * @param world_state 世界状态指针（用于子模块计算）
 */
InfoState::InfoState(WorldState *world_state)
{
	mpPositionInfo  = new PositionInfo( world_state, this );
	mpInterceptInfo = new InterceptInfo( world_state, this );
}

/**
 * @brief InfoState 析构函数
 *
 * 释放内部持有的子模块对象。
 */
InfoState::~InfoState()
{
	delete mpPositionInfo;
	delete mpInterceptInfo;
}

/**
 * @brief 获取位置信息模块
 *
 * 每次调用都会触发 `PositionInfo::Update()`，确保返回的是基于当前周期的最新结果。
 *
 * @return PositionInfo& 位置信息模块引用
 */
PositionInfo & InfoState::GetPositionInfo() const
{
	mpPositionInfo->Update();
	return *mpPositionInfo;
}

/**
 * @brief 获取截球信息模块
 *
 * 每次调用都会触发 `InterceptInfo::Update()`，确保返回的是基于当前周期的最新结果。
 *
 * @return InterceptInfo& 截球信息模块引用
 */
InterceptInfo & InfoState::GetInterceptInfo() const
{
	mpInterceptInfo->Update();
	return *mpInterceptInfo;
}
