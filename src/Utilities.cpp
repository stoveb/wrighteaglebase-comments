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
 * @file Utilities.cpp
 * @brief 工具函数实现 - WrightEagleBase 的通用工具函数库
 * 
 * 本文件实现了 WrightEagleBase 系统的通用工具函数，包括：
 * 1. 时间管理函数：高精度时间获取和时间计算
 * 2. 数学工具函数：几何计算和数值处理
 * 3. 字符串处理函数：格式化和解析工具
 * 4. 调试工具函数：动态调试和时间分析
 * 
 * 工具函数库是 WrightEagleBase 的基础支撑模块，
 * 为整个系统提供通用的功能支持。
 * 
 * 主要功能：
 * - 时间管理：高精度时间获取和时间计算
 * - 数学计算：几何计算、角度处理、距离计算
 * - 字符串处理：格式化、分割、解析
 * - 调试支持：动态调试模式和时间分析
 * - 平台兼容：跨平台的时间获取实现
 * 
 * 技术特点：
 * - 高精度：微秒级时间精度
 * - 跨平台：支持Windows和Linux系统
 * - 动态调试：可配置的调试模式
 * - 性能优化：高效的数学计算实现
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "Utilities.h"
#include "PlayerParam.h"
#include "DynamicDebug.h"

#include <cstring>

/**
 * @brief RealTime 静态成员变量：程序启动时间
 * 
 * 记录程序启动时的基准时间，用于后续的时间计算。
 * 所有时间计算都相对于这个基准时间。
 */
timeval RealTime::mStartTime = GetRealTime();

/**
 * @brief RealTime 静态常量：一毫秒对应的微秒数
 * 
 * 定义一毫秒等于1000000微秒，用于时间单位的转换。
 */
const long RealTime::ONE_MILLION = 1000000;

/**
 * @brief 获取当前系统时间
 * 
 * 获取高精度的系统时间，支持Windows和Linux平台。
 * 返回的时间值包含秒和微秒两部分。
 * 
 * @return timeval 当前系统时间
 * 
 * @note Windows平台使用QueryPerformanceCounter获得高精度时间
 * @note Linux平台使用gettimeofday获得系统时间
 * @note 时间精度为微秒级
 */
timeval GetRealTime() {
	timeval time_val;

#ifdef WIN32
	// === Windows平台实现 ===
	// 使用QueryPerformanceCounter获取高精度时间
	LARGE_INTEGER frq,count;
	QueryPerformanceFrequency(&frq);  // 获取计数器频率
	QueryPerformanceCounter(&count);  // 获取当前计数器值
	time_val.tv_sec = (count.QuadPart / frq.QuadPart);  // 计算秒数
	time_val.tv_usec = (count.QuadPart % frq.QuadPart) * 1000000 / frq.QuadPart;  // 计算微秒数
#else
	// === Linux平台实现 ===
	// 使用gettimeofday获取系统时间
	gettimeofday(&time_val, 0);
#endif

	return time_val;
}

/**
 @brief 获取解析器时间
 * 
 * 获取用于解析器性能分析的时间。
 * 如果启用了动态调试模式，则从调试系统获取时间，
 * 否则获取当前系统时间并添加到调试系统。
 * 
 * @return timeval 解析器时间
 * 
 * @note 动态调试模式可通过PlayerParam配置
 * @note 用于解析器性能分析和优化
 */
timeval GetRealTimeParser() {
	if (PlayerParam::instance().DynamicDebugMode() == true) {
		// 从调试系统获取解析器时间
		return DynamicDebug::instance().GetTimeParser();
	}

	// 获取当前系统时间并添加到调试系统
	timeval time_val = GetRealTime();
	DynamicDebug::instance().AddTimeParser(time_val);
	return time_val;
}

/**
 @brief 获取决策时间
 * 
 * 获取用于决策性能分析的时间。
 * 如果启用了动态调试模式，则从调试系统获取时间，
 * 否则获取当前系统时间并添加到调试系统。
 * 
 * @return timeval 决策时间
 * 
 * @note 动态调试模式可通过PlayerParam配置
 * @note 用于决策性能分析和优化
 */
timeval GetRealTimeDecision() {
	if (PlayerParam::instance().DynamicDebugMode() == true) {
		// 从调试系统获取决策时间
		return DynamicDebug::instance().GetTimeDecision();
	}

	// 获取当前系统时间并添加到调试系统
	timeval time_val = GetRealTime();
	DynamicDebug::instance().AddTimeDecision(time_val);
	return time_val;
}

/**
 @brief 获取命令发送时间
 * 
 * 获取用于命令发送性能分析的时间。
 * 如果启用了动态调试模式，则从调试系统获取时间，
 * 否则获取当前系统时间并添加到调试系统。
 * 
 * @return timeval 命令发送时间
 * 
 * @note 动态调试模式可通过PlayerParam配置
 * @note 用于命令发送性能分析和优化
 */
timeval GetRealTimeCommandSend() {
	if (PlayerParam::instance().DynamicDebugMode() == true) {
		// 从调试系统获取命令发送时间
		return DynamicDebug::instance().GetTimeCommandSend();
	}

	// 获取当前系统时间并添加到调试系统
	timeval time_val = GetRealTime();
	DynamicDebug::instance().AddTimeCommandSend(time_val);
	return time_val;
}

/**
 * @brief RealTime 加法运算符重载
 * 
 * 实现两个RealTime对象的加法运算。
 * 处理微秒进位，确保时间计算的准确性。
 * 
 * @param t 要加的时间对象
 * @return RealTime 相加后的时间对象
 * 
 * @note 当微秒部分超过1000000时，会自动进位到秒
 * @note 保持时间值的正确性和连续性
 */
RealTime RealTime::operator +(const RealTime &t) const {
	// 检查微秒部分是否需要进位
	if (GetUsec() + t.GetUsec() >= ONE_MILLION) {
		// 需要进位：秒数加1，微秒数减去1000000
		return RealTime(GetSec() + t.GetSec() + 1, GetUsec() + t.GetUsec()
				- ONE_MILLION);
	} else {
		// 不需要进位：直接相加
		return RealTime(GetSec() + t.GetSec(), GetUsec() + t.GetUsec());
	}
}

/**
 * @brief RealTime 与毫秒加法运算符重载
 * 
 * 实现RealTime对象与毫秒数的加法运算。
 * 将毫秒转换为微秒后进行时间计算。
 * 
 * @param msec 要加的毫秒数
 * @return RealTime 相加后的时间对象
 * 
 * @note 1毫秒 = 1000微秒
 * @note 自动处理微秒进位
 */
RealTime RealTime::operator +(int msec) const {
	// 将毫秒转换为微秒
	int usec = GetUsec() + msec * 1000;
	
	// 检查微秒部分是否需要进位
	if (usec >= ONE_MILLION) {
		// 需要进位：秒数增加进位部分，微秒数取余数
		return RealTime(GetSec() + usec / ONE_MILLION, usec % ONE_MILLION);
	} else {
		return RealTime(GetSec(), usec);
	}
}

RealTime RealTime::operator -(int msec) const {
	int usec = GetUsec() - msec * 1000;
	if (usec < 0) {
		usec = -usec;
		return RealTime(GetSec() - usec / ONE_MILLION - 1, ONE_MILLION - usec
				% ONE_MILLION);
	} else {
		return RealTime(GetSec(), usec);
	}
}

int RealTime::operator -(const RealTime &t) const {
	return (GetSec() - t.GetSec()) * 1000 + int((GetUsec() - t.GetUsec())
			/ 1000.0);
}

long RealTime::Sub(const RealTime &t) {
	return (GetSec() - t.GetSec()) * 1000000 + GetUsec() - t.GetUsec();
}

bool RealTime::operator <(const RealTime &t) const {
	if (GetSec() < t.GetSec())
		return true;
	if (GetSec() == t.GetSec() && GetUsec() < t.GetUsec())
		return true;
	return false;
}

bool RealTime::operator >(const RealTime &t) const {
	if (GetSec() > t.GetSec())
		return true;
	if (GetSec() == t.GetSec() && GetUsec() > t.GetUsec())
		return true;
	return false;
}

bool RealTime::operator ==(const RealTime &t) const {
	if (GetSec() == t.GetSec() && GetUsec() == t.GetUsec())
		return true;
	return false;
}

bool RealTime::operator !=(const RealTime &t) const {
	if (GetSec() == t.GetSec() && GetUsec() == t.GetUsec())
		return false;
	return true;
}

std::ostream & operator <<(std::ostream &os, RealTime t) {
	return os << t - RealTime::mStartTime;
}

Time Time::operator-(const int a) const {
	int news = S() - a;
	if (news >= 0) {
		return Time(T(), news);
	}
	return Time(T() + news, 0);
}

int Time::operator-(const Time &a) const {
	if (mT == a.T()) {
		return mS - a.S();
	} else {
		return mT - a.T() + mS; //实际可能差更多,不准
	}
}

ServerPlayModeMap & ServerPlayModeMap::instance() {
	static ServerPlayModeMap server_playmode_parser;
	return server_playmode_parser;
}

ServerPlayModeMap::ServerPlayModeMap() {
	Bind("before_kick_off", SPM_BeforeKickOff);
	Bind("time_over", SPM_TimeOver);
	Bind("play_on", SPM_PlayOn);
	Bind("kick_off_l", SPM_KickOff_Left);
	Bind("kick_off_r", SPM_KickOff_Right);
	Bind("kick_in_l", SPM_KickIn_Left);
	Bind("kick_in_r", SPM_KickIn_Right);
	Bind("free_kick_l", SPM_FreeKick_Left);
	Bind("free_kick_r", SPM_FreeKick_Right);
	Bind("corner_kick_l", SPM_CornerKick_Left);
	Bind("corner_kick_r", SPM_CornerKick_Right);
	Bind("goal_kick_l", SPM_GoalKick_Left);
	Bind("goal_kick_r", SPM_GoalKick_Right);
	Bind("goal_l", SPM_AfterGoal_Left);
	Bind("goal_r", SPM_AfterGoal_Right);
	Bind("drop_ball", SPM_Drop_Ball);
	Bind("offside_l", SPM_OffSide_Left);
	Bind("offside_r", SPM_OffSide_Right);
	Bind("penalty_kick_l", SPM_PK_Left);
	Bind("penalty_kick_r", SPM_PK_Right);
	Bind("first_half_over", SPM_FirstHalfOver);
	Bind("pause", SPM_Pause);
	Bind("human_judge", SPM_Human);
	Bind("foul_charge_l", SPM_Foul_Charge_Left);
	Bind("foul_charge_r", SPM_Foul_Charge_Right);
	Bind("foul_push_l", SPM_Foul_Push_Left);
	Bind("foul_push_r", SPM_Foul_Push_Right);
	Bind("foul_multiple_attack_l", SPM_Foul_MultipleAttacker_Left);
	Bind("foul_multiple_attack_r", SPM_Foul_MultipleAttacker_Right);
	Bind("foul_ballout_l", SPM_Foul_BallOut_Left);
	Bind("foul_ballout_r", SPM_Foul_BallOut_Right);
	Bind("back_pass_l", SPM_Back_Pass_Left);
	Bind("back_pass_r", SPM_Back_Pass_Right);
	Bind("free_kick_fault_l", SPM_Free_Kick_Fault_Left);
	Bind("free_kick_fault_r", SPM_Free_Kick_Fault_Right);
	Bind("catch_fault_l", SPM_CatchFault_Left);
	Bind("catch_fault_r", SPM_CatchFault_Right);
	Bind("indirect_free_kick_l", SPM_IndFreeKick_Left);
	Bind("indirect_free_kick_r", SPM_IndFreeKick_Right);
	Bind("penalty_setup_l", SPM_PenaltySetup_Left);
	Bind("penalty_setup_r", SPM_PenaltySetup_Right);
	Bind("penalty_ready_l", SPM_PenaltyReady_Left);
	Bind("penalty_ready_r", SPM_PenaltyReady_Right);
	Bind("penalty_taken_l", SPM_PenaltyTaken_Left);
	Bind("penalty_taken_r", SPM_PenaltyTaken_Right);
	Bind("penalty_miss_l", SPM_PenaltyMiss_Left);
	Bind("penalty_miss_r", SPM_PenaltyMiss_Right);
	Bind("penalty_score_l", SPM_PenaltyScore_Left);
	Bind("penalty_score_r", SPM_PenaltyScore_Right);

	Bind("goalie_catch_ball_l", SPM_GoalieCatchBall_Left);
	Bind("goalie_catch_ball_r", SPM_GoalieCatchBall_Right);
	Bind("foul_l", SPM_Foul_Left);
	Bind("foul_r", SPM_Foul_Right);
	Bind("penalty_onfield_l", SPM_PenaltyOnfield_Left);
	Bind("penalty_onfield_r", SPM_PenaltyOnfield_Right);
	Bind("penalty_foul_l", SPM_PenaltyFoul_Left);
	Bind("penalty_foul_r", SPM_PenaltyFoul_Right);
	Bind("penalty_winner_l", SPM_PenaltyWinner_Left);
	Bind("penalty_winner_r", SPM_PenaltyWinner_Right);
	Bind("half_time", SPM_HalfTime);
	Bind("time_up", SPM_TimeUp);
	Bind("time_extended", SPM_TimeExtended);

	Assert(mString2Enum.size() == SPM_MAX - 1);
}

void ServerPlayModeMap::Bind(const std::string & str, ServerPlayMode spm) {
	Assert(mString2Enum.count(str) == 0);
	Assert(mEnum2String.count(spm) == 0);

	mString2Enum[str] = spm;
	mEnum2String[spm] = str;
}

ServerPlayMode ServerPlayModeMap::GetServerPlayMode(const std::string & str) {
	//special case: server 发过来的是 goal_[lr]_[[:digit:]]
	if (strncmp(str.c_str(), "goal_l", strlen("goal_l")) == 0) {
		return mString2Enum["goal_l"];
	}
	else if (strncmp(str.c_str(), "goal_r", strlen("goal_r")) == 0) {
		return mString2Enum["goal_r"];
	}
	else if (mString2Enum.count(str)) {
		return mString2Enum[str];
	}
	else {
		Assert(!"server playmode error");
		return SPM_Null;
	}
}

const char * ServerPlayModeMap::GetPlayModeString(ServerPlayMode spm) {
	if (mEnum2String.count(spm)) {
		return mEnum2String[spm].c_str();
	}
	else {
		return "";
	}
}

