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
 * @file DynamicDebug.cpp
 * @brief 动态调试与日志回放系统实现
 *
 * DynamicDebug 提供两类能力：
 * 1. **在线记录**：在正常比赛运行时，把 server 消息（解析/决策/发送）以及对应的耗时
 *    记录到磁盘文件中，便于离线分析、复现与性能 profiling。
 * 2. **离线回放/交互调试**：在 DynamicDebugMode 下，从文件或脚本（dynamicdebug.txt）读取指令，
 *    逐步（step）/跳转（goto）/连续运行（run/runto）地回放记录的消息序列。
 *
 * 数据组织方式（保存 server message 时）：
 * - `mMessageTable`：按时间顺序存储消息字符串及其类型（MT_Parse/MT_Run/MT_Send）。
 * - `mIndexTable`：每条消息的索引信息（server time、数据大小、数据偏移、耗时表偏移等）。
 * - `mParserTimeTable/mDecisionTimeTable/mCommandSendTimeTable`：分别记录三个阶段的耗时。
 * - `Flush()`：在退出时将上述表写入文件，并回填文件头（DD + mFileHead）。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include <cstring>
#include "DynamicDebug.h"

//==============================================================================
/**
 * @brief 构造函数
 *
 * 初始化动态调试系统的内部状态。
 * - 未初始化时 `mInitialOK=false`。
 * - 各类“回放指针/表指针/文件句柄”置空。
 * - 保存 cin 的 streambuf，以便在动态调试脚本与标准输入之间切换。
 */
DynamicDebug::DynamicDebug()
{
	mpObserver          = 0;
	mInitialOK          = false;

	mpIndex             = 0;
	mpParserTime        = 0;
	mpDecisionTime      = 0;
	mpCommandSendTime   = 0;
	mpCurrentIndex      = 0;

	mpFile              = 0;
	mpFileStream        = 0;
	mpStreamBuffer      = std::cin.rdbuf(); // 保存std::cin的流，后面要重定向

	mRunning            = false;
	mShowMessage        = false;
	mRuntoCycle         = Time(-3, 0);
}

//==============================================================================
/**
 * @brief 析构函数
 *
 * 负责在退出时调用 `Flush()`（若启用了记录），并释放 load 回放时分配的数组。
 */
DynamicDebug::~DynamicDebug()
{
	Flush();

	if (mpIndex != 0)
	{
		delete[] mpIndex;
	}

	if (mpParserTime != 0)
	{
		delete[] mpParserTime;
	}

	if (mpDecisionTime != 0)
	{
		delete[] mpDecisionTime;
	}

	if (mpCommandSendTime != 0)
	{
		delete[] mpCommandSendTime;
	}
}


//==============================================================================
/**
 * @brief 获取 DynamicDebug 单例
 *
 * @return DynamicDebug& 单例引用
 */
DynamicDebug & DynamicDebug::instance()
{
	static DynamicDebug dynamic_debug;
	return dynamic_debug;
}


//==============================================================================
/**
 * @brief 初始化 DynamicDebug
 *
 * 初始化依赖：Observer 指针、输入流重定向、日志文件句柄、索引表缓存等。
 *
 * - DynamicDebugMode=true：从 dynamicdebug.txt 读取调试命令（重定向 std::cin）。
 * - 否则（正常比赛）：若 SaveServerMessage=true，则打开 msg.log 文件并准备索引表。
 *
 * @param pObserver Observer 指针（用于获取当前时间、自身 unum 等）。
 */
void DynamicDebug::Initial(Observer *pObserver)
{
	if (mInitialOK == true)
	{
		return;
	}

	if (pObserver == 0)
	{
		PRINT_ERROR("Observer Null Pointer");
		return;
	}

	mpObserver = pObserver;

	if (PlayerParam::instance().DynamicDebugMode() == true) // 动态调试
	{
		mpFileStream = new std::ifstream("dynamicdebug.txt");
		if (mpFileStream)
		{
			std::cin.rdbuf(mpFileStream->rdbuf());
		}
	}
	else // 正常比赛时
	{
		if (PlayerParam::instance().SaveServerMessage() == true) // 需要保存server信息
		{
			char file_name[64];
			sprintf(file_name, "%s/%s-%d-msg.log", PlayerParam::instance().logDir().c_str(), PlayerParam::instance().teamName().c_str(), mpObserver->SelfUnum());
			mpFile = fopen(file_name, "wb");
			if (mpFile){
				if (setvbuf(mpFile, 0, _IOFBF, 1024 * 8192) != 0)
				{
					PRINT_ERROR("set buffer for file \"" << file_name << "\" error");
				}
			}
			else {
				PRINT_ERROR("open file \"" << file_name << "\" error");
			}
		}

		if (mpFile != 0)
		{
			fseek(mpFile, sizeof(mFileHead) + 2 * sizeof( char ), SEEK_SET); // 留出mFileHead的地方，最后再填
			mFileHead.mIndexTableSize = 0;
			mIndexTable.reserve(8192);
			mMessageTable.reserve(8192);
		}
	}
	mInitialOK = true;
}


//==============================================================================
/**
 * @brief 记录一条消息（仅在 SaveServerMessage 且非 DynamicDebugMode 时生效）
 *
 * 该接口通常在 3 个阶段被调用：
 * - MT_Parse：解析 server 消息后记录
 * - MT_Run：决策/运行阶段记录
 * - MT_Send：发送命令阶段记录
 *
 * 为保证多线程下写入一致性，这里会对文件相关缓存加锁。
 *
 * @param msg 消息内容（C 字符串）
 * @param msg_type 消息类型
 */
void DynamicDebug::AddMessage(const char *msg, MessageType msg_type)
{
    if (PlayerParam::instance().SaveServerMessage() && 
        !PlayerParam::instance().DynamicDebugMode())
    {
	    if (!mInitialOK)
	    {
		    return; // 没有初始化或者不用记录server信息都返回
	    }

	    mFileMutex.Lock(); // 有可能多个线程同时在写文件，所以要保证互斥

	    MessageIndexTableUnit index_table_unit;
	    index_table_unit.mServerTime = mpObserver->CurrentTime();
	    index_table_unit.mDataSize = strlen(msg);

	    switch (msg_type)
	    {
	    case MT_Parse:
		    index_table_unit.mTimeOffset = mParserTimeTable.size();
		    break;
	    case MT_Run:
		    index_table_unit.mTimeOffset = mDecisionTimeTable.size();
		    break;
	    case MT_Send:
		    index_table_unit.mTimeOffset = mCommandSendTimeTable.size();
		    break;
	    default:
		    break;
	    }

	    mMessageTable.push_back( Message(msg_type, msg));
	    mIndexTable.push_back(index_table_unit);
	    mFileHead.mMaxCycle = Max(mFileHead.mMaxCycle, index_table_unit.mServerTime);

	    mFileMutex.UnLock();
    }
}


//==============================================================================
/**
 * @brief 记录一次解析耗时
 */
void DynamicDebug::AddTimeParser(timeval &time)
{
	if (PlayerParam::instance().SaveServerMessage() && 
        !PlayerParam::instance().DynamicDebugMode())
    {
		mParserTimeTable.push_back(time);
	}
}


//==============================================================================
/**
 * @brief 记录一次决策耗时
 */
void DynamicDebug::AddTimeDecision(timeval &time)
{
	if (PlayerParam::instance().SaveServerMessage() && 
        !PlayerParam::instance().DynamicDebugMode())
    {
		mDecisionTimeTable.push_back(time);
	}
}


//==============================================================================
/**
 * @brief 记录一次发送命令耗时
 */
void DynamicDebug::AddTimeCommandSend(timeval &time)
{
	if (PlayerParam::instance().SaveServerMessage() && 
        !PlayerParam::instance().DynamicDebugMode())
    {
		mCommandSendTimeTable.push_back(time);
	}
}


//==============================================================================
/**
 * @brief 动态调试命令主入口
 *
 * 在 DynamicDebugMode 下，该函数会从输入流读取调试指令并驱动回放。
 * 支持的典型指令：
 * - load：加载某个 dynamicdebug 日志文件
 * - step/s：前进一步
 * - goto/g：跳到指定 cycle
 * - run/r：连续运行
 * - runto/rt：运行到指定时间
 * - msg/m：切换是否打印消息内容
 * - quit/q：退出
 *
 * @param msg 输出缓冲区（用于返回本次取出的消息）
 * @return MessageType 当前返回的消息类型（MT_Parse/MT_Run/MT_Send/MT_Null）
 */
MessageType DynamicDebug::Run(char *msg)
{
	std::cerr << std::endl << mpObserver->CurrentTime(); // 输出当前周期

	if (mRunning == true)
	{
		if (mRuntoCycle >= Time(0, 0))
		{
			if(mRuntoCycle <= mpObserver->CurrentTime())
			{
				mRunning = false;
				mRuntoCycle = 0;
			}
		}

		if (mRunning){
			return GetMessage(msg);
		}
	}

	std::string read_msg;
	while (std::cin)
	{
        if (std::cin.eof())
        {
            std::cin.rdbuf(mpStreamBuffer);
        }

		std::cerr << std::endl << ">>> ";
		std::cin >> read_msg;

		if (*read_msg.c_str() == '\0')
		{
			std::cin.rdbuf(mpStreamBuffer); // 到达文件末尾会读入'\0'，这里重定向
			continue;
		}

		if (read_msg == "load")
		{
			std::string file_name;
			std::cin >> file_name;
			mpFile = fopen(file_name.c_str(), "rb");
			if (mpFile == 0)
			{
				std::cerr << "Can't open dynamicdebug file, exit..." << std::endl;
				continue;
			}

			char ch1( 0 ),ch2( 0 );

			if ( fread( &ch1, sizeof( char ), 1, mpFile ) < 1 )
			{
				Assert( 0 );
			}

			if ( fread( &ch2, sizeof( char ), 1, mpFile ) < 1 )
			{
				Assert( 0 );
			}

			if ( ch1 != 'D' || ch2 != 'D' )
			{
				std::cerr<<"Not a dynamicdebug logfile!"<<std::endl;
				return MT_Null;
			}

			if (fread(&mFileHead, sizeof(mFileHead), 1, mpFile) < 1)
            {
                Assert(0);
            }

			long long size;

			size = mFileHead.mIndexTableSize;
			mpIndex = new MessageIndexTableUnit[size];
			fseek(mpFile, mFileHead.mIndexTableOffset, SEEK_SET);
			if (size > 0 && fread(mpIndex, size * sizeof(MessageIndexTableUnit), 1, mpFile) < 1)
            {
                Assert(0);
            }

			size = mFileHead.mParserTableSize;
			mpParserTime = new timeval[size];
			fseek(mpFile, mFileHead.mParserTableOffset, SEEK_SET);
			if (size > 0 && fread(mpParserTime, size * sizeof(timeval), 1, mpFile) < 1)
            {
                Assert(0);
            }

			size = mFileHead.mDecisionTableSize;
			mpDecisionTime = new timeval[size];
			fseek(mpFile, mFileHead.mDecisionTableOffset, SEEK_SET);
			if (size > 0 && fread(mpDecisionTime, size * sizeof(timeval), 1, mpFile) < 1)
            {
                Assert(0);
            }

			size = mFileHead.mCommandSendTableSize;
			mpCommandSendTime = new timeval[size];
			fseek(mpFile, mFileHead.mCommandSendTableOffset, SEEK_SET);
			if (size > 0 && fread(mpCommandSendTime, size * sizeof(timeval), 1, mpFile) < 1)
            {
                Assert(0);
            }

			fseek(mpFile, sizeof(mFileHead) + 2 * sizeof( char ), SEEK_SET);
			mpCurrentIndex = mpIndex; // load后，第一个为初始化信息，先进行初始化

			std::cerr << "Load finished." << std::endl;
			fseek(mpFile, mpCurrentIndex->mDataOffset, SEEK_SET);
			if (fread(msg, 1, 1, mpFile) < 1)
            {
                Assert(0);
            }
            if (mpCurrentIndex->mDataSize > 0 && fread(msg, mpCurrentIndex->mDataSize, 1, mpFile) < 1)
            {
                Assert(0);
            }
			msg[mpCurrentIndex->mDataSize] = '\0';
			return MT_Parse;
		}
		else if (read_msg == "step" || read_msg == "s")
		{
			if (mpCurrentIndex == 0)
			{
				std::cerr << "no file loaded!";
				continue;
			}
			return GetMessage(msg);
		}
		else if (read_msg == "goto" || read_msg == "g")
		{
			if (mpCurrentIndex == 0)
			{
				std::cerr << "no file loaded!";
				continue;
			}

			int cycle;
			std::cin >> cycle;
			if (FindCycle(cycle) == true)
			{
				std::cerr << "goto finished ..." << std::endl;
			}
			else
			{
				std::cerr << "no such cycle ..." << std::endl;
			}
			continue;
		}
		else if (read_msg == "runto" || read_msg == "rt")
		{
			if (mpCurrentIndex == 0)
			{
				std::cerr << "no file loaded!";
				continue;
			}

			std::string str;
			std::cin >> str;

			char a;
			int t; // time
			int s; // stop time
			if (sscanf(str.c_str(), "%d%c%d", &t, &a, &s) != 3)
			{
				s = 0;
			}

			mRuntoCycle = Time(t, s);

			if (mRuntoCycle == mpObserver->CurrentTime())
			{
				std::cerr << "already here ...";
				continue;
			}
			else if (mRuntoCycle < mpObserver->CurrentTime())
			{
				std::cerr << "can not run to previous cycle";
				continue;
			}
			else
			{
				mRunning = true;
				return GetMessage(msg);
			}
		}
		else if (read_msg == "run" || read_msg == "r")
		{
			mRuntoCycle = -1;
			mRunning = true;
			return GetMessage(msg);
		}
		else if (read_msg == "msg" || read_msg == "m")
		{
			mShowMessage = !mShowMessage;
			std::cerr << "Set ShowMessage: " << mShowMessage << std::endl;
			continue;
		}
		else if (read_msg == "quit" || read_msg == "q")
		{
			std::cerr << "Bye ..." << std::endl;
			return MT_Null;
		}
		else
		{
			std::cerr << "Error command, only the following commands are aviliable: " << std::endl;
			std::cerr << "\tload"<< std::endl;
			std::cerr << "\tstep(s)" <<std::endl;
			std::cerr << "\trunto(rt)" <<std::endl;
			std::cerr << "\tgoto(g)" <<std::endl;
			std::cerr << "\trun(r)" << std::endl;
			std::cerr << "\tmsg(m)" << std::endl;
			std::cerr << "\ttype(t)" << std::endl;
			std::cerr << "\tquit(q)" << std::endl;
			continue;
		}
	}

	return MT_Null;
}


//==============================================================================
/**
 * @brief 取得下一条回放消息
 *
 * 根据 `mpCurrentIndex` 指向的索引表单元，
 * 从日志文件中读取消息类型与消息字符串到 `msg`。
 *
 * @param msg 输出缓冲区
 * @return MessageType 消息类型
 */
MessageType DynamicDebug::GetMessage(char *msg)
{
	if (mpCurrentIndex == 0)
	{
		return MT_Null;
	}

	if (mpCurrentIndex->mServerTime >= mFileHead.mMaxCycle)
	{
		std::cerr << "End ..." << std::endl;
		return MT_Null;
	}
	else
	{
		++mpCurrentIndex;
	}

	fseek(mpFile, mpCurrentIndex->mDataOffset, SEEK_SET);
	if (fread(msg, 1, 1, mpFile) < 1) // 读取信息类型
    {
        Assert(0);
    }
	MessageType msg_type = (MessageType)msg[0];

    if (mpCurrentIndex->mDataSize > 0 && fread(msg, mpCurrentIndex->mDataSize, 1, mpFile) < 1) // 读取信息内容
    {
        Assert(0);
    }
	msg[mpCurrentIndex->mDataSize] = '\0';

	if (mShowMessage == true)
	{
		std::cerr << msg << std::endl;
	}

	return msg_type;
}


//==============================================================================
/**
 * @brief 在索引表中查找指定 cycle
 *
 * 使用二分查找在 `mpIndex`（索引表数组）中定位目标周期，
 * 并将 `mpCurrentIndex` 指向对应项。
 *
 * @param cycle 目标周期（cycle）
 * @return bool 是否找到
 */
bool DynamicDebug::FindCycle(int cycle)
{
	Time cycle_time(cycle, 0);
	if (cycle_time == mpObserver->CurrentTime())
	{
		return true;
	}

	int  begin = 0;
	int end = mFileHead.mIndexTableSize - 1;
	int mid;
	while (begin < end)
	{
		mid = (begin + end) / 2;
		if (mpIndex[mid].mServerTime == cycle_time)
		{
			mpCurrentIndex = &mpIndex[mid];
			return true;
		}
		else if (mpIndex[mid].mServerTime < cycle_time)
		{
			begin = mid;
		}
		else
		{
			end = mid;
		}
	}
	return false;
}


//==============================================================================
/**
 * @brief 获取下一条解析耗时
 */
timeval DynamicDebug::GetTimeParser()
{
	timeval time_val = mpParserTime[mpCurrentIndex->mTimeOffset++];
	return time_val;
}


//==============================================================================
/**
 * @brief 获取下一条决策耗时
 */
timeval DynamicDebug::GetTimeDecision()
{
	timeval time_val = mpDecisionTime[mpCurrentIndex->mTimeOffset++];
	return time_val;
}


//==============================================================================
/**
 * @brief 获取下一条发送命令耗时
 */
timeval DynamicDebug::GetTimeCommandSend()
{
	timeval time_val = mpCommandSendTime[mpCurrentIndex->mTimeOffset++];
	return time_val;
}



/**
 * @brief 将缓存的消息/索引/耗时表写入文件
 *
 * 仅在 SaveServerMessage=true 且非 DynamicDebugMode 时生效。
 * 写入顺序：
 * 1. 逐条写 message（类型字节 + 字符串），并回填每条消息的 data offset；
 * 2. 写 index table；
 * 3. 写 parser/decision/commandSend 三个耗时表；
 * 4. 回到文件头位置写入 "DD" 与 mFileHead。
 */
void DynamicDebug::Flush()
{
    if (PlayerParam::instance().SaveServerMessage() && 
        !PlayerParam::instance().DynamicDebugMode())
    {
	    if (mpFile != 0)
	    {
		    long long i = 0; // 循环变量
		    //mFileHead.mHeadFlag[0] = 'D';
		    //mFileHead.mHeadFlag[1] = 'D';

		    // 赋值4个表的大小，后面将要写到文件中
		    mFileHead.mIndexTableSize = mIndexTable.size();
		    mFileHead.mParserTableSize = mParserTimeTable.size();
		    mFileHead.mDecisionTableSize = mDecisionTimeTable.size();
		    mFileHead.mCommandSendTableSize = mCommandSendTimeTable.size();

		    //message
		    long long size = mFileHead.mIndexTableSize;
		    for (i = 0; i < size; ++i) {
			    mIndexTable[i].mDataOffset = ftell(mpFile);
			    fwrite(& mMessageTable[i].mType, 1, 1, mpFile); // 先写入消息类型
			    fwrite(mMessageTable[i].mString.c_str(), mIndexTable[i].mDataSize, 1, mpFile);
		    }

		    // index table
		    mFileHead.mIndexTableOffset = ftell(mpFile);
		    mpIndex = new MessageIndexTableUnit[size];
		    for (i = 0; i < size; ++i)
		    {
			    memcpy(&mpIndex[i], &mIndexTable[i], sizeof(MessageIndexTableUnit));
		    }
		    fwrite(mpIndex, size * sizeof(MessageIndexTableUnit), 1, mpFile);

		    // observer time
		    mFileHead.mParserTableOffset = ftell(mpFile);
		    size = mFileHead.mParserTableSize;
		    mpParserTime = new timeval[size];
		    for (i = 0; i < size; ++i)
		    {
			    mpParserTime[i] = mParserTimeTable[i];
		    }
		    fwrite(mpParserTime, size * sizeof(timeval), 1, mpFile);

		    // decision time
		    mFileHead.mDecisionTableOffset = ftell(mpFile);
		    size = mFileHead.mDecisionTableSize;
		    mpDecisionTime = new timeval[size];
		    for (i = 0; i < size; ++i)
		    {
			    mpDecisionTime[i] = mDecisionTimeTable[i];
		    }
		    fwrite(mpDecisionTime, size * sizeof(timeval), 1, mpFile);

		    // commandsend time
		    mFileHead.mCommandSendTableOffset = ftell(mpFile);
		    size = mFileHead.mCommandSendTableSize;
		    mpCommandSendTime = new timeval[size];
		    for (i = 0; i < size; ++i)
		    {
			    mpCommandSendTime[i] = mCommandSendTimeTable[i];
		    }
		    fwrite(mpCommandSendTime, size * sizeof(timeval), 1, mpFile);

		    fseek(mpFile, 0, SEEK_SET);
			fprintf( mpFile, "DD" );
		    fwrite(&mFileHead, sizeof(mFileHead), 1, mpFile);
	    }

	    if (mpFile != 0)
	    {
		    fclose(mpFile);
	    }
    }
}


//end of file DynamicDebug.cpp

