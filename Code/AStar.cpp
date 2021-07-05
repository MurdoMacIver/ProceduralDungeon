#include "pch.h"
#include "AStar.h"

/* Astar c++ implementation taken from 
https://www.geeksforgeeks.org/a-search-algorithm/ */

                                                /****************************************************************************************************
                                                *                                                                                                   *
                                                *                                                                                                   *
                                                *           A Star class used for best/optimal path based on the cost of the next cell              *
                                                *                                                                                                   *
                                                *                                                                                                   *
                                                *****************************************************************************************************/
bool isValid(int row, int col)
{
    // Returns true if row number and column number is in range 
    return (row >= 0) && (row < ROW) &&
        (col >= 0) && (col < COL);
}

bool isUnBlocked(int grid[][COL], int row, int col)
{
    // Returns true if the cell is not blocked else false 
    if (grid[row][col] == 0 || grid[row][col] == 2 || grid[row][col] == 3)
        return (true);
    else
        return (false);
}


bool isDestination(int row, int col, Pair dest)
{
    if (row == dest.first && col == dest.second)
        return (true);
    else
        return (false);
}

//used to caluclate the 'h' heurstics 
double calculateHValue(int row, int col, Pair dest)
{
    return ((double)sqrt((row - dest.first) * (row - dest.first)
        + (col - dest.second) * (col - dest.second)));
}


void AStar::tracePath(Cell cellDetails[][COL], Pair dest)
{
    int row = dest.first;
    int col = dest.second;

    stack<Pair> Path;

    while (!(cellDetails[row][col].parent_x == row
        && cellDetails[row][col].parent_z == col))
    {
        Path.push(make_pair(row, col));
        int temp_row = cellDetails[row][col].parent_x;
        int temp_col = cellDetails[row][col].parent_z;
        row = temp_row;
        col = temp_col;
    }

    Path.push(make_pair(row, col));
    while (!Path.empty())
    {
        pair<int, int> p = Path.top();
        Path.pop();
        distanceToPath++;

    }
    return;
}

int* AStar::GetDistance()
{
    int distance = distanceToPath;
    int* distancePointer = &distance;
    return distancePointer;
}

void AStar::ResetDistance() {
    distanceToPath = 0;
}

#pragma region AStar Search

                                         /***************************************************************************************************************************
                                         *                                                                                                                          *
                                         *                                                                                                                          *
                                         *                                                                                                                          *
                                         *      finds the shortest path between a given source cell to a destination cell according to A* Search Algorithm          *
                                         *                                                                                                                          *
                                         *                                                                                                                          *
                                         *                                                                                                                          *
                                         ****************************************************************************************************************************/

int AStar::aStarSearch(int grid[][COL], Pair src, Pair dest)
{
    // Check to see if the source cell is out of range
    if (isValid(src.first, src.second) == false)
    {
        return -1;
    }

    // check to see if the destination cell is out of range
    if (isValid(dest.first, dest.second) == false)
    {
        return -1;
    }

    // check to see if the source or destination is blocked
    if (isUnBlocked(grid, src.first, src.second) == false ||
        isUnBlocked(grid, dest.first, dest.second) == false)
    {
        return -1;
    }

    // Check and see if source and destination are the same
    if (isDestination(src.first, src.second, dest) == true)
    {
        return -1;
    }

    /********************************************************************************************************************
    *                                                                                                                   *
    *                               creation of a closed list and set it to false, meaning no cell is added yet         *
    *                                                   the list is a boolean 2D array                                  *
    *                                                                                                                   *
    *********************************************************************************************************************/

    bool closedList[ROW][COL];
    memset(closedList, false, sizeof(closedList));

    Cell cellDetails[ROW][COL];

    int i, j;

    for (i = 0; i < ROW; i++)
    {
        for (j = 0; j < COL; j++)
        {
            cellDetails[i][j].f = FLT_MAX;
            cellDetails[i][j].g = FLT_MAX;
            cellDetails[i][j].h = FLT_MAX;
            cellDetails[i][j].parent_x = -1;
            cellDetails[i][j].parent_z = -1;
        }
    }


    i = src.first, j = src.second;
    cellDetails[i][j].f = 0.0;
    cellDetails[i][j].g = 0.0;
    cellDetails[i][j].h = 0.0;
    cellDetails[i][j].parent_x = i;
    cellDetails[i][j].parent_z = j;

                            /********************************************************************************************************
                            *                               Create an open list                                                     *
                            *                                              f, i, j                                                  *
                            *                                         where f = g + h,                                              *
                            *                               and i, j are the row and column index of that cell                      *
                            **********************************************************************************************************/
    set<pPair> openList;

    //starting cell is put onto the open list and its f is set to zero
    openList.insert(make_pair(0.0, make_pair(i, j)));

    bool foundDest = false;
                            /****************************************************
                            *                                                   *
                            *       while the open list is not empty            *
                            *                                                   *
                            *****************************************************/
#pragma region while loop
    while (!openList.empty())
    {
        pPair p = *openList.begin();
        //find the node with the least f on 
        //the open list, call it "q" 
        // pop q off the open list
        openList.erase(openList.begin());

        i = p.second.first;
        j = p.second.second;
        closedList[i][j] = true;

#pragma region Algorithm explanation
        /************************************************************************************************************************
        *                                                generate q's 8 successors and set their parents to q                   *
        *                                                for each successor;                                                    *
        *                                                                                                                       *
        *                                                i) if successor is the goal, stop search                               *
        *                                                  successor.g = q.g + distance between                                 *
        *                                                  successor and q                                                      *
        *                                                  successor.h = distance from goal to                                  *
        *                                                  successor (This can be done using many                               *
        *                                                  ways, we will discuss three heuristics-                              *
        *                                                  Manhattan, Diagonal and Euclidean                                    *
        *                                                  Heuristics)                                                          *
        *                                                                                                                       *
        *                                                  successor.f = successor.g + successor.h                              *
        *                                                                                                                       *
        *                                                ii) if a node with the same position as                                *
        *                                                    successor is in the OPEN list which has a                          *
        *                                                   lower f than successor, skip this successor                         *
        *                                                                                                                       *
        *                                                iii) if a node with the same position as                               *
        *                                                    successor  is in the CLOSED list which has                         *
        *                                                    a lower f than successor, skip this successor                      *
        *                                                    otherwise, add  the node to the open list                          *
        *                                                    end (for loop)                                                     *
        *************************************************************************************************************************/
#pragma endregion

         // To store the 'g', 'h' and 'f' of the 8 successors 
        double gNew, hNew, fNew;

#pragma region Successors

#pragma region 1st Successor

        // Only process this cell if this is a valid one 
        if (isValid(i - 1, j) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i - 1, j, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i - 1][j].parent_x = i;
                cellDetails[i - 1][j].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }
            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i - 1][j] == false &&
                isUnBlocked(grid, i - 1, j) == true)
            {
                gNew = cellDetails[i][j].g + 1.0;
                hNew = calculateHValue(i - 1, j, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i - 1][j].f == FLT_MAX ||
                    cellDetails[i - 1][j].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i - 1, j)));

                    // Update the details of this cell 
                    cellDetails[i - 1][j].f = fNew;
                    cellDetails[i - 1][j].g = gNew;
                    cellDetails[i - 1][j].h = hNew;
                    cellDetails[i - 1][j].parent_x = i;
                    cellDetails[i - 1][j].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 2nd Successor 

        // Only process this cell if this is a valid one 
        if (isValid(i + 1, j) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i + 1, j, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i + 1][j].parent_x = i;
                cellDetails[i + 1][j].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }
            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i + 1][j] == false &&
                isUnBlocked(grid, i + 1, j) == true)
            {
                gNew = cellDetails[i][j].g + 1.0;
                hNew = calculateHValue(i + 1, j, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i + 1][j].f == FLT_MAX ||
                    cellDetails[i + 1][j].f > fNew)
                {
                    openList.insert(make_pair(fNew, make_pair(i + 1, j)));
                    // Update the details of this cell 
                    cellDetails[i + 1][j].f = fNew;
                    cellDetails[i + 1][j].g = gNew;
                    cellDetails[i + 1][j].h = hNew;
                    cellDetails[i + 1][j].parent_x = i;
                    cellDetails[i + 1][j].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 3rd Successor

        // Only process this cell if this is a valid one 
        if (isValid(i, j + 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i, j + 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i][j + 1].parent_x = i;
                cellDetails[i][j + 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i][j + 1] == false &&
                isUnBlocked(grid, i, j + 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.0;
                hNew = calculateHValue(i, j + 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i][j + 1].f == FLT_MAX ||
                    cellDetails[i][j + 1].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i, j + 1)));

                    // Update the details of this cell 
                    cellDetails[i][j + 1].f = fNew;
                    cellDetails[i][j + 1].g = gNew;
                    cellDetails[i][j + 1].h = hNew;
                    cellDetails[i][j + 1].parent_x = i;
                    cellDetails[i][j + 1].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 4th Successor

        // Only process this cell if this is a valid one 
        if (isValid(i, j - 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i, j - 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i][j - 1].parent_x = i;
                cellDetails[i][j - 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i][j - 1] == false &&
                isUnBlocked(grid, i, j - 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.0;
                hNew = calculateHValue(i, j - 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i][j - 1].f == FLT_MAX ||
                    cellDetails[i][j - 1].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i, j - 1)));

                    // Update the details of this cell 
                    cellDetails[i][j - 1].f = fNew;
                    cellDetails[i][j - 1].g = gNew;
                    cellDetails[i][j - 1].h = hNew;
                    cellDetails[i][j - 1].parent_x = i;
                    cellDetails[i][j - 1].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 5th Successor 

        // Only process this cell if this is a valid one 
        if (isValid(i - 1, j + 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i - 1, j + 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i - 1][j + 1].parent_x = i;
                cellDetails[i - 1][j + 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i - 1][j + 1] == false &&
                isUnBlocked(grid, i - 1, j + 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.414;
                hNew = calculateHValue(i - 1, j + 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i - 1][j + 1].f == FLT_MAX ||
                    cellDetails[i - 1][j + 1].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i - 1, j + 1)));

                    // Update the details of this cell 
                    cellDetails[i - 1][j + 1].f = fNew;
                    cellDetails[i - 1][j + 1].g = gNew;
                    cellDetails[i - 1][j + 1].h = hNew;
                    cellDetails[i - 1][j + 1].parent_x = i;
                    cellDetails[i - 1][j + 1].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 6th Successor

        // Only process this cell if this is a valid one 
        if (isValid(i - 1, j - 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i - 1, j - 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i - 1][j - 1].parent_x = i;
                cellDetails[i - 1][j - 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i - 1][j - 1] == false &&
                isUnBlocked(grid, i - 1, j - 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.414;
                hNew = calculateHValue(i - 1, j - 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i - 1][j - 1].f == FLT_MAX ||
                    cellDetails[i - 1][j - 1].f > fNew)
                {
                    openList.insert(make_pair(fNew, make_pair(i - 1, j - 1)));
                    // Update the details of this cell 
                    cellDetails[i - 1][j - 1].f = fNew;
                    cellDetails[i - 1][j - 1].g = gNew;
                    cellDetails[i - 1][j - 1].h = hNew;
                    cellDetails[i - 1][j - 1].parent_x = i;
                    cellDetails[i - 1][j - 1].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 7th Successor 

        // Only process this cell if this is a valid one 
        if (isValid(i + 1, j + 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i + 1, j + 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i + 1][j + 1].parent_x = i;
                cellDetails[i + 1][j + 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i + 1][j + 1] == false &&
                isUnBlocked(grid, i + 1, j + 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.414;
                hNew = calculateHValue(i + 1, j + 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i + 1][j + 1].f == FLT_MAX ||
                    cellDetails[i + 1][j + 1].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i + 1, j + 1)));

                    // Update the details of this cell 
                    cellDetails[i + 1][j + 1].f = fNew;
                    cellDetails[i + 1][j + 1].g = gNew;
                    cellDetails[i + 1][j + 1].h = hNew;
                    cellDetails[i + 1][j + 1].parent_x = i;
                    cellDetails[i + 1][j + 1].parent_z = j;
                }
            }
        }
#pragma endregion

#pragma region 8th Successor

        // Only process this cell if this is a valid one 
        if (isValid(i + 1, j - 1) == true)
        {
            // If the destination cell is the same as the 
            // current successor 
            if (isDestination(i + 1, j - 1, dest) == true)
            {
                // Set the Parent of the destination cell 
                cellDetails[i + 1][j - 1].parent_x = i;
                cellDetails[i + 1][j - 1].parent_z = j;
                //printf("The destination cell is found1\n");
                tracePath(cellDetails, dest);
                foundDest = true;
                return 1;
            }

            // If the successor is already on the closed 
            // list or if it is blocked, then ignore it. 
            // Else do the following 
            else if (closedList[i + 1][j - 1] == false &&
                isUnBlocked(grid, i + 1, j - 1) == true)
            {
                gNew = cellDetails[i][j].g + 1.414;
                hNew = calculateHValue(i + 1, j - 1, dest);
                fNew = gNew + hNew;

                // If it isn’t on the open list, add it to 
                // the open list. Make the current square 
                // the parent of this square. Record the 
                // f, g, and h costs of the square cell 
                //                OR 
                // If it is on the open list already, check 
                // to see if this path to that square is better, 
                // using 'f' cost as the measure. 
                if (cellDetails[i + 1][j - 1].f == FLT_MAX ||
                    cellDetails[i + 1][j - 1].f > fNew)
                {
                    openList.insert(make_pair(fNew,
                        make_pair(i + 1, j - 1)));

                    // Update the details of this cell 
                    cellDetails[i + 1][j - 1].f = fNew;
                    cellDetails[i + 1][j - 1].g = gNew;
                    cellDetails[i + 1][j - 1].h = hNew;
                    cellDetails[i + 1][j - 1].parent_x = i;
                    cellDetails[i + 1][j - 1].parent_z = j;
                }
            }
        }
#pragma endregion
#pragma endregion
    }
#pragma endregion

    // When the destination cell is not found and the open 
    // list is empty, then we conclude that we failed to 
    // reach the destiantion cell. This may happen when the 
    // there is no way to destination cell (due to blockages) 
    if (foundDest == false)
    {
        return -1;
    }
        
    return 0;
    
}
#pragma endregion