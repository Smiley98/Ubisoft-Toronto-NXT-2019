#pragma once
#include "Scene.h"
#include "SimpleTileMap.h"
#include "RayCaster.h"
#include "Player.h"

class CMainScene :
	public CScene
{
public:
	CMainScene();
	~CMainScene();

	void Update(float deltaTime) override;
	void Render() override;

private:
	void OnEnter() override;

	CSimpleTileMap m_map{ 16 };
	CRayCaster m_rayCaster{ 4.0f };
	CPlayer m_player;
};