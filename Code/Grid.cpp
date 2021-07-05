#include "pch.h"
#include "Grid.h"
#include <iostream>

#pragma region set up grid
int Grid::getNeighbours(int x, int z)
{
    /*notice in 2d space. only on the x and z plane*/
    int neighbourCount = 0;
    if (cellMatrix[(x - 1)][z].GetState() == 1 && x != 0) // middle left
        neighbourCount++;
    if (cellMatrix[(x - 1)][(z - 1)].GetState() == 1 && x != 0 && z != 0) // top left
        neighbourCount++;
    if (cellMatrix[(x - 1)][(z + 1)].GetState() == 1 && x != 0 && z != gridSize) // bottom left
        neighbourCount++;
    if (cellMatrix[(x)][(z - 1)].GetState() == 1 && z != 0) // middle top
        neighbourCount++;
    if (cellMatrix[(x)][(z + 1)].GetState() == 1 && z != gridSize) // middle bottom
        neighbourCount++;
    if (cellMatrix[(x + 1)][z].GetState() == 1 && x != gridSize) // middle right
        neighbourCount++;
    if (cellMatrix[(x + 1)][(z + 1)].GetState() == 1 && x != gridSize && z != gridSize) // top right
        neighbourCount++;
    if (cellMatrix[(x + 1)][(z - 1)].GetState() == 1 && x != gridSize && z != 0) // bottom right
        neighbourCount++;
    return neighbourCount;
}
#pragma endregion

#pragma region Initialize the grid
void Grid::initializeGrid()
{
    int border = 1;
    m_isPlayerSet = false;
    m_isgoldcoinSet = false;
    m_LevelSolvable = false;
    m_distance = false;
    m_searchResult = -1;
    Clear();
    m_aStar.ResetDistance();
    // i is x, j is z
    while (!m_isPlayerSet && !m_isgoldcoinSet && !m_LevelSolvable)
    {
        for (int i = 0; i <= gridSize - 1; i++)
        {
            for (int j = 0; j <= gridSize - 1; j++)
            {
            
                if (i == 0 || i == gridSize - 1 || j == 0 || j == gridSize - 1) {
                    cellMatrix[i][j].SetState(border);
                    stateMatrix[j][i] = border;
                }
                else {
                    if (!m_isPlayerSet)
                    {
                        int randomValue = (rand() % 100 + 1);
                        if (randomValue > 70 - probabilityToBeAlive)
                        {
                            cellMatrix[i][j].SetState(2); // player
                            m_cellPlayer = cellMatrix[i][j];

                            stateMatrix[j][i] = 2;
                            m_playerIndex = make_pair(j, i);

                            m_isPlayerSet = true;
                            continue;
                        }
                    }
                    if (!m_isgoldcoinSet)
                    {
                        int randomValue = (rand() % 100 + 1);
                        if(randomValue % probabilityToBeAlive  == 0)
                        {
                            cellMatrix[i][j].SetState(3); // goldcoinbox
                            m_cellgoldcoin = cellMatrix[i][j];

                            stateMatrix[j][i] = 3;
                            m_goldcoinIndex = make_pair(j, i);

                            m_isgoldcoinSet = true;
                            continue;
                        }
                    }
                    //else {
                    int randomValue = (rand() % 100 + 1);
                    bool aliveState = randomValue > 100 - probabilityToBeAlive ? 1 : 0;
                    cellMatrix[i][j].SetState(aliveState);
                    stateMatrix[j][i] = aliveState;
                    //}
                }
            }
        }
#pragma region Checking Solvability
                                                    /*using A star to check and see if the maze can be solved*/
        m_searchResult = m_aStar.aStarSearch(stateMatrix, m_playerIndex, m_goldcoinIndex);
        if (*m_aStar.GetDistance() > 10) 
        {
            m_distance = true;
        }
        else {
            m_distance = false;
        }
        if (m_searchResult == 1 && m_distance) 
        {
            m_LevelSolvable = true;
        }
#pragma endregion
    }
}
#pragma endregion

int* Grid::GetDistance()
{
    int distance = *m_aStar.GetDistance();
    int* distancePointer = &distance;
    return distancePointer;
}

void Grid::ResetPlayerInStateMatrix(int r, int c)
{
    for (int z = 0; z < gridSize - 1; z++)
    {
        for (int x = 0; x < gridSize - 1; x++)
        {
            if (cellMatrix[z][x].GetState() == 2) 
            {
                stateMatrix[r][c] = 2;
                m_playerIndex = make_pair(r, c);
            }
        }
    }
}

void Grid::nextGeneration()
{
    gridInitialised = false; // collisions
    int nextGrid[101][101];
    for (int z = 0; z < gridSize - 1; z++)
        for (int x = 0; x < gridSize - 1; x++)
            if (z == 0 || z == gridSize - 1 || x == 0 || x == gridSize - 1) {
                // border, do nothing
            }
            else {
                nextGrid[x][z] = getNeighbours(x, z);
            }
    for (int z = 0; z < gridSize - 1; z++)
        for (int x = 0; x < gridSize - 1; x++)
        {
            if (z == 0 || z == gridSize - 1 || x == 0 || x == gridSize - 1) {
                // border, do nothing
            }
            else {
                if (cellMatrix[x][z].GetState() == 1)
                {
                    if (nextGrid[x][z] > 3)
                    {
                        cellMatrix[x][z].SetState(0);
                        haveChanged.push_back(CellPoint(x, z, 0));
                    }
                    if (nextGrid[x][z] < 2)
                    {
                        cellMatrix[x][z].SetState(0);
                        haveChanged.push_back(CellPoint(x, z, 0));
                    }
                }
                else if (cellMatrix[x][z].GetState() == 3)
                {
                    // do nothing to goldcoinbox
                }
                else
                {
                    if (nextGrid[x][z] == 3) {
                        cellMatrix[x][z].SetState(1);
                        haveChanged.push_back(CellPoint(x, z, 1));
                    }
                }
            }
        }
}

//Clear grid and stateMatrix
void Grid::Clear() 
{
    for (int z = 0; z < gridSize; z++) 
    {
        for (int x = 0; x < gridSize; x++)
        {
            stateMatrix[z][x] = -1;
            cellMatrix[x][z].SetState(-1);
        }
    }
}

//Return grid size
int Grid::Size() {
    return gridSize;
}


//GridInitialisation
bool Grid::GetInitialised() {
    return gridInitialised;
}

void Grid::SetInitialised(bool state) {
    gridInitialised = state;
}

Cell Grid::GetgoldcoinCell() {
    return m_cellgoldcoin;
}

Cell Grid::GetPlayerCell() {
    return m_cellPlayer;
}