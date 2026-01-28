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
 * @file Evaluation.cpp
 * @brief 评估系统实现 - WrightEagleBase 的位置评估核心
 * 
 * 本文件实现了 Evaluation 类，它是 WrightEagleBase 系统的评估核心：
 * 1. 位置评估：评估球场位置的价值
 * 2. 神经网络评估：使用神经网络进行位置评估
 * 3. 敏感性分析：分析位置对比赛的影响
 * 4. 策略支持：为决策系统提供评估支持
 * 
 * 评估系统是 WrightEagleBase 的智能决策支持模块，
 * 为各种决策提供量化的评估结果。
 * 
 * 主要功能：
 * - 位置评估：评估球场位置的战略价值
 * - 神经网络：使用预训练的神经网络进行评估
 * - 归一化处理：对输入数据进行归一化处理
 * - 阵型支持：支持不同阵型的位置评估
 * 
 * 技术特点：
 * - 神经网络：使用神经网络进行智能评估
 * - 归一化：对输入数据进行标准化处理
 * - 单例模式：全局唯一的评估实例
 * - 高效计算：优化的评估算法
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "Evaluation.h"
#include "ServerParam.h"
#include "Agent.h"
#include "PositionInfo.h"
#include "Strategy.h"
#include "Net.h"
#include "WorldState.h"
#include "InfoState.h"

/**
 * @brief Evaluation 构造函数
 * 
 * 初始化评估系统，加载神经网络模型。
 * 该构造函数会创建用于位置评估的神经网络。
 * 
 * @note 构造函数会从文件加载预训练的神经网络
 * @note 神经网络文件路径为 "data/sensitivity.net"
 * @note 如果文件不存在，神经网络创建可能失败
 */
Evaluation::Evaluation()
{
	// === 创建敏感性神经网络 ===
	// 从文件加载预训练的神经网络模型
	mSensitivityNet = new Net("data/sensitivity.net");
}

/**
 * @brief Evaluation 析构函数
 * 
 * 清理评估系统资源，销毁神经网络。
 * 该析构函数会释放神经网络占用的内存。
 * 
 * @note 会安全地删除神经网络实例
 * @note 确保没有内存泄漏
 */
Evaluation::~Evaluation()
{
	// === 销毁神经网络 ===
	delete mSensitivityNet;
}

/**
 * @brief 获取评估系统单例实例
 * 
 * 返回评估系统的全局唯一实例。
 * 使用单例模式确保全局只有一个评估实例。
 * 
 * @return Evaluation& 评估系统的引用
 * 
 * @note 使用静态局部变量实现线程安全的单例模式
 * @note 第一次调用时会创建实例
 */
Evaluation & Evaluation::instance()
{
    // === 创建静态局部变量实现单例模式 ===
    static Evaluation evaluation;
    return evaluation;
}

/**
 * @brief 评估位置价值
 * 
 * 使用神经网络评估指定位置的战略价值。
 * 该函数会将位置坐标归一化后输入神经网络。
 * 
 * 算法流程：
 * 1. 对位置坐标进行归一化处理
 * 2. 根据队伍方向调整坐标
 * 3. 输入神经网络进行评估
 * 4. 返回评估结果
 * 
 * @param pos 要评估的位置坐标
 * @param ourside 是否为己方位置（true为己方，false为对方）
 * @return double 位置评估值（越大表示位置越好）
 * 
 * @note X坐标归一化到[-1,1]范围
 * @note Y坐标归一化到[-1,1]范围
 * @note 对方位置会镜像处理
 * @note 神经网络输出范围通常为[0,1]
 */
double Evaluation::EvaluatePosition(const Vector & pos, bool ourside)
{
	// === 静态变量定义 ===
	// 避免重复分配内存，提高效率
	static double input[2];   // 神经网络输入数组
	static double output[1];  // 神经网络输出数组

	// === 输入数据归一化 ===
	// X坐标：归一化到[-1,1]范围，中心为0
	input[0] = pos.X() / (ServerParam::instance().PITCH_LENGTH * 0.5);
	
	// Y坐标：归一化到[-1,1]范围，中心为0
	// 使用绝对值确保对称性，然后进行线性变换
	input[1] = fabs(pos.Y()) / (ServerParam::instance().PITCH_WIDTH * 0.5) * 2.0 - 1.0;
	
	// === 队伍方向处理 ===
	// 如果是对方位置，镜像X坐标
	if (!ourside){
		// 对方位置需要镜像处理，确保评估的一致性
		input[0] *= -1.0;
	}
	
	// === 神经网络评估 ===
	// 将归一化后的坐标输入神经网络
	mSensitivityNet->Run(input, output);

    // === 返回评估结果 ===
    // 输出值越大表示位置越好
    return output[0];
}

