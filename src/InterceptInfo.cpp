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
 * @file InterceptInfo.cpp
 * @brief 截球信息（InterceptInfo）实现
 *
 * InterceptInfo 是 InfoState 的一个子模块，用于计算并维护：
 * - 每名球员（队友/对手）对球的截球区间（最早/可行窗口/最晚）与最小截球周期；
 * - 截球点（球在最小周期时的预测位置）；
 * - 将所有可用球员按“最小截球周期 + 信息延迟”排序，形成 OIT（Ordered Intercept Table）。
 *
 * 计算流程概览：
 * 1. 通过 `InterceptModel` 先求解简化截球模型（理想截球区间）；
 * 2. 在 Tight 模式下，使用 `Dasher::CycleNeedToPoint` 对区间进行修正（更贴近实际 go_to_point 模型）；
 * 3. 计算最小截球周期对应的截球点，并检查是否在球场范围内（决定 IR_Success/IR_Failure）。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include <cstdlib>
#include "InterceptInfo.h"
#include "InterceptModel.h"
#include "Dasher.h"
#include "Logger.h"
#include <algorithm>

/**
 * @brief 构造函数
 *
 * @param pWorldState 世界状态指针
 * @param pInfoState  信息状态指针
 */
InterceptInfo::InterceptInfo(WorldState *pWorldState, InfoState *pInfoState): InfoStateBase( pWorldState, pInfoState )
{
}

/**
 * @brief 每周期更新例程
 *
 * InterceptInfo 的更新主要工作是：
 * - 重新计算/校验所有球员的截球信息并排序（OIT）。
 */
void InterceptInfo::UpdateRoutine()
{
    SortIntercerptInfo();
}

/**
 * @brief 计算并排序截球信息（OIT）
 *
 * 遍历场上所有存活球员（-11..-1, 1..11）：
 * - 调用 `VerifyIntInfo(unum)` 确保该球员的截球信息更新到当前周期；
 * - 记录其位置延迟（pos delay），并将结果塞入 `mOIT`；
 * - 最终按 OrderedIT 的比较规则排序：优先 mMinCycle，其次 cycleDelay。
 */
void InterceptInfo::SortIntercerptInfo()
{
    mOIT.clear();

    for (int i = -TEAMSIZE; i <= TEAMSIZE; i++){
        if (i == 0) continue;

        if (mpWorldState->GetPlayer(i).IsAlive()){
            mOIT.push_back(OrderedIT(VerifyIntInfo(i), i, mpWorldState->GetPlayer(i).GetPosDelay()));
        }
    }

    std::sort(mOIT.begin(), mOIT.end());

    if (PlayerParam::instance().SaveTextLog()) {
        Logger::instance().GetTextLogger("oit") << "\n" << mpWorldState->CurrentTime() << ": "<< std::endl;
        Logger::instance().GetTextLogger("oit") << "#" << "\t" << "cd" << "\t" << "min" << "\t" << "idl" << "\t" << "its" << "\t" << "it0" << "\t" << "it1" << "\t" << "it2" << std::endl;

        for (std::vector<OrderedIT>::iterator it = mOIT.begin(); it != mOIT.end(); ++it) {
            Logger::instance().GetTextLogger("oit") << it->mUnum
            << "\t" << it->mCycleDelay
            << "\t" << it->mpInterceptInfo->mMinCycle
            << "\t" << it->mpInterceptInfo->mIntervals;

            for (int i = 0; i < it->mpInterceptInfo->mIntervals; ++i) {
                Logger::instance().GetTextLogger("oit") << "\t" << it->mpInterceptInfo->mInterCycle[i];
            }

            Logger::instance().GetTextLogger("oit") << std::endl;
        }
    }
}

/**
 * @brief 校验并更新指定球员的截球信息
 *
 * 该函数确保：
 * - `PlayerInterceptInfo` 已绑定到当前球员对象；
 * - 若该球员的截球信息时间戳不是当前周期，则重新计算 Tight 截球区间。
 *
 * @param unum 球员号码（队友为正，对手为负）
 * @return PlayerInterceptInfo* 对应球员的截球信息指针；球员无效则返回 0
 */
PlayerInterceptInfo *InterceptInfo::VerifyIntInfo(Unum unum)
{
    PlayerInterceptInfo* pInfo = GetPlayerInterceptInfo(unum);
    if (!pInfo) {
        return 0;
    }

    if(pInfo->mTime != mpWorldState->CurrentTime()){
        CalcTightInterception(mpWorldState->GetBall(), pInfo);
        pInfo->mTime = mpWorldState->CurrentTime();
    }

    return pInfo;
}

/**
 * @brief 分析截球解并填充派生字段
 *
 * 根据 InterceptModel 的求解结果（solution.interc/intert）填充：
 * - 可行区间数 `mIntervals`（1 或 2）；
 * - 最小截球周期 `mMinCycle` 与对应截球点 `mInterPos`；
 * - 截球结果 `mRes`（截球点是否落在球场范围内）。
 *
 * @param ball 当前球状态
 * @param pInfo 球员截球信息
 */
void InterceptInfo::AnalyseInterceptSolution(const BallState & ball, PlayerInterceptInfo *pInfo)
{
    pInfo->mIntervals = (pInfo->solution.interc == 1)? 1: 2;
    pInfo->mMinCycle = pInfo->mInterCycle[0];
    if (pInfo->mMinCycle == 0) pInfo->mIntervals = 0; //表示可踢
    pInfo->mInterPos = ball.GetPredictedPos(pInfo->mMinCycle);

    if (!ServerParam::instance().pitchRectanglar().IsWithin(pInfo->mInterPos, 4.0)){
        pInfo->mRes = IR_Failure;
    }
    else {
        pInfo->mRes = IR_Success;
    }
}

/**
 * @brief 计算“理想”截球区间（简化模型）
 *
 * 使用 InterceptModel 根据球的预测位置/速度与 buffer（可截半径）求解拦截解，
 * 并将求得的连续时间区间离散/取整为周期数存入 `mInterCycle`。
 *
 * @param ball 球状态
 * @param pInfo 球员截球信息
 * @param buffer 判定“可截”的距离阈值（例如 kickableArea 或更松的 buffer）
 */
void InterceptInfo::CalcIdealInterception(const BallState & ball, PlayerInterceptInfo *pInfo, const double & buffer)
{
    const int idle_cycle = pInfo->mpPlayer->GetIdleCycle();

    //step 1. 求解简化截球模型
    InterceptModel::instance().CalcInterception(ball.GetPredictedPos(idle_cycle), ball.GetPredictedVel(idle_cycle), buffer, pInfo->mpPlayer, &(pInfo->solution));

    //step 2. 修正截球区间
    //	取整
    if (pInfo->solution.interc == 1){
        pInfo->mInterCycle[0] = (int)floor(pInfo->solution.intert[0]);
    }
    else { //3
        pInfo->mInterCycle[0] = (int)floor(pInfo->solution.intert[0]);
        pInfo->mInterCycle[1] = (int)ceil(pInfo->solution.intert[1]);
        pInfo->mInterCycle[2] = (int)floor(pInfo->solution.intert[2]);


        if (pInfo->mInterCycle[0] > pInfo->mInterCycle[1]){
            pInfo->mInterCycle[0] = pInfo->mInterCycle[2];
            pInfo->solution.interc = 1;
        }
    }

    bool kickable = pInfo->mpPlayer->GetPos().Dist(ball.GetPredictedPos(idle_cycle)) < buffer; //判断开始截球的第一个周期，是否可踢
    if (!kickable) {
        pInfo->mInterCycle[0] = Max(pInfo->mInterCycle[0], 1);
    }
    else {
        pInfo->mInterCycle[0] = 0;
    }

    if (pInfo->solution.interc > 1){
        pInfo->mInterCycle[1] = Max(pInfo->mInterCycle[1], pInfo->mInterCycle[0]);
        pInfo->mInterCycle[2] = Max(pInfo->mInterCycle[2], pInfo->mInterCycle[1]);
    }

    if (pInfo->solution.interc <= 1){
        pInfo->mInterCycle[0] += idle_cycle;
    }
    else {
        pInfo->mInterCycle[0] += idle_cycle;
        pInfo->mInterCycle[1] += idle_cycle;
        pInfo->mInterCycle[2] += idle_cycle;
    }
}

/**
 * @brief 计算“松”的截球区间（不进行 go_to_point 修正）
 *
 * 适用于用较大 buffer（例如传球接应半径）评估“是否能在球出界前接近到某个距离”。
 */
void InterceptInfo::CalcLooseInterception(const BallState & ball, PlayerInterceptInfo *pInfo, const double & buffer)
{
    CalcIdealInterception(ball, pInfo, buffer);
    AnalyseInterceptSolution(ball, pInfo);
}

/**
 * @brief 计算“紧”的截球区间（考虑 go_to_point 修正）
 *
 * 先用 `CalcIdealInterception` 得到理想解，然后使用 Dasher 的 go_to_point
 * 预测（CycleNeedToPoint）对区间的上下界进行收缩，使结果更贴近实际跑动模型。
 *
 * @param ball 球状态
 * @param pInfo 球员截球信息
 * @param can_inverse 是否允许逆跑（影响 CycleNeedToPoint 估计）
 */
void InterceptInfo::CalcTightInterception(const BallState & ball, PlayerInterceptInfo *pInfo, bool can_inverse)
{
    CalcIdealInterception(ball, pInfo, pInfo->mpPlayer->GetKickableArea()); //TODO: 改成从外面传进来 buffer

    const int idle_cycle = pInfo->mpPlayer->GetIdleCycle();

    //根据go_to_point模型修正
    if (pInfo->solution.interc == 1){
        int cycle_sup = pInfo->mInterCycle[0];
        int cycle_inf = MobileState::Predictor::MAX_STEP;
        for (int i = cycle_sup; i <= cycle_inf; ++i){
            int n = Dasher::instance().CycleNeedToPoint(*(pInfo->mpPlayer), ball.GetPredictedPos(i), can_inverse) + idle_cycle;
            if (n <= i){ //n以后完全可截
                break;
            }
            pInfo->mInterCycle[0] ++;
        }
    }
    else { //3
        int cycle_sup = pInfo->mInterCycle[0];
        int cycle_inf = pInfo->mInterCycle[1];
        for (int i = cycle_sup; i <= cycle_inf; ++i){
            int n = Dasher::instance().CycleNeedToPoint(*(pInfo->mpPlayer), ball.GetPredictedPos(i), can_inverse) + idle_cycle;
            if (n <= i){
                break;
            }
            pInfo->mInterCycle[0] ++;
        }
        if (pInfo->mInterCycle[0] <= pInfo->mInterCycle[1]){
            cycle_sup = pInfo->mInterCycle[0];
            cycle_inf = pInfo->mInterCycle[1];
            for (int i = cycle_inf; i >= cycle_sup; --i){
                int n = Dasher::instance().CycleNeedToPoint(*(pInfo->mpPlayer), ball.GetPredictedPos(i), can_inverse) + idle_cycle;
                if (n <= i){
                    break;
                }
                pInfo->mInterCycle[1] --;
            }
            if (pInfo->mInterCycle[0] > pInfo->mInterCycle[1]){ //窗口收缩成点
                pInfo->solution.interc = 1;
            }
        }
        else {
            pInfo->solution.interc = 1;
        }

        cycle_sup = pInfo->mInterCycle[2];
        cycle_inf = MobileState::Predictor::MAX_STEP;
        for (int i = cycle_sup; i <= cycle_inf; ++i){
            int n = Dasher::instance().CycleNeedToPoint(*(pInfo->mpPlayer), ball.GetPredictedPos(i), can_inverse) + idle_cycle;
            if (n <= i){ //n以后完全可截
                break;
            }
            pInfo->mInterCycle[2] ++;
        }

        if (pInfo->solution.interc == 1){
            pInfo->mInterCycle[0] = pInfo->mInterCycle[2];
        }
    }

    AnalyseInterceptSolution(ball, pInfo);
}

PlayerInterceptInfo *InterceptInfo::GetPlayerInterceptInfo(Unum unum) const
{
    if (!mpWorldState->GetPlayer(unum).IsAlive()) { return 0; }

    PlayerInterceptInfo *pInfo = const_cast<PlayerInterceptInfo *>(unum > 0? &(mTeammateInterceptInfo[unum]): &(mOpponentInterceptInfo[-unum]));
    pInfo->mpPlayer = & (mpWorldState->GetPlayer(unum));
    return pInfo;
}
