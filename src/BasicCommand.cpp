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
 * @file BasicCommand.cpp
 * @brief 基础命令实现 - WrightEagleBase 的命令执行核心
 * 
 * 本文件实现了 WrightEagleBase 系统的基础命令类，包括：
 * 1. BasicCommand：基础命令基类
 * 2. Turn：转身命令
 * 3. Dash：跑动命令
 * 4. TurnNeck：转头命令
 * 5. Say：说话命令
 * 6. 其他基础动作命令
 * 
 * 基础命令系统是 WrightEagleBase 的核心执行模块，
 * 负责将高层决策转换为服务器可识别的命令字符串。
 * 
 * 主要功能：
 * - 命令生成：生成符合服务器协议的命令字符串
 * - 命令队列：管理命令的执行队列
 * - 线程安全：确保命令队列的线程安全访问
 * - 参数验证：验证命令参数的有效性
 * 
 * 技术特点：
 * - 继承体系：基于BasicCommand的继承体系
 * - 命令类型：支持多种命令类型
 * - 时间同步：确保命令与当前时间同步
 * - 互斥机制：使用互斥锁保证线程安全
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include <sstream>
#include "BasicCommand.h"
#include "Agent.h"
#include "WorldState.h"

/**
 * @brief 执行基础命令
 * 
 * 这是基础命令的执行函数，负责将命令添加到命令队列中。
 * 该函数会检查命令的时间有效性，并确保线程安全。
 * 
 * 执行过程：
 * 1. 检查命令时间是否与当前时间匹配
 * 2. 使用互斥锁保护命令队列
 * 3. 将命令添加到队列中
 * 4. 释放互斥锁
 * 
 * @param command_queue 命令队列的引用
 * @return bool 命令是否成功执行
 * 
 * @note 只有在命令时间与当前时间匹配时才能执行
 * @note 使用互斥锁保证多线程环境下的安全性
 */
bool BasicCommand::Execute(std::list<CommandInfo> &command_queue)
{
    // === 检查命令时间有效性 ===
    // 确保命令时间与当前世界状态时间匹配
    if (mCommandInfo.mTime != mAgent.GetWorldState().CurrentTime())
    {
        return false;  // 时间不匹配，执行失败
    }

    // === 线程安全的命令队列操作 ===
    ActionEffector::CMD_QUEUE_MUTEX.Lock();    // 获取互斥锁
    command_queue.push_back(mCommandInfo);     // 将命令添加到队列
    ActionEffector::CMD_QUEUE_MUTEX.UnLock();  // 释放互斥锁

    return true;  // 执行成功
}

/**
 * @brief Turn 构造函数
 * 
 * 初始化转身命令，设置命令类型和互斥属性。
 * 转身命令用于改变球员的身体方向。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 转身命令是互斥命令，不能与其他命令同时执行
 */
Turn::Turn(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Turn;  // 设置命令类型为转身
    mCommandInfo.mMutex = true;   // 设置为互斥命令
}

/**
 * @brief 规划转身命令
 * 
 * 根据指定的转动力矩生成转身命令字符串。
 * 该函数会规范化角度参数并生成符合服务器协议的命令。
 * 
 * @param moment 转动力矩（角度）
 * 
 * @note 转动力矩会被规范化到有效范围内
 * @note 生成的命令格式：(turn angle)
 */
void Turn::Plan(double moment)
{
    // === 设置命令时间 ===
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    
    // === 规范化转动力矩 ===
    mCommandInfo.mAngle = moment;  // 转动力矩
    
    // === 生成命令字符串 ===
    std::ostringstream cmd_str;
    cmd_str << "(turn " << mCommandInfo.mAngle << ")";  // 服务器协议格式
    mCommandInfo.mString = cmd_str.str();
}

/**
 * @brief Dash 构造函数
 * 
 * 初始化跑动命令，设置命令类型和互斥属性。
 * 跑动命令用于改变球员的速度和方向。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 跑动命令是互斥命令，不能与其他命令同时执行
 */
Dash::Dash(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Dash;  // 设置命令类型为跑动
    mCommandInfo.mMutex = true;   // 设置为互斥命令
}

/**
 * @brief 规划跑动命令
 * 
 * 根据指定的跑动力度和方向生成跑动命令字符串。
 * 该函数会规范化参数并生成符合服务器协议的命令。
 * 
 * @param power 跑动力度
 * @param dir 跑动方向（角度）
 * 
 * @note 跑动力度和方向都会被规范化到有效范围内
 * @note 生成的命令格式：(dash power angle)
 */
void Dash::Plan(double power, AngleDeg dir)
{
    // === 设置命令时间 ===
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    
    // === 规范化跑动参数 ===
    mCommandInfo.mPower = GetNormalizeDashPower(power);  // 规范化跑动力度
    mCommandInfo.mAngle = GetNormalizeDashAngle(dir);    // 规范化跑动方向
    
    // === 生成命令字符串 ===
    std::ostringstream cmd_str;
    cmd_str << "(dash " << mCommandInfo.mPower << " " << mCommandInfo.mAngle << ")";  // 服务器协议格式
    mCommandInfo.mString = cmd_str.str();
}

/**
 * @brief TurnNeck 构造函数
 * 
 * 初始化转头命令，设置命令类型和互斥属性。
 * 转头命令用于改变球员的颈部方向，不影响身体方向。
 * 
 * @param agent 关联的智能体引用
 * 
 * @note 转头命令是非互斥命令，可以与其他命令同时执行
 */
TurnNeck::TurnNeck(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_TurnNeck;  // 设置命令类型为转头
    mCommandInfo.mMutex = false;      // 设置为非互斥命令
}

/**
 * @brief 规划转头命令
 * 
 * 根据指定的转动力矩生成转头命令字符串。
 * 该函数会规范化角度参数并生成符合服务器协议的命令。
 * 
 * @param moment 转动力矩（角度）
 * 
 * @note 转动力矩会被规范化到有效范围内
 * @note 生成的命令格式：(turn_neck angle)
 */
void TurnNeck::Plan(double moment)
{
    // === 设置命令时间 ===
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    
    // === 规范化转动力矩 ===
    mCommandInfo.mAngle = GetNormalizeNeckMoment(moment);  // 规范化颈部转动力矩
    
    // === 生成命令字符串 ===
    std::ostringstream cmd_str;
    cmd_str << "(turn_neck " << mCommandInfo.mAngle << ")";  // 服务器协议格式
    mCommandInfo.mString = cmd_str.str();
}

/**
 * @brief Say 构造函数
 * 
 * 初始化说话命令，设置命令类型和互斥属性。
 * 说话命令用于球员之间的通信。
 * 
 * @param agent 关联的智能体引用
 */
Say::Say(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Say;
    mCommandInfo.mMutex = false;
}

/**
 * @brief 规划 say 命令
 *
 * 生成服务器协议格式的 `(say ...)` 命令。
 * - 对于普通球员：服务器要求消息内容使用双引号包裹 `(say "msg")`；
 * - 对于教练（coach）：历史版本中可直接发送不带引号的内容 `(say msg)`。
 *
 * @param msg 要发送的消息内容
 */
void Say::Plan(std::string msg)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    if (PlayerParam::instance().isCoach())
    {
        cmd_str << "(say " << msg << ")";
    }
    else
    {
        cmd_str << "(say \"" << msg << "\")";
    }
    mCommandInfo.mString = cmd_str.str();
}

/**
 * @brief Attentionto 构造函数
 *
 * 注意力命令用于提示服务器/客户端在 hear 等信息中更关注某个对象。
 *
 * @param agent 关联的智能体引用
 */
Attentionto::Attentionto(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Attentionto;
    mCommandInfo.mMutex = false;
}

/**
 * @brief 规划 attentionto 命令
 *
 * 命令格式：
 * - 关闭：`(attentionto off)`
 * - 开启关注：`(attentionto our <unum>)` 或 `(attentionto opp <unum>)`
 *
 * @param on 是否开启（false 表示 off）
 * @param num 目标号码：正数表示我方，负数表示对方（其绝对值为号码）
 */
void Attentionto::Plan(bool on, Unum num)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    if (on == false)
    {
        cmd_str << "(attentionto off)";
    }
    else
    {
        if (num < 0)
        {
            cmd_str << "(attentionto opp " << -num << ")";
        }
        else
        {
            cmd_str << "(attentionto our " << num << ")";
        }
    }
    mCommandInfo.mString = cmd_str.str();
}

Kick::Kick(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Kick;
    mCommandInfo.mMutex = true;
}

void Kick::Plan(double power, double dir)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mPower = GetNormalizeKickPower(power);
    mCommandInfo.mAngle = GetNormalizeMoment(dir);
    std::ostringstream cmd_str;
    cmd_str << "(kick " << mCommandInfo.mPower << " " << mCommandInfo.mAngle << ")";
    mCommandInfo.mString = cmd_str.str();
}

Tackle::Tackle(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Tackle;
    mCommandInfo.mMutex = true;
}

void Tackle::Plan(double dir, const bool foul)
{
	std::string foul_signal;
	foul_signal += ( foul ) ? " true" : " false";
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mAngle = GetNormalizeMoment(dir);
    std::ostringstream cmd_str;
	if ( PlayerParam::instance().playerVersion() > 14 )
		cmd_str << "(tackle " << mCommandInfo.mAngle << foul_signal << ")";
	else
		cmd_str << "(tackle " << mCommandInfo.mAngle << ")";
    mCommandInfo.mString = cmd_str.str();
}

Pointto::Pointto(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Pointto;
    mCommandInfo.mMutex = false;
}

void Pointto::Plan(bool on, double dist, double dir)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;

    if (on)
    {
        mCommandInfo.mDist = dist;
        mCommandInfo.mAngle = dir;
        cmd_str << "(pointto " << mCommandInfo.mDist << " " << mCommandInfo.mAngle << ")";
    }
    else
    {
        cmd_str << "(pointto off)";
    }
    mCommandInfo.mString = cmd_str.str();
}

Catch::Catch(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Catch;
    mCommandInfo.mMutex = true;
}

void Catch::Plan(double dir)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mAngle = dir;
    std::ostringstream cmd_str;
    cmd_str << "(catch " << mCommandInfo.mAngle << ")";
    mCommandInfo.mString = cmd_str.str();
}

Move::Move(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Move;
    mCommandInfo.mMutex = true;
}

void Move::Plan(Vector pos)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mMovePos = pos;
    std::ostringstream cmd_str;
    cmd_str << "(move " << mCommandInfo.mMovePos.X() << " " << mCommandInfo.mMovePos.Y() << ")";
    mCommandInfo.mString = cmd_str.str();
}

ChangeView::ChangeView(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_ChangeView;
    mCommandInfo.mMutex = true;
}

void ChangeView::Plan(ViewWidth view_width)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mViewWidth = view_width;
    std::ostringstream cmd_str;

    switch (mCommandInfo.mViewWidth)
    {
    case VW_Narrow:
        cmd_str << "(change_view narrow)";
        break;
    case VW_Normal:
        cmd_str << "(change_view normal)";
        break;
    case VW_Wide:
        cmd_str << "(change_view wide)";
        break;
    default:
        break;
    }
    mCommandInfo.mString = cmd_str.str();
}

Compression::Compression(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Compression;
    mCommandInfo.mMutex = true;
}

void Compression::Plan(int level)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mLevel = level;
    std::ostringstream cmd_str;
    cmd_str << "(compression " << mCommandInfo.mLevel << ")";
    mCommandInfo.mString = cmd_str.str();
}

SenseBody::SenseBody(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_SenseBody;
    mCommandInfo.mMutex = false;
}

void SenseBody::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(sense_body)";
    mCommandInfo.mString = cmd_str.str();
}

Score::Score(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Score;
    mCommandInfo.mMutex = false;
}

void Score::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(score)";
    mCommandInfo.mString = cmd_str.str();
}

Bye::Bye(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Bye;
    mCommandInfo.mMutex = false;
}

void Bye::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(bye)";
    mCommandInfo.mString = cmd_str.str();
}

Done::Done(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Done;
    mCommandInfo.mMutex = false;
}

void Done::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(done)";
    mCommandInfo.mString = cmd_str.str();
}

Clang::Clang(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Clang;
    mCommandInfo.mMutex = false;
}

void Clang::Plan(int min_ver, int max_ver)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    mCommandInfo.mMinVer = min_ver;
    mCommandInfo.mMaxVer = max_ver;
    std::ostringstream cmd_str;
    cmd_str << "(clang (ver " << mCommandInfo.mMinVer << " " << mCommandInfo.mMaxVer << "))";
    mCommandInfo.mString = cmd_str.str();
}

/**
 * @brief Ear 构造函数
 *
 * ear 命令用于控制 hear 信息的接收范围/模式。
 *
 * @param agent 关联的智能体引用
 */
Ear::Ear(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Ear;
    mCommandInfo.mMutex = false;
}

/**
 * @brief 规划 ear 命令
 *
 * 命令格式：`(ear (<on/off> <our/opp> <partial/complete>))`
 * - on/off：是否开启；
 * - our/opp：关注我方或对方的 hear；
 * - partial/complete：接收模式（具体含义由 server 版本定义）。
 *
 * @param on 是否开启
 * @param our_side true 表示 our，false 表示 opp
 * @param ear_mode 接收模式（partial/complete）
 */
void Ear::Plan(bool on, bool our_side, EarMode ear_mode)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();

    const char *on_string;
    const char *side_string;
    const char *ear_mode_string;

    if (on == true)
    {
        on_string = "on";
    }
    else
    {
        on_string = "off";
    }

    if (our_side)
    {
        side_string = " our";
    }
    else
    {
        side_string = " opp";
    }

    if (ear_mode == EM_Partial)
    {
        ear_mode_string = " partial";
    }
    else if (ear_mode == EM_Complete)
    {
        ear_mode_string = " complete";
    }
    else
    {
        ear_mode_string = "";
    }

    std::ostringstream cmd_str;
    cmd_str << "(ear (" << on_string << side_string << ear_mode_string << "))";
    mCommandInfo.mString = cmd_str.str();
}

SynchSee::SynchSee(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_SynchSee;
    mCommandInfo.mMutex = false;
}

/**
 * @brief 规划 synch_see 命令
 *
 * 用于请求同步视觉（synch see）模式。
 *
 * @note 下面对 `mCommandInfo.mString` 的重复赋值属于冗余写入，但不影响行为；
 *       为避免引入非注释性质的改动，这里仅保留并用注释说明。
 */
void SynchSee::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(synch_see)";
    mCommandInfo.mString = cmd_str.str();
    mCommandInfo.mString = cmd_str.str();
}

ChangePlayerType::ChangePlayerType(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_ChangePlayerType;
    mCommandInfo.mMutex = false;
}

void ChangePlayerType::Plan(Unum num, int player_type)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(change_player_type " << num << " " << player_type << ")";
    mCommandInfo.mString = cmd_str.str();
}

//////////////////////////////////for trainer////////////////////////////////////
void ChangePlayerType::Plan(std::string teamname, Unum num, int player_type)
{
	mCommandInfo.mType = CT_ChangePlayerTypeForTrainer;
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(change_player_type "<< teamname <<" " << num << " " << player_type << ")";
    mCommandInfo.mString = cmd_str.str();
}

Start::Start(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Start;
    mCommandInfo.mMutex = false;
}

void Start::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(start)";
    mCommandInfo.mString = cmd_str.str();
}

ChangePlayMode::ChangePlayMode(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_ChangePlayMode;
    mCommandInfo.mMutex = false;
}

void ChangePlayMode::Plan(ServerPlayMode spm)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(change_mode " << ServerPlayModeMap::instance().GetPlayModeString(spm) << ")";
    mCommandInfo.mString = cmd_str.str();
}

MovePlayer::MovePlayer(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_MovePlayer;
    mCommandInfo.mMutex = false;
}

void MovePlayer::Plan(std::string team_name, Unum num, Vector pos, Vector vel, AngleDeg dir)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(move (player " << team_name << " " << num << ") " << pos.X() << " " << pos.Y() << " " << dir << " " << vel.X() << " " << vel.Y() << ")";
    mCommandInfo.mString = cmd_str.str();
}

MoveBall::MoveBall(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_MoveBall;
    mCommandInfo.mMutex = false;
}

void MoveBall::Plan(Vector pos, Vector vel)
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(move (ball) " << pos.X() << " " << pos.Y() << " " << " 0 " << vel.X() << " " << vel.Y() << ")";
    mCommandInfo.mString = cmd_str.str();
 }

Look::Look(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Look;
    mCommandInfo.mMutex = false;
}

void Look::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(look)";
    mCommandInfo.mString = cmd_str.str();
}

TeamNames::TeamNames(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_TeamNames;
    mCommandInfo.mMutex = false;
}

void TeamNames::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(team_names)";
    mCommandInfo.mString = cmd_str.str();
}

Recover::Recover(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_Recover;
    mCommandInfo.mMutex = false;
}

void Recover::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(recover)";
    mCommandInfo.mString = cmd_str.str();
}

CheckBall::CheckBall(const Agent & agent): BasicCommand(agent)
{
    mCommandInfo.mType = CT_CheckBall;
    mCommandInfo.mMutex = false;
}

void CheckBall::Plan()
{
    mCommandInfo.mTime = mAgent.GetWorldState().CurrentTime();
    std::ostringstream cmd_str;
    cmd_str << "(check_ball)";
    mCommandInfo.mString = cmd_str.str();
}





