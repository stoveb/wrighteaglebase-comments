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
 * @file CommunicateSystem.cpp
 * @brief 通信系统实现 - WrightEagleBase 的球员间通信管理核心
 * 
 * 本文件实现了 CommunicateSystem 类，它是 WrightEagleBase 系统的球员间通信管理核心：
 * 1. 管理球员间的信息传递和接收
 * 2. 实现高效的通信编码和解码算法
 * 3. 处理通信信息的优先级和过滤
 * 4. 提供智能的通信策略支持
 * 
 * CommunicateSystem 是 WrightEagleBase 的高级协作系统，
 * 负责在多智能体环境中实现有效的信息共享。
 * 
 * 主要功能：
 * - 信息编码：将各种信息类型编码为紧凑的字符串
 * - 信息解码：解析接收到的通信信息
 * - 通信管理：控制通信的时机和内容
 * - 信息过滤：过滤无用和冗余信息
 * - 协作支持：支持战术协作的信息传递
 * 
 * 技术特点：
 * - 自定义编码：73个字符的自定义编码系统
 * - 位压缩：高效的位级信息压缩
 * - 类型支持：支持多种信息类型的编码
 * - 优先级管理：基于重要性的信息优先级
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include "CommunicateSystem.h"
#include "WorldState.h"
#include "Agent.h"
#include "ServerParam.h"
#include "PlayerParam.h"
#include "Logger.h"
#include "Formation.h"
using namespace std;

/**
 * @brief 通信编码字符表
 * 
 * 包含73个可打印字符，用于自定义的通信编码系统。
 * 这些字符经过精心选择，避免了与服务器命令冲突。
 */
const unsigned char *CommunicateSystem::CODE = (const unsigned char *)"uMKJNPpA1Yh0)f6_x3WU<>SgQ4wbDizV5dc9t2XZ?(/7*s.FEHvLG8yRTkej-OlB+armnoqCI";

/**
 * @brief 字符到整数的映射表
 * 
 * 将CODE中的字符映射为0-72的整数，
 * 用于快速编码和解码操作。
 */
int CommunicateSystem::CODE_TO_INT[128];

/**
 * @brief 编码字符表大小
 * 
 * CODE表中字符的总数，固定为73。
 */
const int CommunicateSystem::CODE_SIZE = 73;

/**
 * @brief 最大消息大小
 * 
 * 单次通信消息的最大字符数限制。
 * RoboCup服务器限制每次say消息最多10个字符。
 */
const int CommunicateSystem::MAX_MSG_SIZE = 10;

/**
 * @brief 最大可用位数
 * 
 * 通信中可以使用的最大位数，初始为61位。
 * 这个值会根据编码需求动态调整。
 */
int CommunicateSystem::MAX_BITS_USED = 61;

/**
 * @brief CommunicateSystem 构造函数
 * 
 * 初始化通信系统，设置编码参数和映射表。
 * 通信系统采用自定义的编码方案，实现高效的信息压缩。
 * 
 * 初始化内容：
 * 1. 建立字符到整数的映射表
 * 2. 初始化各种编码参数
 * 3. 计算各种信息类型的位需求
 * 4. 设置编码掩码和位计数
 * 
 * @note 采用73进制编码系统
 * @note 支持多种信息类型的压缩编码
 */
CommunicateSystem::CommunicateSystem() {
	// === 初始化字符映射表 ===
	// 清空映射表并建立CODE字符到整数的映射
	memset(CODE_TO_INT, 0, sizeof(CODE_TO_INT));
	for (int i = 0; i < CODE_SIZE; ++i){
		CODE_TO_INT[CODE[i]] = i;  // 字符CODE[i]映射为整数i
	}

	// === 初始化编码参数 ===
	memset(mCodecBitCount, 0, sizeof(mCodecBitCount));
	memset(mCodecMask, 0, sizeof(mCodecMask));

	// === 计算通信标志位需求 ===
	// 通信类型需要的位数：ceil(log(COMMU_MAX)/log(2))
	mCommuFlagBitCount = static_cast<int>(ceil(log(static_cast<double>(COMMU_MAX)) / log(2.0)));
	mCommuFlagMask = (1 << mCommuFlagBitCount) - 1;

	// === 计算自由形式标志位需求 ===
	// 自由形式类型需要的位数：ceil(log(FREE_FORM_MAX)/log(2))
	mFreeFormFlagBitCount = static_cast<int>(ceil(log(static_cast<double>(FREE_FORM_MAX)) / log(2.0)));
	mFreeFormFlagMask = (1 << mFreeFormFlagBitCount) - 1;

	// === 计算球员号码位需求 ===
	// 球员号码需要的位数：ceil(log(TEAMSIZE)/log(2))
	mUnumBitCount = static_cast<int>(ceil(log(static_cast<double>(TEAMSIZE)) / log(2.0)));
	mUnumMask = (1 << mUnumBitCount) - 1;

	// === 战术标志位需求 ===
	// 战术信息固定使用5位
	mTacticsBitCount = 5;
	mTacticsFlagMask = (1 << mTacticsBitCount) - 1;

	// === 计算各种数据类型的位需求 ===
	// 基于参数中的精度要求计算需要的位数
	mCodecBitCount[FREE_FORM][POS_X] = BitCountOfEps(PlayerParam::instance().sayPosXEps(), POS_X);
	mCodecMask[FREE_FORM][POS_X] = (1 << mCodecBitCount[FREE_FORM][POS_X]) - 1;
	mCodecBitCount[FREE_FORM][POS_Y] = BitCountOfEps(PlayerParam::instance().sayPosYEps(), POS_Y);
	mCodecMask[FREE_FORM][POS_Y] = (1 << mCodecBitCount[FREE_FORM][POS_Y]) - 1;
	mCodecBitCount[FREE_FORM][BALL_SPEED] = BitCountOfEps(PlayerParam::instance().sayBallSpeedEps(), BALL_SPEED);
	mCodecMask[FREE_FORM][BALL_SPEED] = (1 << mCodecBitCount[FREE_FORM][BALL_SPEED]) - 1;
	mCodecBitCount[FREE_FORM][PLAYER_SPEED] = BitCountOfEps(PlayerParam::instance().sayPlayerSpeedEps(), BALL_SPEED);
	mCodecMask[FREE_FORM][PLAYER_SPEED] = (1 << mCodecBitCount[FREE_FORM][PLAYER_SPEED]) - 1;
	mCodecBitCount[FREE_FORM][DIR] = BitCountOfEps(PlayerParam::instance().sayDirEps(), DIR);
	mCodecMask[FREE_FORM][DIR] = (1 << mCodecBitCount[FREE_FORM][DIR]) - 1;

	// === 计算各种消息类型的总位数需求 ===
	// 球信息（含速度）：位置X + 位置Y + 球速 + 方向 + 标志位
	mFreeFormCodecBitCount[BALL_WITH_SPEED] = mCodecBitCount[FREE_FORM][POS_X] + mCodecBitCount[FREE_FORM][POS_Y] + mCodecBitCount[FREE_FORM][BALL_SPEED] + mCodecBitCount[FREE_FORM][DIR] + mFreeFormFlagBitCount;
	// 球信息（零速度）：位置X + 位置Y + 标志位
	mFreeFormCodecBitCount[BALL_WITH_ZERO_SPEED] = mCodecBitCount[FREE_FORM][POS_X] + mCodecBitCount[FREE_FORM][POS_Y] + mFreeFormFlagBitCount;
	// 球位置信息：位置X + 位置Y + 标志位
	mFreeFormCodecBitCount[BALL_ONLY_POS] = mCodecBitCount[FREE_FORM][POS_X] + mCodecBitCount[FREE_FORM][POS_Y] + mFreeFormFlagBitCount;
	// 队友位置信息：位置X + 位置Y + 标志位
	mFreeFormCodecBitCount[TEAMMATE_ONLY_POS] = mCodecBitCount[FREE_FORM][POS_X] + mCodecBitCount[FREE_FORM][POS_Y] + mFreeFormFlagBitCount;
	// 对手位置信息：位置X + 位置Y + 标志位
	mFreeFormCodecBitCount[OPPONENT_ONLY_POS] = mCodecBitCount[FREE_FORM][POS_X] + mCodecBitCount[FREE_FORM][POS_Y] + mFreeFormFlagBitCount;

	// === 调整最大可用位数 ===
	// 减去必须使用的通信标志位
	MAX_BITS_USED -= mCommuFlagBitCount; //这个是一定要用的位
}

/**
 * @brief CommunicateSystem 析构函数
 * 
 * 清理通信系统资源。
 * 当前为空实现，因为没有需要手动释放的资源。
 */
CommunicateSystem::~CommunicateSystem() {
	// 析构函数体为空
}

/**
 * @brief 获取 CommunicateSystem 单例实例
 * 
 * 返回 CommunicateSystem 的单例实例，确保整个系统只有一个通信系统。
 * 
 * @return CommunicateSystem& 通信系统的引用
 * 
 * @note 采用单例模式确保通信系统的全局一致性
 */
CommunicateSystem & CommunicateSystem::instance()
{
    static CommunicateSystem communicate_system;
    return communicate_system;
}

void CommunicateSystem::Initial(Observer *observer , Agent* agent)
{
    mpObserver = observer;
	mpAgent  = agent;
}

int CommunicateSystem::BitCountOfEps(double eps, CodecType type)
{
	double min = 0.0, max = 1.0;
	GetCodecRange(type, min, max);

	double range = (max - min) / eps;
	return static_cast<int>(ceil(log(range) / log(2.0)));
}


void CommunicateSystem::GetCodecRange(CodecType type, double &min, double &max)
{
	switch (type){
	case POS_X: min = -ServerParam::instance().PITCH_LENGTH * 0.5; max = ServerParam::instance().PITCH_LENGTH * 0.5; break;
	case POS_Y: min = -ServerParam::instance().PITCH_WIDTH * 0.5; max = ServerParam::instance().PITCH_WIDTH * 0.5; break;
	case BALL_SPEED: min = 0.0; max = ServerParam::instance().ballSpeedMax(); break;
	case PLAYER_SPEED: min = 0.0; max = ServerParam::instance().playerSpeedMax(); break;
	case DIR: min = -180.0; max = 180.0; break;
	default: PRINT_ERROR("unknown codec type"); break;
	}
}

CommunicateSystem::DWORD64 CommunicateSystem::DoubleToBit(double value, CommunicateSystem::CodecType type)
{
	int range = static_cast<int>((*mpCodecMask)[type]); // [0, range]

	double min = 0.0, max = 1.0;
	GetCodecRange(type, min, max);

	if(value > max)
	{
		value = max - 0.001 * max;
	}
	else if (value < min)
	{
		value = min + 0.001 * min;
	}

	return static_cast<DWORD64>(((value - min) / (max - min) * static_cast<double>(range)));
}

double CommunicateSystem::BitToDouble(DWORD64 bits, CommunicateSystem::CodecType type)
{
	int range = static_cast<int>((*mpCodecMask)[type]); // [0, range]
	DWORD64 mask = (*mpCodecMask)[type];

	double min = 0.0, max = 1.0;
	GetCodecRange(type, min, max);

	return static_cast<double>(bits & mask) / static_cast<double>(range) * (max - min) + min;
}

void CommunicateSystem::Encode(DWORD64 bits, unsigned char *msg , bool is_coach)
{
	int i = 0, rem;

	while (bits != 0 || (is_coach && i < MAX_MSG_SIZE) ){
		rem = bits % CODE_SIZE;
		bits = (bits - rem) / CODE_SIZE;
		if (i >= MAX_MSG_SIZE){
			PRINT_ERROR("codec msg size greater than " << MAX_MSG_SIZE);
			break;
		}
		msg[i++] = CODE[rem];
	}
	msg[i] = '\0';
}

void CommunicateSystem::Decode(const unsigned char *msg, DWORD64& bits , bool is_coach)
{
	int n = is_coach ? MAX_MSG_SIZE : strlen((const char *)msg);
	bits = 0;
	DWORD64 plus = 1;
	for (int i = 0; i < n; ++i){
		bits += CODE_TO_INT[msg[i]] * plus;
		plus *= CODE_SIZE;
	}
}

void CommunicateSystem::PrintBits(DWORD64 bits)
{
	UDWORD64 mask = 1;
	mask <<= 63;

	while (mask != 0){
		if (bits & mask) cerr << '1' ;
		else cerr << '0';
		mask >>= 1;
	}
	cerr << endl;
}

void CommunicateSystem::Update()
{
	//some reset work
	mCommuBits = 0;
	mBitsUsed = 0;
	mBallSended = false;

	mTeammateSended.bzero();
	mOpponentSended.bzero();

	mCommuType = FREE_FORM;
	SetCommunicateType(FREE_FORM);

	// our coach say
    if (mpObserver->Audio().IsOurCoachSayValid())
    {
    	;
	}

    // teammate say
    if (mpObserver->Audio().IsTeammateSayValid())
    {
        ParseReceivedTeammateMsg((unsigned char *)(mpObserver->Audio().GetTeammateSayContent().c_str()));
    }
}

bool CommunicateSystem::AddDataToCommuBits(double value, CodecType type)
{
	if (mBitsUsed + (*mpCodecBitCount)[type] > MAX_BITS_USED){
		PRINT_ERROR("bits used greater then " << MAX_BITS_USED);
		return false;
	}
	else {
		mCommuBits <<= (*mpCodecBitCount)[type];
		mCommuBits += DoubleToBit(value, type);
		mBitsUsed += (*mpCodecBitCount)[type];

		return true;
	}
}

bool CommunicateSystem::AddFreeFormFlagToCommuBits(FreeFormType type)
{
	if (mBitsUsed + mFreeFormFlagBitCount > MAX_BITS_USED){
		PRINT_ERROR("bits used greater then " << MAX_BITS_USED);
		return false;
	}
	else {
		mCommuBits <<= mFreeFormFlagBitCount;
		mCommuBits += static_cast<DWORD64>(type);
		mBitsUsed += mFreeFormFlagBitCount;

		return true;
	}
}

bool CommunicateSystem::AddCommuFlagToCommuBits(CommuType type)
{
	mCommuBits <<= mCommuFlagBitCount;
	mCommuBits += static_cast<DWORD64>(type);
	mBitsUsed += mCommuFlagBitCount;

	return true;
}

bool CommunicateSystem::AddUnumToCommuBits(Unum num)
{
	if (mBitsUsed + mUnumBitCount > MAX_BITS_USED){
		PRINT_ERROR("bits used greater then " << MAX_BITS_USED);
		return false;
	}
	else {
		mCommuBits <<= mUnumBitCount;
		mCommuBits += static_cast<DWORD64>(num);
		mBitsUsed += mUnumBitCount;

		return true;
	}
}

bool CommunicateSystem::SendBallStatus(const BallState &ball_state, int cd)
{
	if (mCommuType != FREE_FORM){
		return false;
	}

	SetCommunicateType(FREE_FORM);

	if (mBallSended == true){
		return false;
	}

	int pos_cd = ball_state.GetPosDelay();
	int vel_cd = ball_state.GetVelDelay();
	Vector pos = ball_state.GetPos();
	Vector vel = ball_state.GetVel();
	FreeFormType send_type = FREE_FORM_MAX;

	if (pos_cd <= cd){
		if (mFreeFormCodecBitCount[BALL_ONLY_POS] + mBitsUsed <= MAX_BITS_USED){
			if (vel_cd <= cd){
				if (vel.Mod2() > FLOAT_EPS){ //BALL_WITH_SPEED
					if (mFreeFormCodecBitCount[BALL_WITH_SPEED] + mBitsUsed <= MAX_BITS_USED){
						send_type = BALL_WITH_SPEED;
					}
					else { //BALL_ONLY_POS
						send_type = BALL_ONLY_POS;
					}
				}
				else { //BALL_WITH_ZERO_SPEED
					if (mFreeFormCodecBitCount[BALL_WITH_ZERO_SPEED] + mBitsUsed <= MAX_BITS_USED){
						send_type = BALL_WITH_ZERO_SPEED;
					}
					else { //BALL_ONLY_POS
						send_type = BALL_ONLY_POS;
					}
				}
			}
			else { //BALL_ONLY_POS
				send_type = BALL_ONLY_POS;
			}
		}
	}
	if (send_type != FREE_FORM_MAX){
		switch(send_type){
		case BALL_WITH_SPEED:
			AddDataToCommuBits(pos.X(), POS_X);
			AddDataToCommuBits(pos.Y(), POS_Y);
			AddDataToCommuBits(vel.Mod(), BALL_SPEED);
			AddDataToCommuBits(vel.Dir(), DIR);
			AddFreeFormFlagToCommuBits(BALL_WITH_SPEED);
			mBallSended = true;
			Logger::instance().GetTextLogger("freeform") << mpAgent->GetWorldState().CurrentTime() << " send ball: " << pos << " " << vel << endl;
			return true;
		case BALL_WITH_ZERO_SPEED:
			AddDataToCommuBits(pos.X(), POS_X);
			AddDataToCommuBits(pos.Y(), POS_Y);
			AddFreeFormFlagToCommuBits(BALL_WITH_ZERO_SPEED);
			mBallSended = true;
			Logger::instance().GetTextLogger("freeform") << mpAgent->GetWorldState().CurrentTime() << " send ball: " << pos << " " << Vector(0, 0) << endl;
			return true;
		case BALL_ONLY_POS:
			AddDataToCommuBits(pos.X(), POS_X);
			AddDataToCommuBits(pos.Y(), POS_Y);
			AddFreeFormFlagToCommuBits(BALL_ONLY_POS);
			Logger::instance().GetTextLogger("freeform") << mpAgent->GetWorldState().CurrentTime() << " send ball: " << pos << endl;
			mBallSended = true;
			return true;
		default: PRINT_ERROR("send ball status"); break;
		}
	}
	return false;
}

bool CommunicateSystem::SendTeammateStatus(const WorldState *pWorldState, Unum num, int cd)
{
	Assert(num > 0);

	if (mCommuType != FREE_FORM){
		return false;
	}

	SetCommunicateType(FREE_FORM);

	if (mTeammateSended[num] == true){
		return false;
	}
	if (pWorldState->GetTeammate(num).IsAlive() == false){
		return false;
	}
	if (mpAgent->GetInfoState().GetPositionInfo().GetPlayerDistToPlayer(mpAgent->GetSelfUnum(), num) > ServerParam::instance().unumFarLength()) {
		return false;
	}

	int pos_cd = pWorldState->GetTeammate(num).GetPosDelay();
	if (pos_cd <= cd){
		if (mFreeFormCodecBitCount[TEAMMATE_ONLY_POS] + mBitsUsed <= MAX_BITS_USED){
			Vector pos = pWorldState->GetTeammate(num).GetPos();
			AddUnumToCommuBits(num);
			AddDataToCommuBits(pos.X(), POS_X);
			AddDataToCommuBits(pos.Y(), POS_Y);
			AddFreeFormFlagToCommuBits(TEAMMATE_ONLY_POS);
			Logger::instance().GetTextLogger("freeform") << mpAgent->GetWorldState().CurrentTime() << " send tm: " << num << " " << pos << endl;
			mTeammateSended[num] = true;
			return true;
		}
	}
	return false;
}

bool CommunicateSystem::SendOpponentStatus(const WorldState *pWorldState, Unum num, int cd)
{
	// 检查球员号码有效性
	Assert(num > 0);

	// 检查通信类型是否为自由格式
	if (mCommuType != FREE_FORM){
		return false;
	}

	// 设置通信类型为自由格式
	SetCommunicateType(FREE_FORM);

	// 检查该对手信息是否已发送过
	if (mOpponentSended[num] == true){
		return false; // 已发送过，不再重复发送
	}
	// 检查对手是否存活
	if (pWorldState->GetOpponent(num).IsAlive() == false){
		return false; // 对手已死亡，无需发送
	}
	// 检查距离是否在通信范围内
	if (mpAgent->GetInfoState().GetPositionInfo().GetPlayerDistToPlayer(mpAgent->GetSelfUnum(), -num) > ServerParam::instance().unumFarLength()) {
		return false; // 距离太远，无法通信
	}

	// 获取对手位置信息的时间延迟
	int pos_cd = pWorldState->GetOpponent(num).GetPosDelay();
	// 检查信息是否足够新鲜
	if (pos_cd <= cd){
		// 检查是否有足够的位空间
		if (mFreeFormCodecBitCount[OPPONENT_ONLY_POS] + mBitsUsed <= MAX_BITS_USED){
			// 获取对手位置
			Vector pos = pWorldState->GetOpponent(num).GetPos();
			// 编码对手信息
			AddUnumToCommuBits(num); // 添加球员号码
			AddDataToCommuBits(pos.X(), POS_X); // 添加X坐标
			AddDataToCommuBits(pos.Y(), POS_Y); // 添加Y坐标
			AddFreeFormFlagToCommuBits(OPPONENT_ONLY_POS); // 添加标志位
			// 标记已发送
			mOpponentSended[num] = true;
			// 记录日志
			Logger::instance().GetTextLogger("freeform") << mpAgent->GetWorldState().CurrentTime() << " send opp: " << num << " " << pos << endl;
			return true;
		}
	}
	return false;
}

void CommunicateSystem::Decision()
{
	// 执行通信逻辑
	DoCommunication();

	// 检查是否有通信数据需要发送
	if (mBitsUsed != 0){
		// 创建消息缓冲区（最大10个字符）
		unsigned char msg[11]; //MAX_MSG_SIZE == 10

		// 添加通信类型标志
		AddCommuFlagToCommuBits(mCommuType);

		// 进行位操作，准备编码
		// 左边补1，用于解码时的结束标志
		DWORD64 mask = 1;
		mask <<= mBitsUsed;
		mask -= 1;
		mCommuBits += ~mask; //左边补1
		// 去掉高三位，保留有效数据
		mask = 1;
		mask <<= (MAX_BITS_USED + mCommuFlagBitCount);
		mask -= 1;
		mCommuBits &= mask; //去掉高三位

		// 编码为字符串消息
		Encode(mCommuBits, msg);

		// 发送消息
		if (strlen((const char *)msg) > 0){
			mpAgent->Say(string((const char *)msg));
		}
		else {
			// 消息为空，可能是编码错误
			PRINT_ERROR("bug here?");
		}
	}
}

double CommunicateSystem::ExtractDataFromBits(DWORD64 &bits, CodecType type, int &bit_left)
{
	double res;

	// 从位流中提取数据并转换为实际值
	res = BitToDouble(bits, type);
	// 移除已使用的位
	bits >>= (*mpCodecBitCount)[type];
	bit_left -= (*mpCodecBitCount)[type];

	return res;
}

int CommunicateSystem::ExtractUnumFromBits(DWORD64 &bits, int &bit_left)
{
	int num;

	// 从位流中提取球员号码
	num = static_cast<int>(bits & mUnumMask);
	// 移除已使用的位
	bits >>= mUnumBitCount;
	bit_left -= mUnumBitCount;

	return num;
}

void CommunicateSystem::RecvFreeForm(DWORD64 bits)
{
	// 设置通信类型为自由格式
	SetCommunicateType(FREE_FORM);

	FreeFormType type;
	int bit_left = MAX_BITS_USED;
	// 循环解析自由格式消息中的所有数据块
	while (bit_left > 0){
		 // 提取数据块类型
		 type = static_cast<FreeFormType>(bits & mFreeFormFlagMask);
		 // 检查是否到达消息结束标志
		 if (type >= FREE_FORM_DUMMY){
			 break; //mCommuBits 的左端全是1，可以用来作为结束标志，因为FREE_FORM_DUMMY是不用的
		 }
		 // 检查剩余位数是否足够
		 if (bit_left < mFreeFormCodecBitCount[type]){
			 break;
		 }

		 // 移除标志位
		 bits >>= mFreeFormFlagBitCount;
		 bit_left -= mFreeFormFlagBitCount;

		 // 根据数据块类型进行相应的处理
		 if (type == BALL_WITH_SPEED){
			 // 带速度的球信息
			 AngleDeg dir = ExtractDataFromBits(bits, DIR, bit_left);
			 double speed = ExtractDataFromBits(bits, BALL_SPEED, bit_left);
			 double y = ExtractDataFromBits(bits, POS_Y, bit_left);
			 double x = ExtractDataFromBits(bits, POS_X, bit_left);
			 // 更新观察者的球信息
			 mpObserver->HearBall(Vector(x, y), Polar2Vector(speed, dir));
			 Logger::instance().GetTextLogger("freeform") << mpObserver->CurrentTime() << " hear ball: " << Vector(x, y) << " " << Polar2Vector(speed, dir) << endl;
		 }
		 else if (type == BALL_WITH_ZERO_SPEED){
			 // 零速度的球信息
			 double y = ExtractDataFromBits(bits, POS_Y, bit_left);
			 double x = ExtractDataFromBits(bits, POS_X, bit_left);
			 mpObserver->HearBall(Vector(x, y), Vector(0.0, 0.0));
			 Logger::instance().GetTextLogger("freeform") << mpObserver->CurrentTime() << " hear ball: " << Vector(x, y) << " " << Polar2Vector(0, 0) << endl;
		 }
		 else if (type == BALL_ONLY_POS){
			 // 只有位置的球信息
			 double y = ExtractDataFromBits(bits, POS_Y, bit_left);
			 double x = ExtractDataFromBits(bits, POS_X, bit_left);
			 mpObserver->HearBall(Vector(x, y));
			 Logger::instance().GetTextLogger("freeform") << mpObserver->CurrentTime() << " hear ball: " << Vector(x, y) << endl;
		 }
		 else if (type == TEAMMATE_ONLY_POS){
			 // 只有位置的队友信息
			 double y = ExtractDataFromBits(bits, POS_Y, bit_left);
			 double x = ExtractDataFromBits(bits, POS_X, bit_left);
			 Unum num = ExtractUnumFromBits(bits, bit_left);
			 mpObserver->HearTeammate(num, Vector(x, y));
			 Logger::instance().GetTextLogger("freeform") << mpObserver->CurrentTime() << " hear tm: " << num << " " << Vector(x, y) << endl;
		 }
		 else if (type == OPPONENT_ONLY_POS){
			 // 只有位置的对手信息
			 double y = ExtractDataFromBits(bits, POS_Y, bit_left);
			 double x = ExtractDataFromBits(bits, POS_X, bit_left);
			 Unum num = ExtractUnumFromBits(bits, bit_left);
			 mpObserver->HearOpponent(num, Vector(x, y));
			 Logger::instance().GetTextLogger("freeform") << mpObserver->CurrentTime() << " hear opp: " << num << " " << Vector(x, y) << endl;
		 }
		 else {
			 // 未知的数据块类型，报错
			 PRINT_ERROR("recv free form error");
			 return;
		 }
	}
}

void CommunicateSystem::ParseReceivedTeammateMsg(unsigned char *msg)
{
	DWORD64 bits;
	//cerr << mpObserver->CurrentTime() << " (h) " << msg << endl;

	// 解码消息为位流
	Decode(msg, bits);

	// 进行位操作，准备解析
	// 高三位补1，用于解码时的结束标志
	DWORD64 mask = 1;
	mask <<= (MAX_BITS_USED + mCommuFlagBitCount);
	mask -= 1;
	bits += ~mask; //高三位补1

	// 提取通信类型
	CommuType type = static_cast<CommuType>(bits & mCommuFlagMask);
	bits >>= mCommuFlagBitCount;

	// 根据通信类型进行相应的处理
	if (type == FREE_FORM){
		// 自由格式消息
		Logger::instance().GetTextLogger("receive") << "freeform" << endl;
		RecvFreeForm(bits);
	}
	else {
		// 未知通信类型，记录错误
		Logger::instance().GetTextLogger("receive") << "???" << endl;
		PRINT_ERROR(msg);
		//TODO: 其他信息
	}
}


void CommunicateSystem::DoCommunication()
{
	// 检查智能体指针是否有效
	if (!mpAgent) return;

	// 获取位置信息
	PositionInfo & position_info = mpAgent->GetInfoState().GetPositionInfo();
	// 获取离球最近的队友
	const Unum closest_tm = position_info.GetClosestTeammateToBall();

	// 根据自己是否为最近队友调整注意力
	if (closest_tm == mpAgent->GetSelfUnum()) {
		// 自己是最近的队友，关闭注意力
		mpAgent->AttentiontoOff();
	}
	else if (closest_tm) {
		// 有其他队友更近，关注该队友
		if (closest_tm != mpAgent->GetSelf().GetFocusOnUnum()) {
			mpAgent->Attentionto(closest_tm);
		}
	}

	// 发送球状态信息
	SendBallStatus(mpAgent->GetWorldState().GetBall());
	// 发送自己的状态信息
	SendTeammateStatus(& mpAgent->GetWorldState(), mpAgent->GetSelfUnum());

	// 获取对手和队友信息
	const vector<Unum> & o2b = mpAgent->GetInfoState().GetPositionInfo().GetCloseOpponentToTeammate(mpAgent->GetSelfUnum());
	const vector<Unum> & t2b = mpAgent->GetInfoState().GetPositionInfo().GetCloseTeammateToTeammate(mpAgent->GetSelfUnum());

	// 循环发送对手和队友状态信息
	// 限制最多发送TEAMSIZE个球员的信息
	for (uint i = 0; i < TEAMSIZE; ++i) {
		// 检查是否有对手信息需要发送
		if (i < o2b.size()) {
			// 发送对手状态信息
			SendOpponentStatus(& mpAgent->GetWorldState(), o2b[i]);
		}
		// 检查是否有队友信息需要发送
		if (i < t2b.size()) {
			// 发送队友状态信息
			SendTeammateStatus(& mpAgent->GetWorldState(), t2b[i]);
		}
	}
}
