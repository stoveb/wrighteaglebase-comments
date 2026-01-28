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
 * @file Client.h
 * @brief 客户端核心（Client）接口定义
 *
 * Client 是 WrightEagleBase 客户端侧的主控制器之一，负责组织并协调多个核心模块：
 * - `Observer`：感知/消息接收后的结构化数据容器；
 * - `WorldModel`：世界模型（WorldState/HistoryState 等）的组织与更新；
 * - `Agent`：决策与动作生成入口；
 * - `Parser` / `CommandSender`：网络消息解析与命令发送。
 *
 * Client 提供动态调试与正常比赛两种运行入口，并通过 `MainLoop()` 驱动每周期的
 * “感知-更新-决策-执行”流程。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#ifndef CLIENT_H_
#define CLIENT_H_

class Observer;
class WorldModel;
class Agent;
class Parser;
class CommandSender;

class Client {
	friend class Player;
	friend class Coach;
	friend class Trainer;

	Observer        *mpObserver;
	WorldModel      *mpWorldModel;
	Agent           *mpAgent;

	Parser 		    *mpParser;
	CommandSender   *mpCommandSender;

public:
	Client();
	virtual ~Client();

	/**
	* 动态调试时球员入口函数
	*/
	void RunDynamicDebug();

	/**
	* 正常比赛时的球员入口函数
	*/
	void RunNormal();

	/**
	* 正常比赛时的球员主循环函数
	*/
	void MainLoop();

	/**
	 * 创建Agent，并完成相关调用
	 */
	void ConstructAgent();

	/**
	* 球员决策函数，每周期执行1次
	*/
	virtual void Run() = 0;

	/**
	* 给server发送一些选项，如synch_see,eye_on等
	*/
	virtual void SendOptionToServer() = 0;
};

#endif /* CLIENT_H_ */
