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
 ***********************************************************************************/

/**
 * @file FormationTactics.cpp
 * @brief 阵型战术（Formation Tactics）实现
 *
 * 本文件实现了一组“阵型战术策略”类，用于在特定比赛阶段输出预定义的站位点。
 *
 * 设计要点：
 * - 使用 `FormationTacticBase` 提供通用的 **编号映射**（Index <-> Unum），
 *   以支持阵型文件按“位置索引”描述，而运行时按“球员号码（Unum）”查询。
 * - `FormationTacticKickOffPosition` 提供开球前/开球后两套站位点（our kick off / opp kick off）。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "FormationTactics.h"
#include "Types.h"
#include "Utilities.h"
#include "Formation.h"
#include <sstream>
#include <utility>
#include <functional>
#include <cstdlib>
using namespace std;

/**
 * @brief 阵型战术基类
 *
 * 负责提供“索引 <-> 球员号码”映射。
 * - `Index2Unum[i]`：阵型配置中的第 i 个位置对应的球员号码。
 * - `Unum2Index[unum]`：球员号码对应阵型配置中的位置索引。
 */
FormationTacticBase::FormationTacticBase():
	Index2Unum(IndexIsUnum),
	Unum2Index(IndexIsUnum)
{
}

/**
 * @brief 默认映射：索引即球员号码
 *
 * 默认情况下阵型配置中的 index 与球员 unum 相同（1..11）。
 */
int FormationTacticBase::IndexIsUnum[TEAMSIZE+1] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

/**
 * @brief 初始化编号映射
 *
 * @param config 配置行（不同战术可能使用）
 * @param index2unum 可选：自定义 index->unum 映射数组
 * @param unum2index 可选：自定义 unum->index 映射数组
 *
 * @note 若映射参数为空则使用默认映射 `IndexIsUnum`。
 */
void FormationTacticBase::Initial(vector<string> & config, Unum * index2unum, int * unum2index)
{
	(void) config;

	Index2Unum = index2unum ? index2unum : IndexIsUnum;
	Unum2Index = unum2index ? unum2index : IndexIsUnum;
}

/**
 * @brief 开球站位策略构造函数
 *
 * 初始化两套站位数组（0: our kickoff, 1: opp kickoff）。
 */
FormationTacticKickOffPosition::FormationTacticKickOffPosition()
{
	for ( int i(0); i < 2; ++i )
	{
		for ( int j(0); j < TEAMSIZE; ++j )
		{
			mKickOffPosition[j][i] = Vector(0,0);
		}
	}
}

/**
 * @brief 初始化开球站位策略
 *
 * 从配置行中读取每个位置的两套坐标：
 * - mKickOffPosition[i][0]：我方开球时第 i 个位置坐标
 * - mKickOffPosition[i][1]：对方开球时第 i 个位置坐标
 *
 * @param config 配置行列表（每行 4 个数字：x1 y1 x2 y2）
 * @param index2unum 可选映射
 * @param unum2index 可选映射
 */
void FormationTacticKickOffPosition::Initial(vector<string> & config, Unum * index2unum, int * unum2index)
{
	FormationTacticBase::Initial( config, index2unum, unum2index );

	int i(0);
	for ( vector<string>::iterator config_iter(config.begin()); config_iter != config.end() && i<TEAMSIZE; ++config_iter, ++i )
	{
		stringstream sin(*config_iter);
		double p(0);
		sin >> p;
		mKickOffPosition[i][0].SetX(p);
		sin >> p;
		mKickOffPosition[i][0].SetY(p);
		sin >> p;
		mKickOffPosition[i][1].SetX(p);
		sin >> p;
		mKickOffPosition[i][1].SetY(p);
	}
}

/**
 * @brief 获取某个球员的开球站位点
 *
 * 该接口将球员号码映射到配置索引后，返回对应的站位点引用。
 *
 * @param player 球员号码（Unum）
 * @param is_our_kickoff 是否为我方开球
 * @return Vector& 站位点引用
 */
/**
 * Get players' kick off position.
 * @param player unum of player to get position.
 * @param is_our_kickoff true if is our kick off.
 * @return the kick off position of the player.
 */
Vector & FormationTacticKickOffPosition::operator()( Unum player, bool is_our_kickoff )
{
	return mKickOffPosition[Unum2Index[player] - 1][is_our_kickoff? 0: 1];
}
