#pragma once
#include "modelclass.h"
#include "pch.h"

using namespace DirectX;

class Cell {
private:
	int cellState;
	DirectX::SimpleMath::Vector3 cellPosition;
	DirectX::SimpleMath::Vector3 cellCentre;
	DirectX::SimpleMath::Vector3 cellDimensions;

public:
	Cell();
	~Cell();

	void SetPosition(DirectX::SimpleMath::Vector3 position);
	void SetCentre(DirectX::SimpleMath::Vector3 centre);
	DirectX::SimpleMath::Vector3 GetPosition();
	DirectX::SimpleMath::Vector3 GetDimensions();
	DirectX::SimpleMath::Vector3 GetCentre();


	void SetState(int state);
	int GetState();


	int parent_x, parent_z;
	// f = g + h 
	double f, g, h; //used for the calculation of A Star

};