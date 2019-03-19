#include "stdafx.h"
#include "RayCaster.h"
#include "App/app.h"
#include "Viewer.h"
#include "Line.h"

//Increase distance till point of intersection by 5% so that the ray is guaranteed to exceed its current cell.
#define DISTANCE_MULTIPLIER 1.05f
#define MAX_STEPS 32
#define DRAW_2D true

CRayCaster::CRayCaster(float thickness) :
	m_count(APP_VIRTUAL_WIDTH / (uint32_t)thickness), m_thickness(thickness), m_step((float)m_count * thickness), m_rayOriginY(APP_VIRTUAL_HEIGHT * 0.5f)
{	//Make sure thickness is between 1 and 32.
	assert(thickness >= 1.0f && thickness <= 31.0f);
	m_indexBuffer.resize(m_count);
	m_heightBuffer.resize(m_count);
	m_positionBuffer.resize(m_count);
}

CRayCaster::~CRayCaster()
{
}

void CRayCaster::Update(const CSimpleTileMap& map, const CViewer& viewer)
{
	const float angleStep = viewer.m_fov / (float)m_count;
	const float raysStart = viewer.m_angle - viewer.m_fov * 0.5f;
	for (uint32_t i = 0; i < m_count; i++) {
		const float rayAngle = raysStart + angleStep * (float)i;
		march(map, viewer.m_position, rayAngle, viewer.m_angle, i);
	}
}

void CRayCaster::Render(const CSimpleTileMap& map, const CViewer& viewer)
{
#if DRAW_2D
	glViewport(APP_VIRTUAL_WIDTH * 0.5f, 0.0f, APP_VIRTUAL_WIDTH * 0.5f, APP_VIRTUAL_HEIGHT);
	map.Render();
	for (uint32_t i = 0; i < m_count; i++)
		App::DrawLine(viewer.m_position.x, viewer.m_position.y, m_positionBuffer[i].x, m_positionBuffer[i].y);
	glViewport(0.0f, 0.0f, APP_VIRTUAL_WIDTH * 0.5f, APP_VIRTUAL_HEIGHT);
#endif

	//Length of adjacent side of right triangle formed by the screen and the field of view.
	const float projectionDistance = (APP_VIRTUAL_WIDTH * 0.5f) / tan(viewer.m_fov * 0.5f);

	float x = 0.0f;
	glLineWidth(m_thickness);
	for (uint32_t i = 0; i < m_count; i++) {
		//Read based on index found in Update() rather than read copied data.
		const CTile& tile = CTile::tiles[m_indexBuffer[i]];
		//Projected height = actual height / distance to poi * distance to projection plane.
		const float projectedHeight = (tile.height / m_heightBuffer[i]) * projectionDistance;
		//Fake some 3D per verticle slice!
		App::DrawLine(x, m_rayOriginY - projectedHeight, x, m_rayOriginY + projectedHeight, tile.r, tile.g, tile.b);
		x += m_thickness;
	}
}

inline void CRayCaster::march(const CSimpleTileMap & map, const CPoint& position, const float rayAngle, const float viewerAngle, uint32_t index)
{
	const float tileWidth = map.getTileWidth();
	const float tileHeight = map.getTileHeight();

	const CPoint direction = Math::direction(rayAngle);
	const float unitRise = direction.y / direction.x;
	const float unitRun = direction.x / direction.y;

	//Continue searching until we find anything but a floor (air) or exceed the allowed amount of steps.
	EMapValue tileValue = EMapValue::FLOOR;
	uint32_t stepCount = 0;
	CPoint poi = position;
	while (tileValue == EMapValue::FLOOR) {
		const float xRemainder = fmodf(poi.x, tileWidth);
		const float xEdge = direction.x >= 0.0f ? poi.x + tileWidth - xRemainder : poi.x - xRemainder;
		float xDistance = xEdge - poi.x;
		const float xRate = xDistance / direction.x;

		const float yRemainder = fmodf(poi.y, tileHeight);
		const float yEdge = direction.y >= 0.0f ? poi.y + tileHeight - yRemainder : poi.y - yRemainder;
		float yDistance = yEdge - poi.y;
		const float yRate = yDistance / direction.y;

		//Increase the poi by a small percentage in order to ensure its in a new cell.
		//Move x proportional to how we moved y or vice versa based on nearest edge.
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

		//Look up cell. Break out of loop and render border if we get some weird numbers.
		tileValue = map.GetTileMapValue(poi.x, poi.y);
		stepCount++;
		if (stepCount > MAX_STEPS) {
			tileValue = EMapValue::WALL;
			break;
		}
	}

	//Store the index for lookup later (during rendering) rather than copying values.
	m_indexBuffer[index] = tileValue;
	//Calculate the distance from player to point of intersection, than correct fisheye by reducing rays that aren't straight ahead.
	m_heightBuffer[index] = Math::l2norm(poi - position) * cosf(Math::radians(rayAngle - viewerAngle));

#if DRAW_2D
	//Store debug information.
	m_positionBuffer[index] = poi;
#endif
}