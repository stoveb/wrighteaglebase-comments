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
 * @file InterceptModel.cpp
 * @brief 截球模型实现 - WrightEagleBase 的截球动作预测核心
 * 
 * 本文件实现了 InterceptModel 类，它是 WrightEagleBase 系统的截球动作预测核心：
 * 1. 实现高精度的截球点计算
 * 2. 预测球员和球的运动轨迹
 * 3. 计算最优的截球时机和位置
 * 4. 处理复杂的截球几何问题
 * 
 * InterceptModel 是 WrightEagleBase 的高级决策支持系统，
 * 负责为截球行为提供精确的数学模型和计算支持。
 * 
 * 主要功能：
 * - 截球点计算：基于球和球员位置的几何分析
 * - 时间预测：计算截球所需的最短时间
 * - 轨迹分析：分析球和球员的运动轨迹
 * - 特殊情况处理：处理各种边界条件和特殊情况
 * 
 * 技术特点：
 * - 切点算法：计算球运动轨迹的切点
 * - 迭代求解：使用数值方法求解复杂方程
 * - 坐标变换：简化计算的坐标系转换
 * - 速度衰减：考虑球速衰减的物理模型
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include <cmath>
#include "InterceptModel.h"
#include "ServerParam.h"
#include "PlayerParam.h"
#include "PlayerState.h"
#include "InterceptInfo.h"
#include "Utilities.h"
#include "Dasher.h"
#include "Plotter.h"
#include "Logger.h"

/**
 * @brief 不可能截球的速度阈值
 * 
 * 当球的速度超过这个值时，认为截球是不可能的。
 * 这个阈值基于经验值和服务器参数设置。
 */
const double InterceptModel::IMPOSSIBLE_BALL_SPEED = 8.0;

/**
 * @brief InterceptModel 构造函数
 * 
 * 初始化截球模型对象。
 * 当前为空实现，因为没有需要初始化的成员变量。
 */
InterceptModel::InterceptModel()
{
	// 构造函数体为空
}

/**
 * @brief InterceptModel 析构函数
 * 
 * 清理截球模型资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
InterceptModel::~InterceptModel()
{
	// 析构函数体为空
}

/**
 * @brief 获取 InterceptModel 单例实例
 * 
 * 返回 InterceptModel 的单例实例，确保整个系统只有一个截球模型。
 * 
 * @return InterceptModel& 截球模型的引用
 * 
 * @note 采用单例模式确保截球模型的全局一致性
 */
InterceptModel &InterceptModel::instance()
{
	static InterceptModel intercept_model;
	return intercept_model;
}

/**
 * @brief 计算截球方案
 * 
 * 这是截球模型的核心函数，负责计算球员截球的最佳方案。
 * 该函数基于球的运动轨迹和球员的位置能力，计算出截球的
 * 最优时机、位置和所需时间。
 * 
 * 计算过程：
 * 1. 坐标系变换：将问题转换到球运动方向为X轴的坐标系
 * 2. 特殊情况处理：检查是否已经可以截球或球速过低
 * 3. 切点计算：计算球运动轨迹的切点
 * 4. 时间计算：计算到达切点所需的时间
 * 5. 方案生成：生成完整的截球方案
 * 
 * @param ball_pos 球的当前位置
 * @param ball_vel 球的速度向量
 * @param buffer 截球缓冲区域大小
 * @param player 球员状态指针
 * @param sol 截球方案输出参数
 * 
 * @note 坐标系变换简化了计算复杂度
 * @note 考虑了球的衰减和球员的运动能力
 * @note 处理了各种边界条件和特殊情况
 */
void InterceptModel::CalcInterception(const Vector & ball_pos, const Vector & ball_vel, const double buffer, const PlayerState *player, InterceptSolution *sol)
{
	// === 获取服务器参数 ===
	const double & alpha = ServerParam::instance().ballDecay();      // 球的衰减系数
	const double & ln_alpha = ServerParam::instance().logBallDecay(); // 球衰减系数的对数

	// === 坐标系变换 ===
	// 将球员位置转换到以球为原点、球运动方向为X轴的坐标系
	// 这样可以大大简化计算复杂度
	Vector start_pt = (player->GetPos() - ball_pos).Rotate(-ball_vel.Dir());

	// === 提取模型输入参数 ===
	const double x0 = start_pt.X();                    // 球员在变换坐标系中的X坐标
	const double y0 = start_pt.Y();                    // 球员在变换坐标系中的Y坐标
	const double ball_spd = ball_vel.Mod();            // 球的速度大小

    const double & player_spd = player->GetEffectiveSpeedMax(); // 球员的最大有效速度
    const double & kick_area = buffer;                 // 截球缓冲区域大小
	const double cycle_delay = double(player->GetPosDelay()); // 球员的位置延迟周期数

	// === 计算球的最大移动距离 ===
	// 基于球速和衰减系数计算球能够移动的最大距离
	const double max_x = ball_spd / (1.0 - alpha);

	// === 调试代码（已注释） ===
	// 用于特定情况下的轨迹绘制和调试
//	if (Logger::instance().CurrentTime() == Time(205, 0) && player->GetUnum() == 4){
//		PlotInterceptCurve(x0, y0, ball_spd, player_spd, kick_area, cycle_delay, max_x);
//	}

	// === 特殊情况处理 ===
	// 计算球员到球的距离
	const double s = Sqrt(x0 * x0 + y0 * y0);
	// 计算球员在延迟时间内可以移动的距离加上截球缓冲区
	const double self_fix = kick_area + cycle_delay * player_spd;

	// 情况1：球员已经可以截球
	if (s < self_fix){ //最好的情况已经可踢
		sol->tangc = 0;        // 切点数量为0
		sol->interc = 1;        // 截球周期数为1
		sol->interp[0] = 0;     // 截球点X坐标为0
		sol->intert[0] = 0;     // 截球时间为0
		return;
	}

	// 情况2：球速过低，几乎静止
	if (ball_spd < 0.1){
		sol->tangc = 0;        // 切点数量为0
		sol->interc = 1;        // 截球周期数为1
		sol->interp[0] = 0;     // 截球点X坐标为0

		// 计算球员到达球的时间
		double p = (s - kick_area)/player_spd - cycle_delay;
		if (p < 0.0) p = 0.0;
		sol->intert[0] = p;
		return;
	}

	// === 切点计算 ===
	// 先判断切点个数 -- 根据切点个数得到解的个数，并依次选择迭代的初值
	int n = CalcTangPoint(x0, y0, player_spd, kick_area, cycle_delay, sol);

	// === 根据切点个数处理不同情况 ===
	if (n < 1){ //没有切点
		// 当没有切点时，只能在球的末端截球
		sol->interc = 1;

		// 计算在球运动轨迹末端的截球点
		sol->interp[0] = CalcInterPoint(max_x - 1.0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
		// 计算截球时间
		sol->intert[0] = log(1.0 - sol->interp[0] * (1.0 - alpha) / ball_spd) / ln_alpha;
	}
	else if (n == 1){
		/**
		 * n = 1的情况对应只有一个切点，即外切的时候同时内切
		 * 这种情况下只有一个截球方案
		 **/
		sol->interc = 1;

		// 根据球员位置选择截球点
		if (x0 < 0.0) {
			// 球员在球后方，选择在球运动末端截球
			sol->interp[0] = CalcInterPoint(max_x - 1.0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
		}
		else {
			// 球员在球前方，选择在球当前位置截球
			sol->interp[0] = CalcInterPoint(x0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
		}

		// 计算截球时间
		sol->intert[0] = log(1.0 - sol->interp[0] * (1.0 - alpha) / ball_spd) / ln_alpha;
	}
	else {
		// 有两个切点的情况，需要根据球速判断截球策略
		if (ball_spd < sol->tangv[1]){ //没有最佳截球区间，早期就可截
			// 球速较低，可以在早期截球
			sol->interc = 1;

			// 在球当前位置截球
			sol->interp[0] = CalcInterPoint(x0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
			sol->intert[0] = log(1.0 - sol->interp[0] * (1.0 - alpha) / ball_spd) / ln_alpha;
		}
		else if (ball_spd < sol->tangv[0]){ //有最佳截球区间
			// 球速适中，存在最佳截球区间，可以有三个截球方案
			sol->interc = 3;

			// 方案1：在球当前位置截球（最早截球）
			sol->interp[0] = CalcInterPoint(x0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
			sol->intert[0] = log(1.0 - sol->interp[0] * (1.0 - alpha) / ball_spd) / ln_alpha;

			// 方案2：在两个切点之间截球（最佳截球）
			sol->interp[1] = CalcInterPoint((sol->tangp[0] + sol->tangp[1]) * 0.5, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
			sol->intert[1] = log(1.0 - sol->interp[1] * (1.0 - alpha) / ball_spd) / ln_alpha;

			// 方案3：在切点之后截球（较晚截球）
			sol->interp[2] = CalcInterPoint((sol->tangp[1] + max_x) * 0.5, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
			sol->intert[2] = log(1.0 - sol->interp[2] * (1.0 - alpha) / ball_spd) / ln_alpha;
		}
		else { //没有最佳截球区间，只有后期才可截
			// 球速过高，只能在球的末端截球
			sol->interc = 1;

			// 在球运动轨迹末端截球
			sol->interp[0] = CalcInterPoint(max_x - 1.0, x0, y0, ball_spd, player_spd, kick_area, cycle_delay);
			sol->intert[0] = log(1.0 - sol->interp[0] * (1.0 - alpha) / ball_spd) / ln_alpha;
		}
	}
}

/**
 * 计算切点的个数和位置
 *
 * 球运动x距离需要的时间：bt(x) = ln(1 - x(1-α)/v0)/ln(α) -- α为球的decay
 * 人运动到此处所需最短时间：pt(x) = (s(x) - ka)/vp - cd -- s(x) = sqrt((x-x0)**2 + y0**2)，**为幂
 * 切点满足：pt(x) = bt(x) 且 pt'(x) = bt'(x)
 * 消去v0，得到方程：
 * f(x) = 1 - α**pt(x) * (1 - x(x-x0)ln(α)/(s(x)vp))
 * f'(x) = ln(α)/vp * {(f(x)-1)(x-x0)/s(x) + α**pt(x) * {(2x-x0)/s(x) - x(x-x0)**2/s(x)**3}}
 * 取x0为初值，用牛顿迭代法求解，得到外切点，再取一个大一点的x作为初值，求解得到内切点
 *
 * @param x0
 * @param y0
 * @param vp 球员最大速度
 * @param ka 球员可踢范围半径
 * @param cd 球员的cycle_celay
 * @param sol
 * @return 切点个数
 */
int InterceptModel::CalcTangPoint(double x0, double y0, double vp, double ka, double cd, InterceptSolution *sol)
{
	static const double MINERROR = 0.01;

	const double & alpha = ServerParam::instance().ballDecay();
	const double & ln_alpha = ServerParam::instance().logBallDecay();

	double s, p, alpha_p, f, dfdx, last_f = 1000.0, x;
	int iteration_cycle = 0;

    if (fabs(y0) < FLOAT_EPS) {
        sol->tangc = 0;
        return 0; //没有切点
    }

	x = x0;
	do {
		iteration_cycle += 1;
		if (iteration_cycle > 10){
			break;
		}
		s = Sqrt((x - x0) * (x - x0) + y0 * y0);
		p = (s - ka)/vp - cd;
		if (p < 0.0) p = 0.0;
		alpha_p = pow(alpha, (p));
		f = 1.0 - alpha_p * (1 - x * (x - x0) * ln_alpha / (s * vp));

		if(fabs(f) > fabs(last_f)){
			sol->tangc = 0;
			return 0; //没有切点
		}
		else{
			last_f = f;
		}

		dfdx = ln_alpha / vp * ((x - x0) * (f - 1.0) / s + alpha_p * ((x + x - x0) / s - x * (x - x0) * (x - x0) / (s * s * s)));
        dfdx = fabs(dfdx) < FLOAT_EPS? (Sign(dfdx) * FLOAT_EPS): dfdx;
		x = x - f/dfdx;
	} while (fabs(f) > MINERROR);

	sol->tangp[0] = x;

	s = Sqrt((x - x0) * (x - x0) + y0 * y0);
	p = (s - ka)/vp - cd;
	if (p < 0.0) p = 0.0;
	alpha_p = pow(alpha, (p));
	if (1.0 - alpha_p < FLOAT_EPS){ //表示自己到那里几乎不花时间
		sol->tangv[0] = 1000.0;
		sol->tangc = 1;
		return 1;
	}
	else {
		sol->tangv[0] = x * (1.0 - alpha) / (1.0 - alpha_p);
	}

	x += 0.5; //检测是否只有一个切点
	s = Sqrt((x - x0) * (x - x0) + y0 * y0);
	p = (s - ka)/vp - cd;
	if (p < 0.0) p = 0.0;
	alpha_p = pow(alpha, p);
	f = 1.0 - alpha_p * (1 - x * (x - x0) * ln_alpha / (s * vp));
	if (f > 0.0){
		sol->tangc = 1;
		return 1; //只有一个切点
	}
	else {
		do {
			x += 15.0;
			s = Sqrt((x - x0) * (x - x0) + y0 * y0);
			p = (s - ka)/vp - cd;
			if (p < 0.0) p = 0.0;
			alpha_p = pow(alpha, p);
			f = 1.0 - alpha_p * (1 - x * (x - x0) * ln_alpha / (s * vp));
		} while (f < 0.0);
		dfdx = ln_alpha / vp * ((x - x0) * (f - 1.0) / s + alpha_p * ((x + x - x0) / s - x * (x - x0) * (x - x0) / (s * s * s)));
        dfdx = fabs(dfdx) < FLOAT_EPS? (Sign(dfdx) * FLOAT_EPS): dfdx;
		x = x - f/dfdx;

		iteration_cycle = 0;
		while (fabs(f) > MINERROR){
			iteration_cycle += 1;
			if (iteration_cycle > 10){
				break;
			}
			s = Sqrt((x - x0) * (x - x0) + y0 * y0);
			p = (s - ka)/vp - cd;
			if (p < 0.0) p = 0.0;
			alpha_p = pow(alpha, p);
			f = 1.0 - alpha_p * (1 - x * (x - x0) * ln_alpha / (s * vp));
			dfdx = ln_alpha / vp * ((x - x0) * (f - 1.0) / s + alpha_p * ((x + x - x0) / s - x * (x - x0) * (x - x0) / (s * s * s)));
            dfdx = fabs(dfdx) < FLOAT_EPS? (Sign(dfdx) * FLOAT_EPS): dfdx;
			x = x - f/dfdx;
		}

		sol->tangp[1] = x;
		s = Sqrt((x - x0) * (x - x0) + y0 * y0);
		p = (s - ka)/vp - cd;
		if (p < 0.0) p = 0.0;
		alpha_p = pow(alpha, p);
		if (1.0 - alpha_p < FLOAT_EPS){
			sol->tangv[1] = 1000.0;
		}
		else {
			sol->tangv[1] = x * (1.0 - alpha) / (1.0 - alpha_p);
		}
		sol->tangc = 2;
		return 2; //有两个切点
	}
}

/**
 * 求解交点
 *
 * 球运动x距离需要的时间：bt(x) = ln(1 - x(1-α)/v0)/ln(α) -- α为球的decay
 * 人运动到此处所需最短时间：pt(x) = (s(x) - ka)/vp - cd -- s(x) = sqrt((x-x0)**2 + y0**2)，**为幂
 * 截球点满足：pt(x) = bt(x)
 * 得到方程
 * f(x) = pt(x) - bt(x)
 * f'(x) = (x - x0)/(s(x)*vp) + (1/lnα)/(v0/(1-α)-x)
 *
 * @param x_init 迭代的初始值
 * @param x0
 * @param y0
 * @param vb 球的当前速度
 * @param vp 球员最大速度
 * @param ka 球员可踢范围半径
 * @param cd 球员的cycle_celay
 * @param sol
 */
double InterceptModel::CalcInterPoint(double x_init, double x0, double y0, double vb, double vp, double ka, double cd)
{
	static const double MINERROR = 0.01;

	const double & alpha = ServerParam::instance().ballDecay();
	const double & ln_alpha = ServerParam::instance().logBallDecay();

	const double max_x = vb / (1.0 - alpha) - 0.1;

	double s, p, f, dfdx, x;
	int iteration_cycle = 0;

	x = x_init;
	do {
		iteration_cycle += 1;
		if (iteration_cycle > 10){
			break;
		}
		x = Min(x, max_x);
		s = Sqrt((x - x0) * (x - x0) + y0 * y0);
		p = (s - ka)/vp - cd;
		if (p < 0.0) p = 0.0;
		f = p - log(1.0 - x * (1.0 - alpha) / vb) / ln_alpha;
		dfdx = (x - x0) / (s * vp) + (1.0 / ln_alpha) / (vb / (1.0 - alpha) - x);
        dfdx = fabs(dfdx) < FLOAT_EPS? (Sign(dfdx) * FLOAT_EPS): dfdx;
		x = x - f/dfdx;
	}while(fabs(f) > MINERROR);

	return MinMax(0.0, x, max_x);
}

/**
 * 最佳截球点（与当前球速无关，是截球窗口变化时收缩成的那个点，也就是外切点） -- 这里不考虑cd
 * @param relpos
 * @param vp 球员最大速度
 * @param ka 球员可踢范围半径
 * @param fix 是跑动延迟的修正（即player不能全速跑，用全速跑计算，要加个修正）
 * @return
 */
double InterceptModel::CalcPeakPoint(const Vector & relpos, const double & vp, const double & ka, const double fix)
{
	static const double MINERROR = 0.01;

	const double alpha = ServerParam::instance().ballDecay();
	const double ln_alpha = ServerParam::instance().logBallDecay();
	const double x0 = relpos.X();
	const double y0 = relpos.Y();

	if (x0 < 0.0){
		return 150.0;
	}

	if (fabs(y0) < ka){
		return -1.0; //不可能穿越
	}

	double s, p, alpha_p, f, dfdx, last_f = 1000.0, x, last_x = x0;
	int iteration_cycle = 0;

	x = x0;
	do {
		iteration_cycle += 1;
		if (iteration_cycle > 10){
			break;
		}
		s = Sqrt((x - x0) * (x - x0) + y0 * y0);
		p = (s - ka)/vp + fix;
		if (p < 0.0) p = 0.0;
		alpha_p = pow(alpha, (p));
		f = 1.0 - alpha_p * (1 - x * (x - x0) * ln_alpha / (s * vp));

		if(fabs(f) > fabs(last_f)){
			return last_x; //没有切点
		}
		else{
			last_f = f;
		}
		last_x = x;
		dfdx = ln_alpha / vp * ((x - x0) * (f - 1.0) / s + alpha_p * ((x + x - x0) / s - x * (x - x0) * (x - x0) / (s * s * s)));
        dfdx = fabs(dfdx) < FLOAT_EPS? (Sign(dfdx) * FLOAT_EPS): dfdx;
		x = x - f/dfdx;
	} while (fabs(f) > MINERROR);

	return x;
}


double InterceptModel::CalcGoingThroughSpeed(const PlayerState & player, const Ray & ballcourse, const double & distance, const double fix)
{
    Vector rel_pos = (player.GetPredictedPos() - ballcourse.Origin()).Rotate(-ballcourse.Dir());
    double kick_area = (player.IsGoalie())? ServerParam::instance().maxCatchableArea(): player.GetKickableArea();
    double peak = CalcPeakPoint(rel_pos, player.GetEffectiveSpeedMax(), kick_area, fix);
	double gtspeed = 0.0;

	if (peak < 0)
    {
        double d = Max(0.0, kick_area * kick_area - rel_pos.Y() * rel_pos.Y());
		gtspeed = rel_pos.X() + Sqrt(d) + 0.06;
		gtspeed = Max(gtspeed , 1.2);
		return gtspeed;
	}
    else if (peak < distance)
    {
        double cycletopoint = Dasher::instance().RealCycleNeedToPoint(player, ballcourse.GetPoint(peak));
        gtspeed = ServerParam::instance().GetBallSpeed((int)(cycletopoint), peak);

        if (player.IsGoalie() && player.GetUnum() > 0 && player.GetBodyDirConf() > PlayerParam::instance().minValidConf())
        {
            Ray ray(player.GetPredictedPos(), player.GetBodyDir());
			Vector pt;
			if (ballcourse.Intersection(Line(ray), pt))
            {
				double c2p = Dasher::instance().RealCycleNeedToPoint(player, pt);
				double pk = pt.Dist(ballcourse.Origin());
                double gtspd =  ServerParam::instance().GetBallSpeed((int)ceil(c2p), pk);
                gtspeed = Max(gtspeed, gtspd);
			}
		}

        if (gtspeed < ServerParam::instance().ballSpeedMax())
        {
    		double cycletopoint = Dasher::instance().RealCycleNeedToPoint(player, ballcourse.GetPoint(distance));
            double speed = ServerParam::instance().GetBallSpeed((int)ceil(cycletopoint), distance);
    		gtspeed = Max(gtspeed, speed);
    	}
	}
    else
    {
		double cycletopoint = Dasher::instance().RealCycleNeedToPoint(player, ballcourse.GetPoint(distance));
        gtspeed = ServerParam::instance().GetBallSpeed((int)ceil(cycletopoint), distance);
	}

	return gtspeed;
}

void InterceptModel::PlotInterceptCurve(double x0, double y0, double v0, double vp, double ka, double cd, double max_x)
{
	Plotter::instance().GnuplotExecute("alpha = 0.94");
	Plotter::instance().GnuplotExecute("ln(x) = log(x)");
	Plotter::instance().GnuplotExecute("bt(x) = ln(1 - x * (1 - alpha) / v0) / ln(alpha)");
	Plotter::instance().GnuplotExecute("s(x) = sqrt((x - x0)**2 + y0**2)");
	Plotter::instance().GnuplotExecute("pt(x) = (s(x) - ka) / vp - cd");

	Plotter::instance().GnuplotExecute("x0 = %g", x0);
	Plotter::instance().GnuplotExecute("y0 = %g", y0);
	Plotter::instance().GnuplotExecute("v0 = %g", v0);
	Plotter::instance().GnuplotExecute("vp = %g", vp);
	Plotter::instance().GnuplotExecute("ka = %g", ka);
	Plotter::instance().GnuplotExecute("cd = %g", cd);

	Plotter::instance().GnuplotExecute("set xrange[0:%g]", max_x + 1.0);
	Plotter::instance().GnuplotExecute("set yrange[0:]");

	Plotter::instance().GnuplotExecute("plot bt(x), pt(x)");
}
