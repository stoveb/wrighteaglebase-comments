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
 * @file Strategy.cpp
 * @brief 策略系统实现 - WrightEagleBase 的战术决策核心
 * 
 * 本文件实现了 Strategy 类，它是 WrightEagleBase 系统的战术决策核心：
 * 1. 球权分析：分析球的控制状态和归属
 * 2. 战术分析：分析当前比赛局势和战术需求
 * 3. 点球分析：处理点球相关的战术决策
 * 4. 位置计算：计算球员的最佳位置
 * 
 * 策略系统是 WrightEagleBase 的智能决策模块，
 * 负责分析比赛局势并制定相应的战术策略。
 * 
 * 主要功能：
 * - 球权分析：判断球的控制状态和归属
 * - 战术分析：分析进攻、防守、平衡等战术局势
 * - 点球处理：处理点球进攻和防守的战术安排
 * - 位置计算：计算球员在不同战术下的最佳位置
 * - 状态管理：管理战术状态和转换
 * 
 * 技术特点：
 * - 状态机：使用状态机管理战术状态
 * - 实时分析：每个周期实时分析战术局势
 * - 多维度：从多个维度分析比赛情况
 * - 灵活配置：支持不同的战术配置和调整
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include <vector>
#include "Strategy.h"
#include "Formation.h"
#include "InfoState.h"
#include "PositionInfo.h"
#include "InterceptInfo.h"
#include "Tackler.h"
#include "Dasher.h"
#include "Logger.h"
#include "Utilities.h"
using namespace std;

//==============================================================================
/**
 * @brief Strategy 构造函数
 * 
 * 初始化策略系统，设置各种战术状态和参数。
 * 该构造函数会初始化球权分析、战术状态等相关变量。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 继承自 DecisionData 基类
 * @note 初始化为防守状态
 * @note 重置所有球权相关的状态变量
 */
Strategy::Strategy(Agent & agent):
	DecisionData(agent)
{
    // === 初始化战术状态 ===
    mSituation = ST_Defense;     // 默认为防守状态
    mLastController = 0;          // 上一个控球者

    // === 初始化球权状态 ===
    mIsBallActuralKickable = false;  // 球是否实际可踢
    mIsBallFree = false;            // 球是否自由
    mController = 0;                // 当前控球者
    mChallenger = 0;                // 当前挑战者

    // === 初始化战术限制 ===
    mForbiddenDribble = false;     // 是否禁止带球

    // === 初始化点球状态 ===
    mIsPenaltyFirstStep = false;   // 是否为点球第一步
    mPenaltyTaker = 0;             // 点球主罚者
    mPenaltySetupTime = 0;         // 点球设置时间
}

/**
 * @brief Strategy 析构函数
 * 
 * 清理策略系统资源。
 * 该析构函数会释放策略系统占用的资源。
 */
Strategy::~Strategy()
{
}

/**
 * @brief 判断是否为自己控球
 * 
 * 检查当前球的控制者是否为自己。
 * 
 * @return bool 如果是自己控球返回true，否则返回false
 * 
 * @note 用于判断自己是否应该执行控球相关的行为
 * @note 与智能体的球衣号码进行比较
 */
bool Strategy::IsMyControl() const
{
	return mController == mAgent.GetSelfUnum();
}

/**
 * @brief 判断是否为自己上次控球
 * 
 * 检查上一个球的控制者是否为自己。
 * 
 * @return bool 如果是自己上次控球返回true，否则返回false
 * 
 * @note 用于判断自己是否应该继续执行相关行为
 * @note 用于保持行为的连续性
 */
bool Strategy::IsLastMyControl() const
{
    return mLastController == mAgent.GetSelfUnum();
}

/**
 * @brief 判断是否有队友可踢球
 * 
 * 检查是否有队友处于可踢球状态。
 * 
 * @return bool 如果有队友可踢球返回true，否则返回false
 * 
 * @note 通过位置信息获取持球队友的球衣号码
 * @note 正数表示队友持球
 */
bool Strategy::IsTmKickable() const
{
	return (mInfoState.GetPositionInfo().GetTeammateWithBall() > 0);
}

/**
 * @brief 更新策略系统例程
 * 
 * 这是策略系统的主更新函数，负责：
 * 1. 分析当前战术局势
 * 2. 分析点球相关情况
 * 
 * 更新流程：
 * 1. 调用战术分析函数
 * 2. 调用点球分析函数
 * 
 * @note 每个周期都会调用此函数
 * @note 确保战术分析的实时性
 */
void Strategy::UpdateRoutine()
{
	StrategyAnalyze();  // 分析战术局势
	PenaltyAnalyze();   // 分析点球情况
}

/**
 * @brief 战术分析
 * 
 * 分析当前比赛的战术局势，更新球权状态和战术状态。
 * 这是策略系统的核心分析函数。
 * 
 * 分析流程：
 * 1. 保存上次球自由状态
 * 2. 记录上次控球者（如果球不自由）
 * 3. 重置球权状态
 * 4. 分析当前球权情况
 * 
 * @note 参考WE2008的实现方式
 * @note 每个周期都会重新分析球权状态
 * @note 为后续的战术决策提供基础数据
 */
void Strategy::StrategyAnalyze()
{
    // === 保存上次状态 ===
    mIsLastBallFree = mIsBallFree;  // 保存上次球自由状态
    
    // === 记录上次控球者 ===
    // 只有在球不自由时才记录上次控球者（参考WE2008）
    if (mIsBallFree == false)//记录上次不free时的状态，参考WE2008
    {
        mLastController = mController;   // 记录上次控球者
		mLastChallenger = mChallenger;    // 记录上次挑战者
    }

	// === 重置球权状态 ===
	mIsBallActuralKickable = false;  // 重置实际可踢状态
	mIsBallFree = true;               // 默认球是自由的
	mController = 0;                  // 重置控球者

	// 重置所有拦截周期相关变量
	mMyInterCycle = 150;
	mMinTmInterCycle = 150;
	mMinOppInterCycle = 150;
	mMinIntercCycle = 150;
	mSureTmInterCycle = 150;
	mSureOppInterCycle = 150;
	mSureInterCycle = 150;
	mFastestTm = 0;
	mFastestOpp = 0;
	mSureTm = 0;
	mSureOpp = 0;

	// 重置球位置和周期相关变量
	mBallInterPos = mWorldState.GetBall().GetPos();
	mBallFreeCycleLeft = 0;
	mBallOutCycle = 1000;

	// 再次重置控球者（确保一致性）
	mController = 0;
	mChallenger = 0;

	// 检查禁止带球状态
	if (mForbiddenDribble) {
		// 如果自己不可踢球，则取消禁止带球状态
		if (!mSelfState.IsKickable()) {
			mForbiddenDribble = false;
		}
	}

	// 执行球权分析和战术分析
	BallPossessionAnalyse();
	SituationAnalyse();

    // 回滚阵型状态，阵型将在需要时由BehaviorData更新
    mFormation.Rollback("Strategy");

	// 处理抢断结果
	if (mSelfState.GetTackleProb(false) < FLOAT_EPS) {
		// 抢断概率接近零，判断抢断是否失败
		if ( mIsBallFree) {
			// 球自由时，检查自己是否不是最快拦截者或不是控球者
			if (mMyTackleInfo.mMinCycle >= mSureInterCycle || (mController > 0 && mController != mSelfState.GetUnum())) {
				mMyTackleInfo.mRes = IR_Failure;
			}
		}
	}
	else {
		// 抢断成功
		mMyTackleInfo.mRes = IR_Success;
		mMyTackleInfo.mMinCycle = 0;
	}
}

/**
 * 球权分析
 */
void Strategy::BallPossessionAnalyse()
{
	// 获取离球最近的球员列表
	const std::vector<Unum> & p2b = mInfoState.GetPositionInfo().GetClosePlayerToBall();
	const PlayerState & self = mAgent.GetSelf();
	BallState & ball = mAgent.World().Ball();

	// 假设球是自由的，进行拦截计算
	mIsBallFree = true;
	
	// 计算球出场地的时间
	if (ServerParam::instance().pitchRectangul().IsWithin(ball.GetPos())){
		// 球在场地内，计算球出场地的时间
		Ray ballcourse(ball.GetPos(), ball.GetVel().Dir());
		Vector outpos;
		if (ServerParam::instance().pitchRectangul().Intersection(ballcourse, outpos)){
			// 计算球到边界的距离和所需周期
			double distance = outpos.Dist(ball.GetPos());
			mBallOutCycle = (int)ServerParam::instance().GetBallCycle(ball.GetVel().Mod(), distance);
		}
        else
        {
            // 可能没有交点，比如球位置的x坐标正好为52.5
            mBallOutCycle = 0;
        }
	}
	else {
		// 球已在场地外
		mBallOutCycle = 0;
	}
	// 默认自己为控球者
	mController = self.GetUnum();

	// 分析谁拿球
	// 这里对场上所有其他队员计算拦截时都考虑了cyc_delay,得到的拦截周期是最小拦截周期,
	// 认为自己该去拿球的情况是,最快的对手不比自己快太多,最快的队友是自己(暂时不考虑buf)

	// 获取拦截信息表
	const std::vector<OrderedIT> & OIT = mInfoState.GetInterceptInfo().GetOIT();
	std::vector<OrderedIT>::const_iterator it, itr_NULL = OIT.end(), pMyInfo = itr_NULL, pTmInfo = itr_NULL, pOppInfo = itr_NULL;

	// 初始化球自由周期
	mBallFreeCycleLeft = 150;

	// 遍历所有拦截信息，分析最快拦截者
	for (it = OIT.begin(); it != OIT.end(); ++it){
		if (it->mUnum < 0){
			// 对手球员的情况
			if (it->mUnum == -mWorldState.GetOpponentGoalieUnum()) continue; //这里认为对方守门员不会去抢球，否则截球里面就不去抢打身后的球了
			if (it->mpInterceptInfo->mMinCycle < mMinOppInterCycle){
				// 找到更快的对手拦截者
				if (it->mCycleDelay < 16){
					mMinOppInterCycle = it->mpInterceptInfo->mMinCycle;
					mBallFreeCycleLeft = Min(mBallFreeCycleLeft, int(mMinOppInterCycle + it->mCycleDelay * 0.5)); //mOppMinInterCycle + it->cd * 0.5是对截球周期的一个权衡的估计
					mFastestOpp = -it->mUnum;
				}
			}
			if(it->mpInterceptInfo->mMinCycle + it->mCycleDelay < mSureOppInterCycle){
				// 找到更确定的对手拦截者
				mSureOppInterCycle = int(it->mpInterceptInfo->mMinCycle + it->mCycleDelay);
				mSureOpp = -it->mUnum;
				pOppInfo = it;
				// 检查对手身体方向是否有效，如果无效则增加周期
				if (!mWorldState.GetOpponent(mSureOpp).IsBodyDirValid() && mWorldState.GetOpponent(mSureOpp).GetVel().Mod() < 0.26){ //无法估计身体方向
					mSureOppInterCycle += 1;
				}
			}
		}
		else {
			// 队友球员的情况
			if (it->mUnum == self.GetUnum()){
				// 自己的拦截信息
				mMyInterCycle = it->mpInterceptInfo->mMinCycle;
				mBallFreeCycleLeft = Min(mBallFreeCycleLeft, mMyInterCycle);
				pMyInfo = it;
			}
			else {
				// 其他队友的拦截信息
				if (it->mpInterceptInfo->mMinCycle < mMinTmInterCycle){
					// 找到更快的队友拦截者
					mMinTmInterCycle = it->mpInterceptInfo->mMinCycle;
					mBallFreeCycleLeft = Min(mBallFreeCycleLeft, int(mMinTmInterCycle + it->mCycleDelay * 0.5));
					mFastestTm = it->mUnum;
				}
				if(it->mpInterceptInfo->mMinCycle + it->mCycleDelay < mSureTmInterCycle){
					// 找到更确定的队友拦截者
					mSureTmInterCycle = int(it->mpInterceptInfo->mMinCycle + it->mCycleDelay);
					mSureTm = it->mUnum;
					pTmInfo = it;
					// 检查队友身体方向是否有效，如果无效则增加周期
					if (!mWorldState.GetTeammate(mSureTm).IsBodyDirValid() && mWorldState.GetTeammate(mSureTm).GetVel().Mod() < 0.26){ //无法估计身体方向
						mSureTmInterCycle += 1;
					}
				}
			}
		}
	}

	// 计算最确定和最快的拦截周期
	mSureInterCycle = Min(mSureOppInterCycle, mSureTmInterCycle);
	mSureInterCycle = Min(mSureInterCycle, mMyInterCycle);

	mMinIntercCycle = Min(mMinOppInterCycle, mMinTmInterCycle);
	mMinIntercycle = Min(mMyInterCycle, mMinIntercycle);

	// 如果没有找到确定的队友拦截者，使用自己作为默认值
	if (mSureTm == 0) {
		mSureTm = mSelfState.GetUnum();
		mSureTmInterCycle = mMyInterCycle;
		mMinTmInterCycle = mMyInterCycle;
		mFastestTm = mSureTm;
	}

	// 特殊情况：自己正在快速带球
	if (mAgent.IsLastActiveBehaviorInActOf(BT_Dribble) && mAgent.GetLastActiveBehaviorInAct()->mDetailType == BDT_Dribble_Fast &&
			( !mIsLastBallFree || mMyInterCycle < mSureInterCycle + 6 || mWorldState.CurrentTime() - mLastBallFreeTime < 8)){
		// 如果自己正在快速带球且满足条件，则认为自己是控球者
		mController = mSelfState.GetUnum();
		if (pMyInfo != itr_NULL){
			mBallInterPos = ball.GetPredictedPos(mMyInterCycle);
		}
	}
	// 一般情况：自己是最快截到球的人
	else if( /*mMyInterCycle < mBallOutCycle + 2 &&*/ mMyInterCycle <= mSureInterCycle ){ //自己是最快截到球的人
		mController = self.GetUnum();
		// 检查队友拦截信息是否有效（周期延迟小于3）
		if(pTmInfo != itr_NULL && pTmInfo->mCycleDelay < 3){//2004_10
			// 如果自己当前是控球者
			if (IsMyControl()) {
				// 检查队友是否比自己更快或对手更快
				if (mSureTmInterCycle <= mMyInterCycle && mMyInterCycle <= mSureOppInterCycle) {
					// 检查队友的位置信息是否有效
					if (mWorldState.GetTeammate(mSureTm).GetVelDelay() == 0 || (mWorldState.GetTeammate(mSureTm).GetPosDelay() == 0 && mInfoState.GetPositionInfo().GetPlayerDistToPlayer(mSureTm, mSelfState.GetUnum()) < ServerParam::instance().visibleDistance() - 0.5)) {
						// 计算队友在拦截周期的球位置
						Vector ball_int_pos = mBallState.GetPredictedPos(mSureTmInterCycle);
						Vector pos = mWorldState.GetTeammate(mSureTm).GetPos();
						// 检查队友是否在可踢球范围内
						if (pos.Dist(ball_int_pos) < mWorldState.GetTeammate(mSureTm).GetKickableArea() - Dasher::GETBALL_BUFFER) {
							// 队友在可踢球范围内，让队友控球
							mController = mSureTm;
						}
						else {
							// 检查队友的速度是否朝向球
							Vector vel = mWorldState.GetTeammate(mSureTm).GetVel();
							double speed = vel.Mod() * Cos(vel.Dir() - (ball_int_pos - pos).Dir());
							// 如果队友正在截球，让队友控球
							if (speed > mWorldState.GetTeammate(mSureTm).GetEffectiveSpeedMax() * mWorldState.GetTeammate(mSureTm).GetDecay() * 0.9) { //队友正在截球
								mController = mSureTm;
							}
						}
					}
				}
				else if (mSureTmInterCycle < mMyInterCycle && mMyInterCycle <= mMinOppInterCycle) {
					// 队友比自己更快且对手不比自己快，让队友控球
					mController = mSureTm;
				}
			}
			else if (mSureTmInterCycle < mMyInterCycle && mMyInterCycle <= mMinOppInterCycle) {
				// 队友比自己更快且对手不比自己快，让队友控球
				mController = mSureTm;
			}
		}

		// 设置球的拦截位置
		if (mController == mSelfState.GetUnum()) {
			if (pMyInfo != itr_NULL){
				mBallInterPos = ball.GetPredictedPos(mMyInterCycle);
			}
            else if (mWorldState.CurrentTime().T() > 0) {
				// 如果时间大于0但没有拦截信息，可能是bug
				PRINT_ERROR("bug here?");
			}
		}
	}

	// 检查当前可踢球的球员
	int kickable_player = mInfoState.GetPositionInfo().GetTeammateWithBall(); //这里没有考虑buffer
	if (kickable_player == 0){
		// 没有队友可踢球，检查对手
		kickable_player = -mInfoState.GetPositionInfo().GetOpponentWithBall(); //这里没有考虑buffer
	}

	// 如果有球员可踢球，则球不是自由的
	if (kickable_player != 0) {
		mController = kickable_player;
		mIsBallFree = false;
		mBallOutCycle = 1000;
	}

	// 下面判断可踢情况
	// 先判断能踢到球的队员
	// 过程: 看能踢到球的自己人有几个,多于一个按规则决定是谁踢,得到是否自己可以踢球,存为_pMem->ball_kickable
	// 规则: 球离谁基本战位点谁踢
    if (self.IsKickable()){
		// 自己可以踢球
		mController = self.GetUnum();
		mIsBallFree = false;
		// 重置所有拦截周期为0
		mSureInterCycle = 0;
		mMinIntercycle = 0;
		mMyInterCycle = 0;
		mMinTmInterCycle = 0;
		mSureTmInterCycle = 0;
		mBallInterPos = ball.GetPos();
		mBallFreeCycleLeft = 0;
		mIsBallActuralKickable = true;
		mChallenger = mInfoState.GetPositionInfo().GetOpponentWithBall();

		// 守门员特殊处理：检查是否有其他队友更适合踢球
		if (!mSelfState.IsGoalie()) {
			// 计算自己到基本战位点的距离平方
			double self_pt_dis = mAgent.GetFormation().GetTeammateFormationPoint(self.GetUnum(), ball.GetPos()).Dist2(ball.GetPos());

			// 遍历所有离球最近的队友
			for(unsigned int i = 0; i < p2b.size(); ++i){
				Unum unum = p2b[i];
				if(unum > 0 && unum != self.GetUnum()){
					// 检查队友是否可踢球
					if (mWorldState.GetPlayer(unum).IsKickable()){
						// 非play_on时守门员可踢的特殊处理
						if(mWorldState.GetPlayMode() != PM_Play_On && mWorldState.GetPlayer(unum).IsGoalie()/*&& unum == PlayerParam::instance().ourGoalieUnum()*/){
							// 非play_on时如果守门员可踢把自己强行设置成不可踢
							mAgent.Self().UpdateKickable(false);
							mController = unum;
							break;
						}
						// 计算队友到基本战位点的距离平方
						double tm_pt_dis = mAgent.GetFormation().GetTeammateFormationPoint(unum, ball.GetPos()).Dist2(ball.GetPos());
						// 如果队友距离球更近，则让队友踢球
						if(tm_pt_dis < self_pt_dis){
							mAgent.Self().UpdateKickable(false);
							mController = unum;
							break;
						}
					}
					else { //可以认为其他人踢不到了
						break; // 没有更近的队友，退出循环
					}
				}
			}
		}
	}
	else if (kickable_player != 0 && kickable_player != self.GetUnum()){ //自己踢不到球,但有人可以踢
		// 球不自由，有其他球员可以踢
		mIsBallFree = false;
		if (kickable_player > 0){ //自己人可踢
			mChallenger = mInfoState.GetPositionInfo().GetOpponentWithBall();
		}
	}

	// 最后分析，作为修正
	SetPlayAnalyse();
}

bool Strategy::SetPlayAnalyse()
{
	// 获取当前比赛模式
	PlayMode play_mode = mWorldState.GetPlayMode();
	
	// 开球前模式：球是自由的，离球最近的球员控球
	if (play_mode == PM_Before_Kick_Off){
		mIsBallFree = true;
		mController = mInfoState.GetPositionInfo().GetClosestPlayerToBall();
		return true;
	}
	else if (play_mode < PM_Our_Mode && play_mode > PM_Play_On){
		mIsBallFree = true;
		if (mBallState.GetVel().Mod() < 0.16) { //如果有球速，说明已经发球了
			mController = mInfoState.GetPositionInfo().GetClosestTeammateToBall();
		}
		return true;
	}
	else if (play_mode > PM_Opp_Mode){
		mIsBallFree = true;
		if (mBallState.GetVel().Mod() < 0.16) { //如果有球速，说明已经发球了
			mController = -mInfoState.GetPositionInfo().GetClosestOpponentToBall();
		}
		return true;
	}
	return false;
}

void Strategy::SituationAnalyse()
{
	if( mController < 0 && mWorldState.GetBall().GetPos().X() < -10){
		mSituation = ST_Defense;
	}
	else {
		if(!mIsBallFree){
			if(mController >= 0){
				if(mBallInterPos.X() < 32.0){
					if(mInfoState.GetPositionInfo().GetTeammateOffsideLine() > 40.0 && mAgent.GetFormation().GetTeammateRoleType(mController).mLineType == LT_Midfielder && mBallInterPos.X() > 25.0){
						mSituation = ST_Penalty_Attack;
					}
					else {
						mSituation = ST_Forward_Attack;
					}
				}
				else {
					mSituation=ST_Penalty_Attack;
				}
			}
			else {
				mSituation = ST_Defense;
			}
		}
		else{
			if(IsMyControl() || mSureTmInterCycle <= mSureOppInterCycle){
				if(mBallInterPos.X() < 32.0 && mController > 0) {
					if(mInfoState.GetPositionInfo().GetTeammateOffsideLine() > 40.0 && mAgent.GetFormation().GetTeammateRoleType(mController).mLineType == LT_Midfielder && mBallInterPos.X() > 25.0) {
						mSituation = ST_Penalty_Attack;
					}
					else {
						mSituation = ST_Forward_Attack;
					}
				}
				else {
					mSituation=ST_Penalty_Attack;
				}
			}
			else{
				mSituation = ST_Defense;
			}
		}
	}

	mFormation.Update(Formation::Offensive, "Strategy");
}

/**
 * Situation based strategy position
 * \param t unum of teammate.
 * \param ballpos
 * \param normal true normal situation, and position should be adjusted with offside line,
 *               false setplay, return SBSP directly.
 */
Vector Strategy::GetTeammateSBSPPosition(Unum t,const Vector& ballpos)
{
	Vector position;

	if (mController > 0 ||
        (mController == 0 && mBallInterPos.X() > 10.0))
    {
		position = mAgent.GetFormation().GetTeammateFormationPoint(t, mController, ballpos);
	}
    else
    {
        position = mAgent.GetFormation().GetTeammateFormationPoint(t);
	}

	double x = Min(position.X(),
			mInfoState.GetPositionInfo().GetTeammateOffsideLine() - PlayerParam::instance().AtPointBuffer());
	if (mAgent.GetFormation().GetTeammateRoleType(t).mLineType==LT_Defender){		//后卫不过中场，便于回防
		position.SetX(Min(0.0,x));
	}
	else if (mAgent.GetFormation().GetTeammateRoleType(t).mLineType== LT_Forward){		//前锋不回场，便于进攻…………
		position.SetX(Max( - 1.0,x));
	}
	else position.SetX(x);

	return position;
}


Vector Strategy::GetMyInterPos()
{
	return mBallState.GetPredictedPos(mMyInterCycle);
}

Vector Strategy::AdjustTargetForSetplay(Vector target)
{
	if (mWorldState.GetPlayMode() > PM_Opp_Mode) {
		while (target.Dist(mBallState.GetPos()) < ServerParam::instance().offsideKickMargin() + 0.5) {
			target.SetX(target.X() - 0.5);
		}

		if (mWorldState.GetPlayMode() == PM_Opp_Kick_Off) {
			target.SetX(Min(target.X(), -0.1));
		}
		else if (mWorldState.GetPlayMode() == PM_Opp_Offside_Kick) {
			target.SetX(Min(target.X(), mBallState.GetPos().X() - 0.5));
		}
		else if (mWorldState.GetPlayMode() == PM_Opp_Goal_Kick) {
			if (ServerParam::instance().oppPenaltyArea().IsWithin(target)) {
				if (mSelfState.GetPos().X() < ServerParam::instance().oppPenaltyArea().Left()) {
					if (mSelfState.GetPos().Y() < ServerParam::instance().oppPenaltyArea().Top()) {
						target = ServerParam::instance().oppPenaltyArea().TopLeftCorner();
					}
					else if (mSelfState.GetPos().Y() > ServerParam::instance().oppPenaltyArea().Bottom()) {
						target = ServerParam::instance().oppPenaltyArea().BottomLeftCorner();
					}
					else {
						target.SetX(Min(target.X(), ServerParam::instance().oppPenaltyArea().Left() - 0.5));
					}
				}
				else {
					if (mSelfState.GetPos().Y() < ServerParam::instance().oppPenaltyArea().Top()) {
						target.SetY(Min(target.Y(), ServerParam::instance().oppPenaltyArea().Top() - 0.5));
					}
					else if (mSelfState.GetPos().Y() > ServerParam::instance().oppPenaltyArea().Bottom()) {
						target.SetY(Max(target.Y(), ServerParam::instance().oppPenaltyArea().Bottom() + 0.5));
					}
					else {
						target = mSelfState.GetPos(); //do nothing
					}
				}
			}
		}
	}

	return target;
}

bool Strategy::IsMyPenaltyTaken() const
{
	return (mWorldState.GetPlayMode() == PM_Our_Penalty_Taken) &&
	       (mAgent.GetSelfUnum() == mPenaltyTaker);
}

bool Strategy::IsPenaltyPlayMode() const
{
    const PlayMode &play_mode = mWorldState.GetPlayMode();
    return (play_mode == PM_Penalty_On_Our_Field) || (play_mode == PM_Penalty_On_Opp_Field) ||
           (play_mode >= PM_Our_Penalty_Setup && play_mode <= PM_Our_Penalty_Miss) ||
           (play_mode >= PM_Opp_Penalty_Setup && play_mode <= PM_Opp_Penalty_Miss);
}

void Strategy::PenaltyAnalyze()
{
    if (IsPenaltyPlayMode() == false)
    {
        return;
    }

    if (mWorldState.GetPlayModeTime() == mWorldState.CurrentTime())
    {
    	static Unum penalty_taker_seq[] = {1, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
        const PlayMode &play_mode = mWorldState.GetPlayMode();
        if (play_mode == PM_Our_Penalty_Setup)
        {
            while (penalty_taker_seq[++mPenaltySetupTime%TEAMSIZE] == mWorldState.GetTeammateGoalieUnum());
            mPenaltyTaker = penalty_taker_seq[mPenaltySetupTime%TEAMSIZE];
        }
        else if (play_mode == PM_Opp_Penalty_Setup)
        {
            mPenaltyTaker = -1;
        }
    }
}

// end of file Strategyinfo.cpp
