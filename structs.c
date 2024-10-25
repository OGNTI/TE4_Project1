#include <raylib.h>

typedef struct
{
    int cellType;
    Rectangle rect;
    int durability;
} cell_t;

typedef struct
{
    Rectangle rect;
    Vector2 textSize;
    const char *text;
    int fontSize;
} button_t;

typedef struct
{
    int x;
    int y;
} Vector2Int;

typedef struct
{
    Vector2Int pos;
    int dist;
} Distance2;

typedef struct
{
    Vector2Int a;
    Vector2Int b;
} Vect2DInt;

typedef struct
{
    int back;
    int front;
    int size;
    Distance2 *array;
} QueueDist2;

typedef struct
{
    Distance2 *data;
    int capacity;
    int occupied;
} DynArrDist2;

typedef struct
{
    Vector2Int *data;
    int capacity;
    int occupied;
} DynArrVect2;

typedef struct
{
    Vect2DInt *data;
    int capacity;
    int occupied;
} DynArrVect2DInt;

typedef struct
{
    int type;
    int attackType;
    int knockback;
    Vector2Int pos;
    Texture2D texture;
    int maxHp;
    int hp;
    int maxMovement;
    int movement;
    int maxAction;
    int action;
    int dmg;
    int state;
    DynArrDist2 bfsMovement;
    DynArrDist2 bfsAttack;
} player_t;

typedef struct
{
    int type;
    int attackType;
    int knockback;
    Vector2Int pos;
    Texture2D texture;
    int maxHp;
    int hp;
    int movement;
    int dmg;
    DynArrDist2 bfsMovement;
    DynArrDist2 bfsAttack;
    DynArrVect2 targetCells;
    DynArrVect2 attackCells;
} enemy_t;

typedef struct
{
    Vector2Int pos;
    int hp;
    Texture2D unitTexture;
} remains_t;

typedef struct
{
    player_t *data;
    int capacity;
    int occupied;
} DynArrPlayer;

typedef struct
{
    enemy_t *data;
    int capacity;
    int occupied;
} DynArrEnemy;

typedef struct
{
    remains_t *data;
    int capacity;
    int occupied;
} DynArrRemains;

Vector2Int Vector2IntAdd(Vector2Int v1, Vector2Int v2)
{
    return (Vector2Int){v1.x + v2.x, v1.y + v2.y};
}

Vector2Int Vector2IntSubtract(Vector2Int v1, Vector2Int v2)
{
    return (Vector2Int){v1.x - v2.x, v1.y - v2.y};
}

Vector2Int Vector2IntScale(Vector2Int v, int n)
{
    return (Vector2Int){v.x * n, v.y * n};
}

Vector2Int Vector2IntNormalize(Vector2Int v)
{
    float w = sqrt(v.x * v.x + v.y * v.y);
    v.x /= w;
    v.y /= w;

    return v;
}

void InitQueue(QueueDist2 *q, int size)
{
    q->front = -1;
    q->back = -1;
    q->size = size;
    q->array = malloc(size * sizeof(Distance2));
}

bool QueueIsFull(QueueDist2 *q)
{
    return ((q->back + 1) % q->size == q->front);
}

bool QueueIsEmpty(QueueDist2 *q)
{
    return (q->front == -1);
}

void QueueInsert(QueueDist2 *q, Vector2Int pos, int dist)
{
    if (QueueIsFull(q))
    {
        puts("QueueDist2 is full");
        return;
    }

    if (QueueIsEmpty(q))
    {
        q->front = 0;
    }

    q->back = (q->back + 1) % q->size;
    q->array[q->back].pos = pos;
    q->array[q->back].dist = dist;
}

Distance2 QueueRemove(QueueDist2 *q)
{
    if (QueueIsEmpty(q))
    {
        puts("QueueDist2 is empty");
        return (Distance2){(Vector2Int){-1, -1}, -1};
    }
    else
    {

        Distance2 data = q->array[q->front];

        if (q->front == q->back) // if there is only one element, reset
        {
            q->front = -1;
            q->back = -1;
        }
        else
        {
            q->front = (q->front + 1) % q->size;
        }

        return data;
    }
}

Distance2 QueuePeek(QueueDist2 *q)
{
    if (QueueIsEmpty(q))
    {
        puts("QueueDist2 is empty");
        return (Distance2){(Vector2Int){-5, -5}, -5};
    }
    return q->array[q->front];
}

void InitDynArrDist2(DynArrDist2 *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(Distance2));
}

void DynArrDist2Add(DynArrDist2 *d, Vector2Int pos, int dist)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(Distance2));
    }

    d->data[d->occupied].pos = pos;
    d->data[d->occupied].dist = dist;
    d->occupied++;
}

void DynArrDist2Remove(DynArrDist2 *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(Distance2));
    }
}

void DynArrDist2Reset(DynArrDist2 *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(Distance2));
}

void InitDynArrVect2(DynArrVect2 *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(Vector2Int));
}

void DynArrVect2Add(DynArrVect2 *d, Vector2Int value)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(Vector2Int));
    }

    d->data[d->occupied] = value;
    d->occupied++;
}

void DynArrVect2Remove(DynArrVect2 *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(Vector2Int));
    }
}

void DynArrVect2Reset(DynArrVect2 *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(Vector2Int));
}

void InitDynArrVect2DInt(DynArrVect2DInt *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(Vect2DInt));
}

void DynArrVect2DIntAdd(DynArrVect2DInt *d, Vector2Int a, Vector2Int b)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(Vect2DInt));
    }

    d->data[d->occupied].a = a;
    d->data[d->occupied].b = b;
    d->occupied++;
}

void DynArrVect2DIntRemove(DynArrVect2DInt *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(Vect2DInt));
    }
}

void DynArrVect2DIntReset(DynArrVect2DInt *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(Vect2DInt));
}

void InitDynArrPlayer(DynArrPlayer *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(player_t));
}

void DynArrPlayerAdd(DynArrPlayer *d, player_t p)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(player_t));
    }

    d->data[d->occupied] = p;
    d->occupied++;
}

void DynArrPlayerRemove(DynArrPlayer *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(player_t));
    }
}

void DynArrPlayerReset(DynArrPlayer *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(player_t));
}

void InitDynArrEnemy(DynArrEnemy *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(enemy_t));
}

void DynArrEnemyAdd(DynArrEnemy *d, enemy_t e)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(enemy_t));
    }

    d->data[d->occupied] = e;
    d->occupied++;
}

void DynArrEnemyRemove(DynArrEnemy *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(enemy_t));
    }
}

void DynArrEnemyReset(DynArrEnemy *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(enemy_t));
}

void InitDynArrRemains(DynArrRemains *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = malloc(d->capacity * sizeof(remains_t));
}

void DynArrRemainsAdd(DynArrRemains *d, remains_t r)
{
    if (d->occupied >= d->capacity)
    {
        d->capacity *= 2;
        d->data = realloc(d->data, (d->capacity) * sizeof(remains_t));
    }

    d->data[d->occupied] = r;
    d->occupied++;
}

void DynArrRemainsRemove(DynArrRemains *d, int index)
{
    if (d->occupied < 1)
    {
        puts("Darray is empty");
        return;
    }
    else if (index > d->occupied)
    {
        puts("Darray, Index out of range");
        return;
    }

    for (int i = index; i < d->occupied; i++)
    {
        d->data[i] = d->data[i + 1];
    }
    d->occupied--;

    if (d->occupied < d->capacity / 2)
    {
        d->data = realloc(d->data, (d->capacity / 2) * sizeof(remains_t));
    }
}

void DynArrRemainsReset(DynArrRemains *d)
{
    d->capacity = 1;
    d->occupied = 0;
    d->data = realloc(d->data, d->capacity * sizeof(remains_t));
}
