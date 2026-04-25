#ifndef GameCollisionH
#define GameCollisionH

#include <System.Types.hpp>
#include "GameConstants.h"

namespace GameCollision
{
	
	inline bool CircleCircleCollision(const TPointF &pos1, float radius1,
		const TPointF &pos2, float radius2)
	{
		const float dx = pos2.X - pos1.X;
		const float dy = pos2.Y - pos1.Y;
		const float distSq = dx * dx + dy * dy;
		const float radiusSum = radius1 + radius2;
		return distSq <= (radiusSum * radiusSum);
	}
	
	inline bool CirclePointCollision(const TPointF &circlePos, float radius,
		const TPointF &point)
	{
		const float dx = point.X - circlePos.X;
		const float dy = point.Y - circlePos.Y;
		const float distSq = dx * dx + dy * dy;
		return distSq <= (radius * radius);
	}
	
	inline float DistanceSquared(const TPointF &pos1, const TPointF &pos2)
	{
		const float dx = pos2.X - pos1.X;
		const float dy = pos2.Y - pos1.Y;
		return dx * dx + dy * dy;
	}
	
	inline float Distance(const TPointF &pos1, const TPointF &pos2)
	{
		return std::sqrt(DistanceSquared(pos1, pos2));
	}
}

#endif