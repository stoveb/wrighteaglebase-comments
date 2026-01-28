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

#include <cstring>
#include <math.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "Net.h"
#include "Types.h"
#include "Utilities.h"

// === 编译模式定义 ===
#define BINARY

// === 扫描参数宏定义 ===
// 用于从文件中扫描参数的便捷宏
#define SCAN_PARAMS(p, i, d) if (fscanf(fp, "%d %d %d", &p, &i, &d) < 1) { Assert(0); }

// === 扫描训练数据宏定义 ===
// 用于从文件中扫描训练数据的便捷宏
#define SCAN_TRAIN(patterns, input_size, desire_size, input, desire) \
	for (int i = 0; i < patterns; ++i){ \
		for (int j = 0; j < input_size; ++j){ \
	        if (fscanf(fp, "%lf", &input[i][j]) < 1) { Assert(0); } \
		} \
		for (int j = 0; j < desire_size; ++j){ \
	        if (fscanf(fp, "%lf", &desire[i][j]) < 1) { Assert(0); } \
		} \
	}

/**
 * @brief Net 构造函数（基于层数和单元数）
 * 
 * 根据指定的层数和每层的单元数构造神经网络。
 * 这是创建新网络的主要构造函数。
 * 
 * @param layers 网络层数
 * @param units 每层的单元数数组
 * 
 * @note 层数不包括输入层，只包括隐藏层和输出层
 * @note units数组长度应该等于layers
 */
Net::Net(int layers, int *units)
{
	mUnits = 0;      // 初始化单元数数组指针
	mLogName = 0;    // 初始化日志名称指针
	Construct(layers, units);  // 调用构造函数
}

/**
 * @brief Net 构造函数（基于文件）
 * 
 * 从文件中加载神经网络结构。
 * 这个构造函数用于从保存的网络文件中恢复网络。
 * 
 * @param fname 网络文件名
 * 
 * @note 文件应该包含网络的结构信息和权重
 * @note 支持二进制和文本格式的网络文件
 */
Net::Net(const char *fname)
{
	mUnits = 0;      // 初始化单元数数组指针
	mLogName = 0;    // 初始化日志名称指针
	Construct(fname);  // 调用基于文件的构造函数
}

/**
 * @brief Net 析构函数
 * 
 * 清理神经网络资源，释放所有分配的内存。
 */
Net::~Net()
{
	Destroy();  // 调用销毁函数
}

/**
 * @brief 构造神经网络（基于层数和单元数）
 * 
 * 这是网络构造的核心函数，负责分配内存和初始化参数。
 * 
 * @param layers 网络层数
 * @param units 每层的单元数数组
 * 
 * @note 如果layers为0，则不进行构造
 * @note units数组可以为0，此时需要后续设置
 */
void Net::Construct(int layers, int *units)
{
	// === 检查层数 ===
	if (layers == 0)
		return;
		
	// === 设置基本参数 ===
	mLayers = layers;  // 设置层数
	mUnits = new int[mLayers];  // 分配单元数数组
	
	// === 复制单元数信息 ===
	if (units != 0){
		for (int i = 0; i < mLayers; ++i){
			mUnits[i] = units[i];  // 复制每层的单元数
		}
	}

	// === 分配内存和设置默认值 ===
	Memaloc();           // 分配内存
	SetDefaultValue();   // 设置默认值
}

/**
 * @brief 分配神经网络内存
 * 
 * 为神经网络的权重和增量权重分配内存。
 * 每个神经元都有与前一层所有神经元连接的权重，
 * 还包括一个偏置权重。
 */
void Net::Memaloc(){
	// === 分配权重数组 ===
	mWeight = new real**[mLayers];       // 权重数组
	mDeltaWeight = new real**[mLayers];  // 增量权重数组
	
	// === 为每层分配权重 ===
	for (int i = 1; i < mLayers; ++i){  // 从第二层开始（第一层是输入层）
		mWeight[i] = new real*[mUnits[i]];       // 第i层的权重数组
		mDeltaWeight[i] = new real*[mUnits[i]];  // 第i层的增量权重数组
		
		// === 为每个神经元分配权重 ===
		for (int j = 0; j < mUnits[i]; ++j){
			// 每个神经元与前一层所有神经元连接 + 1个偏置
			mWeight[i][j] = new real[mUnits[i-1] + 1];        // 权重（+1为偏置）
			mDeltaWeight[i][j] = new real[mUnits[i-1] + 1];   // 增量权重（+1为偏置）
		}
	}

	mOutput = new real*[mLayers];
	mDelta = new real*[mLayers];
	for (int i = 1; i < mLayers; ++i){
		mOutput[i] = new real[mUnits[i]];
		mDelta[i] = new real[mUnits[i]];
	}
}

void Net::InitWeight()
{
	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			for (int k = 0; k < mUnits[i-1]+1; ++k){
				mWeight[i][j][k] = small_rand();
				mDeltaWeight[i][j][k] = 0.0;
			}
		}
	}
}

void Net::Destroy()
{
	if (mUnits == 0)
		return;
	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			delete[] mWeight[i][j];
			delete[] mDeltaWeight[i][j];
		}
		delete[] mWeight[i];
		delete[] mDeltaWeight[i];
	}
	delete[] mWeight;
	delete[] mDeltaWeight;

	for (int i = 1; i < mLayers; ++i){
		delete[] mOutput[i];
		delete[] mDelta[i];
	}
	delete[] mOutput;
	delete[] mDelta;

	delete[] mUnits;
	if (mLogName != 0)
		delete[] mLogName;
}

void Net::Save(const char *fname)
{
	if (mUnits == 0)
		return;
	FILE *fp;
	if ((fp = fopen(fname, "w")) == 0){
		perror("BPN::Save(char *fname)");
		exit(1);
	}
#ifdef BINARY
	fwrite(&mLayers, sizeof(mLayers), 1, fp);
	fwrite(mUnits, sizeof(*mUnits), mLayers, fp);
	fwrite(&mEta, sizeof(mEta), 1, fp);
	fwrite(&mAlpha, sizeof(mAlpha), 1, fp);
	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			fwrite(mWeight[i][j], sizeof(*mWeight[i][j]), mUnits[i-1]+1, fp);
		}
	}
#else
	fprintf(fp, "#mLayers\n%d\n#mUnits\n", mLayers);
	for (int i = 0; i < mLayers; ++i){
		fprintf(fp, "%d ", mUnits[i]);
	}
	fprintf(fp, "\n#mEta\n%lf\n#mAlpha\n%lf\n#mWeight\n", mEta, mAlpha);
	for (int i = 1; i < mLayers; ++i){
		fprintf(fp, "#Layer %d\n", i);
		for (int j = 0; j < mUnits[i]; ++j){
			for (int k = 0; k < mUnits[i-1]+1; ++k){
				fprintf(fp, "%lf ", mWeight[i][j][k]);
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "\n");
	}
#endif
	fclose(fp);
}

void Net::Construct(const char *fname)
{
	FILE *fp;
	if ((fp = fopen(fname, "r")) == 0){
		perror("BPN::Construct(char *fname)");
		exit(1);
	}
#ifdef BINARY
	if (fread(&mLayers, sizeof(mLayers), 1, fp) < 1)
    {
        Assert(0);
    }
	mUnits = new int[mLayers];
	if (fread(mUnits, sizeof(*mUnits), mLayers, fp) < 1)
    {
        Assert(0);
    }
    if (fread(&mEta, sizeof(mEta), 1, fp) < 1)
    {
        Assert(0);
    }
    if (fread(&mAlpha, sizeof(mAlpha), 1, fp) < 1)
    {
        Assert(0);
    }
	Memaloc();
	InitWeight();

	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			if (fread(mWeight[i][j], sizeof(*mWeight[i][j]), mUnits[i-1]+1, fp) < 1)
            {
                Assert(0);
            }
		}
	}
#else
	fscanf(fp, "#mLayers\n%d\n#mUnits\n", &mLayers);
	mUnits = new int[mLayers];
	for (int i = 0; i < mLayers; ++i){
		fscanf(fp, "%d ", &mUnits[i]);
	}
	fscanf(fp, "\n#mEta\n%lf\n#mAlpha\n%lf\n#mWeight\n", &mEta, &mAlpha);
	int tmp;
	Memaloc();
	InitWeight();

	for (int i = 1; i < mLayers; ++i){
		fscanf(fp, "#Layer %d\n", &tmp);
		for (int j = 0; j < mUnits[i]; ++j){
			for (int k = 0; k < mUnits[i-1]+1; ++k){
				fscanf(fp, "%lf ", &mWeight[i][j][k]);
			}
			fscanf(fp, "\n");
		}
		fscanf(fp, "\n");
	}
#endif
	fclose(fp);
}

inline real Net::sigmoid(real s)
{
	return 1.0/(1.0 + exp(-s));
}

inline real Net::small_rand() //[-1.0, 1.0]
{
	return drand(-1.0, 1.0);
}

inline void Net::FeedForward()
{
	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			mOutput[i][j] = mWeight[i][j][mUnits[i-1]];              //bias
			for (int k = 0; k < mUnits[i-1]; ++k){
				mOutput[i][j] += mOutput[i-1][k] * mWeight[i][j][k];
			}
			mOutput[i][j] = sigmoid(mOutput[i][j]);
		}
	}
}

inline void Net::BackProp()
{
	for (int i = 0; i < mUnits[mLayers-1]; ++i){
		mDelta[mLayers-1][i] = (mDesire[i] - mOutput[mLayers-1][i]) * mOutput[mLayers-1][i] * (1.0 - mOutput[mLayers-1][i]);
	}

	for (int i = mLayers-2; i >= 1; --i){
		for (int j = 0; j < mUnits[i]; ++j){
			mDelta[i][j] = 0.0;
			for (int k = 0; k < mUnits[i+1]; ++k){
				mDelta[i][j] += mDelta[i+1][k] * mWeight[i+1][k][j];
			}
			mDelta[i][j] *= mOutput[i][j] * (1.0 - mOutput[i][j]);
		}
	}
}

inline void Net::UpdateWeight()
{
	for (int i = 1; i < mLayers; ++i){
		for (int j = 0; j < mUnits[i]; ++j){
			for (int k = 0; k < mUnits[i-1]; ++k){
				mDeltaWeight[i][j][k] = mEta * mDelta[i][j] * mOutput[i-1][k] + mAlpha * mDeltaWeight[i][j][k];
				mWeight[i][j][k] += mDeltaWeight[i][j][k];
			}
			mDeltaWeight[i][j][mUnits[i-1]] = mEta * mDelta[i][j] + mAlpha * mDeltaWeight[i][j][mUnits[i-1]];
			mWeight[i][j][mUnits[i-1]] += mDeltaWeight[i][j][mUnits[i-1]];
		}
	}
}

inline void Net::SetInput(real *input)
{
	mOutput[0] = input;
}

inline void Net::SetDesire(real *desire)
{
	mDesire = desire;
}

void Net::SetLearningRate(real rate)
{
	mEta = rate;
}

void Net::SetAlpha(real a)
{
	mAlpha = a;
}

void Net::SetDesiredError(real d)
{
	mDesiredError = d;
}

void Net::SetLogName(const char *s)
{
	mLogName = new char[strlen(s)+1];
	strcpy(mLogName, s);
}

void Net::SetMaxEpochs(int m)
{
	mMaxEpochs = m;
}

void Net::SetDefaultValue()
{
	SetLearningRate(0.7);
	SetAlpha(0.0);
	SetDesiredError(0.01);
	SetMaxEpochs(1000);
	InitWeight();
}

void Net::Run(real *input, real *output)
{
	if (mUnits == 0)
		return;
	SetInput(input);
	FeedForward();
	if (output != 0){
		for (int i = 0; i < mUnits[mLayers-1]; ++i){
			output[i] = mOutput[mLayers-1][i];
		}
	}
}

real Net::Error()
{
	real error = 0.0;
	for (int i = 0; i < mUnits[mLayers-1]; ++i){
		error += (mDesire[i] - mOutput[mLayers-1][i]) * (mDesire[i] - mOutput[mLayers-1][i]);
	}
	return error;
}

real Net::Train(real *input, real *desire)
{
	if (mUnits == 0)
		return -1.0;
	SetInput(input);
	SetDesire(desire);
	FeedForward();
	BackProp();
	UpdateWeight();

	return Error();
}

void Net::TrainOnFile(const char *fname)
{
	if (mUnits == 0)
		return;
	FILE *fp;
	if ((fp = fopen(fname, "r")) == 0){
		perror("BPN::TrainOnFile(char *fname)");
		exit(1);
	}
	int patterns;
	int input_size;
	int desire_size;
	real **input;
	real **desire;

	SCAN_PARAMS(patterns, input_size, desire_size)

	input = new real*[patterns];
	desire = new real*[patterns];
	for (int i = 0; i < patterns; ++i){
		input[i] = new real[input_size];
		desire[i] = new real[desire_size];
	}

	SCAN_TRAIN(patterns, input_size, desire_size, input, desire)

	FILE *log_file = 0;
	if (mLogName != 0){
		log_file = fopen(mLogName, "w");
		if (log_file == 0){
			perror("fopen(mLogName)");
			exit(1);
		}
	}

	int epochs = 0;
	while(epochs <= mMaxEpochs){
		++epochs;
		real error = 0.0;
		for (int p = 0; p < patterns; ++p){
			error += Train(input[p], desire[p]);
		}
		if (mLogName){
			fprintf(log_file, "%d %f\n", epochs, error);
		}
		if (error < mDesiredError){
			break;
		}
	}
	if (mLogName != 0){
		fclose(log_file);
	}

	fclose(fp);
	for (int i = 0; i < patterns; ++i){
		delete[] input[i];
		delete[] desire[i];
	}
	delete[] input;
	delete[] desire;
}

real Net::TestOnFile(const char *fname)
{
	if (mUnits == 0)
		return -1.0;
	FILE *fp;
	if ((fp = fopen(fname, "r")) == 0){
		perror("BPN::TrainOnFile(char *fname)");
		exit(1);
	}
	int patterns;
	int input_size;
	int desire_size;
	real **input;
	real **desire;

	SCAN_PARAMS(patterns, input_size, desire_size)

	input = new real*[patterns];
	desire = new real*[patterns];
	for (int i = 0; i < patterns; ++i){
		input[i] = new real[input_size];
		desire[i] = new real[desire_size];
	}

	SCAN_TRAIN(patterns, input_size, desire_size, input, desire)

	real error = 0.0;
	for (int p = 0; p < patterns; ++p){
		SetDesire(desire[p]);
		Run(input[p], 0);
		error += Error();
	}

	for (int i = 0; i < patterns; ++i){
		delete[] input[i];
		delete[] desire[i];
	}
	delete[] input;
	delete[] desire;

	return error;
}
