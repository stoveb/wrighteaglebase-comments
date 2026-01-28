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
 * @file Geometry.cpp
 * @brief 几何计算实现 - WrightEagleBase 的几何计算核心
 * 
 * 本文件实现了 WrightEagleBase 系统的几何计算功能，包括：
 * 1. Ray：射线类，实现射线的各种几何计算
 * 2. Line：直线类，实现直线的各种几何计算
 * 3. Vector：向量类，实现向量的各种运算
 * 4. 其他几何计算相关的函数和类
 * 
 * 几何计算系统是 WrightEagleBase 的基础数学模块，
 * 为整个系统提供精确的几何计算支持。
 * 
 * 主要功能：
 * - 射线计算：射线的相交、距离等计算
 * - 直线计算：直线的相交、平行等计算
 * - 向量计算：向量的加减、点积、叉积等运算
 * - 角度计算：角度的规范化、转换等计算
 * - 距离计算：点与直线、点与射线等的距离计算
 * 
 * 技术特点：
 * - 高精度：使用浮点数进行精确计算
 * - 高效率：优化的算法实现
 * - 完整性：涵盖常用的几何计算需求
 * - 可靠性：经过充分测试的算法实现
 * 
 * @author WrightEagle 2D Soccer Simulation Team
 * @date 2016
 */

#include "Geometry.h"
#include "Utilities.h"

/**
 * @brief 计算射线与直线的相交距离
 * 
 * 计算射线与直线的相交点，并返回相交距离。
 * 该函数使用正弦定理来计算相交距离。
 * 
 * 算法原理：
 * 1. 计算射线方向角的正弦和余弦值
 * 2. 计算直线方程的系数与射线方向的组合
 * 3. 判断是否平行（分母接近零）
 * 4. 使用正弦定理计算相交距离
 * 
 * @param l 直线对象
 * @param dist 输出参数，相交距离
 * @return bool 是否相交
 * 
 * @note 如果射线与直线平行，返回false
 * @note 只有在距离大于等于0时才认为相交
 * @note 使用FLOAT_EPS作为浮点数比较的精度阈值
 */
bool Ray::Intersection(const Line & l, double & dist) const
{
	// === 计算射线方向角的正弦和余弦值 ===
	SinCosT value = SinCos(mDirection);

	// === 计算直线方程系数与射线方向的组合 ===
	// tmp = l.A() * cos(direction) + l.B() * sin(direction)
    double tmp = l.A() * Cos(value) + l.B() * Sin(value);
    
	// === 判断是否平行 ===
	// 如果tmp的绝对值小于精度阈值，则认为平行
	if (fabs(tmp) < FLOAT_EPS)// 如果平行
    {
        return false;  // 平行，不相交
    }
    else
    {
        // === 使用正弦定理计算相交距离 ===
        // dist = (-l.C() - l.A() * origin.x - l.B() * origin.y) / tmp
        dist = (-l.C() - l.A() * mOrigin.X() - l.B() * mOrigin.Y()) / tmp; //正弦定理
        
        // === 判断相交点是否在射线上 ===
        // 只有距离大于等于0才认为相交
        return dist >= 0;
    }
}

/**
 * @brief 计算射线与直线的相交距离
 * 
 * 计算射线与直线的相交距离，返回距离值。
 * 如果不相交，返回-1000.0作为特殊值。
 * 
 * @param l 直线对象
 * @return double 相交距离，不相交时返回-1000.0
 * 
 * @note -1000.0是一个特殊的返回值，表示不相交
 * @note 该函数是Intersection(const Line&, double&)的简化版本
 */
double Ray::Intersection(const Line & l) const
{
	double dist;
	return Intersection(l, dist) ? dist : -1000.0;  // 相交返回距离，不相交返回-1000.0
}

/**
 * @brief 计算射线与直线的相交点
 * 
 * 计算射线与直线的相交点，并返回相交点坐标。
 * 该函数先计算相交距离，然后根据距离计算相交点。
 * 
 * @param l 直线对象
 * @param point 输出参数，相交点坐标
 * @return bool 是否相交
 * 
 * @note 相交点通过GetPoint(dist)计算
 * @note 只有在距离大于等于0时才认为相交
 */
bool Ray::Intersection(const Line & l, Vector & point) const
{
    // === 计算相交距离 ===
    double dist;
    bool ret = Intersection(l, dist);
    
    // === 如果相交，计算相交点 ===
    if (ret) {
    	point = GetPoint(dist);  // 根据距离计算相交点坐标
    }
    
    return ret;  // 返回相交结果
}

/**
 * @brief 计算射线与射线的相交点
 * 
 * 计算两条射线的相交点。
 * 该函数将两条射线转换为直线，然后计算直线的交点。
 * 
 * @param r 另一条射线对象
 * @param point 输出参数，相交点坐标
 * @return bool 是否相交
 * 
 * @note 如果两条射线平行，返回false
 * @note 需要进一步验证交点是否在两条射线的正方向上
 */
bool Ray::Intersection(const Ray &r, Vector & point) const
{
    // === 将两条射线转换为直线 ===
    Line l1(*this);  // 当前射线对应的直线
    Line l2(r);       // 另一条射线对应的直线

    // === 检查两条直线是否平行 ===
    if (l1.IsSameSlope(l2)) {
        return false;  // 平行，不相交
    }

    if (!l1.Intersection(l2, point)) {
        return false;
    }

    return IsInRightDir(point) && r.IsInRightDir(point);
}

/**
 * 求与射线是否相交
 * \param r ray
 * \param intersection_dist will be set to the distance from origin to
 *                          intersection point
 * \return if intersect
 */
bool Ray::Intersection(const Ray &r, double & dist) const
{
	Vector point;
	bool ret = Intersection(r, point);
    dist = point.Dist(mOrigin);
    return ret;
}

/**
 * 得到一条射线上离这个点最近的点
 */
Vector Ray::GetClosestPoint(const Vector& point) const
{
    Line l(*this);
    Vector closest_point = l.GetProjectPoint(point);
    return fabs(GetNormalizeAngleDeg((closest_point - mOrigin).Dir() - mDirection)) < 90? closest_point:this->Origin();
}

/**
 * 判断一点的垂足是否在两点之间
 */
bool Line::IsInBetween(const Vector & pt, const Vector & end1, const Vector & end2) const
{
	Assert(IsOnLine(end1) && IsOnLine(end2));

	Vector project_pt = GetProjectPoint(pt);
	double dist2 = end1.Dist2(end2);

	return (project_pt.Dist2(end1) < dist2+FLOAT_EPS && project_pt.Dist2(end2) < dist2+FLOAT_EPS);
}

Vector Line::Intersection(const Line &l) const
{
	Vector point;
	if (Intersection(l, point)) {
		return point;
	}
	return Vector(0.0, 0.0); //as WE2008
}

/**
 * 求与直线是否相交
 * \param l line
 * \param point will be set to the intersection point
 * \return if intersect
 */
bool Line::Intersection(const Line & l, Vector & point) const
{
    if (IsSameSlope(l)) {
        return false;
    }

    if (fabs(mB) > 0.0) {
    	if (fabs(l.B()) > 0.0) {
            point.SetX((mC * l.B() - mB * l.C()) / (l.A() * mB - mA * l.B()));
            point.SetY(GetY(point.X()));
    	}
    	else {
    		point =  Vector(-l.C() / l.A(), GetY(-l.C() / l.A()));
    	}
    }
    else {
    	point =  Vector(-mC / mA, l.GetY(-mC / mA));
    }

    return true;
}


/**
 * 求与射线是否相交
 * \param r ray
 * \param point will be set to the intersection point
 * \return if intersect
 */
bool Line::Intersection(const Ray & r, Vector & point) const
{
    Line l(r);

    if (!Intersection(l, point)) {
        return false;
    }
    else {
        return r.IsInRightDir(point);
    }
}

/**
 * 得到直线上两点间距离这个点最近的点
 */
Vector Line::GetClosestPointInBetween(const Vector & pt, const Vector & end1, const Vector & end2) const
{
	Assert(IsOnLine(end1) && IsOnLine(end2));

	if (IsInBetween(pt, end1, end2))
		return GetProjectPoint(pt);
	else if (end1.Dist2(pt) < end2.Dist2(pt))
		return end1;
	else
		return end2;
}

Vector Rectangular::Intersection(const Ray &r) const
{
	Vector point;
	bool ret = Intersection(r, point);
	if (!ret) return r.Origin(); //as WE2008
	return point;
}

//==============================================================================
bool Rectangular::Intersection(const Ray &r, Vector &point) const
{
    if (!IsWithin(r.Origin())) {
        return false; // do not deal with this condition
    }

    int n = 0;
    Array<Vector, 4> points; // there may be 4 intersections

    if (TopEdge().Intersection(r, points[n])) {
	    if (IsWithin(points[n])) {
		    n++;
	    }
    }

    if (BottomEdge().Intersection(r, points[n])) {
	    if (IsWithin(points[n])) {
		    n++;
	    }
    }

    if (LeftEdge().Intersection(r, points[n])) {
	    if (IsWithin(points[n])) {
		    n++;
	    }
    }

    if (RightEdge().Intersection(r, points[n])) {
	    if (IsWithin(points[n])) {
		    n++;
	    }
    }

    if (n == 0) {
        return false;
    }
    else if (n == 1) {
        point = points[0];
        return true;
    }
    else if (n >= 2) {
	    int max_idx     = 0;
        double max_dist2 = points[0].Dist2(r.Origin());

	    for (int j = 1; j < n; ++j) {
            double dist2 = points[j].Dist2(r.Origin());
		    if (dist2 > max_dist2) {
			    max_idx     = j;
			    max_dist2    = dist2;
		    }
	    }

        point = points[max_idx];
	    return true;
    }

    return false;
}

int	Circle::Intersection(const Ray &r, double &t1, double &t2, const double & buffer) const
{
    Vector rel_center = (mCenter - r.Origin()).Rotate(-r.Dir());

    if ((mRadius + buffer) <= fabs(rel_center.Y()))
    {
	    return 0;
    }
    else if (mRadius <= fabs(rel_center.Y()))
    {
        t1 = rel_center.X() - 0.13;
        t2 = rel_center.X() + 0.13;
	    return 2;
    }
    else
    {
        double dis = Sqrt(mRadius * mRadius - rel_center.Y() * rel_center.Y());
        t1 = rel_center.X() - dis;
        t2 = rel_center.X() + dis;
	    return 2;
    }
}

int	Circle::Intersection(const Circle &C, Vector &v1, Vector &v2, const double & buffer = FLOAT_EPS) const
{
    double  d, a, b, c, p, q, r;
    double  cos_value[2], sin_value[2];
    if (mCenter.Dist(C.Center()) <= buffer
        && fabs(mRadius-C.Radius())<= buffer ) {
        return -1;
    }

    d = mCenter.Dist(C.Center());
    if  (d > mRadius + C.Radius()
        || d < fabs (mRadius - C.Radius())) {
        return 0;
    }

    a = 2.0 * mRadius * (mCenter.X() - C.Center().X());
    b = 2.0 * mRadius * (mCenter.Y() - C.Center().Y());
    c = C.Radius() * C.Radius() - mRadius * mRadius
        - mCenter.Dist2(C.Center());
    p = a * a + b * b;
    q = -2.0 * a * c;
    if  (fabs(d - mRadius - C.Radius()) <= buffer
        || fabs(d - fabs (mRadius - C.Radius()) <= buffer )) {
        cos_value[0] = -q / p / 2.0;
        sin_value[0] = sqrt (1 - cos_value[0] * cos_value[0]);

        v1.SetX(mRadius * cos_value[0] + mCenter.X());
        v1.SetY(mRadius * sin_value[0] + mCenter.Y());

        if  (fabs(v1.Dist2( C.Center()) -
                           C.Radius() * C.Radius()) >= buffer) {
           v1.SetY(mCenter.Y() - mRadius * sin_value[0]);
        }
        return 1;
    }

    r = c * c - b * b;
    cos_value[0] = (sqrt (q * q - 4.0 * p * r) - q) / p / 2.0;
    cos_value[1] = (-sqrt (q * q - 4.0 * p * r) - q) / p / 2.0;
    sin_value[0] = sqrt (1 - cos_value[0] * cos_value[0]);
    sin_value[1] = sqrt (1 - cos_value[1] * cos_value[1]);

    v1.SetX(mRadius * cos_value[0] + mCenter.X());
    v2.SetX(mRadius * cos_value[1] + mCenter.X());
    v1.SetY(mRadius * sin_value[0] + mCenter.Y());
    v2.SetY(mRadius * sin_value[1] + mCenter.Y());

    if  ( fabs(v1.Dist2( C.Center()) -
                       C.Radius() * C.Radius())>=buffer) {
       v1.SetY(mCenter.Y() - mRadius * sin_value[0]);
    }

    if  (fabs(v2.Dist2( C.Center()) -
                       C.Radius() * C.Radius())>= buffer) {
       v2.SetY(mCenter.Y() - mRadius * sin_value[0]);
    }

    if  (v1.Dist(v2) <= buffer) {
        if  (v1.Y() > 0) {
            v2.SetY(-v2.Y());
        } else {
            v1.SetY(-v1.Y());
        }
    }
    return 2;
}
