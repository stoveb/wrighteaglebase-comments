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
 * @file Plotter.cpp
 * @brief 绘图系统（Plotter）实现
 *
 * Plotter 是一个基于 gnuplot 的轻量级绘图封装，用于在调试/实验中快速输出曲线或
 * 将绘图结果保存为图片文件。
 *
 * 运行机制：
 * - 在非 Windows 平台，通过 `popen("gnuplot", "w")` 启动 gnuplot 子进程；
 * - 通过向 gnuplot 的标准输入写入命令实现绘图控制；
 * - 可选择输出到 X11 显示或导出 PNG 文件。
 *
 * @note 是否启用由配置项 `PlayerParam::UsePlotter()` 控制。
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#include "Plotter.h"
#include "PlayerParam.h"

#include <cstdio>
#include <cstdarg>

/**
 * @brief 构造函数（私有）
 *
 * 初始化内部状态并尝试启动 gnuplot 进程。
 */
Plotter::Plotter():
	mIsDisplayOk(false),
	mIsGnuplotOk(false),
	mpGnupolot(0)
{
	Init();
}

/**
 * @brief 析构函数
 *
 * 关闭与 gnuplot 的通信管道。
 */
Plotter::~Plotter() {
	Close();
}

/**
 * @brief 获取全局单例
 */
Plotter & Plotter::instance()
{
	static Plotter plotter;
	return plotter;
}

/**
 * @brief 初始化绘图环境
 *
 * 在非 Windows 平台：
 * - 检测 DISPLAY 环境变量判断是否可用 X11；
 * - 若启用 Plotter，则启动 gnuplot 子进程；
 * - 若可显示则设置 terminal 为 x11。
 */
void Plotter::Init()
{
#ifndef WIN32
	mIsDisplayOk = getenv("DISPLAY") != 0;

	if (PlayerParam::instance().UsePlotter()) {
		mpGnupolot = popen("gnuplot", "w");
	}

	if (mpGnupolot){
		if (mIsDisplayOk) {
			GnuplotExecute("set terminal x11");
		}
	}
#endif
}

/**
 * @brief 关闭绘图环境
 *
 * 在非 Windows 平台关闭 popen 创建的管道。
 */
void Plotter::Close()
{
#ifndef WIN32
    if (mpGnupolot && pclose(mpGnupolot) == -1) {
        fprintf(stderr, "problem closing communication to gnuplot\n") ;
        return ;
    }
#endif
}

/**
 * @brief 向 gnuplot 发送命令
 *
 * @param cmd gnuplot 命令模板（printf 风格）
 */
void Plotter::GnuplotExecute(const char *  cmd, ...)
{
#ifndef WIN32
	if (!mpGnupolot) return;

    va_list ap;
    char    local_cmd[GP_CMD_SIZE];

    va_start(ap, cmd);
    vsprintf(local_cmd, cmd, ap);
    va_end(ap);

    strcat(local_cmd, "\n");

    fputs(local_cmd, mpGnupolot);
    fflush(mpGnupolot);
#endif
}

/**
 * @brief 设置 x 轴标签
 * @param label 标签字符串
 */
void Plotter::SetXLabel(char * label)
{
    char    cmd[GP_CMD_SIZE];
    sprintf(cmd, "set xlabel \"%s\"", label);
    GnuplotExecute(cmd);
}

/**
 * @brief 设置 y 轴标签
 * @param label 标签字符串
 */
void Plotter::SetYLabel(char * label)
{
    char    cmd[GP_CMD_SIZE];
    sprintf(cmd, "set ylabel \"%s\"", label);
    GnuplotExecute(cmd);
}

/**
 * @brief 重置 gnuplot 会话
 *
 * 通常用于清空上一张图的状态。
 */
void Plotter::Reset()
{
	GnuplotExecute("reset");
}

/**
 * @brief 设置输出为 PNG 文件
 *
 * @param file_name 输出文件名（保存到 Logfiles 目录下）
 */
void Plotter::PlotToFile(char * file_name)
{
	GnuplotExecute("set terminal png");
	GnuplotExecute("set output Logfiles/\"%s\"", file_name);
}

/**
 * @brief 设置输出到显示窗口
 */
void Plotter::PlotToDisplay()
{
	GnuplotExecute("set termianl x11");
}
