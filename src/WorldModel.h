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
 * @file WorldModel.h
 * @brief 世界模型管理器（WorldModel）接口定义
 *
 * WorldModel 负责组织与维护世界模型的核心对象，典型设计为：
 * - 持有两份 WorldState：一份用于“己方视角”的决策，一份用于“对手视角/反算”的决策；
 * - 为每份 WorldState 配套维护 HistoryState，用于在周期更新时记录历史快照，支持回溯
 *   查询与基于历史的推断。
 *
 * 外部通常通过 `WorldModel::Update()` 推进周期更新，并通过 `GetWorldState()`/`World()`
 * 获取对应视角的世界状态引用。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#ifndef WORLDMODEL_H_
#define WORLDMODEL_H_

class Observer;
class WorldState;
class HistoryState;

/**
 * WorldModel 里面存放两对 WorldState，一对用于队友的决策，另一对用于对手的决策
 */
class WorldModel {
	WorldModel(WorldModel &);
public:
	WorldModel();
	virtual ~WorldModel();

	void Update(Observer *observer);

	const WorldState & GetWorldState(bool reverse) const;
	WorldState       & World(bool reverse);

private:
	WorldState *mpWorldState[2];
	HistoryState *mpHistoryState[2];
};

#endif /* WORLDMODEL_H_ */
