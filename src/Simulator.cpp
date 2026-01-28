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
 * @file Simulator.cpp
 * @brief 模拟器（Simulator）实现
 *
 * Simulator 提供轻量级的球员/球运动模拟，主要用于：
 * - 预测连续 dash 后的位置/速度（PlayerState::GetPredictedPosWithDash/GetPredictedVelWithDash）；
 * - 模拟转向、踢球等原子动作对球员状态的影响；
 * - 估算控球概率（考虑铲球与扑球）；
 * - 随机化初始化（用于测试/蒙特卡洛）。
 *
 * 本文件仅实现 Simulator 单例与 Player::Dash/Act，Ball 与 Player 的主要逻辑
 * 均在头文件中以 inline 形式实现。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "Simulator.h"
#include "ActionEffector.h"
#include "Dasher.h"

/**
 * @brief 构造函数（私有）
 */
Simulator::Simulator() {
}

/**
 * @brief 析构函数
 */
Simulator::~Simulator() {
}

/**
 * @brief 获取全局单例
 *
 * @return Simulator& 全局模拟器实例
 */
Simulator & Simulator::instance()
{
	static Simulator simulator;
	return simulator;
}

/**
 * @brief 球员执行 dash 动作
 *
 * 根据力度、方向、球员类型计算加速度，更新速度与体力，并执行一步运动模拟。
 *
 * @param power dash 力度（可为负，表示后撤）
 * @param dir_idx 方向索引（参见 Dasher::DASH_DIR）
 */
void Simulator::Player::Dash(double power, int dir_idx)
{
	power = GetNormalizeDashPower(power);

	AngleDeg dir = Dasher::DASH_DIR[dir_idx];
	double dir_rate = Dasher::DIR_RATE[dir_idx];;

	bool back_dash = power < 0.0;

	double power_need = ( back_dash
			? power * -2.0
					: power );
	power_need = std::min( power_need, mStamina + PlayerParam::instance().HeteroPlayer(mPlayerType).extraStamina() );
	mStamina = std::max( 0.0, mStamina - power_need );

	power = ( back_dash
			? power_need / -2.0
					: power_need );


	double acc = std::fabs(mEffort * power * dir_rate * PlayerParam::instance().HeteroPlayer(mPlayerType).dashPowerRate());

	if ( back_dash ) {
		dir += 180.0;
	}

	mVel += Polar2Vector(acc, GetNormalizeAngleDeg(mBodyDir + dir));
	Step();
}

/**
 * @brief 执行原子动作
 *
 * 支持 Turn、Dash、Kick 的模拟；Kick 在此仅执行 Step（无球模型）。
 *
 * @param act 原子动作
 */
/**
 * @brief 执行原子动作
 *
 * 支持 Turn、Dash、Kick 的模拟；Kick 在此仅执行 Step（无球模型）。
 *
 * @param act 原子动作
 */
void Simulator::Player::Act(const AtomicAction & act)
{
	switch (act.mType) {
	case CT_Turn: Turn(GetTurnMoment(act.mTurnAngle, mPlayerType, mVel.Mod())); break;
	case CT_Dash: Dash(act.mDashPower, Dasher::GetDashDirIdx(act.mDashDir)); break;
	case CT_Kick: Step(); break;
	case CT_None: break;
	default: Assert(0); break;
	}
}
