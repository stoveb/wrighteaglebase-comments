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
 *       notice * this list of conditions and the following disclaimer in the        *
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
 * @file WorldState.h
 * @brief 世界状态类定义 - WrightEagleBase 的核心数据模型
 * 
 * 本文件定义了 WorldState 类，它是 WrightEagleBase 系统的核心数据模型：
 * 1. 存储完整的比赛世界状态信息
 * 2. 管理球员和球的状态数据
 * 3. 提供历史状态查询接口
 * 4. 支持对手反算功能
 * 
 * WorldState 是整个系统的基础数据结构，为决策系统提供准确、
 * 完整、及时的世界信息。它采用面向对象的设计，将复杂的
 * 足球比赛状态抽象为易于管理和访问的数据结构。
 * 
 * 主要功能：
 * - 球员状态管理（队友和对手）
 * - 球状态管理
 * - 比赛模式和时间管理
 * - 历史状态查询
 * - 对手视角转换
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#ifndef __WORLDSTATE_H__
#define __WORLDSTATE_H__

#include <vector>
#include <cstdlib>
#include "Observer.h"
#include "PlayerState.h"
#include "BallState.h"
#include "CommunicateSystem.h"

// 前向声明，避免循环依赖
class PlayerObserver;
class UnknownPlayerObserver;
class UnknownUnumPlayerObserver;

/**
 * @brief point命令要求的最大角度常量
 * 
 * 定义了球员使用point命令时的最大角度限制。
 * 这是一个系统约定，用于限制point指令的有效范围。
 * 
 * @note 120度表示球员可以向左右各偏转60度进行指向
 */
#define MAX_POINT_DIR 120

// 前向声明
class HistoryState;
class BeliefState;

/**
 * @brief 世界状态类 - WrightEagleBase 的核心数据模型
 * 
 * WorldState 类是整个 WrightEagleBase 系统的核心数据结构，
 * 负责管理和维护完整的比赛世界状态信息。
 * 
 * 设计特点：
 * 1. **对称性设计**: 信息用队友和对手区分，不依赖具体号码
 * 2. **视角无关**: 本质上与自己的边（左/右）无关
 * 3. **历史支持**: 支持历史状态查询和分析
 * 4. **反算能力**: 支持从对手视角重新构建世界状态
 * 
 * 主要数据：
 * - 所有球员的状态（队友11个 + 对手11个）
 * - 球的状态（位置、速度、所有权等）
 * - 比赛状态（模式、时间、比分等）
 * - 历史状态缓存
 * 
 * 使用方式：
 * - 通过 Observer 更新状态
 * - 提供统一的访问接口
 * - 支持读写操作
 * 
 * @note 这是系统的核心数据结构，所有决策都基于此类的数据
 * @note 线程安全需要注意，通常配合Observer的锁机制使用
 */
class WorldState {
    // 友元类声明，允许 WorldStateUpdater 直接访问私有成员
    friend class WorldStateUpdater;
    
    // 禁止拷贝构造，确保世界状态的唯一性
    WorldState(const WorldState &);

public:
    /**
     * @brief 构造函数
     * 
     * 初始化世界状态对象，创建所有球员和球的状态。
     * 
     * @param history_state 历史状态管理器指针，用于历史状态查询
     */
    WorldState(HistoryState * history_state = 0);
    
    /**
     * @brief 虚析构函数
     * 
     * 确保正确的多态析构，虽然当前为空实现，
     * 但为将来扩展提供接口。
     */
    virtual ~WorldState() {}

    /**
     * @brief 从Observer更新世界状态
     * 
     * 这是世界状态的主要更新入口，通过Observer对象
     * 获取最新的服务器信息并更新内部状态。
     * 
     * 更新过程：
     * 1. 解析Observer中的视觉信息
     * 2. 更新球员位置和状态
     * 3. 更新球的位置和状态
     * 4. 更新比赛模式和时间
     * 5. 处理通信信息
     * 
     * @param observer 观察者对象指针，包含从服务器接收的最新信息
     * 
     * @note 这是每个感知周期的核心操作
     * @note 更新过程需要保证原子性和一致性
     */
    void UpdateFromObserver(Observer *observer);

    /**
     * @brief 从指定世界状态创建反向视角
     * 
     * 基于当前世界状态创建一个供反算对手时用的世界状态。
     * 这个功能主要用于分析和预测对手的行为。
     * 
     * 转换内容：
     * 1. 队友和对手角色互换
     * 2. 坐标系统镜像翻转
     * 3. 时间和比赛模式保持不变
     * 4. 守门员号码互换
     * 
     * @param world_state 源世界状态指针
     * 
     * @note 主要用于对手建模和策略分析
     * @note 转换后的状态用于对手视角的决策模拟
     */
    void GetReverseFrom(WorldState *world_state);

    /**
     * @brief 获取指定号码的球员状态（只读）
     * 
     * 根据球员号码获取对应的球员状态，正数表示队友，
     * 负数表示对手。这是最通用的球员状态访问接口。
     * 
     * @param i 球员号码，正数为队友，负数为对手
     * @return const PlayerState& 球员状态的常量引用
     * 
     * @example GetPlayer(1) 获取1号队友
     * @example GetPlayer(-2) 获取2号对手
     */
    const PlayerState & GetPlayer(const Unum & i) const { return i > 0? mTeammate[i]: mOpponent[-i]; }
    
    /**
     * @brief 获取所有球员的列表
     * 
     * 返回包含所有球员（队友+对手）状态指针的列表。
     * 主要用于需要遍历所有球员的场景。
     * 
     * @return const std::vector<PlayerState*>& 所有球员状态指针的列表
     * 
     * @note 列表顺序固定：队友1-11号，对手1-11号
     */
    const std::vector<PlayerState *> & GetPlayerList() const { return mPlayerList; }

    /**
     * @brief 获取球的状态（只读）
     * 
     * @return const BallState& 球状态的常量引用
     */
    const BallState & GetBall() const { return mBall; }
    
    /**
     * @brief 获取当前世界时间
     * 
     * @return const Time& 当前时间的常量引用
     */
    const Time & CurrentTime() const { return mCurrentTime; }
    
    /**
     * @brief 设置当前世界时间
     * 
     * 主要用于时间同步和异常处理。
     * 
     * @param time 要设置的时间
     */
    void  SetCurrentTime(const Time & time) { mCurrentTime = time; }
    
    /**
     * @brief 获取开球模式
     * 
     * @return KickOffMode 当前的开球模式
     */
    KickOffMode GetKickOffMode() const {return mKickOffMode;}

    /**
     * @brief 获取当前比赛模式
     * 
     * 比赛模式包括：正常比赛、角球、任意球、点球等。
     * 
     * @return PlayMode 当前比赛模式
     */
    PlayMode GetPlayMode() const { return mPlayMode; }
    
    /**
     * @brief 获取上一个比赛模式
     * 
     * 用于检测比赛模式的变化，便于状态转换处理。
     * 
     * @return PlayMode 上一个比赛模式
     */
    PlayMode GetLastPlayMode() const { return mLastPlayMode;}
    
    /**
     * @brief 获取当前比赛模式的开始时间
     * 
     * @return const Time& 比赛模式开始时间的常量引用
     */
    const Time & GetPlayModeTime() const { return mPlayModeTime; }

    /**
     * @brief 获取球的状态（可写）
     * 
     * @return BallState& 球状态的引用
     * 
     * @note 主要用于 WorldStateUpdater 更新球状态
     */
    BallState & Ball() { return mBall; }
    
    /**
     * @brief 获取指定号码的球员状态（可写）
     * 
     * @param i 球员号码，正数为队友，负数为对手
     * @return PlayerState& 球员状态的引用
     */
    PlayerState & Player(const Unum & i) {  return i > 0? mTeammate[i]: mOpponent[-i]; }
    
    /**
     * @brief 获取指定号码的队友状态（可写）
     * 
     * @param i 队友号码（1-11）
     * @return PlayerState& 队友状态的引用
     */
    PlayerState & Teammate(const Unum & i) {  return mTeammate[i]; }
    
    /**
     * @brief 获取指定号码的对手状态（可写）
     * 
     * @param i 对手号码（1-11）
     * @return PlayerState& 对手状态的引用
     */
    PlayerState & Opponent(const Unum & i) { return mOpponent[i]; }

    // === 只读球员状态访问接口 ===
    
    /**
     * @brief 获取指定号码的队友状态（只读）
     * 
     * @param i 队友号码（1-11）
     * @return const PlayerState& 队友状态的常量引用
     */
    const PlayerState & GetTeammate(const Unum & i) const { return mTeammate[i]; }
    
    /**
     * @brief 获取指定号码的对手状态（只读）
     * 
     * @param i 对手号码（1-11）
     * @return const PlayerState& 对手状态的常量引用
     */
    const PlayerState & GetOpponent(const Unum & i) const {  return mOpponent[i]; }

    // === 守门员信息访问接口 ===
    
    /**
     * @brief 获取队友守门员的号码
     * 
     * @return Unum 队友守门员的号码，0表示未知
     */
    Unum GetTeammateGoalieUnum() const { return mTeammateGoalieUnum; }
    
    /**
     * @brief 获取对手守门员的号码
     * 
     * @return Unum 对手守门员的号码，0表示未知
     */
    Unum GetOpponentGoalieUnum() const { return mOpponentGoalieUnum; }

    int GetTeammateScore() const { return mTeammateScore; }
    int GetOpponentScore() const { return mOpponentScore; }

	/**
	 * get record history
	 * @param i stand for i cycles before ,if i = 0 ,it means that you get current worldstate
	 */
	WorldState * GetHistory(int i) const;

	/**
	 * get time cycle  before current time
	 * warning : this should not be used when you get WorldState from history  for example: GetHistory(1)->GetTimeBeforeCurrent(1) is not correct
	 * @param cycle should be >= 1 and <= HISTORYSIZE
	 */
	Time GetTimeBeforeCurrent(int cycle = 1) const;

	bool IsBallDropped() const { return mIsBallDropped; }

private:
	HistoryState * mpHistory;

    Time mCurrentTime;

	KickOffMode mKickOffMode;
    PlayMode mPlayMode;
	PlayMode mLastPlayMode;
	Time mPlayModeTime;
	bool mIsBallDropped;

    BallState mBall;
    PlayerArray<PlayerState> mTeammate;
    PlayerArray<PlayerState> mOpponent;
    std::vector<PlayerState *> mPlayerList;

    Unum mTeammateGoalieUnum;
    Unum mOpponentGoalieUnum;

	int mTeammateScore;
	int mOpponentScore;

	bool mIsCycleStopped;
};

/**
 * 负责WorldState的更新
 */
class WorldStateUpdater {
    WorldStateUpdater(const WorldStateUpdater &);

public:
    WorldStateUpdater(Observer *observer, WorldState *world_state):
    	mpObserver( observer ),
    	mpWorldState( world_state ),
    	mSelfSide( mpObserver? mpObserver->OurSide(): '?' ),
    	mSelfUnum( mpObserver? mpObserver->SelfUnum(): 0 )
    {
		mBallConf = 1;
		mPlayerConf = 1;
		mSightDelay = 0;
		mIsHearBallPos = false;
		mIsHearBallVel = false;
    }

     /**
      * 更新例程
      */
    void Run();

private:
     /** 一些只在更新时用的接口 */
    inline BallState   & Ball();
    inline PlayerState & Teammate(Unum i);
    inline PlayerState & Opponent(Unum i);
    inline PlayerState & SelfState();

	/**methods*/
    inline const BallState & GetBall();
    inline const PlayerState & GetTeammate(Unum i);
    inline const PlayerState & GetOpponent(Unum i);
    inline const PlayerState & GetSelf();
	inline AngleDeg  GetNeckGlobalDirFromSightDelay(int sight_delay = 0);
	inline Vector    GetSelfVelFromSightDelay(int sight_delay = 0);
    inline Unum GetSelfUnum();
    inline char GetSelfSide();

private:
    /** 各种更新方法 */
	void UpdateWorldState();

private:
    /**
     * 获得当前可见的最近的队友
     * 没有则返回的号码为0
     */
    Unum GetSeenClosestTeammate();

    /**
     * 获得当前可见的最近的对手
     * 没有则返回的号码为0
     */
    Unum GetSeenClosestOpponent();

	/**
	 * get closest player
	 *  just for EvaluatePlayer
	 */
	Unum GetClosestOpponent();

	/**
	 * get closest player to ball
	 * @return is teammate ? > 0 : < 0
	 */
	Unum GetClosestPlayerToBall();


//=======================================内部更新的接口================================================
public:
    /** 更新Action需要的信息 */
    void UpdateActionInfo();

private:
	/**update sight delay */
	void UpdateSightDelay();

    /** 更新自己信息 */
    void UpdateSelfInfo();

    /** 更新自己的sense */
    void UpdateSelfSense();

    /** 更新所有的队员的信息 */
    void UpdateKnownPlayers();

    /** 更新未知球员的信息 */
    void UpdateUnknownPlayers();

	bool UpdateMostSimilarPlayer(const Vector & pos ,int index);

    /** 更新某一个特定的队员 */
    void UpdateSpecificPlayer(const PlayerObserver& player , Unum unum , bool is_teammate);

    /** 更新某个特定的未知的球员 */
    void UpdateSpecificUnknownPlayer(const UnknownPlayerObserver& player , Unum num , bool is_teammate);

    /** 更新球的信息 */
    void UpdateBallInfo();

    /** 自动将delay加1 */
    void AutoDelay(int delay_add);

    /** 更新特定的位置从特定状态 */
    void UpdateInfoFromPlayMode();

    /** 更新场上的信息 */
    void UpdateFieldInfo();

    /** 根据听觉更新 */
    void UpdateFromAudio();

    /** 更加collide更新 */
    void UpdateInfoWhenCollided();
//=========================================更新和预测的计算工具=============================================
private:
    /**计算自己头的角度*/
    bool ComputeSelfDir(double& angle);

    /**计算自己的位置*/
    bool ComputeSelfPos(Vector& vec , double& eps);

    /**计算下一个周期*/
    bool ComputeNextCycle(MobileState& ms , double decay);

    double ComputePlayerMaxDist(const PlayerState& state);

    /**compute the player may see or not. just for update unknown player */
    bool ComputePlayerMaySeeOrNot(const PlayerState& state);

	/**compute conf from cycle*/
	double ComputeConf(double delay , int cycle);

	/** estimate to now*/
	void EstimateToNow();

      /** 根据运动规律和上周期执行的动作，模拟一周期 */
    void EstimateWorld(bool is_estimate_to_now = false , int cycle = 0);

      /** 预估自己的信息 */
    void EstimateSelf(bool is_estimate_to_now = false , int cycle = 0);

      /** 预估球的信息 */
    void EstimateBall(bool is_estimate_to_now = false , int cycle = 0);

      /** 预估除了自己的其它球员的信息 */
    void EstimatePlayers();

	/**
	 * recompute conf
	 */
	void EvaluateConf();

	/**
	 * Evaluate ball forgotten
	 */
	void EvaluateBall();

	/**
	 * evaluate forget ball
	 */
	void EvaluateForgetBall(bool use_memory);

	/**
	 * evaluate player
	 */
	void EvaluatePlayer(PlayerState& player);

	bool ShouldSee(const Vector & pos); //本应该看到/感知到的

	Vector GetNearSidePos(const Vector & pos, const Vector *expected_pos = 0);

	/**
	 * evaluate to forget player
	 */
	void EvaluateForgetPlayer(PlayerState& player);

	/**
	 * calc player go to point
	 */
	double CalcPlayerGotoPoint(PlayerState& player ,double dist);

	/**
	 * tell whether can see or not
	 */
	bool ComputeShouldSeeOrNot(Vector pos);

	void MaintainPlayerStamina();

	void MaintainConsistency();

	void UpdateOtherKick();
private:
    Observer * const mpObserver;
    WorldState * const mpWorldState;

    char mSelfSide;
    Unum mSelfUnum;

	double mPlayerConf;
	double mBallConf;
	int mSightDelay;

public:
	static const double KICKABLE_BUFFER;
	static const double CATCHABLE_BUFFER;

private:
	bool  mIsOtherKick;
	int   mOtherKickUnum; // >0 : teammate , < 0 opponent;

	bool  mIsOtherMayKick;

	bool mIsHearBallPos;
	bool mIsHearBallVel;

public:
    /** 计算球员的铲球成功率，因为在这里更新，所以放在这里比较好 */
    double ComputeTackleProb(const Unum & unum, bool foul = false);
};

/**
 * 用来设置worldstate里的数据，以供反算队友或对手，用完在析构函数里面恢复状态
 */
/**
 * EXAMPLE:
 * void Behavior*Planner::Plan() {
 *      ActiveBehavior teammate_behavior;
 *
 *      { //用来指明反算存在的代码范围，以使恢复正常进行
 *      	WorldStateSetter setter(mWorldState);
 *      	setter.Ball().UpdatePos(Vector(0, 0));
 *      	//... ...
 *          setter.IncStopTime(); //可以开始反算了
 *      	Agent * agent = mAgent.CreateTeammateAgent(mStrategy.GetSureTm());
 *          ActiveBehaviorList bhv_list;
 *			{ //用于标示Planner的作用域，作用域结束后Planner将被撤销。
 *			  //如此安排是因为必须先撤销Planner再撤销Agent
 *      		BehaviorPassPlanner(*agent).Plan(bhv_list);
 *      	}
 *          teammate_behavior = bhv_list.front();
 *          delete agent;
 *          //这里会掉用setter的析构函数恢复世界状态
 *      }
 *
 *      //do_some_thing_about teammate_behavior
 *      //... ...
 * }
 *
 */
class WorldStateSetter {
public:
	WorldStateSetter (WorldState & world_state):
		mWorldState(world_state),
		mpBackupBallState(0),
		mBackupTime(mWorldState.CurrentTime())
	{
	}

	~WorldStateSetter() {
		mWorldState.SetCurrentTime(mBackupTime);
		if (mpBackupBallState != 0) {
			mWorldState.Ball() = *mpBackupBallState;
			delete mpBackupBallState;
		}
		for (int i = 0; i < TEAMSIZE * 2 + 1; ++i) {
			if (mpBackupPlayerState[i] != 0) { //这个备份过了，先在恢复它
				Assert(i != 0);
				if (i <= TEAMSIZE) {
					mWorldState.Teammate(i) = *mpBackupPlayerState[i];
					delete mpBackupPlayerState[i];
				}
				else {
					mWorldState.Opponent(i - TEAMSIZE) = *mpBackupPlayerState[i];
					delete mpBackupPlayerState[i];
				}
			}
		}
	}

	/**
	 * 要用下面的接口来改state的数据，否则不能备份
	 * @return
	 */
	BallState   & Ball() { if (pBall() == 0) pBall() = new BallState(mWorldState.Ball()); return mWorldState.Ball(); }
	PlayerState & Teammate(Unum i) { if (pTeammate(i) == 0) pTeammate(i) = new PlayerState(mWorldState.Teammate(i)); return mWorldState.Teammate(i); }
	PlayerState & Opponent(Unum i) { if (pOpponent(i) == 0) pOpponent(i) = new PlayerState(mWorldState.Opponent(i)); return mWorldState.Opponent(i); }

	void SetBallInfo(const Vector & pos, const Vector & vel) {
		Ball().UpdatePos(pos);
		Ball().UpdateVel(vel);
	}

	void SetTeammateInfo(const Unum & num, const Vector & pos, const AngleDeg & body_dir, const Vector & vel) {
		Teammate(num).UpdatePos(pos);
		Teammate(num).UpdateBodyDir(body_dir);
		Teammate(num).UpdateVel(vel);
	}

	void SetOpponentInfo(const Unum & num, const Vector & pos, const AngleDeg & body_dir, const Vector & vel) {
		Opponent(num).UpdatePos(pos);
		Opponent(num).UpdateBodyDir(body_dir);
		Opponent(num).UpdateVel(vel);
	}
	void IncStopTime() { WorldStateUpdater(0, & mWorldState).UpdateActionInfo(); mWorldState.SetCurrentTime(Time(mWorldState.CurrentTime().T(), mWorldState.CurrentTime().S() + 1)); }

private:
	BallState   *& pBall() { return mpBackupBallState; }
	PlayerState *& pTeammate(Unum i) { Assert(i >= 1 && i <= TEAMSIZE); return mpBackupPlayerState[i]; }
	PlayerState *& pOpponent(Unum i) { Assert(i >= 1 && i <= TEAMSIZE); return mpBackupPlayerState[TEAMSIZE + i]; }

private:
	WorldState & mWorldState;

	BallState   * mpBackupBallState;
	Array<PlayerState *, 1 + TEAMSIZE * 2, true> mpBackupPlayerState;
	Time mBackupTime;
};

/**记录StateWorld历史信息*/
class HistoryState
{
public:
    HistoryState(): mNum(0) {
    }

    enum {
    	HISTORY_SIZE = 10
    };

    /**将当前世界加入队列
     * @param 现实世界的引用
     */
    void UpdateHistory(const WorldState & world);

    /**获得之前的数组
     * @param 取值范围为1~HISTORYSIZE，代表从最新到最前的搜索
     */
    WorldState *GetHistory(int num);

private:
    /**记录StateWorld的数组*/
    Array<WorldState, HISTORY_SIZE> mRecord;

    /**记录数组当前的置顶前一个空白*/
    int mNum;
};

#endif /* WORLDSTATE_H_ */
