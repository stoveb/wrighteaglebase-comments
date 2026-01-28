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
 * @file PlayerState.cpp
 * @brief 球员状态（PlayerState）实现
 *
 * PlayerState 继承自 MobileState，用于描述场上球员的完整状态，包括：
 * - 位置/速度/身体朝向/脖子朝向等运动学信息（继承自 MobileState）；
 * - 体力/effort/recovery 等身体状态；
 * - 可踢球/可扑球/铲球/碰撞等动作能力状态；
 * - 手臂指向/注视/视角/黄红牌等辅助信息；
 * - 球员类型/号码/守门员标识等身份信息。
 *
 * 本文件实现了构造函数、可踢球判定、镜像状态生成、带球预测、体力修正、
 * 转向角度计算、踢球随机角、控球概率、扑球概率等功能。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "PlayerState.h"
#include "ActionEffector.h"
#include "Simulator.h"
#include "Dasher.h"

/**
 * @brief 构造函数
 *
 * 使用 ServerParam 与 PlayerParam 的默认值初始化所有状态字段。
 * - 继承 MobileState 的初始化（衰减/最大速度）；
 * - 球员能力、体力、动作禁期、卡片等均设为默认值。
 */
PlayerState::PlayerState():
	MobileState(ServerParam::instance().playerDecay(), PlayerParam::instance().HeteroPlayer(0).effectiveSpeedMax())
{
	mBallCatchable = false;
	mCatchBan = 0;
	mCollideWithBall = false;
	mCollideWithPlayer = false;
	mCollideWithPost = false;
	mIsAlive = false;
	mIsGoalie = false;
    mIsSensed = false;
	mIsKicked = false;
	mIsPointing = false;
	mKickRate = 0.0;
	mIsKickable = false;
	mMaxTurnAngle = 0.0;
	mPlayerType = 0;

	mStamina = ServerParam::instance().staminaMax();
	mEffort  = ServerParam::instance().effortInit();
	mCapacity = ServerParam::instance().staminaCapacity();
	mRecovery = ServerParam::instance().recoverInit();

	mTackleBan = 0;
	mUnderDangerousTackleCondition = false;

	mFoulChargedCycle = 0;

	mUnum = 0;

	mViewWidth = VW_Normal;
	mIsTired = false;
    mMinStamina = PlayerParam::instance().MinStamina();

	mCardType = CR_None;
	mIsBodyDirMayChanged = true;
}

/**
 * @brief 判断是否可踢球
 *
 * 考虑位置置信度，使用可踢球区域减去 buffer 作为判定阈值。
 *
 * @param ball_state 球状态
 * @param buffer 缓冲区（通常用于传球/接应等宽松判定）
 * @return bool 是否可踢
 */
bool PlayerState::IsKickable(const BallState & ball_state, const double & buffer) const
{
	if (ball_state.GetPosConf() < FLOAT_EPS || GetPosConf() < FLOAT_EPS) return false;
	double dis2self = GetPos().Dist(ball_state.GetPos());
	return (dis2self <= GetKickableArea() - buffer);
}

/**
 * @brief 从对方视角生成镜像状态
 *
 * 将对方球员的状态映射为“我方视角”，用于对称分析或对手模拟。
 * - 位置/速度/身体/脖子/手臂/注视等方向性信息做 180° 旋转；
 * - 球员能力/体力/动作禁期等标量直接复制。
 *
 * @param o 对方球员状态
 */
void PlayerState::GetReverseFrom(const PlayerState & o)
{
	SetIsAlive(o.IsAlive());

	UpdateIsGoalie(o.IsGoalie());
	UpdateIsSensed(o.IsSensed());

	UpdatePlayerType(o.GetPlayerType());
	UpdateViewWidth(o.GetViewWidth());

	UpdateBallCatchable(o.IsBallCatchable());
	UpdateCatchBan(o.GetCatchBan());

	UpdateKickable(o.IsKickable());
	UpdateKicked(o.IsKicked());
	UpdateCardType(o.GetCardType());

	UpdateStamina(o.GetStamina());
	UpdateEffort(o.GetEffort());
	UpdateCapacity(o.GetCapacity());
	UpdateRecovery(o.GetRecovery());

	UpdateMaxTurnAngle(o.GetMaxTurnAngle());

	UpdateBodyDir(GetNormalizeAngleDeg(o.GetBodyDir() + 180.0), o.GetBodyDirDelay(), o.GetBodyDirConf());
	UpdateNeckDir(GetNormalizeAngleDeg(o.GetNeckDir() + 180.0), o.GetNeckDirDelay(), o.GetNeckDirConf());

	UpdatePos(o.GetPos().Rotate(180.0), o.GetPosDelay(), o.GetPosConf());
	UpdateVel(o.GetVel().Rotate(180.0), o.GetVelDelay(), o.GetVelConf());

	UpdateTackleBan(o.GetTackleBan());
	UpdateTackleProb(o.GetTackleProb(false), false);
	UpdateTackleProb(o.GetTackleProb(true), true);
	UpdateDangerousTackleCondition(o.UnderDangerousTackleCondition());

	UpdateFoulChargedCycle(o.GetFoulChargedCycle());

	UpdateArmPoint(GetNormalizeAngleDeg(o.GetArmPointDir() + 180.0), o.GetArmPointDelay(), o.GetArmPointConf(), o.GetArmPointDist(), o.GetArmPointMovableBan(), o.GetArmPointExpireBan());
	UpdateFocusOn(o.GetFocusOnSide(), o.GetFocusOnUnum(), o.GetFocusOnDelay(), o.GetFocusOnConf());
}

/**
 * @brief 预测连续 dash 后的位置
 *
 * 使用 Simulator::Player 模拟指定步数的 dash 动作，返回最终位置。
 *
 * @param steps 步数
 * @param dash_power dash 力度
 * @param dash_dir dash 方向
 * @return Vector 预测位置
 */
Vector PlayerState::GetPredictedPosWithDash(int steps, double dash_power, AngleDeg dash_dir) const
{
	Simulator::Player self(*this);
	int dash_dir_idx = Dasher::GetDashDirIdx(dash_dir);

	for (int i = 0; i < steps; ++i) {
		self.Dash(dash_power, dash_dir_idx);
	}

	return self.mPos;
}

/**
 * @brief 预测连续 dash 后的速度
 *
 * 使用 Simulator::Player 模拟指定步数的 dash 动作，返回最终速度。
 *
 * @param steps 步数
 * @param dash_power dash 力度
 * @param dash_dir dash 方向
 * @return Vector 预测速度
 */
Vector PlayerState::GetPredictedVelWithDash(int steps, double dash_power, AngleDeg dash_dir) const
{
	Simulator::Player self(*this);
	int dash_dir_idx = Dasher::GetDashDirIdx(dash_dir);

	for (int i = 0; i < steps; ++i) {
		self.Dash(dash_power, dash_dir_idx);
	}

	return self.mVel;
}

/**
 * @brief 根据体力修正 dash 力度
 *
 * 保证 dash 后体力不低于 MinStamina，正/反向 dash 分别处理。
 *
 * @param dash_power 原始 dash 力度
 * @return double 修正后的 dash 力度
 */
double PlayerState::CorrectDashPowerForStamina(double dash_power) const
{
	double stamina = GetStamina();

	if (IsOutOfStamina()) {
		return dash_power; //已经没有容量了，不需要在控制了
	}

	double new_power;

	if (dash_power >= 0) {
		new_power = Min( dash_power, stamina - PlayerParam::instance().MinStamina() );
		if ( new_power < 0 ) new_power = 0;
	}
	else {
		new_power = Min( -dash_power, (stamina - PlayerParam::instance().MinStamina()) / 2.0);
		if ( new_power < 0 ) new_power = 0;

		new_power = -new_power;
	}

	return new_power;
}

/**
 * @brief 获取有效转向角度
 *
 * 根据球员类型与速度计算实际可转动的角度。
 *
 * @param moment 期望转动的力矩
 * @param my_speed 当前速度
 * @return AngleDeg 实际可转动角度
 */
AngleDeg PlayerState::GetEffectiveTurn(AngleDeg moment, double my_speed) const
{
	return GetTurnAngle(moment, GetPlayerType(), my_speed);
}

/**
 * @brief 计算踢球随机角度
 *
 * 根据踢球力度、球速、距离等计算踢球方向的最大随机偏差（角度制）。
 *
 * @param power 踢球力度
 * @param vel 球速
 * @param bs 球状态
 * @return AngleDeg 随机角度（度）
 */
AngleDeg PlayerState::GetRandAngle( const double & power ,const  double & vel , const BallState & bs ) const
{

    double dir_diff = fabs((bs.GetPos()-GetPos()).Dir() - GetBodyDir());
    Vector tmp = bs.GetPos() - GetPos();
    double dist_ball = ( tmp.Mod() - GetPlayerSize() -
                         - ServerParam::instance().ballSize());

	double pos_rate
	            = 0.5
	            + 0.25 * ( dir_diff/180 + dist_ball/GetKickableMargin());
	        // [0.5, 1.0]
	        double speed_rate
	            = 0.5
	            + 0.5 * ( bs.GetVel().Mod()
	                      / ( ServerParam::instance().ballSpeedMax()
	                          * ServerParam::instance().ballDecay() ) );
	        // [0, 2*kick_rand]
	        double max_rand
	            = GetKickRand()
	            * ( power / ServerParam::instance().maxPower() )
	            * ( pos_rate + speed_rate );

	  return Rad2Deg(max_rand/vel);
}

/**
 * @brief 计算控球概率
 *
 * 若已在可踢范围内返回 1.0；
 * 守门员综合扑球概率与铲球概率，普通球员仅考虑铲球概率。
 *
 * @param ball_pos 球位置
 * @return double 控球概率 [0,1]
 */
double PlayerState::GetControlBallProb(const Vector & ball_pos) const
{
	const double dist = GetPos().Dist(ball_pos);

	if (dist < GetKickableArea()) {
		return 1.0; //已经可踢了
	}

	if (IsGoalie()) {
		return Max(GetCatchProb(dist), ::GetTackleProb(ball_pos, GetPos(), GetBodyDir(), false));
	}
	else {
		return ::GetTackleProb(ball_pos, GetPos(), GetBodyDir(), false);
	}
}

/**
 * @brief 计算守门员扑球概率
 *
 * 根据距离与扑球区域参数计算扑球成功概率。
 *
 * @param dist 球到守门员的距离
 * @return double 扑球概率 [0,1]
 */
double PlayerState::GetCatchProb( const double & dist ) const
{
	if ( dist < GetMinCatchArea() ) {
		return ServerParam::instance().catchProb();
	}
	else {
		double min_length = ServerParam::instance().catchAreaLength() * ( 2.0 - GetCatchAreaLStretch() );
		double max_length = ServerParam::instance().catchAreaLength() * GetCatchAreaLStretch();
		if ( max_length < min_length + FLOAT_EPS ) return 0.0;
		double delt = Sqrt( dist * dist - ServerParam::instance().catchAreaWidth() * ServerParam::instance().catchAreaWidth() / 4 );
		if ( delt > max_length ) return 0.0;
		double dx = delt - min_length;
		return MinMax( 0.0, ServerParam::instance().catchProb() - ServerParam::instance().catchProb() * dx / ( max_length - min_length ), ServerParam::instance().catchProb());
	}
}

//end of file pLayerState.cpp
