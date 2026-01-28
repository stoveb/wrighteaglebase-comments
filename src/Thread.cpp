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
 * @file Thread.cpp
 * @brief 线程系统（Thread）实现
 *
 * 本文件提供跨平台（Windows 与 POSIX）的线程原语封装，包括：
 * - ThreadCondition：条件变量，支持带超时等待与唤醒；
 * - ThreadMutex：互斥锁，支持加锁与解锁；
 * - Thread：线程基类，提供启动与等待接口。
 *
 * 设计要点：
 * - Windows 使用 Event/Mutex，POSIX 使用 pthread_cond/pthread_mutex；
 * - 条件变量的 Wait/Set 内部已包含对关联互斥锁的加锁/解锁；
 * - Thread 基类采用静态入口 Spawner 调用纯虚函数 StartRoutine。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "Thread.h"
#include <error.h>


#ifdef WIN32

#include <windows.h>

/**
 * @brief Windows 条件变量构造函数
 */
ThreadCondition::ThreadCondition()
{
	mEvent = CreateEvent(0, false, false, 0);
}

/**
 * @brief Windows 条件变量析构函数
 */
ThreadCondition::~ThreadCondition()
{
}

/**
 * @brief Windows 条件变量等待（可带超时）
 *
 * @param ms 超时毫秒数，0 表示无限等待
 * @return bool 是否超时返回（true 表示超时）
 */
bool ThreadCondition::Wait(int ms)
{
	if (ms == 0)
	{
		ms = INFINITE;
	}

	ResetEvent(mEvent);
	DWORD ret = WaitForSingleObject(mEvent,ms);
	return (ret == WAIT_TIMEOUT);
}

/**
 * @brief Windows 条件变量唤醒
 */
void ThreadCondition::Set()
{
	SetEvent(mEvent);
}

/**
 * @brief Windows 互斥锁构造函数
 */
ThreadMutex::ThreadMutex()
{
	mEvent = CreateMutex(0, false, 0);
}

/**
 * @brief Windows 互斥锁析构函数
 */
ThreadMutex::~ThreadMutex()
{
}

/**
 * @brief Windows 互斥锁加锁
 */
void ThreadMutex::Lock()
{
	WaitForSingleObject(mEvent, 50);
}

/**
 * @brief Windows 互斥锁解锁
 */
void ThreadMutex::UnLock()
{
	ReleaseMutex(mEvent);
}

#else

/**
 * @brief POSIX 条件变量构造函数
 */
ThreadCondition::ThreadCondition()
{
	pthread_mutex_init(&mMutex, 0);
	pthread_cond_init(&mCond, 0);
}

/**
 * @brief POSIX 条件变量析构函数
 */
ThreadCondition::~ThreadCondition()
{
	pthread_mutex_destroy(&mMutex);
	pthread_cond_destroy(&mCond);
}

/**
 * @brief POSIX 条件变量等待（可带超时）
 *
 * @param ms 超时毫秒数，0 表示无限等待
 * @return bool 是否超时返回（true 表示超时）
 */
bool ThreadCondition::Wait(int ms)
{
	while (pthread_mutex_lock(&mMutex))
	{
	}

	int ret = ETIMEDOUT;
	if (ms > 0)
	{
		timespec timeout;
		RealTime outtime = GetRealTime();
		outtime = outtime + ms;
		timeout.tv_sec = outtime.GetSec();
		timeout.tv_nsec = outtime.GetUsec() * 1000;
		while((ret = pthread_cond_timedwait(&mCond, &mMutex, &timeout)) == EINTR )
		{
		}
	}
	else
	{
		ret = pthread_cond_wait(&mCond, &mMutex);
	}

	while (pthread_mutex_unlock(&mMutex))
	{
	}
	return (ret == ETIMEDOUT);
}

/**
 * @brief POSIX 条件变量唤醒
 */
void ThreadCondition::Set()
{
	while(pthread_mutex_lock(&mMutex))
	{
	}
	while(pthread_cond_signal(&mCond))
	{
	}
	while(pthread_mutex_unlock(&mMutex))
	{
	}
}

/**
 * @brief POSIX 互斥锁构造函数
 */
ThreadMutex::ThreadMutex()
{
	pthread_mutex_init (&mMutex,0);
}

/**
 * @brief POSIX 互斥锁析构函数
 */
ThreadMutex::~ThreadMutex() {
	pthread_mutex_destroy(&mMutex);
}

/**
 * @brief POSIX 互斥锁加锁
 */
void ThreadMutex::Lock()
{
	while (pthread_mutex_lock(&mMutex))
	{
	}
}

/**
 * @brief POSIX 互斥锁解锁
 */
void ThreadMutex::UnLock()
{
	while (pthread_mutex_unlock(&mMutex))
	{
	}
}
#endif

/**
 * @brief 线程静态入口函数
 *
 * @param thread 线程对象指针
 * @return void* 固定返回 0
 */
void *Thread::Spawner(void *thread)
{
	static_cast<Thread*>(thread)->StartRoutine();
	return (void*) 0;
}

/**
 * @brief 启动线程
 *
 * 创建系统线程并执行 StartRoutine。
 */
void Thread::Start()
{
#ifdef WIN32
	DWORD dwThreadId;
	mThread = CreateThread(0, 0, &Spawner, this, 0, &dwThreadId);
#else
	pthread_create(&mThread, 0, &Spawner, this);
#endif
}

/**
 * @brief 等待线程结束
 *
 * 阻塞调用线程，直到目标线程退出。
 */
void Thread::Join()
{
#ifdef WIN32
#else
	pthread_join(mThread, 0);
#endif
}

