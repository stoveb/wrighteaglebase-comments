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
 * @file Tackler.cpp
 * @brief 铲球系统（Tackler）实现
 *
 * Tackler 提供铲球动作的建模与决策支持，包括：
 * - 以 1° 为粒度预计算所有铲球角度对应的球速与方向；
 * - 根据目标方向反向求解最优铲球角度与球速；
 * - 判断能否通过铲球停球，并执行停球铲球；
 * - 判断铲球是否危险（可能犯规）。
 *
 * 设计要点：
 * - 使用单例并缓存计算结果（按 AgentID 去重），避免每周期重复计算；
 * - 通过 mDirMap 将“球速方向”映射到“铲球角度区间”，便于快速查询；
 * - 在查询时对相邻区间做线性插值，提高精度。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "Tackler.h"
#include "WorldState.h"

/**
 * @brief 构造函数（私有）
 */
Tackler::Tackler()
{
}

/**
 * @brief 析构函数
 */
Tackler::~Tackler()
{
}

/**
 * @brief 获取全局单例
 *
 * @return Tackler& 全局铲球系统实例
 */
Tackler & Tackler::instance()
{
    static Tackler tackler;
    return tackler;
}

/**
 * @brief 更新铲球相关数据（按 Agent 缓存）
 *
 * 以 1° 为粒度，预计算 [-180°, 180°] 范围内每个铲球角度对应的球速与方向，
 * 并建立方向到角度的映射（mDirMap），用于后续快速查询。
 *
 * @param agent 当前智能体（用于获取自身与球状态）
 */
void Tackler::UpdateTackleData(const Agent & agent)
{
    if (mAgentID == agent.GetAgentID())  {
        return; /** no need to update */
    }

    mAgentID = agent.GetAgentID();

    const BallState & ball_state     = agent.GetWorldState().GetBall();
    const PlayerState & player_state = agent.GetSelf();
    Vector ball_2_player = (ball_state.GetPos() - player_state.GetPos()).Rotate(-player_state.GetBodyDir());

    Vector ball_vel;
    mMaxTackleSpeed = -1.0;
    mDirMap.clear();

    const double max_tackle_power = ServerParam::instance().maxTacklePower();
    const double min_back_tackle_power = ServerParam::instance().maxBackTacklePower();

    double factor = 1.0 - 0.5 * (fabs(Deg2Rad(ball_2_player.Dir())) / M_PI);

    for (AngleDeg tackle_angle = -180.0 + FLOAT_EPS; tackle_angle <= 180.0 + FLOAT_EPS; tackle_angle += 1.0) {
    	Vector ball_vel(ball_state.GetVel());
        double eff_power = (min_back_tackle_power + ((max_tackle_power - min_back_tackle_power) * (1.0 - fabs(Deg2Rad(tackle_angle)) / M_PI))) * ServerParam::instance().tacklePowerRate();
        eff_power *= factor;
        ball_vel += Polar2Vector(eff_power, tackle_angle + player_state.GetBodyDir());
        ball_vel = ball_vel.SetLength(Min(ball_vel.Mod(), ServerParam::instance().ballSpeedMax()));

        int angle_idx = ang2idx(tackle_angle);
        int dir_idx = dir2idx(ball_vel.Dir());

        mTackleAngle[angle_idx] = tackle_angle;
        mBallVelAfterTackle[angle_idx] = ball_vel;
        mDirMap[dir_idx].push_back(std::make_pair(angle_idx, ang2idx(tackle_angle + 1.0)));

        if (ball_vel.Mod() > mMaxTackleSpeed){
            mMaxTackleSpeed = ball_vel.Mod();
        }

        if (ball_vel.Mod() * ServerParam::instance().ballDecay() < FLOAT_EPS) {
        	mCanTackleStopBall = true;
        	mTackleStopBallAngle = tackle_angle;
        }
    }
//
//
//    for (AngleDeg tackle_angle = -180.0 + 0.3; tackle_angle <= 180.0 + FLOAT_EPS; tackle_angle += 1.0) {
//    	Vector ball_vel = GetBallVelAfterTackle(agent, tackle_angle);
//    	AngleDeg tackle_angle2 = 0.0;
//    	GetTackleAngleToDir(agent, ball_vel.Dir(), & tackle_angle2);
//
//    	std::cout << tackle_angle << " " << tackle_angle2 << std::endl;
//    }
//
//    exit(0);
}

/**
 * @brief 获取指定铲球角度对应的球速
 *
 * 对预计算表做线性插值，返回连续的球速结果。
 *
 * @param agent 当前智能体
 * @param tackle_angle 铲球角度（相对于身体朝向）
 * @return Vector 铲球后的球速矢量
 */
Vector Tackler::GetBallVelAfterTackle(const Agent & agent, AngleDeg tackle_angle)
{
    UpdateTackleData(agent);

    tackle_angle = GetNormalizeAngleDeg(tackle_angle, 0.0);
    int idx1 = ang2idx(tackle_angle); //下界
    int idx2 = ang2idx(tackle_angle + 1.0); //上界
    double rate = MinMax(0.0, idx2 - tackle_angle, 1.0);

    return mBallVelAfterTackle[idx1] * rate + mBallVelAfterTackle[idx2] * (1.0 - rate);
}

/**
 * @brief 获取将球铲到指定方向所需的信息
 *
 * 包括铲球角度与球速。
 *
 * @param agent 当前智能体
 * @param dir 目标方向（全局坐标系）
 * @param p_tackle_angle 输出：所需铲球角度
 * @param p_ball_vel 输出：所需球速
 * @return bool 是否可行
 */
bool Tackler::GetTackleInfoToDir(const Agent & agent, AngleDeg dir, AngleDeg *p_tackle_angle, Vector *p_ball_vel)
{
	UpdateTackleData(agent);

	Array<int, 3> dir_idx;

	dir_idx[0] = dir2idx(dir); //当前区间
	dir_idx[1] = dir2idx(dir - 1.0); //左边相邻区间
	dir_idx[2] = dir2idx(dir + 1.0); //右边相邻区间

	Vector optimal_ball_vel;
	AngleDeg best_tackle_angle = 0.0;
	double max_ball_speed = -1.0;
	bool ret = false;

	for (int j = 0; j < 3; ++j) {
		for (uint i = 0; i < mDirMap[dir_idx[j]].size(); ++i) {
			const int angle_idx1 = mDirMap[dir_idx[j]][i].first;
			const int angle_idx2 = mDirMap[dir_idx[j]][i].second;

			AngleDeg dir1 = mBallVelAfterTackle[angle_idx1].Dir();
			AngleDeg dir2 = mBallVelAfterTackle[angle_idx2].Dir();

			if (IsAngleDegInBetween(dir1, dir, dir2)) {
				ret = true;

				if (p_tackle_angle || p_ball_vel) { //这里要找出使出球速度最大的tackle_angle
					dir2 = GetNormalizeAngleDeg(dir2, dir1);
					dir = GetNormalizeAngleDeg(dir, dir1);

					double rate = (dir2 - dir) / (dir2 - dir1);
					Vector ball_vel = mBallVelAfterTackle[angle_idx1] * rate + mBallVelAfterTackle[angle_idx2] * (1.0 - rate);
					double ball_speed = ball_vel.Mod();

					if (ball_speed > max_ball_speed) {
						optimal_ball_vel = ball_vel;
						max_ball_speed = ball_speed;
						best_tackle_angle = mTackleAngle[angle_idx1] * rate + mTackleAngle[angle_idx2] * (1.0 - rate);
					}

					continue;
				}

				return true;
			}
		}
	}

	if (ret) {
		if (p_ball_vel) {
			*p_ball_vel = optimal_ball_vel;
		}

		if (p_tackle_angle) {
			*p_tackle_angle = best_tackle_angle;
		}
	}

	return ret;
}

/**
 * @brief 判断能否将球铲到指定方向（仅返回布尔值）
 *
 * @param agent 当前智能体
 * @param dir 目标方向（全局坐标系）
 * @return bool 是否可行
 */
bool Tackler::CanTackleToDir(const Agent & agent, const AngleDeg & dir)
{
	return GetTackleInfoToDir(agent, dir, 0, 0);
}

/**
 * @brief 获取将球铲到指定方向所需的铲球角度
 *
 * @param agent 当前智能体
 * @param dir 目标方向（全局坐标系）
 * @param tackle_angle 输出：所需铲球角度
 * @return bool 是否可行
 */
bool Tackler::GetTackleAngleToDir(const Agent & agent, const AngleDeg & dir, AngleDeg *tackle_angle)
{
	return GetTackleInfoToDir(agent, dir, tackle_angle, 0);
}

/**
 * @brief 获取当前铲球能产生的最大球速
 *
 * @param agent 当前智能体
 * @return double 最大球速
 */
double Tackler::GetMaxTackleSpeed(const Agent & agent)
{
    UpdateTackleData(agent);

    return mMaxTackleSpeed;
}

/**
 * @brief 执行铲球，使球朝指定方向飞出
 *
 * 若可行，计算铲球角度并调用 Agent::Tackle。
 *
 * @param agent 当前智能体
 * @param dir 目标方向（全局坐标系）
 * @param foul 是否为犯规铲球
 * @return bool 是否成功执行铲球
 */
bool Tackler::TackleToDir(Agent & agent, const AngleDeg & dir, bool foul)
{
    AngleDeg tackle_angle;
    if (GetTackleAngleToDir(agent, dir, &tackle_angle)){
        agent.Tackle(tackle_angle, foul);
        return true;
    }
    return false;
}

/**
 * @brief 判断能否通过铲球将球停住
 *
 * @param agent 当前智能体
 * @return bool 是否可行
 */
bool Tackler::CanTackleStopBall(Agent & agent)
{
	UpdateTackleData(agent);

	return mCanTackleStopBall;
}

/**
 * @brief 执行停球铲球
 *
 * @param agent 当前智能体
 * @return bool 是否成功执行
 */
bool Tackler::TackleStopBall(Agent & agent)
{
	UpdateTackleData(agent);

	return mCanTackleStopBall && TackleToDir(agent, mTackleStopBallAngle);
}

}

/**
 * @brief 判断铲球是否危险（可能犯规）
 *
 * 根据对手位置、身体朝向、可踢状态等判断铲球是否可能犯规。
 *
 * @param tackler 铲球者状态
 * @param world_state 世界状态
 * @return bool 是否危险
 */
bool Tackler::MayDangerousIfTackle(const PlayerState & tackler, const WorldState & world_state)
{
	if (tackler.GetTackleProb(false) < FLOAT_EPS && tackler.GetTackleProb(true) < FLOAT_EPS) { //不可铲
		return false;
	}

	const Vector tackler_pos = tackler.GetPos();
	const Vector ball_pos = world_state.GetBall().GetPos();
	const Vector ball_2_tackler = ball_pos - tackler_pos;
	const double ball_dist2 = ball_2_tackler.Mod2();
	const AngleDeg ball_dir = ball_2_tackler.Dir();

	for (unsigned i = 0; i < world_state.GetPlayerList().size(); ++i) {
		const PlayerState & opp = *world_state.GetPlayerList()[i];

		if (!opp.IsAlive()) continue; //dead
		if (opp.GetUnum() * tackler.GetUnum() > 0) continue; //not opp
		if (opp.IsIdling()) continue; //idling
		if (!opp.IsKickable()) continue; //not kickable

		//cond. 1. opp no dashing -- 无从知晓，这里认为对手一定会dash
		//cond. 2. ball near -- 球离自己更近，不构成危险情况
		Vector opp_2_tackler = opp.GetPos() - tackler_pos;
		if (opp_2_tackler.Mod2() > ball_dist2) continue;
		//cond. 3. behind or big y_diff -- 对手在自己与球连线方向上在自己身后或错开位置太大，不构成危险情况
		opp_2_tackler = opp_2_tackler.Rotate(-ball_dir);
		if (opp_2_tackler.X() < 0.0 || std::fabs( opp_2_tackler.Y() ) > opp.GetPlayerSize() + tackler.GetPlayerSize()) continue;
		//cond. 4. over body_diff -- 对手身体角度跟自己与球连线方向的夹角大于90度，不构成危险情况
		AngleDeg body_diff = std::fabs( GetNormalizeAngleDeg( opp.GetBodyDir() - ball_dir ) );
		if (body_diff > 90.0) continue;

		return true;
	}

	return false;
}
