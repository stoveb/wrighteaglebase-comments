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
 * @file ActionEffector.cpp
 * @brief 动作执行器（ActionEffector）实现
 *
 * ActionEffector 是 WrightEagleBase 的动作执行核心，负责将决策系统的行为
 * 转换为符合 rcssserver 协议的具体命令，并管理动作队列、执行顺序与约束。
 *
 * 主要职责：
 * - 管理所有动作对象（基本动作、通信、视觉、管理类动作）；
 * - 控制每周期动作执行次数与互斥关系（如 turn/dash/kick 互斥）；
 * - 提供原子动作与复合动作的执行接口；
 * - 将最终命令通过 UDPSocket 发送给服务器。
 *
 * 设计要点：
 * - 每个动作类型都有对应的计数器与标志位；
 * - 使用 CMD_QUEUE_MUTEX 保证命令队列的线程安全；
 * - 通过 SendCommands 统一打包并发送命令。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "ActionEffector.h"
#include "Agent.h"
#include "UDPSocket.h"
#include "Observer.h"
#include "WorldState.h"
#include "VisualSystem.h"
#include "NetworkTest.h"

/**
 * @brief 动作命令队列的互斥锁
 * 
 * 用于保护动作命令队列的线程安全访问。
 * 在多线程环境下，确保动作命令的添加和执行不会发生冲突。
 */
ThreadMutex ActionEffector::CMD_QUEUE_MUTEX;

/**
 * @brief 原子动作执行函数
 * 
 * 执行单个原子动作，这是动作执行系统的基本单元。
 * 原子动作是不可再分割的最小动作单位。
 * 
 * 支持的原子动作类型：
 * - CT_Turn: 转身动作
 * - CT_Dash: 跑动动作  
 * - CT_Kick: 踢球动作
 * 
 * @param agent 执行动作的智能体引用
 * @return bool 动作是否成功执行
 * 
 * @note 当前实现中mSucceed参数未完全使用
 * @note 这是动作执行的基础函数，所有复杂动作都由原子动作组合而成
 */
bool AtomicAction::Execute(Agent & agent) const
{
	// 当前实现中，无论mSucceed是true还是false都会执行动作
	// TODO: 需要完善失败情况的处理逻辑
	if (mSucceed || !mSucceed){ 
		switch (mType){
		case CT_Turn: 
			// 执行转身动作
			return agent.Turn(mTurnAngle);
			
		case CT_Dash: 
			// 执行跑动动作，包含力量和方向
			return agent.Dash(mDashPower, mDashDir);
			
		case CT_Kick: 
			// 执行踢球动作，将速度向量分解为大小和方向
			return agent.Kick(mKickVel.Mod(), mKickVel.Dir());
			
		default: 
			// 未知动作类型，默认返回成功
			return true;
		}
	}
	else {
		// 动作失败的情况
		return false;
	}
}

/**
 * @brief ActionEffector 构造函数
 * 
 * 初始化动作执行器，设置所有动作对象和计数器。
 * ActionEffector 是整个动作执行系统的核心管理器。
 * 
 * 初始化内容：
 * 1. 设置智能体和状态对象的引用
 * 2. 初始化所有动作对象（turn, dash, kick等）
 * 3. 重置所有动作计数器
 * 4. 准备动作命令队列
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 构造函数会初始化22种不同类型的动作对象
 * @note 每个动作对象都有对应的计数器来跟踪执行次数
 */
ActionEffector::ActionEffector(Agent & agent):
	// 设置核心对象引用
	mAgent( agent ),
	mWorldState( agent.GetWorldState() ),
	mBallState( agent.GetWorldState().GetBall()),
    mSelfState( agent.GetSelf()),
    
	// 初始化基本动作对象
	mTurn( agent ),           // 转身动作
	mDash( agent ),           // 跑动动作
	mTurnNeck( agent ),       // 转颈动作
	mSay( agent ),            // 说话动作
	mAttentionto( agent ),    // 注意力动作
	mKick( agent ),           // 踢球动作
	mTackle( agent ),         // 铲球动作
	mPointto( agent ),        // 指向动作
	mCatch( agent ),          // 接球动作
	mMove( agent ),           // 移动动作
	mChangeView( agent ),     // 改变视角动作
	mCompression( agent ),    // 压缩动作
	mSenseBody( agent ),      // 身体感知动作
	mScore( agent ),          // 比分动作
	mBye( agent ),            // 告别动作
	mDone( agent ),           // 完成动作
	
	// 初始化管理动作对象
	mClang( agent ),          // 教练语言动作
	mEar( agent ),            // 听觉动作
	mSynchSee( agent ),       // 同步视觉动作
	mChangePlayerType( agent ), // 改变球员类型动作
	mStart( agent ),          // 开始动作
	mChangePlayMode( agent ), // 改变比赛模式动作
	mMovePlayer( agent ),     // 移动球员动作
	mMoveBall( agent ),       // 移动球动作
	mLook( agent ),           // 查看动作
	mTeamNames( agent ),      // 队伍名称动作
	mRecover( agent ),        // 恢复动作
	mCheckBall( agent )       // 检查球动作
{
	// 初始化所有动作计数器为0
	// 这些计数器用于跟踪每个周期内各种动作的执行次数
	mTurnCount          = 0;  // 转身动作计数
	mDashCount          = 0;  // 跑动动作计数
	mTurnNeckCount      = 0;  // 转颈动作计数
	mSayCount           = 0;  // 说话动作计数
	mAttentiontoCount   = 0;  // 注意力动作计数
	mKickCount          = 0;  // 踢球动作计数
	mTackleCount        = 0;  // 铲球动作计数
	mPointtoCount       = 0;  // 指向动作计数
	mCatchCount         = 0;  // 接球动作计数
	mMoveCount          = 0;  // 移动动作计数
	mChangeViewCount    = 0;  // 改变视角动作计数
	mCompressionCount   = 0;
	mSenseBodyCount     = 0;
	mScoreCount         = 0;
	mByeCount           = 0;
	mDoneCount          = 0;
	mClangCount         = 0;
	mEarCount           = 0;
	mSynchSeeCount      = 0;
	mChangePlayerTypeCount = 0;

	mIsMutex        = false;

	mIsTurn         = false;
	mIsDash         = false;
	mIsTurnNeck     = false;
	mIsSay          = false;
	mIsAttentionto  = false;
	mIsKick         = false;
	mIsTackle       = false;
	mIsPointto      = false;
	mIsCatch        = false;
	mIsMove         = false;
	mIsChangeView   = false;
	mIsCompression  = false;
	mIsSenseBody    = false;
	mIsScore        = false;
	mIsBye          = false;
	mIsDone         = false;
	mIsClang        = false;
	mIsEar          = false;
	mIsSynchSee     = false;
	mIsChangePlayerType = false;

	mIsSayMissed	= false;
    mLastCommandType = CT_None;
}

bool ActionEffector::SetTurnAction(AngleDeg turn_angle)
{
	// 检查是否已经执行过转身动作或处于互斥状态
	// 转身动作与dash、kick等基本动作互斥，每周期只能执行一种
	if (mIsTurn == true || mIsMutex == true)
	{
		return false; // 已执行转身或互斥动作，无法再次执行
	}

	AngleDeg moment = GetTurnMoment(turn_angle, mSelfState.GetPlayerType(), mSelfState.GetVel().Mod());

	moment = GetNormalizeMoment(moment);

	// 检查转身角度是否过小，避免无意义的微小转身
	// 如果角度小于浮点精度阈值，则认为不需要转身
	if (fabs(moment) < FLOAT_EPS){
		return false; // 转身角度过小，无需执行
	}

	mTurn.Plan(moment);
	mTurn.Execute(mCommandQueue);
	++mTurnCount;
	mIsTurn = true;
	mIsMutex = true;
	return true;
}

bool ActionEffector::SetDashAction(double power, AngleDeg dir)
{
	Assert(dir > -180.0 - FLOAT_EPS && dir < 180.0 + FLOAT_EPS);

	// 检查是否已经执行过跑动动作或处于互斥状态
	// 跑动动作与turn、kick等基本动作互斥，每周期只能执行一种
	if (mIsDash == true || mIsMutex == true)
	{
		return false; // 已执行跑动或互斥动作，无法再次执行
	}

	power = GetNormalizeDashPower(power);
    dir = GetNormalizeDashAngle(dir);

    // 注意，在rcssserver13.1.0版本下，dash只需要考虑下面四种情况即可：
    // @ -power 0.0
    // @ +power 0.0
    // @ +power 90.0
    // @ +power -90.0
    //
    // from rcssserver13.1.0

    // 注意，在rcssserver v14 下，dash需要考虑的情况包括：
    // @ +power, 0
    // @ +power, 45
    // @ +power, 90
    // @ +power, 135 | -power, -45
    // @ -power, 0 | +power, 180
    // @ +power, -45
    // @ +power, -90
    // @ -power, 45 | +power, -135
    	// 根据服务器版本处理跑动方向的离散化
	// 服务器版本不同，跑动方向的处理方式也不同
	// 旧版本服务器支持任意方向的跑动，而新版本服务器则将跑动方向离散化为固定步长
	if (ServerParam::instance().dashAngleStep() < FLOAT_EPS )
	{
		// 旧版本服务器：支持任意方向的跑动
		// players can dash in any direction.
	}
	else
	{
		// 新版本服务器：跑动方向被离散化为固定步长
		// The dash direction is discretized by server::dash_angle_step
		// 通过取整函数Rint将方向角度转换为离散化后的值
		dir = ServerParam::instance().dashAngleStep() * Rint(dir / ServerParam::instance().dashAngleStep());
	}

    TransformDash(power, dir);

	// 根据体力限制调整跑动力量
	// 确保跑动不会消耗超过当前可用体力的力量
	// 体力限制是指跑动时消耗的体力不能超过当前可用体力的最大值
	// 通过比较跑动力量与当前可用体力，调整跑动力量以避免体力不足
	double max_stamina = mSelfState.GetStamina() + mSelfState.GetExtraStamina();
	if (power < 0.0)
	{
		// 负向跑动（后退）：消耗双倍体力，检查是否足够
		if ((-2.0 * power) > max_stamina)
		{
			power = -max_stamina * 0.5 + FLOAT_EPS; // 调整到最大可用负向力量
		}
	}
	else
	{
		// 正向跑动：消耗单倍体力，检查是否足够
		if (power > max_stamina)
		{
			power = max_stamina - FLOAT_EPS; // 调整到最大可用正向力量
		}
	}

	if (!mAgent.GetSelf().IsOutOfStamina()) {
		power = mAgent.GetSelf().CorrectDashPowerForStamina(power); // 保证不降到最低体力以下
	}

    if (std::fabs(power) < FLOAT_EPS) return false;

    TransformDash(power, dir);

	mDash.Plan(power, dir);
	mDash.Execute(mCommandQueue);
	++mDashCount;
	mIsDash = true;
	mIsMutex = true;
	return true;
}

bool ActionEffector::SetTurnNeckAction(AngleDeg angle)
{
	// 检查是否已经执行过转颈动作
	// 转颈动作不与其他基本动作互斥，但每周期只能执行一次
	if (mIsTurnNeck == true)
	{
		return false; // 本周期已执行过转颈，无法再次执行
	}

	// 检查转颈角度是否在服务器允许范围内
	// 服务器对转颈角度有最小和最大限制
	if (angle < ServerParam::instance().minNeckMoment() || angle > ServerParam::instance().maxNeckMoment())
	{
		return false; // 转颈角度超出允许范围
	}

	mTurnNeck.Plan(angle);
	mTurnNeck.Execute(mCommandQueue);
	++mTurnNeckCount;
	mIsTurnNeck = true;
	return true;
}

bool ActionEffector::SetSayAction(std::string msg)
{
	// 检查是否已经执行过说话动作
	// 说话动作不与其他基本动作互斥，但每周期只能执行一次
	if (mIsSay == true)
	{
		return false; // 本周期已执行过说话，无法再次执行
	}

	// 根据客户端类型检查消息长度限制
	if (PlayerParam::instance().isCoach())
	{
		// 教练端：检查自由格式消息长度限制
		if (msg.length() > ServerParam::instance().freeformMsgSize())
		{
			//false; // 注释掉的代码，可能原计划返回失败
		}
	}
	else
	{
		// 球员端：检查普通say消息长度限制
		if (msg.length() > ServerParam::instance().sayMsgSize())
		{
			return false; // 消息超长，无法发送
		}
	}

	mSay.Plan(msg);
	mSay.Execute(mCommandQueue);
	++mSayCount;
	mIsSay = true;
	return true;
}

bool ActionEffector::SetAttentiontoAction(Unum num)
{
	if (mIsAttentionto == true)
	{
		return false;
	}

	// 验证注意力目标的有效性
	// num=0表示无目标，num=mSelfUnum表示自己，都不允许
	// 超出球队号码范围也不允许
	if (num == 0 || num == mAgent.GetSelfUnum() || num < -TEAMSIZE || num > TEAMSIZE)
	{
		return false; // 目标无效
	}

	// 检查是否已经关注相同目标
	// 如果已经关注相同目标，则无需再次执行
	if (mAgent.GetSelf().GetFocusOnUnum() == num)
	{
		mIsAttentionto = true; // 标记本周期不再attentionto
		return false; // 无需重复关注
	}

	mAttentionto.Plan(true, num);
	mAttentionto.Execute(mCommandQueue);
	++mAttentiontoCount;
	mIsAttentionto = true;
	return true;
}

bool ActionEffector::SetAttentiontoOffAction()
{
	if (mIsAttentionto == true)
	{
		return false;
	}

    if (mAgent.GetSelf().GetFocusOnUnum() == 0)
    {
        mIsAttentionto = true; // 本周期不再attentionto
        return false;
    }

	mAttentionto.Plan(false);
	mAttentionto.Execute(mCommandQueue);
	++mAttentiontoCount;
	mIsAttentionto = true;
	return true;
}

bool ActionEffector::SetKickAction(double power, AngleDeg angle)
{
	if (mIsKick == true || mIsMutex == true)
	{
		return false;
	}

	power = GetNormalizeKickPower(power);
	angle = GetNormalizeMoment(angle);

	if (mWorldState.GetPlayMode() == PM_Before_Kick_Off ||
			mWorldState.GetPlayMode() == PM_Goal_Ours ||
			mWorldState.GetPlayMode() == PM_Goal_Opps ||
			mWorldState.GetPlayMode() == PM_Opp_Offside_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Offside_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Foul_Charge_Kick || 
            mWorldState.GetPlayMode() == PM_Opp_Foul_Charge_Kick ||
            mWorldState.GetPlayMode() == PM_Our_Back_Pass_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_Back_Pass_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Free_Kick_Fault_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_Free_Kick_Fault_Kick ||
			mWorldState.GetPlayMode() == PM_Our_CatchFault_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_CatchFault_Kick ||
			mWorldState.GetPlayMode() == PM_Time_Over)
	{
		return false;
	}

	if (mSelfState.IsKickable() == false)
	{
		return false;
	}

    if (power < 1.0) // 避免不必要的kick
    {
        return false;
    }

    if (power > ServerParam::instance().maxPower() - 1.0)
    {
        power = ServerParam::instance().maxPower();
    }

	mKick.Plan(power, angle);
	mKick.Execute(mCommandQueue);
	++mKickCount;
	mIsKick = true;
	mIsMutex = true;
	return true;
}

bool ActionEffector::SetTackleAction(AngleDeg angle, const bool foul)
{
	if (mIsTackle == true || mIsMutex == true)
	{
		return true;
	}

	if (angle < ServerParam::instance().minMoment() || angle > ServerParam::instance().maxMoment())
	{
		return false;
	}

	if (mSelfState.GetIdleCycle() > 0)
	{
		return false;
	}

	double tackle_prob = mSelfState.GetTackleProb(foul);
	if (tackle_prob < FLOAT_EPS && 
        mWorldState.GetPlayMode() != PM_Opp_Penalty_Taken) // 守门员防点球时能tackle就tackle
	{
		return false;
	}

	if (mWorldState.GetPlayMode() == PM_Before_Kick_Off ||
			mWorldState.GetPlayMode() == PM_Goal_Ours ||
			mWorldState.GetPlayMode() == PM_Goal_Opps ||
			mWorldState.GetPlayMode() == PM_Opp_Offside_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Offside_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Foul_Charge_Kick || 
            mWorldState.GetPlayMode() == PM_Opp_Foul_Charge_Kick || 
            mWorldState.GetPlayMode() == PM_Our_Back_Pass_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_Back_Pass_Kick ||
			mWorldState.GetPlayMode() == PM_Our_Free_Kick_Fault_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_Free_Kick_Fault_Kick ||
			mWorldState.GetPlayMode() == PM_Our_CatchFault_Kick ||
			mWorldState.GetPlayMode() == PM_Opp_CatchFault_Kick ||
			mWorldState.GetPlayMode() == PM_Time_Over)
	{
		return false;
	}

	mTackle.Plan(angle, foul);
	mTackle.Execute(mCommandQueue);
	++mTackleCount;
	mIsTackle = true;
	mIsMutex = true;
	return true;
}

bool ActionEffector::SetPointtoAction(double dist, AngleDeg angle)
{
	if (mIsPointto == true)
	{
		return false;
	}

	if (mSelfState.GetArmPointMovableBan() > 0)
	{
		return false;
	}

	mPointto.Plan(true, dist, angle);
	mPointto.Execute(mCommandQueue);
	++mPointtoCount;
	mIsPointto = true;
	return true;
}

bool ActionEffector::SetPointtoOffAction()
{
	if (mIsPointto == true)
	{
		return false;
	}

	if (mSelfState.GetArmPointMovableBan() > 0)
	{
		return false;
	}

	mPointto.Plan(false);
	mPointto.Execute(mCommandQueue);
	++mPointtoCount;
	mIsPointto = true;
	return true;
}

bool ActionEffector::SetCatchAction(AngleDeg angle)
{
    if (mIsCatch == true || mIsMutex == true)
    {
        return false;
    }

    if (angle < ServerParam::instance().minMoment() || angle > ServerParam::instance().maxMoment())
    {
        return false;
    }

//    if (mSelfState.IsGoalie() == false) //FIXME
//    {
//        return false;
//    }

    if (mSelfState.GetCatchBan() > 0)
    {
        return false;
    }

    if (mWorldState.GetPlayMode() != PM_Play_On &&
    		mWorldState.GetPlayMode() != PM_Our_Penalty_Setup &&
    		mWorldState.GetPlayMode() != PM_Opp_Penalty_Setup &&
    		mWorldState.GetPlayMode() != PM_Our_Penalty &&
    		mWorldState.GetPlayMode() != PM_Opp_Penalty_Ready &&
    		mWorldState.GetPlayMode() != PM_Our_Penalty_Taken &&
    		mWorldState.GetPlayMode() != PM_Opp_Penalty_Taken &&
    		mWorldState.GetPlayMode() != PM_Our_Penalty_Miss &&
    		mWorldState.GetPlayMode() != PM_Opp_Penalty_Miss &&
    		mWorldState.GetPlayMode() != PM_Our_Penalty_Score &&
    		mWorldState.GetPlayMode() != PM_Opp_Penalty_Score)
    {
        return false;
    }

    /*if (mSelfState.IsBallCatchable() == false)
    {
        return false;
    }*/

    mCatch.Plan(angle);
    mCatch.Execute(mCommandQueue);
    ++mCatchCount;
    mIsCatch = true;
    mIsMutex = true;
    return true;
}

bool ActionEffector::SetMoveAction(Vector pos)
{
	if (mIsMove == true || mIsMutex == true)
	{
		return false;
	}

	//goalie move, see SoccerServer player.cpp

	mMove.Plan(pos);
	mMove.Execute(mCommandQueue);
	++mMoveCount;
	mIsMove = true;
	mIsMutex = true;
	return true;
}

bool ActionEffector::SetChangeViewAction(ViewWidth view_width)
{
	if (view_width != VW_Narrow &&
			view_width != VW_Normal &&
			view_width != VW_Wide)
	{
		return false;
	}

	VisualSystem::instance().ChangeViewWidth(mAgent, view_width); //与视觉系统同步

	if (view_width == mSelfState.GetViewWidth())
	{
		return true;
	}

	mChangeView.Plan(view_width);
	mChangeView.Execute(mCommandQueue);
	++mChangeViewCount;
	mIsChangeView = true;
	return true;
}

bool ActionEffector::SetCompressionAction(int level)
{
	if (mIsCompression == true)
	{
		return false;
	}

	if (level > 9)
	{
		return false;
	}

	mCompression.Plan(level);
	mCompression.Execute(mCommandQueue);
	++mCompressionCount;
	mIsCompression = true;
	return true;
}

bool ActionEffector::SetSenseBodyAction()
{
	if (mIsSenseBody == true)
	{
		return false;
	}

	mSenseBody.Plan();
	mSenseBody.Execute(mCommandQueue);
	++mSenseBodyCount;
	mIsSenseBody = true;
	return true;
}

bool ActionEffector::SetScoreAction()
{
	if (mIsScore == true)
	{
		return false;
	}

	mScore.Plan();
	mScore.Execute(mCommandQueue);
	++mScoreCount;
	mIsScore = true;
	return true;
}

bool ActionEffector::SetByeAction()
{
	if (mIsBye == true)
	{
		return false;
	}

	mBye.Plan();
	mBye.Execute(mCommandQueue);
	++mByeCount;
	mIsBye = true;
	return true;
}

bool ActionEffector::SetDoneAction()
{
	if (mIsDone == true)
	{
		return false;
	}

	mDone.Plan();
	mDone.Execute(mCommandQueue);
	++mDoneCount;
	mIsDone = true;
	return true;
}

bool ActionEffector::SetClangAction(int min_ver, int max_ver)
{
	//    if (mIsClang == true)
	//    {
	//        return false;
	//    }

	mClang.Plan(min_ver, max_ver);
	mClang.Execute(mCommandQueue);
	++mClangCount;
	mIsClang = true;
	return true;
}

bool ActionEffector::SetEarOnAction(bool our_side, EarMode ear_mode)
{
	// ear命令一个周期可以发多个
	if (ear_mode != EM_Partial && ear_mode != EM_Complete)
	{
		return false;
	}

	mEar.Plan(true, our_side, ear_mode);
	mEar.Execute(mCommandQueue);
	++mEarCount;
	mIsEar = true;
	return true;
}

bool ActionEffector::SetEarOffAction(bool our_side, EarMode ear_mode)
{
	// ear命令一个周期可以发多个
	if (ear_mode != EM_Partial && ear_mode != EM_Complete && ear_mode != EM_All)
	{
		return false;
	}

	mEar.Plan(false, our_side, ear_mode);
	mEar.Execute(mCommandQueue);
	++mEarCount;
	mIsEar = true;
	return true;
}

bool ActionEffector::SetSynchSeeAction()
{
	//    if (mIsSynchSee == true)
	//    {
	//        return false;
	//    }

	mSynchSee.Plan();
	mSynchSee.Execute(mCommandQueue);
	++mSynchSeeCount;
	mIsSynchSee = true;
	return true;
}

bool ActionEffector::SetChangePlayerTypeAction(Unum num, int player_type)
{
	mChangePlayerType.Plan(num, player_type);
	mChangePlayerType.Execute(mCommandQueue);
	++mChangePlayerTypeCount;
	mIsChangePlayerType = true;
	return true;
}

/**********************************for trainer*************************************/
bool ActionEffector::SetChangePlayerTypeAction(std::string teamname,Unum num, int player_type)
{
	mChangePlayerType.Plan(teamname,num, player_type);
	mChangePlayerType.Execute(mCommandQueue);
	++mChangePlayerTypeCount;
	mIsChangePlayerType = true;
	return true;
}

//Trainer就不检测是否漏掉了，没有必要
bool ActionEffector::SetStartAction()
{
	mStart.Plan();
	mStart.Execute(mCommandQueue);
	mIsStart = true;
	return true;
}
bool ActionEffector::SetChangePlayModeAction(ServerPlayMode spm)
{
	mChangePlayMode.Plan(spm);
	mChangePlayMode.Execute(mCommandQueue);
	mIsChangePlayMode = true;
	return true;
}
bool ActionEffector::SetMovePlayerAction(std::string team_name, Unum num, Vector pos, Vector vel, AngleDeg dir)
{
	mMovePlayer.Plan(team_name, num, pos, vel, dir);
	mMovePlayer.Execute(mCommandQueue);
	mIsMovePlayer = true ;
	return true;
}
bool ActionEffector::SetMoveBallAction(Vector pos, Vector vel)
{
	mMoveBall.Plan(pos,vel);
	mMoveBall.Execute(mCommandQueue);
	mIsMoveBall = true;
	return true;
}

bool ActionEffector::SetLookAction()
{
	mLook.Plan();
	mLook.Execute(mCommandQueue);
	mIsLook = true;
	return true;
}
bool ActionEffector::SetTeamNamesAction()
{
	mTeamNames.Plan();
	mTeamNames.Execute(mCommandQueue);
	mIsTeamNames = true ;
	return true;
}
bool ActionEffector::SetRecoverAction()
{
	mRecover.Plan();
	mRecover.Execute(mCommandQueue);
	mIsRecover = true;
	return true;
}
bool ActionEffector::SetCheckBallAction()
{
	mCheckBall.Plan();
	mCheckBall.Execute(mCommandQueue);
	mIsCheckBall = true;
	return true;
}

/**
 * 通过传入kick的参数，计算kick后球的位置和速度
 * Calculate ball position and velocity after a kick action.
 * \param kick_power.
 * \param kick_angle.
 * \param player_state state of the player who is performing kick.
 * \param ball_state ball state before the kick action is performed.
 * \param ball_pos will be set to ball position after kick.
 * \param ball_vel will be set to ball velocity after kick.
 * \param is_self if the action is performed by the agent this process represents.
 */
void ActionEffector::ComputeInfoAfterKick(const double kick_power, const double kick_angle,
		const PlayerState &player_state, const BallState &ball_state, Vector &ball_pos, Vector &ball_vel, bool is_self)
{
	double power = GetNormalizeKickPower(kick_power);
	double dir = GetNormalizeMoment(kick_angle);

	Vector ball_2_player = (ball_state.GetPos() - player_state.GetPos()).Rotate(-player_state.GetBodyDir());
    double eff_power = power *
        (is_self ? player_state.GetKickRate() : GetKickRate(ball_2_player, player_state.GetPlayerType()));
	Vector accel = Polar2Vector(eff_power, player_state.GetBodyDir() + dir);

	ball_vel = ball_state.GetVel() + accel;
	ball_pos = ball_state.GetPos() + ball_vel;
	ball_vel *= ServerParam::instance().ballDecay();
}

/**
 * 通过传入dash的参数，计算dash后球员的位置和速度
 * Calculate player position and velocity after a dash action. Conflicts or forbidden areas are not
 * considered.
 * \param dash_power.
 * \param dash_dir.
 * \param player_state state of the player who is dashing.
 * \param player_pos will be set to player position after dash.
 * \param player_vel will be set to player velocity after dash.
 */
void ActionEffector::ComputeInfoAfterDash(const double dash_power, double dash_dir,
		const PlayerState &player_state, Vector &player_pos, Vector &player_vel)
{
	double dir_rate = GetDashDirRate(dash_dir);

	if (dash_power < 0.0) {
		dash_dir += 180.0;
	}
    double eff_dash_power = fabs(dash_power) * player_state.GetEffort() * player_state.GetDashPowerRate() * dir_rate;

	Vector accel = Polar2Vector(eff_dash_power, GetNormalizeAngleDeg(player_state.GetBodyDir() + dash_dir));

	player_vel = player_state.GetVel() + accel;
	player_pos = player_state.GetPos() + player_vel;
	player_vel *= player_state.GetPlayerDecay();
}

/**
 * 通过传入move的参数，计算move后球员的位置和速度
 * Calculate player position and velocity after a move action.
 * \param move_pos.
 * \param player_pos will be set to player position after move.
 * \param player_vel will be set to player velocity after move.
 */
void ActionEffector::ComputeInfoAfterMove(const Vector & move_pos, Vector &player_pos, Vector &player_vel)
{
	player_pos = move_pos;
	player_vel.SetValue(0, 0);
}

/**
 * 通过传入 turn 的参数（moment），计算 turn 后球员的身体朝向
 * Calculate player body direction after a turn action.
 * \param moment.
 * \param player_state state of the player who is turning.
 * \param body_dir will be set to player's body direction after turn.
 */
void ActionEffector::ComputeInfoAfterTurn(const AngleDeg moment,
		const PlayerState &player_state, AngleDeg &body_dir)
{
	double turn_angle = GetTurnAngle(moment, player_state.GetPlayerType(), player_state.GetVel().Mod());
	body_dir = GetNormalizeAngleDeg(player_state.GetBodyDir() + turn_angle);
}

/**
 * 通过传入turn_neck的参数，计算turn_neck后球员的脖子朝向
 * Calculate player neck direction after a turn_neck action.
 * \param turn_neck_angle.
 * \param player_state state of the player who is turning neck.
 * \param neck_dir will be set to player's neck direction after turn_neck.
 */
void ActionEffector::ComputeInfoAfterTurnNeck(const AngleDeg turn_neck_angle,
		const PlayerState &player_state, AngleDeg &neck_dir)
{
	double eff_moment = GetNormalizeNeckMoment(turn_neck_angle);
	neck_dir = GetNormalizeNeckAngle(player_state.GetNeckDir() + eff_moment);
}

/**
 * 检查上周期发给server的命令，用来辅助WorldState的更新
 * Check commands sent to server in last cycle. Help to update WorldState.
 * It predicts information by doing actions in commands queue, assuming only this one agent influences
 * world state. And it will flush the commands queue.
 */
void ActionEffector::CheckCommandQueue(Observer *observer)
{
    mLastCommandType = CT_None;

	ActionEffector::CMD_QUEUE_MUTEX.Lock();
	if (!mCommandQueue.empty())
	{
		for (std::list<CommandInfo>::iterator it = mCommandQueue.begin(); it != mCommandQueue.end(); ++it)
		{
			switch (it->mType)
			{
				case CT_Kick:
					// 检查踢球命令是否被服务器接收
					// 通过比较服务器返回的踢球计数确认命令执行
					if (observer->Sense().GetKickCount() == mKickCount)
					{
                    mLastCommandType = CT_Kick;

					Vector ball_pos = Vector(0.0, 0.0);
					Vector ball_vel = Vector(0.0, 0.0);
					ComputeInfoAfterKick(it->mPower, it->mAngle, mSelfState, mBallState, ball_pos, ball_vel);

					observer->SetBallKickTime(observer->CurrentTime());
					observer->SetBallPosByKick(ball_pos);
					observer->SetBallVelByKick(ball_vel);
				}
				break;
				case CT_Dash:
					// 检查跑动命令是否被服务器接收
					// 通过比较服务器返回的跑动计数确认命令执行
					if (observer->Sense().GetDashCount() == mDashCount)
					{
                    mLastCommandType = CT_Dash;
					
                    Vector player_pos = Vector(0.0, 0.0);
					Vector player_vel = Vector(0.0, 0.0);
					ComputeInfoAfterDash(it->mPower, it->mAngle, mSelfState, player_pos, player_vel);

					observer->SetPlayerDashTime(observer->CurrentTime());
					observer->SetPlayerPosByDash(player_pos);
					observer->SetPlayerVelByDash(player_vel);
				}
				break;
				case CT_Move:
					// 检查移动命令是否被服务器接收
					// 通过比较服务器返回的移动计数确认命令执行
					if (observer->Sense().GetMoveCount() == mMoveCount)
					{
                    mLastCommandType = CT_Move;
					
                    Vector player_pos = Vector(0.0, 0.0);
					Vector player_vel = Vector(0.0, 0.0);
					ComputeInfoAfterMove(it->mMovePos, player_pos, player_vel);

					observer->SetPlayerMoveTime(observer->CurrentTime());
					observer->SetPlayerPosByMove(player_pos);
					observer->SetPlayerVelByMove(player_vel);
				}
				break;
				case CT_Turn:
					// 检查转身命令是否被服务器接收
					// 通过比较服务器返回的转身计数确认命令执行
					if (observer->Sense().GetTurnCount() == mTurnCount)
					{
                    mLastCommandType = CT_Turn;
					
                    AngleDeg body_dir = 0.0;
					ComputeInfoAfterTurn(it->mAngle, mSelfState, body_dir);

					observer->SetPlayerTurnTime(observer->CurrentTime());
					observer->SetPlayerBodyDirByTurn(body_dir);
				}
				break;
				case CT_TurnNeck:
					// 检查转颈命令是否被服务器接收
					// 通过比较服务器返回的转颈计数确认命令执行
					if (observer->Sense().GetTurnNeckCount() == mTurnNeckCount)
					{
					AngleDeg neck_dir = 0.0;
					ComputeInfoAfterTurnNeck(it->mAngle, mSelfState, neck_dir);

					observer->SetPlayerTurnNeckTime(observer->CurrentTime());
					observer->SetPlayerNeckDirByTurnNeck(neck_dir);
				}
				break;
			default:
				break;
			}
		}

		mCommandQueue.clear(); // 清空命令队列
	}
	ActionEffector::CMD_QUEUE_MUTEX.UnLock();
}

/**
 * 检查上周期的命令是否漏发，每周期获得信息后被调用
 * Check commands sent to server in last cycle, see if there are any commands were lost, and print
 * "miss xxx" if there is one.
 * Called after all information has been fetched in every cycle.
 */
void ActionEffector::CheckCommands(Observer *observer)
{
	CheckCommandQueue(observer);
	Reset();

	/**
	* 下面通过sense中server发来的命令数据和自己的纪录数据进行比较，判断是否漏发命令
	* 按照sense中的顺序排列
	*/
	// 检查踢球命令是否漏发
	// 通过比较本地记录与服务器返回的踢球计数判断
	if (observer->Sense().GetKickCount() != mKickCount)
	{
		// 服务器返回的计数小于本地记录，说明命令可能丢失
		if (observer->Sense().GetKickCount() < mKickCount)
		{
			// 输出漏发警告信息
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a kick" << std::endl;
		}
		// 同步本地计数器为服务器返回的值
		mKickCount = observer->Sense().GetKickCount();
	}

	// 检查跑动命令是否漏发
	// 通过比较本地记录与服务器返回的跑动计数判断
	if (observer->Sense().GetDashCount() != mDashCount)
	{
		// 服务器返回的计数小于本地记录，说明命令可能丢失
		if (observer->Sense().GetDashCount() < mDashCount)
		{
			// 输出漏发警告信息
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a dash" << std::endl;
		}
		// 同步本地计数器为服务器返回的值
		mDashCount = observer->Sense().GetDashCount();
	}

	// 检查转身命令是否漏发
	// 通过比较本地记录与服务器返回的转身计数判断
	if (observer->Sense().GetTurnCount() != mTurnCount)
	{
		// 服务器返回的计数小于本地记录，说明命令可能丢失
		if (observer->Sense().GetTurnCount() < mTurnCount)
		{
			// 输出漏发警告信息
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a turn" << std::endl;
		}
		// 同步本地计数器为服务器返回的值
		mTurnCount = observer->Sense().GetTurnCount();
	}

	// 检查说话命令是否漏发
	// 通过比较本地记录与服务器返回的说话计数判断
	if (observer->Sense().GetSayCount() != mSayCount)
	{
		// 服务器返回的计数小于本地记录，说明命令可能丢失
		if (observer->Sense().GetSayCount() < mSayCount)
		{
			// 输出漏发警告信息
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a say" << std::endl;
			// 标记说话命令漏发，用于后续处理
			mIsSayMissed = true;
		}
		// 同步本地计数器为服务器返回的值
		mSayCount = observer->Sense().GetSayCount();
	}

	if (observer->Sense().GetTurnNeckCount() != mTurnNeckCount)
	{
		if (observer->Sense().GetTurnNeckCount() < mTurnNeckCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a turn_neck" << std::endl;
		}
		mTurnNeckCount = observer->Sense().GetTurnNeckCount();
	}

	if (observer->Sense().GetCatchCount() != mCatchCount)
	{
		if (observer->Sense().GetCatchCount() < mCatchCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a catch" << std::endl;
		}
		mCatchCount = observer->Sense().GetCatchCount();
	}

	if (observer->Sense().GetMoveCount() != mMoveCount)
	{
		if (observer->Sense().GetMoveCount() < mMoveCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a move" << std::endl;
		}
		mMoveCount = observer->Sense().GetMoveCount();
	}

	if (observer->Sense().GetChangeViewCount() != mChangeViewCount)
	{
		if (observer->Sense().GetChangeViewCount() < mChangeViewCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a change_view" << std::endl;
		}
		mChangeViewCount = observer->Sense().GetChangeViewCount();
	}

	if (observer->Sense().GetArmCount() != mPointtoCount)
	{
		if (observer->Sense().GetArmCount() < mPointtoCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a pointto" << std::endl;
		}
		mPointtoCount = observer->Sense().GetArmCount();
	}

	if (observer->Sense().GetFocusCount() != mAttentiontoCount)
	{
		if (observer->Sense().GetFocusCount() < mAttentiontoCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a attentionto" << std::endl;
		}
		mAttentiontoCount = observer->Sense().GetFocusCount();
	}

	if (observer->Sense().GetTackleCount() != mTackleCount)
	{
		if (observer->Sense().GetTackleCount() < mTackleCount)
		{
			std::cout << observer->CurrentTime() << " " << PlayerParam::instance().teamName()
			<< " " << observer->SelfUnum() << " miss a tackle" << std::endl;
		}
		mTackleCount = observer->Sense().GetTackleCount();
	}
}

void ActionEffector::Reset()
{
	ActionEffector::CMD_QUEUE_MUTEX.Lock();
	mCommandQueue.clear();
	ActionEffector::CMD_QUEUE_MUTEX.UnLock();

	mIsMutex        = false;

	mIsTurn         = false;
	mIsDash         = false;
	mIsTurnNeck     = false;
	mIsSay          = false;
	mIsAttentionto  = false;
	mIsKick         = false;
	mIsTackle       = false;
	mIsPointto      = false;
	mIsCatch        = false;
	mIsMove         = false;
	mIsChangeView   = false;
	mIsCompression  = false;
	mIsSenseBody    = false;
	mIsScore        = false;
	mIsBye          = false;
	mIsDone         = false;
	mIsClang        = false;
	mIsEar          = false;
	mIsSynchSee     = false;
	mIsChangePlayerType = false;

	mIsSayMissed	= false;
}

void ActionEffector::ResetForScan()
{
	//清空互斥命令
	//清除turn neck命令
	ActionEffector::CMD_QUEUE_MUTEX.Lock();
	if (!mCommandQueue.empty())
	{
		for (std::list<CommandInfo>::iterator it = mCommandQueue.begin(); it != mCommandQueue.end();)
		{
			switch (it->mType)
			{
			case CT_Kick:
				mIsKick = false;
				--mKickCount;
				it = mCommandQueue.erase(it);
				break;
			case CT_Dash:
				mIsDash = false;
				--mDashCount;
				it = mCommandQueue.erase(it);
				break;
			case CT_Move:
				mIsMove = false;
				--mMoveCount;
				it = mCommandQueue.erase(it);
				break;
			case CT_Turn:
				mIsTurn = false;
				--mTurnCount;
				it = mCommandQueue.erase(it);
				break;
			case CT_TurnNeck:
				mIsTurnNeck = false;
				--mTurnNeckCount;
				it = mCommandQueue.erase(it);
				break;
			case CT_Tackle:
				mIsTackle = false;
				--mTackleCount;
				it = mCommandQueue.erase(it);
				break;
			default:
				++it;
				break;
			}
		}
		mIsMutex = false;
	}
	ActionEffector::CMD_QUEUE_MUTEX.UnLock();
}

ViewWidth ActionEffector::GetSelfViewWidthWithQueuedActions()
{
	if (IsChangeView()){
		ViewWidth view_width = mSelfState.GetViewWidth();
		ActionEffector::CMD_QUEUE_MUTEX.Lock();
		for (std::list<CommandInfo>::iterator it = mCommandQueue.begin(); it != mCommandQueue.end(); ++it){
			if (it->mType == CT_ChangeView) {
				view_width = it->mViewWidth;
			}
		}
		ActionEffector::CMD_QUEUE_MUTEX.UnLock();
		return view_width;
	}
	else {
		return mSelfState.GetViewWidth();
	}
}

Vector ActionEffector::GetSelfPosWithQueuedActions()
{
	if (IsDash()) {
		return mSelfState.GetPredictedPosWithDash(1, mDash.GetPower());
	}
	else if (IsMove()){
		Vector player_pos = Vector(0.0, 0.0);
		Vector player_vel = Vector(0.0, 0.0);
		ComputeInfoAfterMove(mMove.GetMovePos(), player_pos, player_vel);
		return player_pos;
	}
	else return mSelfState.GetPredictedPos();
}

Vector ActionEffector::GetSelfVelWithQueuedActions()
{
	if (IsDash()){
		return mSelfState.GetPredictedVelWithDash(1, mDash.GetPower());
	}
	else if (IsMove()){
		Vector player_pos = Vector(0.0, 0.0);
		Vector player_vel = Vector(0.0, 0.0);
		ComputeInfoAfterMove(mMove.GetMovePos(), player_pos, player_vel);
		return player_vel;
	}
	else return mSelfState.GetPredictedVel();
}

AngleDeg ActionEffector::GetSelfBodyDirWithQueuedActions()
{
	if (IsTurn()){
		AngleDeg body_dir = 0.0;
		ComputeInfoAfterTurn(mTurn.GetAngle(), mSelfState, body_dir);
		return body_dir;
	}
	else return mSelfState.GetBodyDir();
}

Vector ActionEffector::GetBallPosWithQueuedActions()
{
	if (IsKick()){
		Vector ball_pos = Vector(0.0, 0.0);
		Vector ball_vel = Vector(0.0, 0.0);
		ComputeInfoAfterKick(mKick.GetPower(), mKick.GetAngle(), mSelfState, mBallState, ball_pos, ball_vel);
		return ball_pos;
	}
	else return mBallState.GetPredictedPos();
}

Vector ActionEffector::GetBallVelWithQueuedActions()
{
	if (IsKick()){
		Vector ball_pos = Vector(0.0, 0.0);
		Vector ball_vel = Vector(0.0, 0.0);
		ComputeInfoAfterKick(mKick.GetPower(), mKick.GetAngle(), mSelfState, mBallState, ball_pos, ball_vel);
		return ball_vel;
	}
	else return mBallState.GetPredictedVel();
}

/**
 * 向server发送命令队列中的命令
 * Send commands in queue to server.
 * \param msg if save_server_msg is on and a c-style string is passed to this method, it will record
 *            this string to msg-log as well.
 */
void ActionEffector::SendCommands(char *msg)
{
	// 根据客户端类型选择不同的命令发送方式
	if (PlayerParam::instance().isCoach() || PlayerParam::instance().isTrainer())
	{
		// 教练/训练师模式：逐条发送命令
		// 教练端需要单独处理每条命令，以便精确控制
		ActionEffector::CMD_QUEUE_MUTEX.Lock();
		// 遍历命令队列中的所有命令
		for (std::list<CommandInfo>::iterator it = mCommandQueue.begin(); it != mCommandQueue.end(); ++it)
		{
			// 检查命令是否有效且属于当前周期
			// 只有当前周期的有效命令才需要发送
			if (it->mType != CT_None && it->mTime == mWorldState.CurrentTime()) //TODO: seg fault here
			{
				// 记录命令发送计数，用于网络测试和统计
				NetworkTest::instance().SetCommandSendCount((*it));
			}
			// 检查命令字符串是否为空
			if (!it->mString.empty())
			{
				// 根据调试模式选择不同的输出方式
				if (PlayerParam::instance().DynamicDebugMode())
				{
					// 动态调试模式：直接输出命令到标准错误流
					std::cerr << std::endl << it->mString.c_str();
				}
				else if (UDPSocket::instance().Send(it->mString.c_str()) < 0) // 发送命令
				{
					// 网络发送失败，输出错误信息
					PRINT_ERROR("UDPSocket error!");
				}
			}
			// 检查是否需要记录服务器消息
			// 用于日志记录和调试分析
			if (PlayerParam::instance().SaveServerMessage() && msg != 0) //说明要记录命令信息
			{
				// 将命令字符串追加到消息缓冲区
				strcat(msg, it->mString.c_str());
			}
		}
		ActionEffector::CMD_QUEUE_MUTEX.UnLock();
	}
	else
	{
		// 球员模式：将所有命令合并后一次性发送
		// 球员端通常需要在一个周期内发送多个动作命令
		static char command_msg[MAX_MESSAGE];

		// 初始化命令消息缓冲区
		command_msg[0] = '\0';
		ActionEffector::CMD_QUEUE_MUTEX.Lock();
		// 遍历命令队列，合并当前周期的所有命令
		for (std::list<CommandInfo>::iterator it = mCommandQueue.begin(); it != mCommandQueue.end(); ++it)
		{
			// 检查命令是否有效且属于当前周期
			if (it->mType != CT_None && it->mTime == mWorldState.CurrentTime())
			{
				// 将命令字符串追加到合并缓冲区
				strcat(command_msg, it->mString.c_str());
				// 记录命令发送计数，用于网络测试和统计
				NetworkTest::instance().SetCommandSendCount((*it));
			}
		}
		ActionEffector::CMD_QUEUE_MUTEX.UnLock();

		// 检查是否有命令需要发送
		if (command_msg[0] != '\0')
		{
			// 根据调试模式选择不同的输出方式
			if (PlayerParam::instance().DynamicDebugMode())
			{
				// 动态调试模式：直接输出合并后的命令到标准错误流
				std::cerr << std::endl << command_msg;
			}
			else if (UDPSocket::instance().Send(command_msg) < 0) // 发送命令
			{
				// 网络发送失败，输出错误信息
				PRINT_ERROR("UDPSocket error!");
			}
		}
		// 检查是否需要记录服务器消息
		// 用于日志记录和调试分析
		if (PlayerParam::instance().SaveServerMessage() && msg != 0) //说明要记录命令信息
		{
			// 将合并后的命令字符串追加到消息缓冲区
			strcat(msg, command_msg);
		}
	}
}

// end of ActionEffector.cpp

