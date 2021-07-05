#pragma once
#include "pch.h"
#include "Cell.h"
#include "CellPoint.h"
#include "AStar.h"

														/****************************************************************************
														*																			*
														*																			*
														*					class used to when creating the grid					*
														*							creates the size of the grid					*
														*																			*
														*																			*
														*																			*
														*****************************************************************************/


class Grid {
public:
	Cell cellMatrix[27][27];
	

	int getNeighbours(int x, int z);
	void initializeGrid();
	void nextGeneration();
	bool GetInitialised();
	void SetInitialised(bool state);
	void Clear();
	int Size();

	Cell GetgoldcoinCell();
	Cell GetPlayerCell();

	//AStar
	int* GetDistance();
	void ResetPlayerInStateMatrix(int,int);

	std::vector<CellPoint> haveChanged;

private:
	const int gridSize = 27;// size of the grid
	const int probabilityToBeAlive = 30; // max number of alive cells at 1 time
	bool gridInitialised = false;
	bool m_isPlayerSet, m_isgoldcoinSet;
	
	Cell m_cellgoldcoin;// adds the gold onto the grid
	Cell m_cellPlayer;// adds the player onto the grid

	AStar m_aStar;
	std::pair<int,int> m_goldcoinIndex;
	std::pair<int, int> m_playerIndex;
	int stateMatrix[27][27];
	bool m_LevelSolvable;
	bool m_distance;
	int m_searchResult;
};