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
 * @file BehaviorBase.h
 * @brief 行为系统基础类定义 - WrightEagleBase 行为架构的核心
 * 
 * 本文件定义了 WrightEagleBase 行为系统的基础架构：
 * 1. 行为类型枚举 (BehaviorType)
 * 2. 行为细节类型枚举 (BehaviorDetailType)
 * 3. 活跃行为类 (ActiveBehavior)
 * 4. 行为工厂类 (BehaviorFactory)
 * 5. 行为规划器和执行器基类
 * 
 * 行为系统采用分离式设计：
 * - Planner 类负责决策规划（Plan方法）
 * - Executer 类负责执行（Execute方法）
 * - ActiveBehavior 类封装行为数据和评估
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#ifndef BEHAVIORBASE_H_
#define BEHAVIORBASE_H_

#include <list>
#include <string>
#include "Geometry.h"
#include "ActionEffector.h"
#include "Formation.h"

// 前向声明，避免循环依赖
class WorldState;
class BallState;
class PlayerState;
class InfoState;
class PositionInfo;
class InterceptInfo;
class Strategy;
class Analyser;
class Agent;

/**
 * @brief 行为类型枚举
 * 
 * 定义了 WrightEagleBase 中所有可能的行为类型，
 * 这些类型在决策树中按优先级排序：
 * 
 * 优先级从高到低：
 * BT_Penalty > BT_Setplay > BT_Attack > BT_Defense/BT_Goalie
 * 
 * @note BT_None 表示无行为，用于初始化或错误情况
 * @note BT_Max 用于数组边界检查和循环控制
 */
enum BehaviorType {
	BT_None,           ///< 无行为，用于初始化或错误情况

	BT_Penalty,         ///< 点球相关行为（最高优先级）
	BT_Goalie,          ///< 守门员专用行为
	BT_Setplay,         ///< 定位球行为（角球、任意球等）
	BT_Position,        ///< 位置行为（移动到指定位置）
	BT_Dribble,         ///< 带球行为
	BT_Hold,            ///< 持球行为（控制球权）
	BT_Pass,            ///< 传球行为
	BT_Shoot,           ///< 射门行为
	BT_Intercept,       ///< 截球行为
	BT_Formation,       ///< 阵型行为（保持阵型）
	BT_Block,           ///< 阻挡行为（阻挡对手）
	BT_Mark,            ///< 标记行为（盯防对手）

	BT_Max              ///< 行为类型数量，用于边界检查
};

/**
 * @brief 行为细节类型枚举
 * 
 * 定义了每种行为类型的具体实现方式，
 * 用于更精确地描述和区分不同的行为策略。
 * 
 * 这些细节类型帮助系统：
 * 1. 选择最优的具体行为实现
 * 2. 记录和分析行为模式
 * 3. 优化行为执行策略
 */
enum BehaviorDetailType {
	BDT_None,           ///< 无细节类型

	// 截球行为细节
	BDT_Intercept_Normal,///< 普通截球

	// 传球行为细节
    BDT_Pass_Ahead,     ///< 向前传球（传向空档）
    BDT_Pass_Direct,    ///< 直接传球（传向队友）
    BDT_Pass_Clear,     ///< 解围传球（清除危险）

	// 带球行为细节
	BDT_Dribble_Fast,    ///< 快速带球
	BDT_Dribble_Normal,  ///< 普通带球

	// 位置行为细节
    BDT_Position_Normal,///< 普通位置移动
    BDT_Position_Rush,  ///< 快速位置移动

	// 射门行为细节
	BDT_Shoot_Tackle,   ///< 铲球射门

	// 持球行为细节
	BDT_Hold_Turn,      ///< 转身持球
	BDT_Hold_Kick,       ///< 踢球持球

	// 守门员行为细节
    BDT_Goalie_Position,///< 守门员位置调整
    BDT_Goalie_Catch,    ///< 守门员接球
    BDT_Goalie_Tackle,   ///< 守门员铲球

	// 定位球行为细节
    BDT_Setplay_Move,   ///< 定位球移动
    BDT_Setplay_Scan,   ///< 定位球扫描
    BDT_Setplay_GetBall ///< 定位球取球
};

class BehaviorExecutable;

class BehaviorFactory {
	BehaviorFactory();
	virtual ~BehaviorFactory();

public:
	typedef BehaviorExecutable * (*BehaviorCreator)(Agent & agent);
	static BehaviorFactory & instance();

	/**
	* 创建行为
	* @param agent
	* @param type
	* @return
	*/
	BehaviorExecutable * CreateBehavior(Agent & agent, BehaviorType type);

	/**
	* 注册行为创建函数，只有具有执行效果的行为需要注册，比如 BehaviorAttackPlanner, BehaviorDefensePlanner 不需要注册
	* @param type 行为类型
	* @param creator 创建函数
	* @return
	*/
	bool RegisterBehavior(BehaviorType type, BehaviorCreator creator, const char *behavior_name);

	const char *GetBehaviorName(BehaviorType type) {
		return mNameMap[type].c_str();
	}

private:
	Array<BehaviorCreator, BT_Max, true> mCreatorMap;
	Array<std::string, BT_Max> mNameMap;
};

class ActiveBehavior
{
	/** behavior type and agent pointer */
	BehaviorType mType;
	Agent * mpAgent;

public:

	ActiveBehavior(Agent & agent, BehaviorType type, BehaviorDetailType detail_type = BDT_None) :
		mType(type), mpAgent(&agent), mEvaluation(0.0),
		mKickCycle(0), mAngle(0), mTarget(Vector(0.0, 0.0)),
		mPower(0.0), mDistance(0.0), mKickSpeed(0.0),
		mFoul(false), mDetailType(detail_type),
		mBuffer(0.0)
	{

	}

	void Clear() {
		mEvaluation = 0.0;
	}

	const BehaviorType & GetType() const {
		return mType;
	}
	void SetType(const BehaviorType &type) {
		mType = type;
	}

	Agent & GetAgent() const {
		return *mpAgent;
	}

	bool Execute(); //不依赖于指向 behavior 的指针，但要求 BehaviorType 无误
	void SubmitVisualRequest(double plus);

	bool operator>(const ActiveBehavior & ab) const {
		return this->mEvaluation > ab.mEvaluation;
	}
	bool operator ==(const ActiveBehavior & ab) const {
		return this->mEvaluation == ab.mEvaluation;
	}
	bool operator <(const ActiveBehavior & ab) const {
		return this->mEvaluation < ab.mEvaluation;
	}

	double mEvaluation;

	//behavior detail
	int mKickCycle; // 踢球的周期，可能多脚踢球
	AngleDeg mAngle; // 角度
	Vector mTarget; // 目标点地位置，可以是把球踢到该点和自己跑到该点等
	double mPower; // dash的power
	double mDistance; // 目标点到自己的距离
	double mKickSpeed; // kick的球速
	bool mFoul; //是否要故意犯规铲球

	KeyPlayerInfo mKeyTm; // teammate
	KeyPlayerInfo mKeyOpp; // opponent
	KeyPlayerInfo mKeyOppGB; // opponent to get ball
	KeyPlayerInfo mKeyOppGT; // opponent to go through

	BehaviorDetailType mDetailType; //细节类型
	double mBuffer; //有些行为执行时的buffer是在plan时算好的，要先存到这个变量里
};

class BehaviorAttackData {
public:
	BehaviorAttackData(Agent & agent);
	virtual ~BehaviorAttackData();

	Agent & mAgent;

	const WorldState & mWorldState;

	const BallState & mBallState;
	const PlayerState & mSelfState;

	PositionInfo & mPositionInfo;
	InterceptInfo & mInterceptInfo;

	Strategy & mStrategy;

    Formation & mFormation;
};

class BehaviorDefenseData: public BehaviorAttackData {
public:
	BehaviorDefenseData(Agent & agent);
	virtual ~BehaviorDefenseData();

	Analyser & mAnalyser;
};

template < class BehaviorDataType >
class BehaviorPlannerBase: public BehaviorDataType {
	BehaviorPlannerBase(const BehaviorPlannerBase &);

public:
	BehaviorPlannerBase(Agent & agent): BehaviorDataType(agent) {}
	virtual ~BehaviorPlannerBase() {}

	/**
	* 做决策，产生最好的ActiveBehavior，存到behavior_list里面
	*/
	virtual void Plan(std::list<ActiveBehavior> & behavior_list) = 0;

public:
	const std::list<ActiveBehavior> & GetActiveBehaviorList() {
		return mActiveBehaviorList;
	}

protected:
	std::list<ActiveBehavior> mActiveBehaviorList; // record the active behaviors for each high level behavior
};

class BehaviorExecutable {
	BehaviorExecutable(const BehaviorExecutable&);

public:
	BehaviorExecutable() {}
	virtual ~BehaviorExecutable() {}

	/**
	* 执行 ActiveBehavior
	* @param act ActiveBehavior
	* @return
	*/
	virtual bool Execute(const ActiveBehavior & act_bhv) = 0; //每周期只有一个行为被执行
	virtual void SubmitVisualRequest(const ActiveBehavior & act_bhv, double plus = 0.0) { //但可以有多个行为提交视觉请求
		(void) act_bhv;
		(void) plus;
	}

	/// behavior factory interfaces
	template <class BehaviorDerived >
	static BehaviorExecutable* Creator(Agent & agent) { return new BehaviorDerived(agent); }

	template<class BehaviorDerived>
	static bool AutoRegister() {
		std::string behavior_name;

#ifndef WIN32
		std::string function_name = __PRETTY_FUNCTION__;

		size_t end = function_name.find("Executer");
		if (end != std::string::npos) {
			size_t begin = end;
			while (begin && isalpha(function_name[begin--])) ;
			begin += 2;
			if (begin < end) {
				behavior_name = function_name.substr(begin, end - begin);
			}
		}
#endif

		return BehaviorFactory::instance().RegisterBehavior(
				BehaviorDerived::BEHAVIOR_TYPE, Creator<BehaviorDerived>, behavior_name.c_str());
	}
};

template < class BehaviorDataType >
class BehaviorExecuterBase: public BehaviorDataType, public BehaviorExecutable {
	BehaviorExecuterBase(const BehaviorExecuterBase &);

public:
	BehaviorExecuterBase(Agent & agent): BehaviorDataType(agent) {}
	virtual ~BehaviorExecuterBase() {}
};


typedef std::list<ActiveBehavior> ActiveBehaviorList;
typedef std::list<ActiveBehavior>::iterator ActiveBehaviorPtr;

#define TeammateFormationTactic(TacticName) (*(FormationTactic##TacticName *)mFormation.GetTeammateTactic(FTT_##TacticName))
#define OpponentFormationTactic(TacticName) (*(FormationTactic##TacticName *)mFormation.GetOpponentTactic(FTT_##TacticName))

#endif /* BEHAVIORBASE_H_ */
