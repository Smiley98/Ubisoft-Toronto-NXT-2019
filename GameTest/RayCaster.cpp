#include "stdafx.h"
#include "RayCaster.h"
#include "App/app.h"
#include "Viewer.h"
#include "Line.h"

//Increase distance till point of intersection by 5% so that the ray is guaranteed to exceed its current cell.
#define DISTANCE_MULTIPLIER 1.05f
#define MAX_STEPS 32
#define DEBUG_DRAW true

CRayCaster::CRayCaster(float thickness) :
	m_count(APP_VIRTUAL_WIDTH / (size_t)thickness), m_thickness(thickness), m_step((float)m_count * thickness), m_rayOriginY(APP_VIRTUAL_HEIGHT * 0.5f)
{	//Make sure thickness is between 1 and 32.
	assert(thickness >= 1.0f && thickness <= 31.0f);
	m_indexBuffer.resize(m_count);
	m_heightBuffer.resize(m_count);
}

CRayCaster::~CRayCaster()
{
}

void CRayCaster::Update(const CSimpleTileMap& map, const CViewer& viewer)
{
}

void CRayCaster::Render(const CSimpleTileMap& map, const CViewer& viewer)
{
	float halfFov = viewer.m_fov * 0.5f;
	//Half resolution divided by right triangle based on fov gives adj of right triangle (which is projection distance).
	float projectionDistance = (APP_VIRTUAL_WIDTH * 0.5f) / tan(halfFov);

	float x = 0.0f;
	glLineWidth(m_thickness);
	for (size_t i = 0; i < m_count; i++) {
		const CTile& tile = CTile::tiles[m_indexBuffer[i]];
		//Projected height = actual height / distance to poi * distance to projection plane.
		float projectedHeight = (tile.height / m_heightBuffer[i]) * projectionDistance;
		App::DrawLine(x, m_rayOriginY - projectedHeight, x, m_rayOriginY + projectedHeight, tile.r, tile.g, tile.b);
		x += m_thickness;
		//App::DrawLine(x, m_rayOriginY - m_heightBuffer[i], x, m_rayOriginY + m_heightBuffer[i], tile.r, tile.g, tile.b);
	}
}

void CRayCaster::Debug(const CSimpleTileMap & map, const CViewer & viewer)
{
	const float angleStep = viewer.m_fov / (float)m_count;
	const float raysStart = viewer.m_angle - viewer.m_fov * 0.5f;
	for (size_t i = 0; i < m_count; i++) {
		float rayAngle = raysStart + angleStep * (float)i;
		march(map, viewer.m_position, Math::direction(rayAngle), i);
	}
}

inline void CRayCaster::march(const CSimpleTileMap & map, const CPoint& position, const CPoint& direction, size_t index)
{
	float tileWidth = map.getTileWidth();
	float tileHeight = map.getTileHeight();

	float unitRise = direction.y / direction.x;
	float unitRun = direction.x / direction.y;

	CPoint poi = position;

	//Continue searching until we find anything but a floor (air).
	EMapValue tileValue = EMapValue::FLOOR;
	uint32_t count = 0;
	while (tileValue == EMapValue::FLOOR) {
		float xRemainder = fmodf(poi.x, tileWidth);
		float xEdge = direction.x >= 0.0f ? poi.x + tileWidth - xRemainder : poi.x - xRemainder;
		float xDistance = xEdge - poi.x;
		float xRate = xDistance / direction.x;

		float yRemainder = fmodf(poi.y, tileHeight);
		float yEdge = direction.y >= 0.0f ? poi.y + tileHeight - yRemainder : poi.y - yRemainder;
		float yDistance = yEdge - poi.y;
		float yRate = yDistance / direction.y;

		if (abs(yRate) < abs(xRate)) {
			yDistance *= DISTANCE_MULTIPLIER;
			poi.y = poi.y + yDistance;
			poi.x = poi.x + yDistance * unitRun;
		}
		else {
			xDistance *= DISTANCE_MULTIPLIER;
			poi.x = poi.x + xDistance;
			poi.y = poi.y + xDistance * unitRise;
		}

		tileValue = map.GetTileMapValue(poi.x, poi.y);
		count++;
		if (count > MAX_STEPS) {
			tileValue = EMapValue::WALL;
			break;
		}
	}

	//The poi should be being corrected, but the stepping is irregular which makes it impossible to effectively compensate.
	m_indexBuffer[index] = tileValue;
	m_heightBuffer[index] = Math::l2norm(poi - position);
	//Could calculate projected height here, but it makes the most sense to do this while rendering.
	//Jk. We need to calculate it here cause we need the angle.
	//It would be most efficient to render here, seeing as we have all the info we need,
	//but decoupling is worth the minor overhead plus evenly distributing the work will better utilize parallel processing.

#if DEBUG_DRAW
	App::DrawLine(position, poi);
#endif
}