#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "structs.c"

#define SCREEN_WITDH 1280
#define SCREEN_HEIGHT 720

#define BOARD_SIZE 8
#define MAX_QUEUE_SIZE (((BOARD_SIZE * BOARD_SIZE) / 2))

typedef enum
{
    north = 0,
    east,
    south,
    west
} dirs;

typedef enum
{
    game_state_start_screen = 0,
    game_state_start_game,
    game_state_enemy_turn,
    game_state_player_turn,
    game_state_action_turn,
    game_state_game_over,
    game_state_victory
} game_state;

typedef enum
{
    cell_empty = 0,
    cell_flat,
    cell_forest,
    cell_water,
    cell_mountain,
    cell_hole,
    cell_type_total
} cell_type;

int cell_type_walkable[3] = {cell_flat, cell_forest, cell_water};
int cell_type_attackable[2] = {cell_flat, cell_forest};
int cell_type_all[cell_type_total - 1] = {cell_flat, cell_forest, cell_water, cell_mountain, cell_hole};

typedef enum
{
    button_endTurn = 0,
    button_attack,
    button_types_total
} button_type;

typedef enum
{
    unit_melee = 0,
    unit_ranged,
    unit_artillery
} unit_type;

typedef enum
{
    state_unselected = 0,
    state_move,
    state_attack
} player_state;

typedef enum
{
    attack_single = 0,
    attack_line,
} attack_type;

typedef enum
{
    entity_none = 0,
    entity_player,
    entity_enemy,
    entity_remains
} entity_type;

game_state g_gameState = game_state_start_screen;
int cellSize;
int xMargin;
int yMargin;
int totBoardSize;
Vector2 boardOrigin;
cell_t board[BOARD_SIZE][BOARD_SIZE];

button_t buttons[2];

DynArrPlayer g_players;
DynArrEnemy g_enemies;
DynArrRemains g_remains;

player_t *g_selected;

Texture2D tex_disabled;
Texture2D tex_cracked;

int turn = 1;

int showRules = 0;

long oldseed;

int trigg;
Vector2Int triggTarget;

int GetRand(int min, int max)
{
    time_t seed;
    seed = time(NULL) + oldseed;
    oldseed = seed;
    srand(seed);
    return (rand() % (max - min + 1)) + min;
}

float GetScale(float ownSize, float targetSize)
{
    return targetSize / ownSize;
}

bool ComparePos(Vector2Int a, Vector2Int b)
{
    return (a.x == b.x && a.y == b.y);
}

entity_type IsOccupied(Vector2Int pos)
{
    for (int i = 0; i < g_players.occupied; i++)
    {
        if (ComparePos(g_players.data[i].pos, pos))
        {
            return entity_player;
        }
    }
    for (int i = 0; i < g_enemies.occupied; i++)
    {
        if (ComparePos(g_enemies.data[i].pos, pos))
        {
            return entity_enemy;
        }
    }
    for (int i = 0; i < g_remains.occupied; i++)
    {
        if (ComparePos(g_remains.data[i].pos, pos))
        {
            return entity_remains;
        }
    }

    return entity_none;
}

bool IsSelected(player_t *p)
{
    if (g_selected != NULL)
    {
        if (ComparePos(g_selected->pos, p->pos))
        {
            return true;
        }
    }

    return false;
}

void InitCell(cell_t *c, cell_type type, Vector2 pos, int size)
{
    if (type != cell_empty)
    {
        c->cellType = type;
        c->rect = (Rectangle){pos.x, pos.y, size, size};
    }
}

cell_t GetCell(Vector2Int pos)
{
    return board[pos.x][pos.y];
}

void DamageCell(Vector2Int pos)
{
    cell_t *c = &board[pos.x][pos.y];
    int temp = c->durability;
    c->durability--;

    if (c->durability == 0 && temp > 0)
    {
        c->cellType = cell_flat;
    }
}

void CreateMountains(int quantity)
{
    int currentAmount = 0;
    Vector2Int lastPlaced = {0};
    int maxChain = 8;
    int currentChain = 0;
    int growChainChance = 5;
    for (int i = currentAmount; i < quantity; i++)
    {
        int xtemp = 0;
        int ytemp = 0;
        int dice = GetRand(1, 20);

        if (currentAmount == 0) // if first, then create chain
        {
            dice = growChainChance - 1;
            currentChain = 1;
        }

        if (dice >= growChainChance && currentChain < maxChain) // grow chain
        {
            if (dice >= 18)
            {
                maxChain = GetRand(5, 9);
            }

            int checkedDirs[4] = {0};
            int validPos = 0;
            while (validPos != 1)
            {
                int dir = GetRand(north, west);

                switch (dir)
                {
                case north:
                {
                    if (board[lastPlaced.x][lastPlaced.y - 1].cellType == cell_flat)
                    {
                        xtemp = lastPlaced.x;
                        ytemp = lastPlaced.y - 1;
                        validPos = 1;
                    }
                    else
                        checkedDirs[north] = 1;
                }
                break;
                case east:
                {
                    if (board[lastPlaced.x + 1][lastPlaced.y].cellType == cell_flat)
                    {
                        xtemp = lastPlaced.x + 1;
                        ytemp = lastPlaced.y;
                        validPos = 1;
                    }
                    else
                        checkedDirs[east] = 1;
                }
                break;
                case south:
                {
                    if (board[lastPlaced.x][lastPlaced.y + 1].cellType == cell_flat)
                    {
                        xtemp = lastPlaced.x;
                        ytemp = lastPlaced.y + 1;
                        validPos = 1;
                    }
                    else
                        checkedDirs[south] = 1;
                }
                break;
                case west:
                {
                    if (board[lastPlaced.x - 1][lastPlaced.y].cellType == cell_flat)
                    {
                        xtemp = lastPlaced.x - 1;
                        ytemp = lastPlaced.y;
                        validPos = 1;
                    }
                    else
                        checkedDirs[west] = 1;
                }
                break;
                }

                if (checkedDirs[north] == 1 && checkedDirs[east] == 1 && checkedDirs[south] == 1 && checkedDirs[west] == 1)
                {
                    i--;
                    break;
                }
            }
            currentChain++;
        }
        else // start new chain
        {
            int validPos = 0;
            while (validPos != 1)
            {
                xtemp = GetRand(0, BOARD_SIZE - 1);
                ytemp = GetRand(0, BOARD_SIZE - 1);

                if (board[xtemp][ytemp].cellType == cell_flat)
                {
                    validPos = 1;
                    currentChain = 1;
                }
            }
        }

        currentAmount++;
        board[xtemp][ytemp].cellType = cell_mountain;
        board[xtemp][ytemp].durability = 2;
        lastPlaced = (Vector2Int){xtemp, ytemp};
    }
    puts("Mountains done.");
}

void CreateWater(int quantity)
{
    int currentAmount = 0;
    Vector2Int chainOrigin = {0};
    int maxChain = 4;
    int currentChain = 0;
    int growChainChance = 3;
    int oceanChance = 0;
    for (int i = currentAmount; i < quantity; i++)
    {
        int xtemp = 0;
        int ytemp = 0;
        int dice = GetRand(1, 20);

        if (currentAmount == 0) // if first
        {
            dice = growChainChance - 1;
            currentChain = 1;
        }

        if (dice >= growChainChance && currentChain < maxChain)
        {
            if (oceanChance >= dice)
            {
                maxChain = GetRand(8, 15);
            }
            else
            {
                maxChain = GetRand(2, 4);
            }

            int checkedDirs[4] = {0};
            int validPos = 0;
            while (validPos != 1)
            {
                int dir = GetRand(north, west);

                switch (dir)
                {
                case north:
                {
                    if (board[chainOrigin.x][chainOrigin.y - 1].cellType == cell_flat)
                    {
                        xtemp = chainOrigin.x;
                        ytemp = chainOrigin.y - 1;
                        validPos = 1;
                    }
                    else
                        checkedDirs[north] = 1;
                }
                break;
                case east:
                {
                    if (board[chainOrigin.x + 1][chainOrigin.y].cellType == cell_flat)
                    {
                        xtemp = chainOrigin.x + 1;
                        ytemp = chainOrigin.y;
                        validPos = 1;
                    }
                    else
                        checkedDirs[east] = 1;
                }
                break;
                case south:
                {
                    if (board[chainOrigin.x][chainOrigin.y + 1].cellType == cell_flat)
                    {
                        xtemp = chainOrigin.x;
                        ytemp = chainOrigin.y + 1;
                        validPos = 1;
                    }
                    else
                        checkedDirs[south] = 1;
                }
                break;
                case west:
                {
                    if (board[chainOrigin.x - 1][chainOrigin.y].cellType == cell_flat)
                    {
                        xtemp = chainOrigin.x - 1;
                        ytemp = chainOrigin.y;
                        validPos = 1;
                    }
                    else
                        checkedDirs[west] = 1;
                }
                break;
                }

                if (checkedDirs[north] == 1 && checkedDirs[east] == 1 && checkedDirs[south] == 1 && checkedDirs[west] == 1)
                {
                    i--;
                    break;
                }
            }
            currentChain++;
        }
        else
        {
            int validPos = 0;
            while (validPos != 1)
            {
                xtemp = GetRand(0, BOARD_SIZE - 1);
                ytemp = GetRand(0, BOARD_SIZE - 1);

                if (board[xtemp][ytemp].cellType == cell_flat)
                {
                    validPos = 1;
                    currentChain = 1;
                }
            }

            oceanChance = GetRand(1, 20);
            chainOrigin = (Vector2Int){xtemp, ytemp};
        }

        int dice2 = GetRand(1, 20);
        if (dice2 >= 12)
        {
            chainOrigin = (Vector2Int){xtemp, ytemp};
        }

        currentAmount++;
        board[xtemp][ytemp].cellType = cell_water;
    }
    puts("Water done.");
}

void InitBoard(void)
{
    xMargin = 0;
    int cellMargin = 0;
    int totCellMargin = 0;
    cellSize = 0;
    int totCellSize = 0;
    yMargin = 0;

    if (SCREEN_WITDH > SCREEN_HEIGHT)
    {
        yMargin = SCREEN_HEIGHT / 15;
        cellMargin = SCREEN_HEIGHT / 100;
        totCellMargin = cellMargin * (BOARD_SIZE + 1);
        cellSize = ((SCREEN_HEIGHT - (yMargin * 2)) - totCellMargin) / BOARD_SIZE;
        totCellSize = cellSize * BOARD_SIZE;
        totBoardSize = totCellMargin + totCellSize;
        xMargin = (SCREEN_WITDH - totBoardSize) / 2;
        boardOrigin = (Vector2){xMargin, yMargin};
    }
    else if (SCREEN_WITDH <= SCREEN_HEIGHT)
    {
        xMargin = SCREEN_WITDH / 5;
        cellMargin = SCREEN_WITDH / 100;
        totCellMargin = cellMargin * (BOARD_SIZE + 1);
        cellSize = ((SCREEN_WITDH - (xMargin * 2)) - totCellMargin) / BOARD_SIZE;
        totCellSize = cellSize * BOARD_SIZE;
        totBoardSize = totCellMargin + totCellSize;
        yMargin = (SCREEN_HEIGHT - totBoardSize) / 2;
        boardOrigin = (Vector2){xMargin, yMargin};
    }

    // init cells
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            Vector2 pos = (Vector2){boardOrigin.x + (x * (cellSize + cellMargin)) + cellMargin, boardOrigin.y + (y * (cellSize + cellMargin)) + cellMargin};
            InitCell(&board[x][y], cell_flat, pos, cellSize);
        }
    }

    // add cell variety
    int cells = BOARD_SIZE * BOARD_SIZE;
    int amountMountains = 0;
    int amountWater = 0;
    while (amountMountains < 1 && amountWater < 1)
    {
        float percentMountains = GetRand(5, 25) / 100.00f;
        float percentWater = GetRand(5, 35 - (percentMountains * 100)) / 100.00f;
        amountMountains = cells * percentMountains;
        amountWater = cells * percentWater;
        printf("cells: %d\n", cells);
        printf("percentMountains: %f\n", percentMountains);
        printf("amountMountains: %d\n", amountMountains);
        printf("percentWater: %f\n", percentWater);
        printf("amountWater: %d\n", amountWater);
    }

    // mountain chains
    CreateMountains(amountMountains);

    // oceanChance/lakes
    CreateWater(amountWater);
}

void DrawBoard(void)
{
    Color color = WHITE;
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            int cracked = 0;
            switch (board[x][y].cellType)
            {
            case cell_flat:
            {
                color = (Color){211, 176, 131, 180};
            }
            break;

            case cell_hole:
            {
                color = (Color){30, 30, 30, 255};
            }
            break;

            case cell_forest:
            {
                color = (Color){0, 117, 44, 210};
            }
            break;

            case cell_mountain:
            {
                color = (Color){76, 63, 47, 200};

                if (board[x][y].durability != 2)
                {
                    cracked = 1;
                }
            }
            break;

            case cell_water:
            {
                color = (Color){0, 121, 241, 170};
            }
            break;

            default:
            {
                color = YELLOW;
            }
            break;
            }

            DrawRectangleRec(board[x][y].rect, color);

            if (cracked != 0)
            {
                DrawTextureEx(tex_cracked, (Vector2){board[x][y].rect.x, board[x][y].rect.y}, 0, GetScale(tex_cracked.width, cellSize), WHITE);
            }
        }
    }

    // borders
    DrawLineV(boardOrigin, (Vector2){boardOrigin.x + totBoardSize, boardOrigin.y}, WHITE);
    DrawLineV((Vector2){boardOrigin.x, boardOrigin.y + totBoardSize}, (Vector2){boardOrigin.x + totBoardSize, boardOrigin.y + totBoardSize}, WHITE);
    DrawLineV(boardOrigin, (Vector2){boardOrigin.x, boardOrigin.y + totBoardSize}, WHITE);
    DrawLineV((Vector2){boardOrigin.x + totBoardSize, boardOrigin.y}, (Vector2){boardOrigin.x + totBoardSize, boardOrigin.y + totBoardSize}, WHITE);
}

void InitButtons(Vector2 origin)
{
    for (int i = 0; i < button_types_total; i++)
    {
        button_t *b = &buttons[i];
        b->rect = (Rectangle){origin.x, origin.y + (i * yMargin) + (i * 50), xMargin / 2, yMargin};

        switch (i)
        {
        case button_endTurn:
        {
            b->text = "End Turn";
        }
        break;
        case button_attack:
        {
            b->text = "Attack";
        }
        break;
        }

        b->fontSize = b->rect.width / TextLength(b->text);
    }
}

void DrawButtons(void)
{
    for (int i = 0; i < 2; i++)
    {
        button_t *b = &buttons[i];
        DrawRectangleRec(b->rect, DARKGRAY);
        Vector2 temp = MeasureTextEx(GetFontDefault(), b->text, b->fontSize, 1);
        DrawText(b->text, b->rect.x + (b->rect.width / 2 - temp.x / 2), b->rect.y + (b->rect.height - temp.y) / 2, b->fontSize, WHITE);
    }
}

bool HoverCell(Vector2Int pos)
{
    return CheckCollisionPointRec(GetMousePosition(), GetCell(pos).rect);
}

void EndTurn(void)
{
    switch (g_gameState)
    {
    case game_state_enemy_turn:
    {
        g_gameState = game_state_player_turn;
    }
    break;
    case game_state_player_turn:
    {
        g_gameState = game_state_action_turn;
    }
    break;
    case game_state_action_turn:
    {
        g_gameState = game_state_enemy_turn;
    }
    break;

    default:
    {
    }
    break;
    }

    g_selected = NULL;
    for (int i = 0; i < g_players.occupied; i++)
    {
        g_players.data[i].state = state_unselected;
    }
}

void Bfs(DynArrDist2 *d, Vector2Int start, int range, int *allowedTypes, int lengthTypes, entity_type avoidEntity)
{
    DynArrDist2Reset(d);

    QueueDist2 frontier = {0};
    InitQueue(&frontier, MAX_QUEUE_SIZE);
    DynArrDist2 reached = {0};
    InitDynArrDist2(&reached);

    QueueInsert(&frontier, start, 0);
    DynArrDist2Add(&reached, start, 0);
    Distance2 current = {0};

    while (!QueueIsEmpty(&frontier))
    {
        current = QueueRemove(&frontier);
        for (int i = north; i <= west; i++) // go through all neighbours of current
        {
            Vector2Int next = {0};
            switch (i)
            {
            case north:
            {
                next = (Vector2Int){current.pos.x, current.pos.y - 1};
            }
            break;
            case east:
            {
                next = (Vector2Int){current.pos.x + 1, current.pos.y};
            }
            break;
            case south:
            {
                next = (Vector2Int){current.pos.x, current.pos.y + 1};
            }
            break;
            case west:
            {
                next = (Vector2Int){current.pos.x - 1, current.pos.y};
            }
            break;
            }

            // in range of board and allowed cell
            int validCellType = 0;
            int occupiedCell = 0;
            if ((next.x < BOARD_SIZE && next.y < BOARD_SIZE) && (next.x >= 0 && next.y >= 0))
            {
                for (int j = 0; j < lengthTypes; j++)
                {
                    if (GetCell(next).cellType == allowedTypes[j])
                    {
                        validCellType = 1;
                    }
                }

                int tempEntity = IsOccupied(next);
                if (avoidEntity != entity_none)
                {
                    if (tempEntity == entity_remains)
                    {
                        occupiedCell = 1;
                    }
                }
                if (avoidEntity == entity_player)
                {
                    if (tempEntity == entity_player)
                    {
                        occupiedCell = 1;
                    }
                }
                if (avoidEntity == entity_enemy)
                {
                    if (tempEntity == entity_enemy)
                    {
                        occupiedCell = 1;
                    }
                }
            }

            // is it already reached
            int exists = 0;
            for (int j = 0; j < reached.occupied; j++)
            {
                if (ComparePos(reached.data[j].pos, next))
                {
                    exists = 1;
                }
            }

            // add to frontier and reached if not reached
            if (validCellType == 1 && occupiedCell == 0 && exists == 0)
            {
                QueueInsert(&frontier, next, current.dist + 1);
                DynArrDist2Add(&reached, next, current.dist + 1);
            }
        }
    }

    for (int i = 0; i < reached.occupied; i++)
    {
        if (reached.data[i].dist <= range)
        {
            DynArrDist2Add(d, reached.data[i].pos, reached.data[i].dist);
        }
    }
}

void InitRemains(remains_t *r, Vector2Int pos, Texture2D texture)
{
    r->pos = pos;
    r->hp = 2;
    r->unitTexture = texture;
}

void RemainsDraw(remains_t *r)
{
    DrawTextureEx(r->unitTexture, (Vector2){GetCell(r->pos).rect.x, GetCell(r->pos).rect.y}, 0, GetScale(r->unitTexture.width, cellSize), WHITE);
    DrawTextureEx(tex_disabled, (Vector2){GetCell(r->pos).rect.x, GetCell(r->pos).rect.y}, 0, GetScale(tex_disabled.width, cellSize), WHITE);
}

void CheckRemains(remains_t *r)
{
    if (r->hp <= 0)
    {
        for (int i = 0; i < g_remains.occupied; i++)
        {
            if (ComparePos(g_remains.data[i].pos, r->pos))
            {
                DynArrRemainsRemove(&g_remains, i);
            }
        }
    }
}

void IsPlayerDead(player_t *p)
{
    if (p->hp <= 0)
    {
        p->hp = 0;

        remains_t remains1 = {0};
        InitRemains(&remains1, p->pos, p->texture);
        DynArrRemainsAdd(&g_remains, remains1);

        for (int i = 0; i < g_players.occupied; i++)
        {
            if (ComparePos(g_players.data[i].pos, p->pos))
            {
                DynArrPlayerRemove(&g_players, i);
            }
        }
    }

    if (g_players.occupied == 0)
    {
        g_gameState = game_state_game_over;
    }
}

void IsEnemyDead(enemy_t *e)
{
    if (e->hp <= 0)
    {
        e->hp = 0;

        for (int i = 0; i < g_enemies.occupied; i++)
        {
            if (ComparePos(g_enemies.data[i].pos, e->pos))
            {
                DynArrEnemyRemove(&g_enemies, i);
            }
        }
    }

    if (g_enemies.occupied == 0)
    {
        g_gameState = game_state_victory;
    }
}

void EnemyAttack(enemy_t *e, Vector2Int target, attack_type type)
{
    DynArrVect2Reset(&e->targetCells);
    DynArrVect2Reset(&e->attackCells);

    switch (type)
    {
    case attack_single:
    {
        DynArrVect2Add(&e->targetCells, target);
        DynArrVect2Add(&e->attackCells, target);
    }
    break;
    case attack_line:
    {
        Vector2Int dir = Vector2IntSubtract(e->pos, target);
        Vector2Int start = e->pos;

        int reachedStop = 0;
        while (reachedStop == 0)
        {
            int outside = 0;
            Vector2Int next = Vector2IntAdd(start, dir);

            // check if attack reaches a stop
            if (GetCell(next).cellType == cell_mountain)
            {
                reachedStop = 1;
            }
            if ((next.x < 0 || next.y < 0) || (next.x >= BOARD_SIZE || next.y >= BOARD_SIZE)) // outside board
            {
                outside = 1;
            }

            int tempEntity = IsOccupied(next);
            if (tempEntity != entity_none)
            {
                reachedStop = 1;
            }

            if (outside == 0)
            {
                DynArrVect2Add(&e->targetCells, next);
                start = next;
            }
            else
            {
                reachedStop = 1;
            }

            if (reachedStop == 1)
            {
                if (outside == 1)
                {
                    next = Vector2IntSubtract(next, dir);
                }

                DynArrVect2Add(&e->attackCells, next);
            }
        }
    }
    break;
    }
}

void Push(Vector2Int origin, Vector2Int target, int knockback)
{
    Vector2Int dir = Vector2IntSubtract(target, origin);
    Vector2Int next = target;

    int targetType = 0;
    player_t *p;
    enemy_t *e;
    remains_t *r;

    for (int i = 0; i < g_players.occupied; i++)
    {
        if (ComparePos(g_players.data[i].pos, target))
        {
            targetType = entity_player;
            p = &g_players.data[i];
        }
    }
    for (int i = 0; i < g_enemies.occupied; i++)
    {
        if (ComparePos(g_enemies.data[i].pos, target))
        {
            targetType = entity_enemy;
            e = &g_enemies.data[i];
        }
    }
    for (int i = 0; i < g_remains.occupied; i++)
    {
        if (ComparePos(g_remains.data[i].pos, target))
        {
            targetType = entity_remains;
            r = &g_remains.data[i];
        }
    }

    if (targetType != entity_none) // if target is entity
    {
        if ((next.x != 0 && next.y != 0) && (next.x != BOARD_SIZE - 1 && next.y != BOARD_SIZE - 1)) // not on board edge
        {
            for (int i = 0; i < knockback; i++)
            {
                next = Vector2IntAdd(next, dir);
                int clear = 1;

                if ((next.x == 0 || next.y == 0) || (next.x == BOARD_SIZE - 1 || next.y == BOARD_SIZE - 1)) // if at board edge
                {
                    knockback = 0;
                }

                if (GetCell(next).cellType == cell_mountain) // check if stopped by mountain, damage both
                {
                    DamageCell(next);
                    switch (targetType)
                    {
                    case entity_player:
                    {
                        p->hp--;
                    }
                    break;
                    case entity_enemy:
                    {
                        e->hp--;
                    }
                    break;
                    case entity_remains:
                    {
                        r->hp--;
                    }
                    break;
                    }
                    clear = 0;
                }
                else
                {
                    for (int j = 0; j < g_players.occupied; j++) // check if stopped by player, damage both
                    {
                        if (ComparePos(g_players.data[j].pos, next))
                        {
                            g_players.data[j].hp--;
                            switch (targetType)
                            {
                            case entity_player:
                            {
                                p->hp--;
                            }
                            break;
                            case entity_enemy:
                            {
                                e->hp--;
                            }
                            break;
                            case entity_remains:
                            {
                                r->hp--;
                            }
                            break;
                            }
                            clear = 0;
                        }
                    }
                    for (int j = 0; j < g_enemies.occupied; j++) // check if stopped by enemy, damage both
                    {
                        if (ComparePos(g_enemies.data[j].pos, next))
                        {
                            g_enemies.data[j].hp--;
                            switch (targetType)
                            {
                            case entity_player:
                            {
                                p->hp--;
                            }
                            break;
                            case entity_enemy:
                            {
                                e->hp--;
                            }
                            break;
                            case entity_remains:
                            {
                                r->hp--;
                            }
                            break;
                            }
                            clear = 0;
                        }
                    }
                    for (int j = 0; j < g_remains.occupied; j++) // check if stopped by remains, damage both
                    {
                        if (ComparePos(g_remains.data[j].pos, next))
                        {
                            g_remains.data[j].hp--;
                            switch (targetType)
                            {
                            case entity_player:
                            {
                                p->hp--;
                            }
                            break;
                            case entity_enemy:
                            {
                                e->hp--;
                            }
                            break;
                            case entity_remains:
                            {
                                r->hp--;
                            }
                            break;
                            }
                            clear = 0;
                        }
                    }
                }

                if (clear == 1)
                {
                    switch (targetType)
                    {
                    case entity_player:
                    {
                        p->pos = next;
                    }
                    break;
                    case entity_enemy:
                    {
                        Vector2Int attackDir = {0};
                        Vector2Int target = {0};
                        if (e->attackCells.occupied != 0) // if attacking, gain attack dir
                        {
                            attackDir = Vector2IntNormalize(Vector2IntSubtract(e->pos, e->attackCells.data[0]));
                        }

                        e->pos = next; // push

                        if (e->attackCells.occupied != 0) // if attacking, move attack with push
                        {
                            target = Vector2IntAdd(e->pos, attackDir);
                            EnemyAttack(e, target, e->attackType);
                        }
                    }
                    break;
                    case entity_remains:
                    {
                        r->pos = next;
                    }
                    break;
                    }
                }
                else
                {
                    knockback = 0;
                }
            }
        }
    }
}

void Attack(Vector2Int origin, Vector2Int target, int dmg, int knockback, attack_type type)
{
    if (type == attack_single)
    {
    }
    triggTarget = target;
    trigg = 1;

    DamageCell(target);

    for (int i = 0; i < g_players.occupied; i++)
    {
        if (ComparePos(g_players.data[i].pos, target))
        {
            g_players.data[i].hp -= dmg;
        }
    }

    for (int i = 0; i < g_enemies.occupied; i++)
    {
        if (ComparePos(g_enemies.data[i].pos, target))
        {
            g_enemies.data[i].hp -= dmg;
        }
    }

    for (int i = 0; i < g_remains.occupied; i++)
    {
        if (ComparePos(g_remains.data[i].pos, target))
        {
            g_remains.data[i].hp--;
        }
    }

    if (knockback > 0)
    {
        Push(origin, target, knockback);
    }

    for (int i = 0; i < g_players.occupied; i++)
    {
        IsPlayerDead(&g_players.data[i]);
    }
    for (int i = 0; i < g_enemies.occupied; i++)
    {
        IsEnemyDead(&g_enemies.data[i]);
    }
}

void DrawHealth(Vector2Int pos, int maxHp, int hp)
{
    Rectangle cellRect = GetCell(pos).rect;
    Rectangle rect = (Rectangle){cellRect.x, cellRect.y, cellRect.width / 2, cellRect.height / 5};
    // Rectangle rect = cellRect;

    int xtemp = (cellRect.width - rect.width) / 2;
    int ytemp = (rect.height / 3);
    int xTempMargin = rect.width / 15;
    int yTempMargin = rect.height / 4;
    // int width = rect.width / maxHp - xTempMargin;
    int width = ((cellRect.width - (xtemp * 2)) - (xTempMargin * (maxHp + 1))) / maxHp;

    rect = (Rectangle){rect.x + xtemp, rect.y + ytemp, rect.width, rect.height};

    DrawRectangleRec(rect, DARKGRAY);
    for (int i = 0; i < maxHp; i++)
    {
        Color color = GREEN;
        if (hp <= i)
        {
            color = (Color){120, 120, 120, 255};
        }

        DrawRectangle((rect.x + xTempMargin) + (i * (width + xTempMargin)), rect.y + yTempMargin, width, rect.height - (yTempMargin * 2), color);
    }

    if (maxHp == 1 && hp == 1)
    {
    }
}

void DrawBorder(Vector2Int pos, int size, Color color)
{
    Rectangle rect = GetCell(pos).rect;
    int width = (rect.width / 25) * size;
    int length = rect.width;

    DrawRectangle(rect.x, rect.y, width, length, color);
    DrawRectangle(rect.x, rect.y, length, width, color);

    DrawRectangle(rect.x + rect.width - width, rect.y, width, length, color);
    DrawRectangle(rect.x, rect.y + rect.height - width, length, width, color);
}

Rectangle GetPlayerRect(player_t *p)
{
    return (Rectangle){GetCell(p->pos).rect.x, GetCell(p->pos).rect.y, cellSize, cellSize};
}

void InitPlayer(player_t *p, unit_type type)
{
    int xtemp = 0;
    int ytemp = 0;
    int valid = 0;
    while (valid != 1)
    {
        xtemp = GetRand(0, BOARD_SIZE - 1);
        ytemp = GetRand(1, 2);
        Vector2Int temp = (Vector2Int){xtemp, ytemp};

        if (board[xtemp][ytemp].cellType == cell_flat)
        {
            valid = 1;
            for (int i = 0; i < g_players.occupied; i++)
            {
                if (ComparePos(g_players.data[i].pos, temp))
                {
                    valid = 0;
                }
            }
        }
    }

    p->type = type;
    p->pos = (Vector2Int){xtemp, ytemp};
    p->state = 0;
    p->maxAction = 1;
    p->action = p->maxAction;

    switch (p->type)
    {
    case unit_melee:
    {
        p->dmg = 2;
        p->maxHp = 3;
        p->maxMovement = 4;
        p->attackType = attack_single;
        p->knockback = 1;
        p->texture = LoadTexture("IMG/mech1.png");
    }
    break;
    case unit_ranged:
    {
        p->dmg = 1;
        p->maxHp = 2;
        p->maxMovement = 4;
        p->attackType = attack_line;
        p->knockback = 1;
        p->texture = LoadTexture("IMG/tank1.png");
    }
    break;
    case unit_artillery:
    {
        p->dmg = 1;
        p->maxHp = 2;
        p->maxMovement = 3;
        p->attackType = attack_line;
        p->knockback = 0;
        p->texture = LoadTexture("IMG/artillery1.png");
    }
    break;
    }
    p->hp = p->maxHp;
    p->movement = p->maxMovement;

    InitDynArrDist2(&p->bfsMovement);
    InitDynArrDist2(&p->bfsAttack);
}

void PlayerStatReset(player_t *p)
{
    p->movement = p->maxMovement;
    p->action = p->maxAction;
}

void PlayerGetMovement(player_t *p)
{
    Bfs(&p->bfsMovement, p->pos, p->movement, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_enemy);

    for (int i = 0; i < p->bfsMovement.occupied; i++)
    {
        int tempEntity = IsOccupied(p->bfsMovement.data[i].pos);
        if (tempEntity != entity_none)
        {
            DynArrDist2Remove(&p->bfsMovement, i);
            i--;
        }
    }
}

void PlayerMove(player_t *p, Distance2 target)
{
    p->pos = target.pos;
    p->movement -= target.dist;
    PlayerGetMovement(p);

    // update enemy attacks to match the potential new units in the way
    for (int i = 0; i < g_enemies.occupied; i++)
    {
        enemy_t *e = &g_enemies.data[i];
        if (e->attackCells.occupied != 0) // if attacking, update it
        {
            Vector2Int attackDir = Vector2IntNormalize(Vector2IntSubtract(e->pos, e->attackCells.data[0]));
            Vector2Int target = Vector2IntAdd(e->pos, attackDir);
            EnemyAttack(e, target, e->attackType);
        }
    }
}

void PlayerAttack(player_t *p, Vector2Int target)
{
    switch (p->attackType)
    {
    case attack_single:
    {
        Attack(p->pos, target, p->dmg, p->knockback, p->attackType);
    }
    break;
    case attack_line:
    {
        Vector2Int dir = Vector2IntSubtract(target, p->pos);
        Vector2Int start = p->pos;

        int reachedStop = 0;
        while (reachedStop == 0)
        {
            int outside = 0;
            Vector2Int next = Vector2IntAdd(start, dir);

            // check if attack reaches a stop
            if (GetCell(next).cellType == cell_mountain)
            {
                reachedStop = 1;
            }
            if ((next.x < 0 || next.y < 0) || (next.x >= BOARD_SIZE || next.y >= BOARD_SIZE)) // outside board
            {
                outside = 1;
            }

            int tempEntity = IsOccupied(next);
            if (tempEntity != entity_none)
            {
                reachedStop = 1;
            }

            if (outside == 0)
            {
                start = next;
            }
            else
            {
                reachedStop = 1;
            }

            if (reachedStop == 1)
            {
                if (outside == 1)
                {
                    next = Vector2IntSubtract(next, dir);
                }

                Attack(p->pos, next, p->dmg, p->knockback, p->attackType);
            }
        }
    }
    break;
    }

    p->action--;
}

void PlayerUpdate(player_t *p, Vector2 mouse_pos)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (HoverCell(p->pos))
        {
            if (IsSelected(p))
            {
                p->state = state_unselected;
                g_selected = NULL;
            }
            else
            {
                int index = -1;
                for (int i = 0; i < g_players.occupied; i++)
                {
                    if (g_players.data[i].state == state_attack)
                    {
                        index = i;
                    }
                }

                int temp = 0;
                if (index != -1)
                {
                    for (int i = 0; i < g_players.data[index].bfsAttack.occupied; i++)
                    {
                        if (ComparePos(g_players.data[index].bfsAttack.data[i].pos, p->pos))
                        {
                            temp = 1;
                        }
                    }
                }

                if (temp == 0)
                {
                    g_selected = p;
                    p->state = state_move;
                    PlayerGetMovement(p);
                }
            }
        }
        else if (CheckCollisionPointRec(mouse_pos, buttons[button_attack].rect))
        {
            if (IsSelected(p))
            {
                if (p->state == state_move)
                {
                    if (GetCell(p->pos).cellType != cell_water)
                    {
                        p->state = state_attack;
                        Bfs(&p->bfsAttack, p->pos, 1, cell_type_all, sizeof(cell_type_all) / sizeof(*cell_type_all), entity_none);

                        for (int i = 0; i < p->bfsAttack.occupied; i++)
                        {
                            if (p->bfsAttack.data[i].dist != 1)
                            {
                                DynArrDist2Remove(&p->bfsAttack, i);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (IsSelected(p))
            {
                switch (p->state)
                {
                case state_move:
                {
                    for (int i = 0; i < p->bfsMovement.occupied; i++)
                    {
                        if (HoverCell(p->bfsMovement.data[i].pos))
                        {
                            if (p->action > 0)
                            {
                                PlayerMove(p, p->bfsMovement.data[i]);
                            }
                        }
                    }
                }
                break;
                case state_attack:
                {
                    for (int i = 0; i < p->bfsAttack.occupied; i++)
                    {
                        if (HoverCell(p->bfsAttack.data[i].pos))
                        {
                            if (p->action > 0)
                            {
                                PlayerAttack(p, p->bfsAttack.data[i].pos);
                            }
                        }
                    }
                }
                break;
                }
            }
        }
    }
}

void PlayerDraw(player_t *p)
{
    if (IsSelected(p))
    {
        if (p->state == state_move)
        {
            for (int i = 0; i < p->bfsMovement.occupied; i++)
            {
                if (HoverCell(p->bfsMovement.data[i].pos))
                {
                    DrawRectangleRec(GetCell(p->bfsMovement.data[i].pos).rect, (Color){0, 228, 48, 100});
                }
                else
                {
                    DrawBorder(p->bfsMovement.data[i].pos, 2, (Color){0, 228, 48, 200});
                }
            }
        }
        else if (p->state == state_attack)
        {
            for (int i = 0; i < p->bfsAttack.occupied; i++)
            {
                if (HoverCell(p->bfsAttack.data[i].pos))
                {
                    DrawRectangleRec(GetCell(p->bfsAttack.data[i].pos).rect, (Color){230, 41, 55, 120});
                }
                else
                {
                    DrawBorder(p->bfsAttack.data[i].pos, 2, (Color){230, 41, 55, 200});
                }
            }
        }
    }

    if (HoverCell(p->pos) || IsSelected(p))
    {
        DrawBorder(p->pos, 1, GREEN);
    }

    DrawTextureEx(p->texture, (Vector2){GetCell(p->pos).rect.x, GetCell(p->pos).rect.y}, 0, GetScale(p->texture.width, cellSize), WHITE);
    DrawHealth(p->pos, p->maxHp, p->hp);
}

void InitEnemy(enemy_t *e, unit_type type)
{
    int xtemp = 0;
    int ytemp = 0;
    int valid = 0;
    while (valid != 1)
    {
        xtemp = GetRand(0, BOARD_SIZE - 1);
        ytemp = GetRand(BOARD_SIZE - 3, BOARD_SIZE - 1);
        Vector2Int temp = (Vector2Int){xtemp, ytemp};

        if (board[xtemp][ytemp].cellType == cell_flat)
        {
            valid = 1;
            int tempEntity = IsOccupied(temp);
            if (tempEntity == entity_enemy)
            {
                valid = 0;
            }
        }
    }

    e->type = type;
    e->pos = (Vector2Int){xtemp, ytemp};
    e->maxHp = 2;
    e->movement = 4;
    InitDynArrDist2(&e->bfsMovement);
    InitDynArrDist2(&e->bfsAttack);
    InitDynArrVect2(&e->targetCells);
    InitDynArrVect2(&e->attackCells);

    switch (e->type)
    {
    case unit_melee:
    {
        e->maxHp = 3;
        e->dmg = 2;
        e->attackType = attack_single;
        e->knockback = 1;
        e->texture = LoadTexture("IMG/angryface1.png");
    }
    break;
    case unit_ranged:
    {
        e->maxHp = 2;
        e->dmg = 1;
        e->attackType = attack_line;
        e->knockback = 0;
        e->texture = LoadTexture("IMG/angryface2.png");
    }
    break;
    }
    e->hp = e->maxHp;
}

void EnemyGetMovement(enemy_t *e)
{
    Bfs(&e->bfsMovement, e->pos, e->movement, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_player);

    for (int i = 0; i < e->bfsMovement.occupied; i++)
    {
        int tempEntity = IsOccupied(e->bfsMovement.data[i].pos);
        if (tempEntity != entity_none)
        {
            DynArrDist2Remove(&e->bfsMovement, i);
            i--;
        }
    }
}

void EnemyMove(enemy_t *e, Vector2Int target)
{
    e->pos = target;

    EnemyGetMovement(e);
}

void EnemyUpdate(enemy_t *e)
{
    int dice = GetRand(1, 20);
    int pIndex = 0;
    int canAttack = 0;
    Vector2Int target = {0};

    if (dice >= 8) // aggro on closest player
    {
        int value = 0;
        int lowestValue = 0;
        for (int i = 0; i < g_players.occupied; i++)
        {
            DynArrDist2 tempArea;
            InitDynArrDist2(&tempArea);
            Bfs(&tempArea, g_players.data[i].pos, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_none);

            for (int j = 0; j < tempArea.occupied; j++)
            {
                if (ComparePos(tempArea.data[j].pos, e->pos))
                {
                    value = tempArea.data[j].dist;
                    if (i == 0)
                    {
                        lowestValue = value;
                        pIndex = i;
                    }

                    if (value < lowestValue)
                    {
                        lowestValue = value;
                        pIndex = i;
                    }
                }
            }
        }
    }
    else // random player
    {
        pIndex = GetRand(0, g_players.occupied - 1);
    }
    player_t *p = &g_players.data[pIndex];

    EnemyGetMovement(e);

    DynArrDist2 playerArea;
    InitDynArrDist2(&playerArea);
    Bfs(&playerArea, p->pos, BOARD_SIZE * BOARD_SIZE, cell_type_attackable, sizeof(cell_type_attackable) / sizeof(*cell_type_attackable), entity_none);

    Distance2 ownDist2 = {0};
    int hasPath = 0;
    for (int i = 0; i < playerArea.occupied; i++) // possible to avoid water?
    {
        if (ComparePos(e->pos, playerArea.data[i].pos))
        {
            ownDist2 = playerArea.data[i];
            hasPath = 1;
        }
    }
    if (hasPath == 0) // not possible to avoid water or possibly no path at all
    {
        DynArrDist2Reset(&playerArea);
        Bfs(&playerArea, p->pos, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_none);

        for (int i = 0; i < playerArea.occupied; i++)
        {
            if (ComparePos(e->pos, playerArea.data[i].pos))
            {
                ownDist2 = playerArea.data[i];
                hasPath = 1;
            }
        }
    }

    if (hasPath == 1) // move towards attack position
    {
        switch (e->type)
        {
        case unit_melee:
        {
            Distance2 temp1 = {0};
            Distance2 temp2 = ownDist2;
            for (int i = 0; i < e->bfsMovement.occupied; i++)
            {
                for (int j = 0; j < playerArea.occupied; j++)
                {
                    if (ComparePos(e->bfsMovement.data[i].pos, playerArea.data[j].pos))
                    {
                        temp1 = playerArea.data[j];

                        if (temp1.dist < temp2.dist && temp1.dist != 0 && GetCell(temp1.pos).cellType != cell_water)
                        {
                            temp2 = temp1;
                        }
                    }
                }
            }

            for (int i = 0; i < e->bfsMovement.occupied; i++)
            {
                if (ComparePos(e->bfsMovement.data[i].pos, temp2.pos))
                {
                    EnemyMove(e, temp2.pos);
                }
            }
            target = p->pos;
        }
        break;
        case unit_ranged:
        {
            DynArrVect2 validPositions;
            InitDynArrVect2(&validPositions);

            for (int i = north; i < west; i++) // go through all directions from target and add valid positions to array
            {
                Vector2Int dir = {0};
                switch (i) // set dir
                {
                case north:
                {
                    dir = (Vector2Int){0, -1};
                }
                break;
                case east:
                {
                    dir = (Vector2Int){1, 0};
                }
                break;
                case south:
                {
                    dir = (Vector2Int){0, 1};
                }
                break;
                case west:
                {
                    dir = (Vector2Int){-1, 0};
                }
                break;
                }

                Vector2Int start = p->pos;

                int reachedStop = 0;
                while (reachedStop == 0)
                {
                    puts("while");
                    Vector2Int next = Vector2IntAdd(start, dir);

                    if (GetCell(next).cellType == cell_mountain)
                    {
                        reachedStop = 1;
                    }
                    if ((next.x < 0 || next.y < 0) || (next.x >= BOARD_SIZE || next.y >= BOARD_SIZE)) // outside board
                    {
                        reachedStop = 1;
                    }

                    int tempEntity = IsOccupied(next);
                    if (tempEntity != entity_none)
                    {
                        reachedStop = 1;
                    }

                    if ((reachedStop != 1 && GetCell(next).cellType != cell_water) || ComparePos(next, e->pos))
                    {
                        DynArrVect2Add(&validPositions, next);
                    }

                    start = next;
                }
            }

            if (validPositions.occupied > 0)
            {
                // get closest valid position
                DynArrDist2 ownArea;
                InitDynArrDist2(&ownArea);
                Bfs(&ownArea, e->pos, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_none);

                Distance2 temp1 = {0};
                Distance2 temp2 = {0};
                for (int i = 0; i < ownArea.occupied; i++) // get a position for reference to start from
                {
                    for (int j = 0; j < validPositions.occupied; j++)
                    {
                        if (ComparePos(ownArea.data[i].pos, validPositions.data[j]))
                        {
                            temp2 = ownArea.data[i];
                            i = ownArea.occupied;
                            j = validPositions.occupied;
                        }
                    }
                }
                for (int i = 0; i < ownArea.occupied; i++)
                {
                    for (int j = 0; j < validPositions.occupied; j++)
                    {
                        if (ComparePos(ownArea.data[i].pos, validPositions.data[j]))
                        {
                            temp1 = ownArea.data[i];

                            if (temp1.dist < temp2.dist && GetCell(temp1.pos).cellType != cell_water)
                            {
                                temp2 = temp1;
                            }
                        }
                    }
                }

                // pathfind from closest valid position
                DynArrDist2 positionArea;
                InitDynArrDist2(&positionArea);
                Bfs(&positionArea, temp2.pos, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_none);

                Distance2 temp3 = {0};
                Distance2 temp4 = {0};
                for (int i = 0; i < positionArea.occupied; i++) // find own for reference
                {
                    if (ComparePos(e->pos, positionArea.data[i].pos))
                    {
                        temp4 = positionArea.data[i];
                    }
                }
                for (int i = 0; i < e->bfsMovement.occupied; i++)
                {
                    for (int j = 0; j < positionArea.occupied; j++)
                    {
                        if (ComparePos(e->bfsMovement.data[i].pos, positionArea.data[j].pos))
                        {
                            temp3 = positionArea.data[j];

                            if (temp3.dist < temp4.dist && GetCell(temp3.pos).cellType != cell_water)
                            {
                                temp4 = temp3;
                            }
                        }
                    }
                }

                for (int i = 0; i < e->bfsMovement.occupied; i++)
                {
                    if (ComparePos(e->bfsMovement.data[i].pos, temp4.pos))
                    {
                        EnemyMove(e, temp4.pos);
                    }
                }

                for (int i = 0; i < validPositions.occupied; i++)
                {
                    if (ComparePos(e->pos, validPositions.data[i]))
                    {
                        puts("can attack");
                        canAttack = 1;
                        Vector2Int temp7 = Vector2IntNormalize(Vector2IntSubtract(e->pos, p->pos));
                        target = Vector2IntAdd(e->pos, temp7);
                    }
                }
            }
        }
        break;
        }
    }
    else // move towards and destroy mountain blocking pathfinding
    {
        puts("No path");
        DynArrDist2 ownArea;
        InitDynArrDist2(&ownArea);
        Bfs(&ownArea, e->pos, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_none);

        // Get all mountains
        DynArrVect2 mountains;
        InitDynArrVect2(&mountains);
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            for (int x = 0; x < BOARD_SIZE; x++)
            {
                if (board[x][y].cellType == cell_mountain)
                {
                    DynArrVect2Add(&mountains, (Vector2Int){x, y});
                }
            }
        }
        DynArrVect2DInt validMountainSides;
        InitDynArrVect2DInt(&validMountainSides);
        for (int i = 0; i < mountains.occupied; i++) // Check each mountain
        {
            DynArrVect2 sides = {0};
            InitDynArrVect2(&sides);
            Vector2Int current = mountains.data[i];
            for (int j = north; j < west; j++)
            {
                Vector2Int next = {0};
                switch (j)
                {
                case north:
                {
                    next = (Vector2Int){current.x, current.y - 1};
                }
                break;
                case east:
                {
                    next = (Vector2Int){current.x + 1, current.y};
                }
                break;
                case south:
                {
                    next = (Vector2Int){current.x, current.y + 1};
                }
                break;
                case west:
                {
                    next = (Vector2Int){current.x - 1, current.y};
                }
                break;
                }

                // Can you stand and attack from cell
                int validCellType = 0;
                for (int k = 0; k < (int)(sizeof(cell_type_walkable) / sizeof(*cell_type_walkable)); k++)
                {
                    if (GetCell(next).cellType == cell_type_walkable[k] && GetCell(next).cellType != cell_water)
                    {
                        validCellType = 1;
                    }
                }

                if (validCellType == 1)
                {
                    DynArrVect2Add(&sides, next);
                }
            }

            int ownSide = 0;
            int playerSide = 0;
            int pastOwnSide = 0;
            for (int j = 0; j < sides.occupied; j++) // Check if mountain has sides to player and enemy
            {
                for (int k = 0; k < ownArea.occupied; k++)
                {
                    if (ComparePos(sides.data[j], ownArea.data[k].pos))
                    {
                        ownSide++;
                    }
                }

                for (int k = 0; k < playerArea.occupied; k++)
                {
                    if (ComparePos(sides.data[j], playerArea.data[k].pos))
                    {
                        playerSide++;
                    }
                }

                if (ownSide == pastOwnSide) // remove any side not accessible to enemy
                {
                    DynArrVect2Remove(&sides, j);
                    j--;
                }

                pastOwnSide = ownSide;
            }

            if (ownSide > 0 && playerSide > 0) // remember side and its mountain if mountain has sides to enemy and player
            {
                for (int j = 0; j < sides.occupied; j++)
                {
                    DynArrVect2DIntAdd(&validMountainSides, sides.data[j], mountains.data[i]);
                }
            }
        }

        int temp = GetRand(0, validMountainSides.occupied - 1); // get random side
        for (int i = 0; i < validMountainSides.occupied; i++)
        {
            if (ComparePos(e->pos, validMountainSides.data[i].a)) // if already at a side, override the random
            {
                temp = i;
            }
        }

        int abc = 0;
        for (int i = 0; i < ownArea.occupied; i++)
        {
            if (ComparePos(e->pos, validMountainSides.data[temp].a) == false)
            {
                for (int j = 0; j < e->bfsMovement.occupied; j++)
                {
                    if (ComparePos(e->bfsMovement.data[j].pos, validMountainSides.data[temp].a)) // if in range move to it
                    {
                        EnemyMove(e, validMountainSides.data[temp].a);
                    }
                    else // find path
                    {
                        abc = 1;
                    }
                }
                if (abc == 1) // find path to it
                {
                    DynArrDist2 pathToMountain;
                    InitDynArrDist2(&pathToMountain);
                    Bfs(&pathToMountain, validMountainSides.data[temp].a, BOARD_SIZE * BOARD_SIZE, cell_type_walkable, sizeof(cell_type_walkable) / sizeof(*cell_type_walkable), entity_player);

                    Distance2 temp1 = {0};
                    Distance2 temp2 = ownDist2;
                    for (int j = 0; j < e->bfsMovement.occupied; j++)
                    {
                        for (int k = 0; k < pathToMountain.occupied; k++)
                        {
                            if (ComparePos(e->bfsMovement.data[j].pos, pathToMountain.data[k].pos))
                            {
                                temp1 = pathToMountain.data[k];

                                if (temp1.dist < temp2.dist && temp1.dist != 0)
                                {
                                    temp2 = temp1;
                                }
                            }
                        }
                    }

                    for (int i = 0; i < e->bfsMovement.occupied; i++)
                    {
                        if (ComparePos(e->bfsMovement.data[i].pos, temp2.pos))
                        {
                            EnemyMove(e, temp2.pos);
                        }
                    }
                }
            }
        }

        if (ComparePos(e->pos, validMountainSides.data[temp].a))
        {
            EnemyAttack(e, validMountainSides.data[temp].b, e->attackType);
        }
    }

    for (int i = 0; i < playerArea.occupied; i++)
    {
        if (ComparePos(e->pos, playerArea.data[i].pos))
        {
            ownDist2 = playerArea.data[i];
        }
    }

    if (e->attackType == attack_single && ownDist2.dist == 1)
    {
        canAttack = 1;
    }

    if (canAttack == 1 && GetCell(e->pos).cellType != cell_water)
    {
        EnemyAttack(e, target, e->attackType);
    }
}

void EnemyDraw(enemy_t *e)
{
    for (int i = 0; i < e->targetCells.occupied; i++)
    {
        DrawRectangleRec(GetCell(e->targetCells.data[i]).rect, (Color){255, 0, 0, 45});
    }

    if (HoverCell(e->pos))
    {
        DrawBorder(e->pos, 1, RED);
    }

    DrawTextureEx(e->texture, (Vector2){GetCell(e->pos).rect.x, GetCell(e->pos).rect.y}, 0, GetScale(e->texture.width, cellSize), WHITE);
    DrawHealth(e->pos, e->maxHp, e->hp);
}

void EnemyAction(enemy_t *e)
{
    for (int i = 0; i < e->attackCells.occupied; i++)
    {
        Attack(e->pos, e->attackCells.data[i], e->dmg, 0, e->attackType);
    }

    DynArrVect2Reset(&e->targetCells);
    DynArrVect2Reset(&e->attackCells);
}

void InitPlayerUnits(void)
{
    player_t player1 = {0};
    InitPlayer(&player1, unit_melee);
    DynArrPlayerAdd(&g_players, player1);

    player_t player2 = {0};
    InitPlayer(&player2, unit_ranged);
    DynArrPlayerAdd(&g_players, player2);

    player_t player3 = {0};
    InitPlayer(&player3, unit_artillery);
    DynArrPlayerAdd(&g_players, player3);
}

void InitEnemyUnits(int amount)
{
    for (int i = 0; i < amount; i++)
    {
        unit_type type = 0;
        int temp = GetRand(0, 20);
        if (temp >= 8)
        {
            type = unit_melee;
        }
        else
        {
            type = unit_ranged;
        }

        enemy_t enemy = {0};
        InitEnemy(&enemy, type);
        DynArrEnemyAdd(&g_enemies, enemy);
    }
}

void InitStartScreen(void)
{

    for (int i = 0; i < 2; i++)
    {
        button_t *b = &buttons[i];

        switch (i)
        {
        case 0:
        {
            b->text = "Play";
        }
        break;
        case 1:
        {
            b->text = "Rules";
        }
        break;
        }

        b->fontSize = 45;
        b->textSize.x = MeasureText(b->text, b->fontSize);
        b->textSize.y = MeasureTextEx(GetFontDefault(), b->text, b->fontSize, 1).y;
        int width = b->textSize.x + 40;
        int height = b->textSize.y + 20;

        b->rect = (Rectangle){SCREEN_WITDH / 2 - width / 2, (SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / 16) - height / 2 + (i * (height + 20)), width, height};
    }
}

void UpdateStartScreen(void)
{
    Vector2 mousePos = GetMousePosition();

    if (showRules == 0)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mousePos, buttons[0].rect))
            {
                g_gameState++;
            }
            else if (CheckCollisionPointRec(mousePos, buttons[1].rect))
            {
                showRules = 1;
            }
        }
    }
    else
    {
        const char *text = "Back";
        int temp = MeasureText(text, 40);
        int width = temp + 20;
        int height = MeasureTextEx(GetFontDefault(), text, 40, 1).y + 10;
        Rectangle btn = {SCREEN_WITDH / 2 - width / 2, SCREEN_HEIGHT / 1.1, width, height};
        buttons[0].rect = btn;
        buttons[0].fontSize = 40;
        buttons[0].text = text;
        buttons[0].textSize = MeasureTextEx(GetFontDefault(), text, 40, 1);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mousePos, buttons[0].rect))
            {
                showRules = 0;
                InitStartScreen();
            }
        }
    }
}

void DrawStartScreen(void)
{
    const char *text = "Game Gaem";
    int fontSize = 70;
    DrawText(text, SCREEN_WITDH / 2 - MeasureText(text, fontSize) / 2, (SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / 3.5f), fontSize, WHITE);

    if (showRules == 0)
    {
        for (int i = 0; i < 2; i++)
        {
            button_t *b = &buttons[i];
            DrawRectangleRec(b->rect, GRAY);
            DrawText(b->text, b->rect.x + b->rect.width / 2 - b->textSize.x / 2, b->rect.y + b->rect.height / 2 - b->textSize.y / 2, b->fontSize, WHITE);
        }
    }
    else
    {
        const char *ruleText = "The game is played on an 8x8 grid\n\
There are different cells on this board.\n\
    Flat ground, shown as beige.\n\
        Just regular ground.\n\
    Water, shown as blue.\n\
        If a unit is standing in water then they cannot attack.\n\
    Mountains, shown as brown.\n\
        Mountains block movement and attacks but can be destroyed by hitting them twice.\n\
\n\
You have three units, a Mech, a Tank and an Artillery. These can be selected by clicking on them.\n\
    The Mech is a melee unit, attacking only one cell next to it, doing 2 damage and knockback\n\
    The Tank is a ranged units, shooting in one direction and damaging the first hit thing for 1 damage and knockback\n\
    The Artillery is a ranged units, shooting in one direction and damaging the first hit thing for 1 damage\n\
Each unit has a set amount movement points and one action point.\n\
    Movement points limit how far a unit can be moved each turn, they are reduced by the distance moved.\n\
        You can move a selected unit by clicking on one of the green highlighted cells.\n\
    Action point limits units to doing beign able to do one attack per turn.\n\
        You can attack with a Selected unit by clicking on the attack button and then clicking on one red highlighted cells.\n\
\n\
Enemies can be identified by their very angry faces, these come in two variations.\n\
    The bigger meaner melee angry dude who will walk up to your units and punch them for 2 damage.\n\
    The smaller, still as mean, ranged dude who will try to shoot your units from afar for 1 damage.\n\
";

        int temp = MeasureText("        You can attack with a Selected unit by clicking on the attack button and then clicking on one red highlighted cells.", 20);
        DrawTextEx(GetFontDefault(), ruleText, (Vector2){SCREEN_WITDH / 2 - temp / 2, SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / 7}, 20, 2, WHITE);

        DrawRectangleRec(buttons[0].rect, GRAY);
        DrawText(buttons[0].text, buttons[0].rect.x + buttons[0].rect.width / 2 - buttons[0].textSize.x / 2, buttons[0].rect.y + buttons[0].rect.height / 2 - buttons[0].textSize.y / 2, buttons[0].fontSize, WHITE);
    }
}

int main(void)
{
    InitWindow(SCREEN_WITDH, SCREEN_HEIGHT, "gaem");
    SetTargetFPS(60);

    InitStartScreen();
    InitBoard();
    InitDynArrPlayer(&g_players);
    InitDynArrEnemy(&g_enemies);
    InitDynArrRemains(&g_remains);
    InitPlayerUnits();
    InitEnemyUnits(3);

    tex_disabled = LoadTexture("IMG/disabled.png");
    tex_cracked = LoadTexture("IMG/cracked.png");

    float timer = 0;
    int trackTime = 0;

    float delay = 0;

    while (!WindowShouldClose())
    {
        float delta_time = GetFrameTime();
        Vector2 mouse_pos = GetMousePosition();

        switch (g_gameState)
        {
        case game_state_start_screen:
        {
            UpdateStartScreen();

            if (g_gameState != game_state_start_screen)
            {
                InitButtons((Vector2){50, SCREEN_HEIGHT / 2});
            }
        }
        break;
        case game_state_start_game:
        {
            if (IsKeyPressed(KEY_ENTER))
            {
                g_gameState++;
            }
        }
        break;
        case game_state_enemy_turn:
        {
            for (int i = 0; i < g_enemies.occupied; i++) // update all g_enemies
            {
                EnemyUpdate(&g_enemies.data[i]);
            }

            EndTurn();
        }
        break;
        case game_state_player_turn:
        {
            for (int i = 0; i < g_players.occupied; i++) // update all g_players
            {
                PlayerUpdate(&g_players.data[i], mouse_pos);
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (CheckCollisionPointRec(mouse_pos, buttons[button_endTurn].rect)) // end turn
                {
                    for (int i = 0; i < g_players.occupied; i++) // reset player stats
                    {
                        PlayerStatReset(&g_players.data[i]);
                    }
                    EndTurn();
                }
            }
        }
        break;
        case game_state_action_turn:
        {
            delay += delta_time;

            for (int i = 0; i < g_enemies.occupied; i++)
            {
                EnemyAction(&g_enemies.data[i]);
            }

            if (delay > 1)
            {
                delay = 0;
                EndTurn();
            }
        }
        break;

        default:
            break;
        }

        for (int i = 0; i < g_remains.occupied; i++)
        {
            CheckRemains(&g_remains.data[i]);
        }

        BeginDrawing(); // Draw -------------------------------------------------------------------------------------------------------------------
        ClearBackground(BLACK);

        switch (g_gameState)
        {
        case game_state_start_screen:
        {
            DrawStartScreen();
        }
        break;
        case game_state_victory:
        {
            const char *text = "You won";
            DrawText(text, SCREEN_WITDH / 2 - MeasureText(text, 50)/2, SCREEN_HEIGHT / 2- SCREEN_HEIGHT/6, 50, WHITE);
        }
        break;
        case game_state_game_over:
        {
            const char *text = "You lost";
            DrawText(text, SCREEN_WITDH / 2 - MeasureText(text, 50)/2, SCREEN_HEIGHT / 2 - SCREEN_HEIGHT/10, 50, WHITE);
        }
        break;

        default:
        {
            DrawBoard();

            for (int i = 0; i < g_players.occupied; i++) // draw player units
            {
                PlayerDraw(&g_players.data[i]);
            }
            for (int i = 0; i < g_enemies.occupied; i++) // draw enemies
            {
                EnemyDraw(&g_enemies.data[i]);
            }
            for (int i = 0; i < g_remains.occupied; i++) // draw player remains
            {
                RemainsDraw(&g_remains.data[i]);
            }

            DrawButtons();
            DrawText(TextFormat("Turn: %d", turn), boardOrigin.x, boardOrigin.y - MeasureTextEx(GetFontDefault(), "T", 20, 1).y, 20, WHITE);

            if (trigg == 1)
            {
                trackTime = 1;
                trigg = 0;
            }

            if (trackTime == 1)
            {
                timer += delta_time;
                DrawRectangleRec(GetCell(triggTarget).rect, (Color){255, 0, 0, 200});
                if (timer > 0.3f)
                {
                    timer = 0;
                    trackTime = 0;
                }
            }

            if (g_gameState == game_state_start_game)
            {
                const char *text = "Press ENTER to begin.";
                DrawText(text, SCREEN_WITDH/2 - MeasureText(text, 35)/2, SCREEN_HEIGHT/2 - SCREEN_HEIGHT/10, 35, WHITE);
            }
        }
        break;
        }

        // alignment help
        // DrawLine(SCREEN_WITDH / 2, 0, SCREEN_WITDH / 2, SCREEN_HEIGHT, BLUE);
        // DrawLine(0, SCREEN_HEIGHT / 2, SCREEN_WITDH, SCREEN_HEIGHT / 2, BLUE);
        // DrawLine(GetCell(g_players[0].pos).rect.x + (GetCell(g_players[0].pos).rect.width / 2),
        //          GetCell(g_players[0].pos).rect.y,
        //          GetCell(g_players[0].pos).rect.x + (GetCell(g_players[0].pos).rect.width / 2),
        //          GetCell(g_players[0].pos).rect.y + GetCell(g_players[0].pos).rect.height,
        //          WHITE);

        EndDrawing();
    }

    return 0;
}
