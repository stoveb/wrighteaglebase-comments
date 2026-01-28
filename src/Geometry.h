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
 * @file Geometry.h
 * @brief 几何计算与基础数学类型（Vector/Angle/几何对象）定义
 *
 * 本文件提供 WrightEagleBase 中广泛使用的几何/数学基础：
 * - `Vector`：二维向量及常用运算（加减、极坐标设置、距离/角度计算等）；
 * - 角度与三角函数相关封装（AngleDeg/SinCos 等，具体以本文件后续定义为准）；
 * - 场地区域/线段/圆等几何对象与判定（若在本文件中定义）。
 *
 * 这些类型为世界模型、拦截预测、跑动/踢球计算、阵型评估等模块提供基础能力。
 *
 * @note 本文件仅补充注释，不改动任何原有逻辑。
 */

#ifndef __Geometry_H__
#define __Geometry_H__

#include <iostream>
#include <cmath>
#include <cstring>
#include "Utilities.h"
#include "Plotter.h"
#include <fstream>

/**
* 向量类
*/
class Vector
{
public:
	explicit Vector(const double & x = 0.0, const double & y = 0.0): mX(x), mY(y) {
		AssertValid();
	}

	const double & X() const { return mX; }
	const double & Y() const { return mY; }

	void SetX(const double & x) { mX = x; Assert(!IsInvalid(mX)); }
	void SetY(const double & y) { mY = y; Assert(!IsInvalid(mY)); }

	void SetValue(const double & x, const double & y) { mX = x; mY = y; AssertValid(); }
	void SetValuePolar(const double & r, const AngleDeg & theta) {
		SinCosT value = SinCos(theta);

		mX = r * Cos(value);
		mY = r * Sin(value);

		AssertValid();
	}

	Vector operator-() const { return Vector(-mX, -mY); }
	Vector operator+(const Vector &a) const { return Vector(mX + a.mX, mY + a.mY); }
	Vector operator-(const Vector &a) const { return Vector(mX - a.mX, mY - a.mY); }
	Vector operator*(const double & a) const { return Vector(mX * a, mY * a); }
	Vector operator/(double a) const {
		// 检查除数是否接近零，避免除零错误
		if (a > -FLOAT_EPS && a < FLOAT_EPS) {
			// 将接近零的除数调整为最小非零值
			a = Sign(a) * FLOAT_EPS;
		}

		return Vector(mX / a, mY / a);
	}

	void operator+=(const Vector &a) { mX += a.mX; mY += a.mY; AssertValid(); }
	void operator+=(const double &a) { mX += a; mY += a; AssertValid(); }
	void operator-=(const Vector &a) { mX -= a.mX; mY -= a.mY; AssertValid(); }
	void operator-=(const double &a) { mX -= a; mY -= a; AssertValid(); }
	void operator*=(const double &a) { mX *= a; mY *= a; AssertValid(); }
	void operator/=(const double &a) { mX /= a; mY /= a; AssertValid(); }

	bool operator!=(const Vector &a) const { return (mX != a.mX) || (mY != a.mY); }
	bool operator!=(const double &a) const { return (mX != a) || (mY != a); }
	bool operator==(const Vector &a) const { return (mX == a.mX) && (mY == a.mY); }

	friend std::ostream& operator<<(std::ostream & os, const Vector & v) { return os << "(" << v.mX << ", " << v.mY << ")"; }

	double Mod() const { return Sqrt(mX * mX + mY * mY); }
	double Mod2() const { return mX * mX + mY * mY; }
	double Dist(const Vector &a) const { return (*this - a).Mod(); }
	double Dist2(const Vector &a) const { return (*this - a).Mod2(); }

	AngleDeg Dir() const { return ATan2(mY, mX); }

	/**
	* \return a Vector with length "length" at the same direction, or Vector (0, 0) if the original
	*         Vector was (0, 0).
	*/
	Vector SetLength(const double & length) const
	{
		// 检查原向量长度是否大于零
		if (Mod() > 0.0) {
			// 按比例缩放向量到指定长度
			return (*this) * (length / Mod());
		}
		// 原向量为零向量，返回零向量
		return Vector(0.0, 0.0);
	}

	/**
	* \return a Vector with length 1.0 at the same direction.
	*/
	Vector Normalize() const { return SetLength(1.0); }

	/**
	* \return a Vector rotated by angle.
	*/
	Vector Rotate(const AngleDeg & angle) const
	{
		return Rotate(SinCos(angle));
	}

	Vector Rotate(const SinCosT & value) const
	{
		return Vector(mX * Cos(value) - mY * Sin(value), mY * Cos(value) + mX * Sin(value));
	}

	/**
	* check if a point is approximate equal to *this;
	* @param point to be checked.
	* return true when they are approximate equal, false else;
	*/
	bool ApproxEqual(const Vector& a) const
	{
		// 检查两个向量的X和Y分量是否都在浮点精度范围内相等
		return fabs(mX-a.X()) < FLOAT_EPS && fabs(mY-a.Y()) < FLOAT_EPS;
	}

private:
	void AssertValid() {
		Assert(!IsInvalid(mX));
		Assert(!IsInvalid(mY));
	}

private:
	double mX;
	double mY;
};

inline Vector Polar2Vector(const double & mod, const AngleDeg & ang)
{
	// 计算角度的正弦和余弦值
	SinCosT value = SinCos(ang);

	// 将极坐标转换为笛卡尔坐标
	return Vector(mod * Cos(value), mod * Sin(value));
}

/**
* 射线类
*/
class Line;

class Ray
{
public:
	Ray() {

	}

	explicit Ray(const Vector & origin, const AngleDeg & direction) { SetValue(origin, direction); }

	const Vector & Origin() const      { return mOrigin; }
	const AngleDeg & Dir() const { return mDirection; }

	void SetOrigin(const Vector & origin)       { mOrigin = origin; }
	void SetDirection(const AngleDeg & direction) { mDirection = direction; }
	void SetValue(const Vector & origin, const AngleDeg & direction) { mOrigin = origin; mDirection = direction; }

	Vector GetPoint(const double & dist) const            { return mOrigin + Polar2Vector(dist, mDirection); }

	bool IsInRightDir(const Vector & point) const 
	{ 
		// 检查点是否在射线的正确方向上
		// 计算从原点到点的向量与射线方向的夹角
		// 如果夹角小于10度，则认为在正确方向上
		return fabs(GetNormalizeAngleDeg((point - mOrigin).Dir() - mDirection)) < 10.0; 
	}

	bool OnRay(const Vector& point, const double & buffer = FLOAT_EPS) const
	{
		// 计算从射线原点到目标点的向量
		Vector v = point - mOrigin;
		// 检查点到射线的距离是否在缓冲范围内，并且点在正确方向上
		// 使用点到直线的距离公式：|sin(角度差)| * 距离
		return fabs(Sin(v.Dir() - mDirection) * v.Mod()) < buffer && IsInRightDir(point);
	}

	bool Intersection(const Line & l, double & intersection_dist) const;
	bool Intersection(const Line & l, Vector & point) const;
	bool Intersection(const Ray & r, Vector & point) const;
	bool Intersection(const Ray & r, double & intersection_dist) const;
	double Intersection(const Line & l) const;

	/*得到一条射线上离这个点最近的点*/
	Vector GetClosestPoint(const Vector& point) const;// add by wang yu hang -09.1.14

	inline double GetDistanceFromOrigin(const Vector& point) const
	{
		return (point - mOrigin).Mod();
	}

private:
	Vector mOrigin;
	AngleDeg mDirection;
};


/**
* 直线类
*/
class Line
{
public:
	/** a*x+b*y+c=0 */
	explicit Line(const double & a = 0.0, const double & b = 0.0, const double & c = 0.0): mA(a), mB(b), mC(c) {}

	Line(const Vector & point1, const Vector & point2)
	{
		// 检查两点是否近似垂直（X坐标差接近零）
		if (fabs(point1.X() - point2.X()) < FLOAT_EPS) {
			// 垂直线：x = 常数
			mA = 1.0;
			mB = 0.0;
			mC = -point1.X();
		}
		else {
			// 非垂直线：计算斜率和截距
			mA = (point2.Y() - point1.Y()) / (point2.X() - point1.X()); // 斜率
			mB = -1.0;
			mC = point1.Y() - point1.X() * mA; // 截距
		}
	}

	Line(const Ray &r)
	{
		// 从射线构造直线
		// 使用射线的原点和原点+单位方向向量确定直线
		*this = Line(r.Origin(), r.Origin() + Polar2Vector(1.0, r.Dir()));
	}

	const double & A() const         { return mA; }
	const double & B() const         { return mB; }
	const double & C() const         { return mC; }
	double Dir() const       { return ATan(-mA / mB); }

	double GetX(const double & y) const
	{
		// 根据Y坐标求解X坐标：ax + by + c = 0 => x = (-c - by) / a
		if (fabs(mA) > 0.0) {
			return ((-mC - mB * y) / mA);
		}
		else {
			// 如果A接近零，说明是水平线，X坐标无意义，返回0
			return 0.0;
		}
	}

	double GetY(const double & x) const
	{
		// 根据X坐标求解Y坐标：ax + by + c = 0 => y = (-c - ax) / b
		if (fabs(mB) > 0.0) {
			return ((-mC - mA * x) / mB);
		}
		else {
			// 如果B接近零，说明是垂直线，Y坐标无意义，返回0
			return 0.0;
		}
	}

	/** 是否在直线上 */
	bool IsOnLine(const Vector & point, const double & buffer = FLOAT_EPS) const
	{
		return fabs(mA * point.X() + mB * point.Y() + mC) < buffer;
	}

	/** 是否在直线上方 */
	bool IsUpLine(const Vector & point) const
	{
		// 检查点是否在直线上方（不在直线上且直线方程值大于0）
		return !IsOnLine(point) && (mA * point.X() + mB * point.Y() + mC > 0);
	}

	bool HalfPlaneTest(const Vector & pt)
	{
		// 半平面测试：判断点是否在直线的某一侧
		if (fabs(mB) > 0.0) {
			// 如果B不为零，使用Y坐标比较
			return pt.Y() > GetY(pt.X());
		}
		// 如果B接近零，说明是垂直线，使用X坐标比较
		return pt.X() < -mC / mA;
	}

	/** 斜率是否相等 */
	bool IsSameSlope(const Line & l, const double & buffer = FLOAT_EPS) const
	{
		// 检查两条直线的斜率是否相等
		// 如果两条直线都是垂直线（B接近零），则认为斜率相等
		// 否则比较斜率值（-A/B）
		return (fabs(mB) < buffer && fabs(l.mB) < buffer) || fabs(mA / mB - l.mA / l.mB) < buffer;
	}

	/**
	* 判断一点的垂足是否在两点之间
	*/
	bool IsInBetween(const Vector & pt, const Vector & end1, const Vector & end2) const;

	/** 求交点 */
	bool Intersection(const Line &l, Vector &point) const;
	Vector Intersection(const Line &l) const;

	/** 求与射线的交点 */
	bool Intersection(const Ray &r, Vector &point) const;

	/** 点到直线的距离 */
	double Dist(const Vector& point) const
	{
		// 使用点到直线的距离公式：|Ax + By + C| / sqrt(A² + B²)
		return fabs(mA * point.X() + mB * point.Y() + mC) / Sqrt(mA * mA + mB * mB);
	}

	/** 两点是否在直线同侧 */
	bool IsPointInSameSide(const Vector & pt1, const Vector & pt2) const
	{
		// 构造通过两点的直线
		Line tl(pt1, pt2);
		// 如果两条直线斜率相同，说明两点在同一直线上，返回true
		if(IsSameSlope(tl)) return true;

		// 求两条直线的交点
		Vector inter_point;
		Intersection(tl, inter_point);

		// 检查两点是否在交点的同侧
		// 通过乘积符号判断：如果乘积<=0，说明在同侧
		return (inter_point.X() - pt1.X()) * (pt2.X() - inter_point.X()) <= 0;
	}

	/** 过pt点的垂线 **/
	Line GetPerpendicular(const Vector & pt) const
	{
		// 构造过给定点的垂线
		// 原直线：Ax + By + C = 0
		// 垂线：Bx - Ay + (Ay - Bx) = 0
		return Line(mB, -mA, mA * pt.Y() - mB * pt.X());
	}

	/**
	* Set this line be the perpendicular bisector of pos1 and pos2;
	* @param one point;
	* @param another point;
	*/
	void PerpendicularBisector(const Vector & pos1, const Vector & pos2)
	{
		// 构造两点连线的垂直平分线
		// 垂直平分线方程：2(x2-x1)x + 2(y2-y1)y + (x1²-x2²+y1²-y2²) = 0
		mA = 2 * (pos2.X() - pos1.X());
		mB = 2 *(pos2.Y() - pos1.Y());
		mC = pos1.X() * pos1.X() - pos2.X() * pos2.X() + pos1.Y() * pos1.Y() - pos2.Y() * pos2.Y();
	}

	/** 得到投影点 */
	Vector GetProjectPoint(const Vector & pt) const
	{
		// 计算点在直线上的投影
		// 通过求原直线与过该点的垂线的交点得到投影点
		Vector joint_pt;
		Intersection(GetPerpendicular(pt), joint_pt);
		return joint_pt;
	}

	//得到对称点
	inline Vector MirrorPoint(const Vector & pt)
	{
		// 计算点关于直线的对称点
		// 对称点 = 投影点 * 2 - 原点
		return GetProjectPoint(pt) * 2.0 - pt;
	}

	/**
	* 得到直线上两点间距离这个点最近的点
	*/
	Vector GetClosestPointInBetween(const Vector & pt, const Vector & end1, const Vector & end2) const;

	void LineFromPline(const Vector & pos1, const Vector & pos2)
	{
		// 从两点构造垂直平分线（与PerpendicularBisector功能相同）
		// 垂直平分线方程：2(x2-x1)x + 2(y2-y1)y + (x1²-x2²+y1²-y2²) = 0
		mA = 2* (pos2.X() - pos1.X());
		mB = 2 *(pos2.Y() - pos1.Y());
		mC= pos1.X() * pos1.X() - pos2.X() * pos2.X() + pos1.Y() * pos1.Y() - pos2.Y() * pos2.Y();
	}

	const double & GetA() const { return mA; }
	const double & GetB() const { return mB; }
	const double & GetC() const { return mC; }

private:
	/**
	* Ax + By + C = 0
	*/
	double mA;
	double mB;
	double mC;
};


/**
* 矩形类
*/
class Rectangular
{
public:
	Rectangular(): mLeft(0.0), mRight(0.0), mTop(0.0), mBottom(0.0) {}
	Rectangular(const double & left, const double & right, const double & top, const double & bottom): mLeft(left), mRight(right), mTop(top), mBottom(bottom) {}

	Rectangular(const Vector & center, const Vector & size)
	{
		// 根据中心点和尺寸构造矩形
		// 左边界 = 中心X - 宽度/2
		mLeft   = center.X() - size.X() / 2.0;
		// 右边界 = 中心X + 宽度/2
		mRight  = center.X() + size.X() / 2.0;
		// 上边界 = 中心Y - 高度/2
		mTop    = center.Y() - size.Y() / 2.0;
		// 下边界 = 中心Y + 高度/2
		mBottom = center.Y() + size.Y() / 2.0;
	}

	const double & Left() const { return mLeft; }
	const double & Right() const { return mRight; }
	const double & Top() const { return mTop; }
	const double & Bottom() const { return mBottom; }

	void SetLeft(const double & left) { mLeft = left; }
	void SetRight(const double & right) { mRight = right; }
	void SetTop(const double & top) { mTop = top; }
	void SetBottom(const double & bottom) { mBottom = bottom; }

	Vector TopLeftCorner() const { return Vector(mLeft, mTop); }
	Vector TopRightCorner() const { return Vector(mRight, mTop); }
	Vector BottomLeftCorner() const { return Vector(mLeft, mBottom); }
	Vector BottomRightCorner() const { return Vector(mRight, mBottom); }

	Line TopEdge() const     { return Line(TopLeftCorner(), TopRightCorner()); }
	Line BottomEdge() const  { return Line(BottomLeftCorner(), BottomRightCorner()); }
	Line LeftEdge() const    { return Line(TopLeftCorner(), BottomLeftCorner()); }
	Line RightEdge() const   { return Line(TopRightCorner(), BottomRightCorner()); }

	bool IsWithin(const Vector &v, const double & buffer = FLOAT_EPS) const
	{
		// 检查点是否在矩形内（包括边界）
		// 允许设置缓冲区来扩大或缩小检查范围
		return (v.X() >= mLeft - buffer) && (v.X() <= mRight + buffer) && (v.Y() >= mTop - buffer) && (v.Y() <= mBottom + buffer);
	}

	bool Intersection(const Ray &r, Vector &point) const;
	Vector Intersection(const Ray &r) const;

	Vector AdjustToWithin(const Vector &v) const
	{
		// 将点调整到矩形内部
		Vector r = v;

		// 检查并调整X坐标
		if (r.X() < mLeft)
		{
			// X坐标小于左边界，设置为左边界
			r.SetX(mLeft);
		}
		else if (r.X() > mRight)
		{
			// X坐标大于右边界，设置为右边界
			r.SetX(mRight);
		}

		// 检查并调整Y坐标
		if (r.Y() < mTop)
		{
			// Y坐标小于上边界，设置为上边界
			r.SetY(mTop);
		}
		else if (r.Y() > mBottom)
		{
			// Y坐标大于下边界，设置为下边界
			r.SetY(mBottom);
		}

		return r;
	}

private:
	double mLeft; // 矩形左边
	double mRight; // 矩形右边
	double mTop; // 矩形上边
	double mBottom; // 矩形下边
};


/**
* 曲线类
*/
class ReciprocalCurve
{
public:
	explicit ReciprocalCurve(const double & a = 0.0, const double & b = 0.0, const double & c = 0.0, const double & out_min = 0.0, const double & out_max = 1.0): mA(a), mB(b), mC(c), mOutMin(out_min), mOutMax(out_max) { }

	const double & A() const { return mA; }
	const double & B() const { return mB; }
	const double & C() const { return mC; }
	const double & OutMin() const { return mOutMin; }
	const double & OutMax() const { return mOutMax; }

	void SetABC(const double & a, const double & b, const double & c)
	{
		mA = a;
		mB = b;
		mC = c;
	}

	void SetOutMinMax(const double & out_min, const double & out_max)
	{
		mOutMin = out_min;
		mOutMax = out_max;
	}

	void Interpolate(const double & x1, const double & y1, const double & x2, const double & y2, const double & x3, const double & y3)
	{
		Assert(((x1 - x2) / (y2 - y1) - (x1 - x3) / (y3 - y1)) != 0);
		Assert((y1 - y2) != 0);

		mA = ((x1 * y1 - x2 * y2) / (y2 - y1) - (x1 * y1 - x3 * y3) / (y3 - y1)) / ((x1 - x2) / (y2 - y1) - (x1 - x3) / (y3 - y1));
		mC = (mA * (x1 - x2) - (x1 * y1 - x2 * y2)) / (y1 - y2);
		mB = (y1 - mA) * (x1 + mC);
		mB = (y2 - mA) * (x2 + mC);
		mB = (y3 - mA) * (x3 + mC);
	}

	double GetOutput(const double & x, const bool limited = true) const
	{
		double value = mA + mB / (x + mC);
		if (limited == true)
		{
			if (value < mOutMin)
			{
				value = mOutMin;
			}
			else if (value > mOutMax)
			{
				value = mOutMax;
			}
		}
		return value;
	}

	void Show(const char * title, double minx, double maxx) {
		std::cerr << mA << " + " << mB << " / ( x + " << mC << " )" << std::endl;

		Plotter::instance().GnuplotExecute("set xrange [%g:%g]", minx, maxx);
		Plotter::instance().GnuplotExecute("plot %g + %g / (x + %g) title \"%s\"", mA, mB, mC, title);
	}

private:
	double mA;
	double mB;
	double mC;
	double mOutMin;
	double mOutMax;
};

/** Get the central perpendicular line from two points */
inline Line GetCentralPerpendicularLine(const Vector &pos1, const Vector &pos2)
{
	double a = 2.0 * (pos2.X() - pos1.X());
	double b = 2.0 * (pos2.Y() - pos1.Y());
	double c = pos1.X() * pos1.X() - pos2.X() * pos2.X() + pos1.Y() * pos1.Y() - pos2.Y() * pos2.Y();
	return Line(a, b, c);
}

/**
* 圆类
*/
class Circle
{
public:
	explicit Circle(const Vector & center = Vector(0.0, 0.0), const double & radius = 0.0): mCenter(center), mRadius(radius) {}
	Circle(const double & center_x, const double & center_y, const double & radius): mCenter(Vector(center_x, center_y)), mRadius(radius) {}
	Circle(const Vector & point1, const Vector & point2, const Vector & point3)
	{
		Line l1 = GetCentralPerpendicularLine(point1, point2);
		Line l2 = GetCentralPerpendicularLine(point2, point3);

		if (l1.Intersection(l2, mCenter) == false)
		{
			mCenter = Vector(0.0, 0.0);
		}
		mRadius = mCenter.Dist(point1);
	}

	const Vector & Center() const { return mCenter; }
	const double & Radius() const { return mRadius; }
	void SetCenter(const Vector & center) { mCenter = center; }
	void SetRadius(const double & radius) { mRadius = radius; }

	bool IsWithin(const Vector & p, const double & buffer = FLOAT_EPS)
	{
		return mCenter.Dist(p) <= mRadius + buffer;
	}

	/**
	* Get intersection points between the circle and the ray.
	* t1 is nearer to the origin than t2.
	* \param r the ray.
	* \param t1 will be set to the distance from origin of the ray to intersection point 1.
	* \param t2 will be set to the distance from origin of the ray to intersection point 2.
	* \param buffer controls precision.
	* \return number of intersection points.
	*/
	int	Intersection(const Ray &r, double &t1, double &t2, const double & buffer = 0.0) const;
	int	Intersection(const Circle &c, Vector &v1, Vector &v2, const double & buffer) const;

private:
	Vector mCenter;
	double mRadius;
};

#endif
