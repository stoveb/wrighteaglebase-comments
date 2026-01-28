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
 * @file VisualSystem.cpp
 * @brief 视觉系统实现 - WrightEagleBase 的智能视觉管理
 * 
 * 本文件实现了 VisualSystem 类，它是 WrightEagleBase 系统的智能视觉管理器：
 * 1. 管理视觉请求的生成和优化
 * 2. 根据战术需求调整视觉参数
 * 3. 实现智能的视觉搜索策略
 * 4. 优化视觉信息的使用效率
 * 
 * VisualSystem 是连接决策系统和视觉感知的重要桥梁，
 * 负责将战术需求转换为具体的视觉请求。
 * 
 * 主要功能：
 * - 视觉请求管理：根据战术需要生成视觉请求
 * - 视角优化：动态调整视角宽度和方向
 * - 搜索策略：实现智能的视觉搜索算法
 * - 优先级管理：管理视觉对象的优先级
 * - 性能优化：在信息质量和性能之间平衡
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "VisualSystem.h"
#include "Strategy.h"
#include "Formation.h"
#include "TimeTest.h"
#include "PositionInfo.h"
#include "InterceptInfo.h"
#include "InterceptModel.h"
#include "Agent.h"
#include "BehaviorShoot.h"
#include "Logger.h"

/**
 * @brief VisualSystem 构造函数
 * 
 * 初始化视觉系统对象，设置所有视觉参数的初始状态。
 * VisualSystem 是单例模式，负责管理整个团队的视觉策略。
 * 
 * 初始化内容：
 * 1. 设置视觉控制标志位
 * 2. 初始化视觉搜索状态
 * 3. 重置视觉请求对象
 * 4. 准备视觉优化参数
 */
VisualSystem::VisualSystem()
{
	// === 视觉控制标志初始化 ===
	mCanForceChangeViewWidth = false;  // 是否可以强制改变视角宽度
	mIsSearching = false;              // 是否处于搜索状态
	mIsCritical = false;                // 是否处于关键状态
	mForbidden = false;                // 是否禁止视觉调整
	
	// === 强制观察对象初始化 ===
	mForceToSeeObject.bzero();          // 强制观察的对象位图
}

/**
 * @brief VisualSystem 析构函数
 * 
 * 清理视觉系统资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
VisualSystem::~VisualSystem() {
	// 当前为空实现
}

/**
 * @brief 获取 VisualSystem 单例实例
 * 
 * 返回 VisualSystem 的单例实例，确保整个系统只有一个视觉管理器。
 * 
 * @return VisualSystem& 视觉系统的引用
 * 
 * @note 采用单例模式确保视觉策略的全局一致性
 */
VisualSystem & VisualSystem::instance()
{
	static VisualSystem info_system;
	return info_system;
}

/**
 * @brief 初始化视觉系统
 * 
 * 设置视觉系统与智能体的关联，初始化所有视觉请求对象。
 * 这个函数必须在每个智能体创建后调用。
 * 
 * 初始化过程：
 * 1. 设置智能体和状态对象的引用
 * 2. 初始化球的视觉请求
 * 3. 初始化所有球员的视觉请求
 * 4. 设置视觉请求的优先级
 * 
 * @param agent 关联的智能体指针
 * 
 * @note 每个智能体都有自己的视觉系统实例
 * @note 视觉请求对象与球员对象一一对应
 */
void VisualSystem::Initial(Agent * agent)
{
	// === 设置核心对象引用 ===
	mpAgent         = agent;                    // 智能体引用
	mpWorldState    = & (agent->World());        // 世界状态引用
	mpInfoState     = & (agent->Info());         // 信息状态引用
	mpBallState     = & (agent->World().Ball()); // 球状态引用
	mpSelfState     = & (agent->Self());         // 自身状态引用

	// === 初始化球的视觉请求 ===
	mVisualRequest.GetOfBall().mpObject = mpBallState;  // 球对象指针
	mVisualRequest.GetOfBall().mUnum = 0;               // 球的号码为0

	// === 初始化所有球员的视觉请求 ===
	for (Unum i = 1; i <= TEAMSIZE; ++i) {
		// 队友视觉请求
		mVisualRequest.GetOfTeammate(i).mpObject = & (mpWorldState->GetTeammate(i));
		mVisualRequest.GetOfTeammate(i).mUnum = i;
		
		// 对手视觉请求
		mVisualRequest.GetOfOpponent(i).mpObject = & (mpWorldState->GetOpponent(i));
		mVisualRequest.GetOfOpponent(i).mUnum = -i;
	}
}

/**
 * @brief 重置视觉请求
 * 
 * 重置所有视觉请求和相关状态，为新的决策周期做准备。
 * 这个函数在每个决策周期的开始时调用。
 * 
 * 重置过程：
 * 1. 检查是否有新的视觉信息
 * 2. 重置视觉控制标志位
 * 3. 清理所有视觉请求
 * 4. 重置优先级集合
 * 5. 处理特殊情况（位置不确定等）
 * 
 * @note 只有在有新视觉信息时才进行完整重置
 * @note 重置操作需要考虑性能影响
 */
void VisualSystem::ResetVisualRequest()
{
	// === 基本状态重置 ===
	mCanTurn = false;           // 是否可以转身
	mSenseBallCycle = 0;        // 感知球的周期数

	// === 检查是否有新视觉信息 ===
	if (mpAgent->IsNewSight()) { //有新视觉才重置，否则维持上周期的结果
		// === 视觉控制标志重置 ===
		mCanForceChangeViewWidth = false;  // 重置视角宽度控制
		mIsSearching = false;              // 重置搜索状态
		mIsCritical = false;                // 重置关键状态
		mForbidden = false;                // 重置禁止标志

		// === 清理所有视觉请求 ===
		// 遍历所有可能的球员号码（-11到11，0为球）
		for (Unum i = -TEAMSIZE; i <= TEAMSIZE; ++i){
			mVisualRequest[i].Clear();  // 清理第i个对象的视觉请求
		}

		// === 重置优先级集合 ===
		mHighPriorityPlayerSet.clear();  // 清空高优先级球员集合
		
		// === 重置强制观察对象 ===
		mForceToSeeObject.bzero();        // 清空强制观察对象位图
	}

	// === 特殊情况处理 ===
	// 如果自身位置不确定，需要特殊处理
	if (mpSelfState->GetPosConf() < FLOAT_EPS) {
		mIsCritical = true;
		mCanForceChangeViewWidth = false;
	}
	if (mpBallState->GetPosConf() < FLOAT_EPS) {
		RaiseForgotObject(0);
	}

	ViewModeDecision();
}

/**
 * @brief 视觉决策主函数
 * 
 * 这是视觉系统的主决策函数，负责：
 * 1. 检查是否可以执行视觉动作
 * 2. 处理定位球模式下的特殊视觉需求
 * 3. 执行常规视觉决策
 * 
 * 决策流程：
 * 1. 检查是否已有转脖子动作
 * 2. 检查是否被禁止视觉调整
 * 3. 处理定位球模式
 * 4. 执行常规决策
 * 
 * @note 如果已经有转脖子动作，则跳过视觉决策
 * @note 定位球模式有特殊的视觉需求
 */
void VisualSystem::Decision()
{
	// === 检查动作冲突 ===
	if (mpAgent->GetActionEffector().IsTurnNeck()) return; //其他地方已经产生了转脖子动作
	if (mForbidden) return;  // 如果被禁止则返回

	// === 处理定位球模式 ===
	if (!DealWithSetPlayMode()) {
		// === 执行常规视觉决策 ===
		DoDecision();
	}
}

/**
 * @brief 视角模式决策
 * 
 * 根据当前比赛情况智能选择视角宽度。
 * 这是视觉系统的核心优化函数。
 * 
 * 决策策略：
 * 1. 维持当前视角宽度
 * 2. 如果没有新视觉信息，再次设置视角
 * 3. 根据球距离选择最佳视角宽度
 * 4. 特殊模式下的视角处理
 * 
 * 视角选择逻辑：
 * - 球距离 > 60m：使用宽视角（VW_Wide）
 * - 球距离 > 40m：使用正常视角（VW_Normal）
 * - 球距离 <= 40m：使用窄视角（VW_Narrow）
 * - 开球前：使用窄视角
 * 
 * @note 视角宽度影响视觉信息的质量和更新频率
 * @note 需要在信息质量和性能之间平衡
 */
void VisualSystem::ViewModeDecision()
{
	// === 维持当前视角 ===
	ChangeViewWidth(mpSelfState->GetViewWidth());

	// === 检查视觉信息更新 ===
	if (mpAgent->IsNewSight() == false){
		// 如果没有新视觉信息，再次设置视角
		ChangeViewWidth(mpSelfState->GetViewWidth());
		return;
	}

	// === 根据比赛模式选择视角 ===
	if (mpWorldState->GetPlayMode() != PM_Before_Kick_Off){
		// === 根据球距离选择视角宽度 ===
		double balldis  = mpInfoState->GetPositionInfo().GetBallDistToTeammate(mpSelfState->GetUnum());
		if (balldis > 60.0){
			ChangeViewWidth(VW_Wide);      // 远距离使用宽视角
		}
		else if (balldis > 40.0){
			ChangeViewWidth(VW_Normal);    // 中距离使用正常视角
		}
		else {
			ChangeViewWidth(VW_Narrow);    // 近距离使用窄视角
		}
	}
	else{
		// === 开球前使用窄视角 ===
		ChangeViewWidth(VW_Narrow);
	}
}

/**
 * @brief 处理视觉请求
 * 
 * 处理所有视觉请求，生成最佳的视觉动作。
 * 这是视觉系统的执行函数。
 * 
 * 处理流程：
 * 1. 处理特殊对象的视觉请求
 * 2. 设置视觉搜索环
 * 3. 生成最佳视觉动作
 * 
 * @note 特殊对象包括球、关键球员等
 * @note 视觉搜索环用于优化视觉搜索效率
 */
void VisualSystem::DealVisualRequest()
{
	DealWithSpecialObjects();  // 处理特殊对象
	SetVisualRing();           // 设置视觉搜索环
	GetBestVisualAction();     // 获取最佳视觉动作
}

/**
 * @brief 评估视觉请求
 * 
 * 评估所有视觉请求的有效性和优先级。
 * 这是视觉系统的核心评估函数。
 * 
 * 评估内容：
 * 1. 评估球的视觉请求
 * 2. 评估球员的视觉请求
 * 3. 处理特殊情况（点球等）
 * 4. 更新高优先级集合
 * 
 * 评估标准：
 * - 位置置信度
 * - 信息延迟
 * - 战术重要性
 * - 视觉质量
 * 
 * @note 评估结果用于指导视觉决策
 * @note 特殊情况有特殊的评估逻辑
 */
void VisualSystem::EvaluateVisualRequest()
{
	// === 评估球的视觉请求 ===
	{
		VisualRequest *vr = &mVisualRequest.GetOfBall();
		vr->mValid = mpBallState->GetPosConf() > FLOAT_EPS;  // 检查位置置信度

		if (vr->mValid) {
			// === 计算相对位置和延迟 ===
			vr->mPrePos = mPreBallPos - mPreSelfPos;           // 相对位置
			vr->mCycleDelay = mpBallState->GetPosDelay();       // 信息延迟

			// === 更新评估结果 ===
			vr->UpdateEvaluation();

			// === 检查是否需要强制观察 ===
			if (vr->mConf < FLOAT_EPS){
				mForceToSeeObject.GetOfBall() = true;  // 强制观察球
			}
		}
	}

	// === 获取当前比赛模式 ===
	PlayMode play_mode = mpWorldState->GetPlayMode();

	// === 处理点球特殊情况 ===
	if (play_mode == PM_Our_Penalty_Ready || play_mode == PM_Our_Penalty_Taken) {
		// === 获取对方守门员 ===
		const int goalie = mpWorldState->GetOpponentGoalieUnum();
		if (!goalie) return;  // 如果没有守门员则返回

		// === 评估守门员的视觉请求 ===
		VisualRequest *vr = &mVisualRequest.GetOfOpponent(goalie);

		if (mpWorldState->GetOpponent(goalie).IsAlive()) {
			// === 计算守门员的预测位置 ===
			vr->mPrePos = mpWorldState->Opponent(goalie).GetPredictedPos(1) - mPreSelfPos;
			vr->mValid = true;
			vr->mCycleDelay = vr->mpObject->GetPosDelay();

			// === 更新评估结果 ===
			vr->UpdateEvaluation();

			// === 检查是否需要高优先级 ===
			if (vr->mConf < FLOAT_EPS) {
				mHighPriorityPlayerSet.insert(vr);  // 加入高优先级集合
			}
		}
		else {
			vr->mValid = false;  // 守门员不存在，标记为无效
		}
	}
	else {
		for (int i = -TEAMSIZE; i <= TEAMSIZE; ++i) {
			if (!i || i == mpSelfState->GetUnum()) continue;

			VisualRequest *vr = &mVisualRequest[i];
			vr->mValid = mpWorldState->GetPlayer(i).IsAlive();

			if (vr->mValid) {
				vr->mPrePos = mpWorldState->GetPlayer(i).GetPredictedPos() - mPreSelfPos;
				vr->mCycleDelay = vr->mpObject->GetPosDelay();

				vr->UpdateEvaluation();

				if (vr->mConf < FLOAT_EPS){
					mHighPriorityPlayerSet.insert(vr);
				}
			}
		}
	}
}

void VisualSystem::DoInfoGather()
{
	const Strategy & strategy = mpAgent->GetStrategy();

	if (!mCanTurn){
		if (mpAgent->GetActionEffector().IsMutex() == false && !mIsCritical){
			SetCanTurn(true);
		}
	}
	else {
		if (mpAgent->GetActionEffector().IsMutex() || mIsCritical){
			SetCanTurn(false);
		}
	}

	if (mpSelfState->IsIdling()) {
		SetCanTurn(false);
	}

	UpdatePredictInfo();

	if(mpWorldState->GetPlayMode() == PM_Our_Penalty_Ready || mpWorldState->GetPlayMode() == PM_Our_Penalty_Taken /*|| mpWorldState->GetPlayMode() == PM_Our_Penalty_Setup*/ ){
		RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 1.0);
		RaiseBall();
		return;
	}

	if (mpWorldState->GetPlayMode() > PM_Opp_Mode && mpInfoState->GetPositionInfo().GetClosestOpponentDistToBall() < 3.0 && mpBallState->GetPos().Dist(mpSelfState->GetPos()) < 20.0 && mpBallState->GetPosDelay() > 1){
		SetForceSeeBall(); //关注对手发球
	}

	if(mpSelfState->GetPos().X() - mpInfoState->GetPositionInfo().GetTeammateOffsideLine() > -5.0 && mpInfoState->GetPositionInfo().GetTeammateOffsideLineOpp() != Unum_Unknown){//offside look
		RaisePlayer(-mpInfoState->GetPositionInfo().GetTeammateOffsideLineOpp(), 2.0);
	}

	if (mpSelfState->IsGoalie()){
		if (strategy.IsMyControl() || mpWorldState->GetPlayMode() == PM_Our_Goal_Kick){
			DoInfoGatherForDefense();
		}
		else {
			DoInfoGatherForGoalie();
		}
	}
	else {
		if (strategy.IsBallFree()
				&& strategy.GetController() != 0
				&& mpAgent->GetFormation().GetTeammateRoleType(mpSelfState->GetUnum()).mLineType != LT_Defender
				&& (strategy.GetBallFreeCycleLeft() > 3 || (strategy.IsMyControl() && strategy.GetMyInterCycle() > 3)))
		{
			DoInfoGatherForBallFree();
		}
		else {
			switch(strategy.GetSituation()){
			case ST_Defense:
				DoInfoGatherForDefense();
				break;
			case ST_Forward_Attack:
				DoInfoGatherForFastForward();
				break;
			case ST_Penalty_Attack:
				DoInfoGatherForPenaltyAttack();
				break;
			}
		}
	}

	if (mpWorldState->GetPlayMode() != PM_Play_On) {
		DoInfoGatherForBallFree();
	}
}

void VisualSystem::DoInfoGatherForBallFree()
{
	const Strategy & strategy = mpAgent->GetStrategy();

	double ball_free_cyc_left; //ball free cycle left,这里是经过调整的有概率意义的freecyc,不同于StrategyInfo里的那个值
	const double rate = 0.6; //可以认为是调整值符合真实值的成功率
	double my_int_cycle, tm_int_cycle, opp_int_cycle;

	my_int_cycle = strategy.GetMyInterCycle();
	tm_int_cycle = strategy.GetMinTmInterCycle() * rate + strategy.GetSureTmInterCycle() * (1 - rate);
	opp_int_cycle = strategy.GetMinOppInterCycle() * rate + strategy.GetSureOppInterCycle() * (1 - rate);

	ball_free_cyc_left = Min(tm_int_cycle, opp_int_cycle);
	ball_free_cyc_left = Min(my_int_cycle, ball_free_cyc_left);

	if(opp_int_cycle < ball_free_cyc_left + 2 && ball_free_cyc_left > 3){
		ball_free_cyc_left -= 1;
	}

	std::vector<OrderedIT>::const_iterator it;
	const std::vector<OrderedIT> & OIT = mpInfoState->GetInterceptInfo().GetOIT();

	if (strategy.IsMyControl() && mpAgent->GetFormation().GetTeammateRoleType(mpSelfState->GetUnum()).mLineType != LT_Defender && ball_free_cyc_left < 6 && ball_free_cyc_left > 2){
		RaiseBall(1.0);
	}
	else{
		RaiseBall();
	}

	if(!strategy.IsMyControl()){//自己不用拿球
		int eva = 3;
		for (it = OIT.begin(); it != OIT.end(); ++it){
			if(it->mpInterceptInfo->mMinCycle > 50) break;
			RaisePlayer(it->mUnum, eva++);
		}
	}
	else {//自己需要拿球
		RaiseBall(2.0);
		int eva = 3;
		for (it = OIT.begin(); it != OIT.end(); ++it){
			if(it->mpInterceptInfo->mMinCycle > 50) break;
			if (it->mUnum == mpSelfState->GetUnum()) continue;
			RaisePlayer(it->mUnum, ++eva);
			if (mpWorldState->GetPlayer(it->mUnum).GetPosDelay() > it->mpInterceptInfo->mMinCycle) {
				mHighPriorityPlayerSet.insert(GetVisualRequest(it->mUnum));
			}
		}
	}
}

void VisualSystem::DoInfoGatherForFastForward()
{
	const Strategy & strategy = mpAgent->GetStrategy();

	RaiseBall();
	switch (mpAgent->GetFormation().GetTeammateRoleType(mpSelfState->GetUnum()).mLineType){
	case LT_Defender:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 8.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 5.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 5.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
		}
		break;
	case LT_Midfielder:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 100.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 5.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 2.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			if (mpWorldState->GetOpponent(i).GetPosConf() > FLOAT_EPS){
				if(mpWorldState->GetOpponent(i).GetPos().X() > mPreBallPos.X() - 8.0)
					RaisePlayer(-i, 4.0);
				else
					RaisePlayer(-i, 100.0);
			}
		}
		if (mpWorldState->GetOpponentGoalieUnum() && mpWorldState->GetOpponent(mpWorldState->GetOpponentGoalieUnum()).GetPosConf() > FLOAT_EPS){
			RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 20.0);
		}
		break;
	case LT_Forward:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 100.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 5.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 3.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			if (mpWorldState->GetOpponent(i).GetPosConf() > FLOAT_EPS){
				double opp_x = mpWorldState->GetOpponent(i).GetPos().X();
				if (!strategy.IsMyControl() && fabs(opp_x - mpInfoState->GetPositionInfo().GetTeammateOffsideLine()) < 1.0){
					RaisePlayer(-i, 2.6);
				}
				else if (!strategy.IsMyControl() && fabs(opp_x - mpInfoState->GetPositionInfo().GetTeammateOffsideLine()) < 3.6){
					RaisePlayer(-i, 3);
				}
				else if (opp_x > mPreBallPos.X() - 8.0){
					if (opp_x > mPreBallPos.X() + 36.0){
						RaisePlayer(-i, 6);
					}
					else{
						RaisePlayer(-i, 4);
					}
				}
				else{
					RaisePlayer(-i, 100);
				}
			}
		}
		if (mpWorldState->GetOpponentGoalieUnum() && mpWorldState->GetOpponent(mpWorldState->GetOpponentGoalieUnum()).GetPosConf() > FLOAT_EPS){
			RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 15.0);
		}
		break;
	default:
		PRINT_ERROR("line type error");
		break;
	}
}

void VisualSystem::DoInfoGatherForPenaltyAttack()
{
	const Strategy & strategy = mpAgent->GetStrategy();

	RaiseBall();
	switch (mpAgent->GetFormation().GetTeammateRoleType(mpSelfState->GetUnum()).mLineType){
	case LT_Defender:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				RaisePlayer(i);
			}
			RaisePlayer(-i);
		}
		break;
	case LT_Midfielder:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 100.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 5.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 5.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			if (mpWorldState->GetOpponent(i).GetPosConf() > FLOAT_EPS){
				if(mpWorldState->GetOpponent(i).GetPos().X() > mPreBallPos.X() - 8.0)
					RaisePlayer(-i, 3.0);
				else
					RaisePlayer(-i, 100.0);
			}
		}
		if (mpWorldState->GetOpponentGoalieUnum() && mpWorldState->GetOpponent(mpWorldState->GetOpponentGoalieUnum()).GetPosConf() > FLOAT_EPS){
			RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 20.0);
		}
		break;
	case LT_Forward:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 100.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 5.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 3.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			if (mpWorldState->GetOpponent(i).GetPosConf() > FLOAT_EPS){
				if(mpWorldState->GetOpponent(i).GetPos().X() > mPreBallPos.X() - 8.0)
					RaisePlayer(-i, 3.0);
				else
					RaisePlayer(-i, 100.0);
			}
		}
		break;
	default:
		PRINT_ERROR("line type error");
		break;
	}

	//特殊视觉请求
	if(ServerParam::instance().oppPenaltyArea().IsWithin(mPreSelfPos)
			&& (strategy.IsMyControl()
					|| (strategy.IsTmControl() && mpInfoState->GetPositionInfo().GetBallDistToTeammate(mpSelfState->GetUnum()) < 20.0))){
		if (mpWorldState->GetOpponentGoalieUnum() && mpWorldState->GetOpponent(mpWorldState->GetOpponentGoalieUnum()).GetPosConf() > FLOAT_EPS){
			if (strategy.IsMyControl()
					&& (mpSelfState->GetPos().X() > 38.0 || (mpInfoState->GetPositionInfo().GetBallDistToOpponent(mpWorldState->GetOpponentGoalieUnum()) < 8.0 && mpSelfState->GetPos().X() > 36.0)))
			{
				if (mpWorldState->GetPlayMode() == PM_Our_Back_Pass_Kick || mpWorldState->GetPlayMode() == PM_Our_Indirect_Free_Kick){
					RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 1.2);
				}
				else {
					RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 1.0);
				}
			}
			else {
				RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 2.0);
			}
		}
	}
}

void VisualSystem::DoInfoGatherForDefense()
{
	if(mVisualRequest.GetOfBall().mConf < FLOAT_EPS && !mCanTurn && !mIsCritical){
		mForceToSeeObject.GetOfBall() = true;
	}

	RaiseBall();
	switch (mpAgent->GetFormation().GetTeammateRoleType(mpSelfState->GetUnum()).mLineType){
	case LT_Goalie:
	case LT_Defender:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 5.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 10.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 100.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			RaisePlayer(-i);
		}
		break;
	case LT_Midfielder:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				RaisePlayer(i);
			}
			RaisePlayer(-i, 12.0);
		}
		if (mpWorldState->GetOpponentGoalieUnum() && mpWorldState->GetOpponent(mpWorldState->GetOpponentGoalieUnum()).GetPosConf() > FLOAT_EPS){
			RaisePlayer(-mpWorldState->GetOpponentGoalieUnum(), 20.0);
		}
		break;
	case LT_Forward:
		for (Unum i = 1; i <= TEAMSIZE; ++i){
			if (i != mpSelfState->GetUnum() && i != mpWorldState->GetTeammateGoalieUnum()){
				switch (mpAgent->GetFormation().GetTeammateRoleType(i).mLineType){
				case LT_Defender:
					RaisePlayer(i, 12.0);
					break;
				case LT_Midfielder:
					RaisePlayer(i, 8.0);
					break;
				case LT_Forward:
					RaisePlayer(i, 8.0);
					break;
				default:
					PRINT_ERROR("line type error");
					break;
				}
			}
			RaisePlayer(-i, 12.0);
		}
		break;
	default:
		PRINT_ERROR("line type error");
		break;
	}
}

void VisualSystem::DoInfoGatherForGoalie()
{
	const Strategy & strategy = mpAgent->GetStrategy();

	RaiseBall();
	for (Unum i = 1; i <= TEAMSIZE; i++){
		if (i != mpSelfState->GetUnum()){
			RaisePlayer(i, 50);
		}
		RaisePlayer(-i, 50);
	}

	//特殊视觉请求
	if(mpInfoState->GetPositionInfo().GetBallDistToTeammate(mpSelfState->GetUnum()) < 26.0){
		SetForceSeeBall();
		if (strategy.IsOppControl()){
			RaisePlayer(strategy.GetController(), 2.0);
		}
	}
}

void VisualSystem::DoVisualExecute()
{
	mBestVisualAction.mDir = GetNormalizeAngleDeg(mBestVisualAction.mDir);

	AngleDeg finalnec = GetNormalizeAngleDeg(mBestVisualAction.mDir - mPreBodyDir); //最后要看的方向与当前（或者说执行过本周期动作后，如turn）身体正对方向的夹角，即最后的neckrelangle。

	if (fabs(finalnec) > ServerParam::instance().maxNeckAngle()) {
		if (mCanTurn) {
			if (finalnec < 0.0) {
				mpAgent->Turn(finalnec - ServerParam::instance().minNeckAngle());
				mpAgent->TurnNeck(ServerParam::instance().minNeckMoment());
			}
			else {
				mpAgent->Turn(finalnec - ServerParam::instance().maxNeckAngle());
				mpAgent->TurnNeck(ServerParam::instance().maxNeckMoment());
			}
		}
		else {
			if (finalnec < 0.0) {
				mpAgent->TurnNeck(ServerParam::instance().minNeckMoment());
			}
			else {
				mpAgent->TurnNeck(ServerParam::instance().maxNeckMoment());
			}
		}
	}
	else {
		finalnec -= mpSelfState->GetNeckDir();
		mpAgent->TurnNeck(finalnec);
	}

	if (mViewWidth != mpSelfState->GetViewWidth()) {
		mpAgent->ChangeView(mViewWidth);
	}
}

/**
* 提出球的视觉请求
* @param eva 视觉请求的强度，理解成每eva周期看一次球
*/
void VisualSystem::RaiseBall(double eva)
{
	const Strategy & strategy = mpAgent->GetStrategy();

	if (mpWorldState->GetPlayMode() < PM_Our_Mode && mpWorldState->GetPlayMode() > PM_Play_On){
		eva = Min(2.0, eva);
	}

	if (eva < FLOAT_EPS ){
		if(strategy.IsBallFree() && mpWorldState->GetPlayMode() == PM_Play_On){
			if(mpBallState->GetVelDelay() <= mpWorldState->CurrentTime() - strategy.GetLastBallFreeTime()){//球free后曾看到过球速
				//在球即将不再free时要看球,其他可以不看,但上限是3个周期,其间可以收集些别的信息
				eva = Max(Min(strategy.IsMyControl()? double(strategy.GetMyInterCycle()): strategy.GetBallFreeCycleLeft(), 3.0), 1.0);
			}
			else{ //球free后还未看到过球速
				eva = 1.0;
				SetForceSeeBall();
			}
		}
		else if(strategy.IsTmControl()){
			double ball_dis = (mPreBallPos - mPreSelfPos).Mod();
			eva = ball_dis / 20.0 + 1.0;
			eva = Max(eva, 3.0);
		}
		else{
			double ball_dis = (mPreBallPos - mPreSelfPos).Mod();
			eva = ball_dis / 20.0;
			eva = Max(eva, 2.0);
		}
	}

	mVisualRequest.GetOfBall().mFreq = Min(mVisualRequest.GetOfBall().mFreq, eva);
}

/**
* 提出球员的视觉请求
* @param num 球员号码，+ 队友，- 对手
* @param eva 视觉请求的强度，理解成每eva周期看一次这个人
*/
void VisualSystem::RaisePlayer(ObjectIndex unum, double eva)
{
	if (unum == 0 || !mpWorldState->GetPlayer(unum).IsAlive() || unum == mpSelfState->GetUnum()) {
		return;
	}

	if (eva < FLOAT_EPS ){
		double dis = mpInfoState->GetPositionInfo().GetPlayerDistToPlayer(mpSelfState->GetUnum(), unum);
		if(dis < 3.0){
			eva = 6.0;
		}
		else if(dis < 6.0){
			eva = 5.0;
		}
		else if(dis < 20.0){
			eva = 10.0;
		}
		else if(dis < 40.0){
			eva = 26.0;
		}
		else{
			eva = 50.0;
		}
	}

	VisualRequest *vr = GetVisualRequest(unum);
	vr->mFreq = Min(vr->mFreq, eva);

	if (mpWorldState->GetPlayer(unum).GetPosConf() < FLOAT_EPS) {
		if(eva <= 2){
			RaiseForgotObject(unum);
		}
	}

	PlayMode pm = mpWorldState->GetPlayMode();
	if ( ((/*pm == PM_Our_Penalty_Setup ||*/ pm == PM_Our_Penalty_Ready || pm == PM_Our_Penalty_Taken) && mpWorldState->GetPlayer(unum).IsGoalie())
			|| (-unum == mpInfoState->GetPositionInfo().GetTeammateOffsideLineOpp() && mpAgent->GetFormation().GetMyRole().mLineType == LT_Forward && eva <= mpWorldState->GetPlayer(unum).GetPosDelay())) {
		RaiseForgotObject(unum);
	}
}

void VisualSystem::RaiseForgotObject(ObjectIndex unum)
{
	mIsSearching = true;

	if (unum == 0) {
		mForceToSeeObject.GetOfBall() = true;
	}
	else {
		mHighPriorityPlayerSet.insert(GetVisualRequest(unum));
	}
}

inline void VisualSystem::UpdatePredictInfo()
{
	mPreBodyDir = mpAgent->GetSelfBodyDirWithQueuedActions();
	mPreSelfPos = mpAgent->GetSelfPosWithQueuedActions();

	const Unum kickale_player = mpAgent->GetInfoState().GetPositionInfo().GetPlayerWithBall();
	if (kickale_player != 0 && kickale_player != mpSelfState->GetUnum()) {
		mPreBallPos = mpBallState->GetPos();
	}
	else {
		mPreBallPos = mpAgent->GetBallPosWithQueuedActions();
	}
}

VisualSystem::VisualAction VisualSystem::VisualRing::GetBestVisualAction(const AngleDeg left_most, const AngleDeg right_most, const AngleDeg interval_length)
{
	//区间定义成： [left, right]
	AngleDeg left = left_most;
	AngleDeg right = left;

	double sum = 0.0;

	for (; right < left + interval_length; ++right) {
		sum += Score(right);
	}
	sum += Score(right);

	double max = sum;
	AngleDeg best = (left + right) * 0.5;

	for (; right < right_most; ++right, ++left) {
		AngleDeg in = (Score(right + 1) - Score(left));

		sum += in;

		if (in < FLOAT_EPS) continue;

		if (sum > max) {
			max = sum;

			AngleDeg alpha = left + 1;
			while (Score(alpha) < FLOAT_EPS && alpha < right_most) alpha ++;
			AngleDeg beta = right + 1;
			while (Score(beta) < FLOAT_EPS && beta > alpha) beta --;
			best = (alpha + beta) * 0.5;
		}
	}

	return VisualAction(best, max);
}

bool VisualSystem::DealWithSetPlayMode()
{
	static bool check_both_side = false;
	static bool check_one_side = false;

	PlayMode play_mode = mpWorldState->GetPlayMode();
	PlayMode last_play_mode = mpWorldState->GetLastPlayMode();

	if (play_mode == PM_Our_Penalty_Ready || play_mode == PM_Our_Penalty_Setup || play_mode == PM_Our_Penalty_Taken) return false;

	if (play_mode != PM_Play_On && !(mForceToSeeObject.GetOfBall()) && !mpSelfState->IsGoalie()
			&& play_mode != PM_Before_Kick_Off
			&& last_play_mode != PM_Before_Kick_Off
			&& last_play_mode != PM_Our_Kick_Off
			&& last_play_mode != PM_Opp_Kick_Off
	){
		//特殊模式下要强制看看全场， 防止遗漏
		if (mpWorldState->CurrentTime() < mpWorldState->GetPlayModeTime() + 3){ //前三个周期等视觉信息到来
			check_both_side = false;
			check_one_side = false;
		}
		else {
			if (!mpAgent->IsNewSight()) return true;

			if (!check_both_side) {
				if (!check_one_side){
					int diff = mpWorldState->CurrentTime() - mpWorldState->GetPlayModeTime();
					int flag = diff % 6;
					int base = mpSelfState->GetUnum() % 6; //不要大家都一起改变视角，这样可能一起错过对方发球的瞬间

					if (flag == base) {
						mPreBodyDir = mpAgent->GetSelfBodyDirWithQueuedActions();
						mCanTurn = false;
						mBestVisualAction.mDir = GetNormalizeAngleDeg(mPreBodyDir + 90.0);
						ChangeViewWidth(VW_Wide); //new info will come in 3 cycle
						mCanForceChangeViewWidth = false;
						DoVisualExecute();
						check_one_side = true;

						return true;
					}
				}
				else {
					mBestVisualAction.mDir = GetNormalizeAngleDeg(mpSelfState->GetNeckGlobalDir() + 180.0);
					ChangeViewWidth(VW_Wide); //new info will come in 3 cycle
					mCanForceChangeViewWidth = false;
					DoVisualExecute();
					check_one_side = false;
					check_both_side = true;

					return true;
				}
			}
		}
	}

	return false;
}

void VisualSystem::DealWithSpecialObjects()
{
	static const double buffer = 0.25;
	static const int object_count = TEAMSIZE * 2 + 1;

	static const double high_priority_multi = object_count * 0.5;
	static const double force_to_see_player_multi = object_count * high_priority_multi;
	static const double force_to_see_ball_multi = object_count * force_to_see_player_multi;

	if (!mHighPriorityPlayerSet.empty()) {
		for (std::set<VisualRequest*>::iterator it = mHighPriorityPlayerSet.begin(); it != mHighPriorityPlayerSet.end(); ++it) {
			if (!(*it)->mValid) continue;
			(*it)->mScore = (*it)->Multi() * (mForceToSeeObject[(*it)->mUnum]? force_to_see_player_multi: high_priority_multi);

			if ((*it)->mpObject->GetPosConf() > 0.9 && (*it)->PreDistance() < ServerParam::instance().visibleDistance() - buffer) {
				SetCritical(true);
			}
		}
	}

	if (mForceToSeeObject[0] && mVisualRequest[0].mValid) {
		if (mpBallState->GetPosConf() > 0.9 && mVisualRequest[0].PreDistance() < ServerParam::instance().visibleDistance() - buffer) {
			if (mpBallState->GetPosDelay() == 0) {
				mVisualRequest[0].mScore = FLOAT_EPS; //just sense it
			}

			SetCritical(true);
		}
		else {
			mVisualRequest[0].mScore = mVisualRequest[0].Multi() * force_to_see_ball_multi;
		}
	}
}

void VisualSystem::SetVisualRing()
{
	mVisualRing.Clear();

	for (int i = -TEAMSIZE; i <= TEAMSIZE; ++i) {
		const VisualRequest & visual_request = mVisualRequest[i];

		if (!visual_request.mValid) continue;

		for (double dir = -5.0; dir <= 5.0; dir += 1.0) {
			mVisualRing.Score(visual_request.mPrePos.Dir() - mPreBodyDir + dir) += visual_request.mScore / 11.0;
		}
	}
}

void VisualSystem::GetBestVisualAction()
{
	if (mIsCritical) {
		if (NewSightComeCycle(VW_Wide) == 1) {
			ChangeViewWidth(VW_Wide);
		}
		else if (NewSightComeCycle(VW_Normal) == 1) {
			ChangeViewWidth(VW_Normal);
		}
		else {
			ChangeViewWidth(VW_Narrow);
		}

		mBestVisualAction = GetBestVisualActionWithViewWidth(mViewWidth, true);
	}
	else {
		if (mViewWidth != VW_Narrow && !mIsSearching) {
			mBestVisualAction = GetBestVisualActionWithViewWidth(mViewWidth, true);
		}
		else {
			mCanForceChangeViewWidth = true;


			Array<VisualAction, 3> best_visual_action;

			//球在3.0米以内，尽量保证每周期都能感知到球，除非对球的下周期状态预测很有把握
			const bool force =
					mpWorldState->GetPlayMode() != PM_Our_Penalty_Taken
					&& mpWorldState->GetBall().GetPosDelay() == 0 && mpWorldState->GetHistory(1)->GetBall().GetPosDelay() == 0
					&& mpAgent->GetInfoState().GetPositionInfo().GetClosestOpponentDistToBall() > 3.0;

			if (force && PlayerParam::instance().SaveTextLog()) {
				Logger::instance().GetTextLogger("sure_ball") << mpWorldState->CurrentTime() << std::endl;
			}

			if (NewSightComeCycle(VW_Wide) == 1) {
				best_visual_action[2] = GetBestVisualActionWithViewWidth(VW_Wide, force);
			}
			else if (NewSightComeCycle(VW_Normal) == 1) {
				best_visual_action[1] = GetBestVisualActionWithViewWidth(VW_Normal, force);
				best_visual_action[2] = GetBestVisualActionWithViewWidth(VW_Wide, force);
			}
			else {
				best_visual_action[0] = GetBestVisualActionWithViewWidth(VW_Narrow, force);
				best_visual_action[1] = GetBestVisualActionWithViewWidth(VW_Normal, force);
				best_visual_action[2] = GetBestVisualActionWithViewWidth(VW_Wide, force);
			}

			const double buffer = FLOAT_EPS;
			if (best_visual_action[0].mScore > best_visual_action[1].mScore - buffer) {
				if (best_visual_action[0].mScore > best_visual_action[2].mScore - buffer) {
					//narrow
					ChangeViewWidth(VW_Narrow);
					mBestVisualAction = best_visual_action[0];
				}
				else {
					//wide
					ChangeViewWidth(VW_Wide);
					mBestVisualAction = best_visual_action[2];
				}
			}
			else {
				if (best_visual_action[1].mScore > best_visual_action[2].mScore - buffer) {
					//normal
					ChangeViewWidth(VW_Normal);
					mBestVisualAction = best_visual_action[1];
				}
				else {
					//wide
					ChangeViewWidth(VW_Wide);
					mBestVisualAction = best_visual_action[2];
				}
			}
		}
	}

	mBestVisualAction.mDir += mPreBodyDir;
	//Assert(mBestVisualAction.mScore > 0.0);
}

VisualSystem::VisualAction VisualSystem::GetBestVisualActionWithViewWidth(ViewWidth view_width, bool force)
{
	if (force || GetSenseBallCycle() >= NewSightComeCycle(view_width)) {
		AngleDeg max_turn_ang = mCanTurn? mpSelfState->GetMaxTurnAngle(): 0.0;
		AngleDeg half_view_angle = sight::ViewAngle(view_width) * 0.5;
		AngleDeg neck_left_most = ServerParam::instance().minNeckAngle() - max_turn_ang; //脖子可以到达的极限角度（相对于当前身体正前方而言）
		AngleDeg neck_right_most = ServerParam::instance().maxNeckAngle() + max_turn_ang;
		AngleDeg left_most = neck_left_most - half_view_angle;
		AngleDeg right_most = neck_right_most + half_view_angle;

		VisualAction best_visual_action = mVisualRing.GetBestVisualAction(left_most, right_most, half_view_angle * 2.0);

		best_visual_action.mScore /= NewSightWaitCycle(view_width);

		Assert(!IsInvalid(best_visual_action.mScore));

		return best_visual_action;
	}
	else {
		return VisualAction();
	}
}

bool VisualSystem::ForceSearchBall()
{
	if (!mpSelfState->IsIdling() && !mVisualRequest[0].mValid) { //force to scan
		mpAgent->GetActionEffector().ResetForScan();

		if (NewSightComeCycle(VW_Wide) == 1) {
			mpAgent->ChangeView(VW_Wide);
		}
		else if (NewSightComeCycle(VW_Normal) == 1){
			mpAgent->ChangeView(VW_Normal);
		}
		else {
			mpAgent->ChangeView(VW_Narrow);
		}

		mpAgent->Turn(sight::ViewAngle(VW_Narrow) - 5.0);

		return true;
	}
	return false;
}

void VisualSystem::DoDecision()
{
	if (!mpAgent->IsNewSight()){
		if (mCanForceChangeViewWidth) {
			mViewWidth = VW_Narrow; //重置为窄视角
		}
		else {
			mViewWidth = mpSelfState->GetViewWidth();
		}
	}

	DoInfoGather(); //提请求
	EvaluateVisualRequest(); //评价

	if (!ForceSearchBall()) {
		DealVisualRequest(); //处理请求
		DoVisualExecute(); //视觉执行
	}
}

int VisualSystem::GetSenseBallCycle()
{
	if (mSenseBallCycle == 0) { //用拦截模型来算什么时候可以感知到球
		if (mVisualRequest[0].mValid) {
			if (mVisualRequest[0].PreDistance() < ServerParam::instance().visibleDistance()) {
				mSenseBallCycle = 1;
			}
			else {
				PlayerInterceptInfo int_info;
				VirtualSelf virtual_self(*mpSelfState);

				virtual_self.UpdateIsGoalie(false); //否则会用可扑范围代替感知范围计算截球

				int_info.mRes = IR_None;
				int_info.mpPlayer = & virtual_self;

				InterceptInfo::CalcTightInterception(*mpBallState, & int_info);

				mSenseBallCycle = int_info.mMinCycle;
				mSenseBallCycle = Max(mSenseBallCycle, 1);
			}
		}
		else {
			mSenseBallCycle = 1000;
		}
	}

	return mSenseBallCycle;
}

void VisualSystem::SetForceSeeBall()
{
	mForceToSeeObject.GetOfBall() = true;

	Logger::instance().GetTextLogger("force_to_see") << mpWorldState->CurrentTime() << ": ball" << std::endl;
}

void VisualSystem::SetForceSeePlayer(Unum i)
{
	if (i != 0) {
		mForceToSeeObject[i] = true;
		mHighPriorityPlayerSet.insert(GetVisualRequest(i));

		Logger::instance().GetTextLogger("force_to_see") << mpWorldState->CurrentTime() << ": player " << i << std::endl;
	}
}
