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
  * @file CommandSender.cpp
  * @brief 命令发送线程（CommandSender）实现
  *
  * CommandSender 的职责是将 Agent 在每个周期生成的动作命令
  * （dash/turn/kick/say/turn_neck 等）统一打包并通过网络发送给 rcssserver。
  *
  * 典型运行方式：
  * - 在独立线程中循环等待 `Observer` 允许发送（`WaitForCommandSend()`）。
  * - 触发一次发送后，调用 `Agent::SendCommands()` 将本周期指令写入缓冲区 `msg`。
  * - 记录调试信息（DynamicDebug）与网络时间点（NetworkTest），便于性能分析。
 */

#include "CommandSender.h"
#include "DynamicDebug.h"
#include "Observer.h"
#include "Agent.h"
#include "Logger.h"
#include "NetworkTest.h"

/**
 * @brief 构造函数
 *
 * @param pObserver Observer 指针，用于：
 * - 判断是否到达“可发送命令”的时机
 * - 获取当前时间戳，用于网络测试统计
 */
CommandSender::CommandSender(Observer *pObserver): mpObserver(pObserver), mpAgent(0)
{
}

/**
 * @brief 析构函数
 */
CommandSender::~CommandSender()
{
}

/**
 * @brief 命令发送线程主循环
 *
 * 该循环会一直等待 Observer 发出“允许发送”的信号。
 * 每次允许发送时：
 * 1. 记录网络发送起点时间（NetworkTest）；
 * 2. 预置消息前缀并调用 `Run()` 让 Agent 将本周期命令写入缓冲；
 * 3. 追加到动态调试消息队列（MT_Send）；
 * 4. 记录网络发送终点时间。
 *
 * @note `msg` 使用静态缓冲区以避免频繁分配；线程安全由调用侧保证。
 */
void CommandSender::StartRoutine()
{
    static char msg[MAX_MESSAGE];

    while (mpObserver->WaitForCommandSend())
    {
        NetworkTest::instance().AddCommandSendBegin();

        strcpy(msg, "record_cmd: ");
        Run(msg);
        DynamicDebug::instance().AddMessage(msg, MT_Send);

        NetworkTest::instance().AddCommandSendEnd(mpObserver->CurrentTime());
    }
}

/**
 * @brief 执行一次命令打包与发送
 *
 * 如果 `mpAgent` 非空，则调用 `Agent::SendCommands(msg)` 将当前周期的指令
 * 追加/写入到消息缓冲中。
 *
 * @param msg 消息缓冲区（外部已写入固定前缀）
 */
void CommandSender::Run(char *msg)
{
    if (mpAgent != 0){
        mpAgent->SendCommands(msg);
    }
    else {
        PRINT_ERROR("null agent");
    }
}
