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
  * @file BehaviorSetplay.cpp
  * @brief 定位球（Setplay）行为实现
  *
  * 该文件实现定位球阶段（含开球前、任意球/角球/球门球/界外球等非 PlayOn 模式）的行为：
  * - `BehaviorSetplayExecuter`：根据 `ActiveBehavior::mDetailType` 执行具体动作（move/turn/getball）。
  * - `BehaviorSetplayPlanner`：在非 `PM_Play_On` 时生成合适的定位球行为，决定：
  *   - 开球前：走位到阵型开球站位点，站到位后转身扫描；
  *   - 我方定位球且自己最近：不可踢则取球、可踢则禁止带球并进行扫描/（特殊条件下）守门员微调站位。
  *
  * @note 本文件仅增加注释，不改变任何原有决策逻辑。
  */

#include "BehaviorSetplay.h"
#include "Formation.h"
#include "Kicker.h"
#include "Dasher.h"
#include "VisualSystem.h"
#include "BehaviorPass.h"
#include "Geometry.h"
#include "ServerParam.h"
#include "BehaviorShoot.h"
#include "BehaviorIntercept.h"
#include "Evaluation.h"
#include "Utilities.h"
#include <stdio.h>
#include <algorithm>
using namespace std;

// === 定位球行为类型定义 ===
const BehaviorType BehaviorSetplayExecuter::BEHAVIOR_TYPE = BT_Setplay;

namespace
{
bool ret = BehaviorExecutable::AutoRegister<BehaviorSetplayExecuter>();
}

/**
 * @brief 定位球执行器构造函数
 *
 * @param agent 代理对象
 */
BehaviorSetplayExecuter::BehaviorSetplayExecuter(Agent & agent): BehaviorExecuterBase<BehaviorAttackData>( agent )
{
	Assert(ret);
}

/**
 * @brief 定位球执行器析构函数
 */
BehaviorSetplayExecuter::~BehaviorSetplayExecuter(void)
{
}

/**
 * @brief 执行定位球行为
 *
 * 该函数根据 `setplay.mDetailType` 执行对应动作。
 * - `BDT_Setplay_Move`：使用 `move` 指令瞬移/走位到目标点（取决于当前 play mode）。
 * - `BDT_Setplay_Scan`：转身扫描（开球前需要主动转身；其他定位球阶段则交给视觉系统）。
 * - `BDT_Setplay_GetBall`：调用 Dasher 的取球动作（通常用于“最近且需要去拿球”的队友）。
 *
 * @param setplay 定位球行为
 * @return 执行结果
 */
bool BehaviorSetplayExecuter::Execute(const ActiveBehavior &setplay)
{
	if (mWorldState.GetPlayMode() == PM_Before_Kick_Off) {
		if (setplay.mDetailType == BDT_Setplay_Move) {
			return mAgent.Move(setplay.mTarget);
		}
		else if (setplay.mDetailType == BDT_Setplay_Scan) {
			VisualSystem::instance().ForbidDecision(mAgent);
			return mAgent.Turn(50.0);
		}
		else {
			PRINT_ERROR("Setplay Detail Type Error");
			return false;
		}
	}
	else {
		if (setplay.mDetailType == BDT_Setplay_Scan) {
			return true; //交给视觉决策
		}
		else if (setplay.mDetailType == BDT_Setplay_GetBall) {
			return Dasher::instance().GetBall(mAgent);
		}
		if (setplay.mDetailType == BDT_Setplay_Move) {
			return mAgent.Move(setplay.mTarget);
		}
		else {
			PRINT_ERROR("Setplay Detail Type Error");
			return false;
		}
	}
}


/**
 * @brief 定位球规划器构造函数
 *
 * @param agent 代理对象
 */
BehaviorSetplayPlanner::BehaviorSetplayPlanner(Agent & agent): BehaviorPlannerBase<BehaviorAttackData>( agent )
{
}

/**
 * @brief 定位球规划器析构函数
 */
BehaviorSetplayPlanner::~BehaviorSetplayPlanner(void)
{
}

/**
 * @brief 规划定位球行为
 *
 * 仅在非 `PM_Play_On` 时产生定位球行为。
 * - `PM_Before_Kick_Off`：使用阵型/战术模块给出的开球站位点；到位后转身扫描。
 * - 我方定位球（`play_mode < PM_Our_Mode`）：只有“离球最近的队友”会产生动作：
 *   - 若不可踢：去拿球（GetBall）。
 *   - 若可踢：禁止带球（定位球一般要求停球/传球/射门），并在一定时间内扫描等待。
 *   - 特殊：时间点=20 且守门员时，尝试移动到禁区边缘的更优位置（避免干扰队友且保证安全）。
 *
 * @param behavior_list 行为列表
 */
void BehaviorSetplayPlanner::Plan(std::list<ActiveBehavior> & behavior_list)
{
	ActiveBehavior setplay(mAgent, BT_Setplay);

	setplay.mBuffer = 0.5;

	if (mWorldState.GetPlayMode() != PM_Play_On) {
		if (mWorldState.GetPlayMode() == PM_Before_Kick_Off) {
			setplay.mDetailType = BDT_Setplay_Move;
			setplay.mTarget = TeammateFormationTactic(KickOffPosition)(mSelfState.GetUnum(), mWorldState.GetKickOffMode() == KO_Ours);

			if (setplay.mTarget.Dist(mSelfState.GetPos()) < setplay.mBuffer) {
				setplay.mDetailType = BDT_Setplay_Scan;
			}

			setplay.mEvaluation = Evaluation::instance().EvaluatePosition(setplay.mTarget, true);

			behavior_list.push_back(setplay);
		}
		else if (mWorldState.GetPlayMode() < PM_Our_Mode) {
			if (mPositionInfo.GetClosestTeammateToBall() == mSelfState.GetUnum()) {
				if (!mSelfState.IsKickable()) {
					setplay.mDetailType = BDT_Setplay_GetBall;
					setplay.mTarget = mBallState.GetPos();
					setplay.mEvaluation = Evaluation::instance().EvaluatePosition(setplay.mTarget, true);

					behavior_list.push_back(setplay);
				}
				else {
					mStrategy.SetForbidenDribble(true); //禁止带球

					if (mWorldState.GetLastPlayMode() != PM_Before_Kick_Off) {
						if (mWorldState.CurrentTime().T() - mWorldState.GetPlayModeTime().T() < 20) {
							setplay.mDetailType = BDT_Setplay_Scan;
							setplay.mEvaluation = Evaluation::instance().EvaluatePosition(setplay.mTarget, true);

							behavior_list.push_back(setplay);
						}
						else if(mWorldState.CurrentTime().T() - mWorldState.GetPlayModeTime().T() == 20 && mSelfState.IsGoalie()){
							setplay.mDetailType = BDT_Setplay_Move;
							for (list<KeyPlayerInfo>::const_iterator it =mPositionInfo.GetXSortTeammate().begin(); it != mPositionInfo.GetXSortTeammate().end(); ++it){
								if (	mWorldState.GetTeammate((*it).mUnum).GetPos().X() > ServerParam::instance().ourPenaltyArea().Right() &&
										(mWorldState.GetOpponent(mPositionInfo.GetClosestOpponentToTeammate((*it).mUnum)).GetPos()- mWorldState.GetTeammate((*it).mUnum).GetPos()).Mod() >= 1.0){
									double y = MinMax(ServerParam::instance().ourPenaltyArea().Top() + 1, mWorldState.GetTeammate((*it).mUnum).GetPos().Y(), ServerParam::instance().ourPenaltyArea().Bottom() - 1);
									setplay.mTarget = Vector(ServerParam::instance().ourPenaltyArea().Right() - 1 , y);
									setplay.mEvaluation = Evaluation::instance().EvaluatePosition(setplay.mTarget, true);
									behavior_list.push_back(setplay);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}
