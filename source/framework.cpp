#include <enum.h>
#include <framework.h>
#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <queue>

// #include <cstring>
#define WASHPLATESHIFT
#define TRUEMOVE
// #define SIMPLEBRAKECONTROL
// #define NEOBRAKECONTROL
#define COOPERATIVEDISTRIBUTION

struct Step
{
    int desx, desy;      // 终点坐标
    DirectionInteract d; // 交互方向
    TypeInteract ta;     // 交互方式
    TypeStep ts;         // 任务内容
    std::string product; // 产品名
};

struct Task
{
    Step stp[15];  // 每步需要完成的任务
    int stpsum;    // 总步数
    int completed; // 完成数量
    int cooktime;  // 加工所需时间
    bool panused;  // 是否需要使用Pan 仅初始化时使用
    bool potused;  // 是否需要使用Pot 仅初始化时使用
};

struct OrderTask
{
    Task tsk[3]; // 订单对应的任务
    int tsksum;  // 订单对应的任务数
    // int playerdistributed;        // 分配给的玩家数
    std::pair<int, int> platepos; // 使用的盘子的坐标
    // int tskdistributed;        // 已分配的任务数
};

struct IngAss
{
    std::string IngName; // 菜品名
    int before;          // 前置菜品在辅助数组中的下标
    int cooktime;        // 从初始起总加工时间
    int cookmethod;      // 从前置菜品到当前菜品的加工方式
    int x;               // 原材料的坐标
    int y;               // 原材料的坐标
    int price;           // 原材料的价格
};

/* 按照读入顺序定义 */
int width, height;
char Map[20 + 5][20 + 5];
int IngredientCount;
struct Ingredient Ingredient[20 + 5];
int recipeCount;
struct Recipe Recipe[20 + 5];
int totalTime, randomizeSeed, totalOrderCount;
struct Order totalOrder[20 + 5];
int orderCount;
struct Order Order[20 + 5];
int k;
struct Player Players[2 + 5];
int entityCount;
struct Entity Entity[20 + 5];
int remainFrame, Fund;

// 自定义全局变量
int xservicewindows, yservicewindows;    // 送菜窗口坐标
int xsink, ysink;                        // 水槽坐标
int xplatereturn, yplatereturn;          // 归还盘子处坐标
int xchoppingstation, ychoppingstation;  // 切菜处坐标
int xplaterack, yplaterack;              // 洗完盘子后盘子出现处坐标
int xplatewashshift, yplatewashshift;    // 洗完盘子后转移盘子处
int xpan, ypan;                          // Pan坐标
int xpot, ypot;                          // Pot坐标
Task WashDirtyPlate;                     // 洗盘子任务
std::set<std::pair<int, int>> plateused; // 已分配的盘子的坐标
std::pair<int, int> platefree[2];        // 当前帧需要释放的分配盘子坐标
PlateFlag dirtyplateflag;                // 脏盘标志
int platenum = 0;                        // 盘子总数
Task ptask[2 + 2];                       // 玩家正在执行的任务
Task ptaskbackup[2 + 2];                 // 死亡时重新分配的备份
bool FreePlayer[2] = {};                 // 当前帧玩家是否空闲
int CollisionAvoidenceTime = 0;          // 碰撞避免行动时间
int CollisionAvoidenceRet = 0;           // 碰撞应对策略
int OrderInDeque = 0;                    // 已解析后入队列的订单数
IngAss ingass[20 + 5];                   // 记录一种菜品的相关信息
int ingasscount;                         // 菜品种类数
Task ingTask[20 + 5];                    // 与上述辅助数组相对应的拿到该菜品所需要的任务
OrderTask totalOrderTaskParse[20 + 5];   // 订单对应的任务数组
bool checkorderass[20 + 5];              // 订单解析辅助数组
std::deque<OrderTask> NewdeqOrder;       // 重置版订单任务队列
int PlateRackNum = 0;                    // 在洗完盘子归还处的盘子数
#ifdef COOPERATIVEDISTRIBUTION
std::deque<Task> PlayerTaskDeque[2 + 5];
#endif

// 抛弃或暂无用的全局变量
// int RunningTaskSum = 0;
// Plate platearr[20];
// bool panused, potused;
// Task totalOrderParseTask[20 + 5];        // 订单解析数组
// std::deque<Task> deqOrder;               // 订单任务队列

// 判断浮点数相等
const double epsilon = 1e-4;
const double sq2 = sqrt(2);
// 浮点数四舍五入取整
double fround(double r)
{
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

// 交互相关

// 找到一个可以和位于坐标(y,x)的实体或特殊地砖交互的地点，相关信息记录在步骤stp中
void CheckInteractPos(Step &stp, const int x, const int y)
{
    if ((x > 0) && (!isupper(Map[y][x - 1])) && (getTileKind(Map[y][x - 1]) == TileKind::Floor))
    {
        stp.desx = x - 1;
        stp.desy = y;
        stp.d = RIGHT;
    }
    else if ((x < width - 1) && (!isupper(Map[y][x + 1])) && (getTileKind(Map[y][x + 1]) == TileKind::Floor))
    {
        stp.desx = x + 1;
        stp.desy = y;
        stp.d = LEFT;
    }
    else if ((y > 0) && (!isupper(Map[y - 1][x])) && (getTileKind(Map[y - 1][x]) == TileKind::Floor))
    {
        stp.desx = x;
        stp.desy = y - 1;
        stp.d = DOWN;
    }
    else if ((y < height - 1) && (!isupper(Map[y + 1][x])) && (getTileKind(Map[y + 1][x]) == TileKind::Floor))
    {
        stp.desx = x;
        stp.desy = y + 1;
        stp.d = UP;
    }
    else
        assert(0);
}

// 确认交互是否成功
bool CheckInteractSuc(Step &stp, const int op)
{
    if (stp.ts == GO_TO_INGREDIENT)
        return (!Players[op].entity.empty());
    else if ((stp.ts == TAKING_INGREDIENT_TO_PLATE) || (stp.ts == TAKING_INGREDIENT_TO_CHOP) || (stp.ts == TAKING_INGREDIENT_TO_PAN) || (stp.ts == TAKING_INGREDIENT_TO_POT))
        return (Players[op].entity.empty() && (Players[op].containerKind == ContainerKind::None));
    else if (stp.ts == TAKE_UP_PLATE)
        return (Players[op].containerKind == ContainerKind::Plate);
    else if (stp.ts == TAKING_PLATE_TO_SERVICEWINDOWS)
        return (Players[op].entity.empty() && (Players[op].containerKind == ContainerKind::None));
    else if (stp.ts == TAKE_UP_DIRTYPLATE)
        return (Players[op].containerKind == ContainerKind::DirtyPlates);
    else if (stp.ts == TAKING_DIRTYPLATE_TO_SINK)
        return (Players[op].containerKind == ContainerKind::None);
    else if (stp.ts == WASHING)
    {
        bool flag4 = false;
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::DirtyPlates) && (fabs(Entity[i].x - xsink) < epsilon) && (fabs(Entity[i].y - ysink) < epsilon))
            {
                flag4 = true;
                break;
            }
        }
        if (!flag4)
        {
            if (dirtyplateflag == TWODISTRIBUTED)
                dirtyplateflag = DISTRIBUTED;
            else
                dirtyplateflag = NONE;
        }
        return !flag4;
    }
    else if (stp.ts == CHOPING)
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::None) &&
                (!Entity[i].entity.empty()) &&
                (fabs(Entity[i].x - xchoppingstation) < epsilon) &&
                (fabs(Entity[i].y - ychoppingstation) < epsilon) &&
                (Entity[i].entity.front() == stp.product))
                return true;
        }
        return false;
    }
    else if ((stp.ts == TAKING_PLATE_TO_POT) || (stp.ts == TAKING_PLATE_TO_PAN))
    {
        for (auto it : Players[op].entity)
        {
            if (it == stp.product)
                return true;
        }
        return false;
    }
#ifdef WASHPLATESHIFT
    else if (stp.ts == CHECK_PLATE_STACK_TAKE_UP)
        return (Players[op].containerKind == ContainerKind::Plate);
    else if (stp.ts == CHECK_PLATE_STACK_TAKE_DOWN)
        return (Players[op].entity.empty() && (Players[op].containerKind == ContainerKind::None));
#endif
    else
        return false;
}

// 分配一个可用的盘子
bool CheckPlatePos(Step &stp)
{
    for (int i = 0; i < entityCount; i++)
    {
        if (Entity[i].containerKind == ContainerKind::Plate)
        {
            int x = fround(Entity[i].x);
            int y = fround(Entity[i].y);
            std::pair<int, int> pos = std::make_pair(x, y);
            if (plateused.find(pos) == plateused.end())
            {
                CheckInteractPos(stp, x, y);
                plateused.emplace(pos);
                return true;
            }
        }
    }
    return false;
}

// 移动和行动相关

#ifdef TRUEMOVE
// const double CellSize = 1;

inline int XY_TO_NUM(const int &x, const int &y)
{
    return (x + y * width);
}

std::vector<std::pair<int, double>> Edge[20 * 20] = {};
bool Unavailable[20 * 20] = {};

void initMapEdge()
{
    for (int i = 0; i < width * height; i++)
    {
        Edge[i].clear();
    }
    // memset(Unavailable, 0, sizeof(Unavailable));
    for (int i = 0; i < width; i++) // x
    {
        for (int j = 0; j < height; j++) // y
        {
            if (isupper(Map[j][i]) || (getTileKind(Map[j][i]) != TileKind::Floor))
                continue;
            int num = i + j * width;
            if ((i > 0) && (!isupper(Map[j][i - 1])) &&
                (getTileKind(Map[j][i - 1]) == TileKind::Floor))
                Edge[num].emplace_back(num - 1, 1.0);
            if ((i < width - 1) && (!isupper(Map[j][i + 1])) &&
                (getTileKind(Map[j][i + 1]) == TileKind::Floor))
                Edge[num].emplace_back(num + 1, 1.0);
            if ((j > 0) && (!isupper(Map[j - 1][i])) &&
                (getTileKind(Map[j - 1][i]) == TileKind::Floor))
                Edge[num].emplace_back(num - width, 1.0);
            if ((j < height - 1) && (!isupper(Map[j + 1][i])) &&
                (getTileKind(Map[j + 1][i]) == TileKind::Floor))
                Edge[num].emplace_back(num + width, 1.0);
            if ((i > 0) && (!isupper(Map[j][i - 1])) &&
                (getTileKind(Map[j][i - 1]) == TileKind::Floor) &&
                (j > 0) && (!isupper(Map[j - 1][i])) &&
                (getTileKind(Map[j - 1][i]) == TileKind::Floor) &&
                (!isupper(Map[j - 1][i - 1])) &&
                (getTileKind(Map[j - 1][i - 1]) == TileKind::Floor))
                Edge[num].emplace_back(num - 1 - width, sq2);
            if ((i < width - 1) && (!isupper(Map[j][i + 1])) &&
                (getTileKind(Map[j][i + 1]) == TileKind::Floor) &&
                (j < height - 1) && (!isupper(Map[j + 1][i])) &&
                (getTileKind(Map[j + 1][i]) == TileKind::Floor) &&
                (!isupper(Map[j + 1][i + 1])) &&
                (getTileKind(Map[j + 1][i + 1]) == TileKind::Floor))
                Edge[num].emplace_back(num + 1 + width, sq2);
            if ((i < width - 1) && (!isupper(Map[j][i + 1])) &&
                (getTileKind(Map[j][i + 1]) == TileKind::Floor) &&
                (j > 0) && (!isupper(Map[j - 1][i])) &&
                (getTileKind(Map[j - 1][i]) == TileKind::Floor) &&
                (!isupper(Map[j - 1][i + 1])) &&
                (getTileKind(Map[j - 1][i + 1]) == TileKind::Floor))
                Edge[num].emplace_back(num + 1 - width, sq2);
            if ((i > 0) && (!isupper(Map[j][i - 1])) &&
                (getTileKind(Map[j][i - 1]) == TileKind::Floor) &&
                (j < height - 1) && (!isupper(Map[j + 1][i])) &&
                (getTileKind(Map[j + 1][i]) == TileKind::Floor) &&
                (!isupper(Map[j + 1][i - 1])) &&
                (getTileKind(Map[j + 1][i - 1]) == TileKind::Floor))
                Edge[num].emplace_back(num - 1 + width, sq2);
        }
    }
}

// 寻路
bool D_vis[20 * 20] = {};
double D_dis[20 * 20] = {};
int D_pre[20 * 20] = {};

// 此处s应为终点 t为玩家所在格
int Dijkstra(const int s, const int t)
{
    for (int i = 0; i < height * width; i++)
    {
        D_dis[i] = 512.0;
        D_vis[i] = false;
    }
    std::priority_queue<std::pair<double, int>> que = {};
    que.emplace(0, s);
    D_dis[s] = 0;
    D_pre[s] = s;
    while (!que.empty())
    {
        auto cur = que.top().second;
        que.pop();
        if (cur == t)
        {
            D_vis[t] = true;
            break;
        }
        if (!D_vis[cur])
        {
            D_vis[cur] = true;
            for (auto it : Edge[cur])
            {
                if (Unavailable[it.first])
                    continue;
                if (D_dis[it.first] > D_dis[cur] + it.second)
                {
                    D_dis[it.first] = D_dis[cur] + it.second;
                    D_pre[it.first] = cur;
                    que.emplace(-D_dis[it.first], it.first);
                }
            }
        }
    }
    if (!D_vis[t])
        return -1;
    else
        return D_pre[t];
}

int PlayerPosCell(const int op)
{
    int x = floor(Players[op].x);
    int y = floor(Players[op].y);
    return XY_TO_NUM(x, y);
}

int PointInCell(const double &px, const double &py)
{
    int x = floor(px);
    int y = floor(py);
    return XY_TO_NUM(x, y);
}

int CheckPlayerPosCell(const int op)
{
    for (int i = 0; i < width * height; i++)
    {
        Unavailable[i] = false;
    }
    int x = floor(Players[op].x);
    int y = floor(Players[op].y);
    int ret = XY_TO_NUM(x, y);
    int pos = PlayerPosCell(op ^ 1);
    Unavailable[pos] = true;
#ifdef NEOBRAKECONTROL
    pos = PointInCell(Players[op ^ 1].x - 0.35, Players[op ^ 1].y);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x + 0.35, Players[op ^ 1].y);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x, Players[op ^ 1].y - 0.35);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x, Players[op ^ 1].y + 0.35);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x - 0.35, Players[op ^ 1].y - 0.35);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x + 0.35, Players[op ^ 1].y - 0.35);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x - 0.35, Players[op ^ 1].y + 0.35);
    Unavailable[pos] = true;
    pos = PointInCell(Players[op ^ 1].x + 0.35, Players[op ^ 1].y + 0.35);
#endif
    Unavailable[ret] = false;
    return ret;
}
#endif

int NothingTodo(const int op)
{
    double px = Players[op].x;
    double py = Players[op].y;
    double dx = floor(Players[op].x) + 0.5;
    double dy = floor(Players[op].y) + 0.5;
    int ret = 0x10;
    if (px <= dx - 0.15)
    {
        ret |= 0x1;
    }
    else if (px >= dx + 0.15)
    {
        ret |= 0x2;
    }
    if (py <= dy - 0.15)
    {
        ret |= 0x4;
    }
    else if (py >= dy + 0.15)
    {
        ret |= 0x8;
    }
    return ret;
}

// ret 低四位表示移动方向 0001-右 0010-左 0100-下 1000-上 第5位表示刹车
int Move(const int op, const int dx, const int dy)
{
    Task &ct = ptask[op];
    Step &cs = ct.stp[ct.completed];

    // 处理地点冲突则停止移动
    if (cs.ts == TAKING_INGREDIENT_TO_CHOP)
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::None) &&
                (fabs(Entity[i].x - xchoppingstation) < epsilon) &&
                (fabs(Entity[i].y - ychoppingstation) < epsilon) &&
                (!Entity[i].entity.empty()))
            {
#ifdef NEOBRAKECONTROL
                return NothingTodo(op);
#else
                return 0x10;
#endif
            }
        }
    }
    else if (cs.ts == TAKING_INGREDIENT_TO_PAN)
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::Pan) &&
                (!Entity[i].entity.empty()))
            {
#ifdef NEOBRAKECONTROL
                return NothingTodo(op);
#else
                return 0x10;
#endif
            }
        }
    }
    else if (cs.ts == TAKING_INGREDIENT_TO_POT)
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::Pot) &&
                (!Entity[i].entity.empty()))
            {
#ifdef NEOBRAKECONTROL
                return NothingTodo(op);
#else
                return 0x10;
#endif
            }
        }
    }

    double px = Players[op].x;
    double py = Players[op].y;
    int ret = 0;
#ifdef TRUEMOVE
    int pnum = CheckPlayerPosCell(op);
    int dnum = XY_TO_NUM(dx, dy);
    if ((pnum == dnum) &&
        (Players[op].X_Velocity * Players[op].X_Velocity + Players[op].Y_Velocity * Players[op].Y_Velocity > 4))
        return 0x10;
#ifdef SIMPLEBRAKECONTROL
    int ppx = floor(px);
    int ppy = floor(py);
    if ((ppx == 1) && (Players[op].X_Velocity < -1.8))
        return 0x10;
    else if ((ppx == width - 2) && (Players[op].X_Velocity > 1.8))
        return 0x10;
    else if ((ppy == 1) && (Players[op].Y_Velocity < -1.8))
        return 0x10;
    else if ((ppy == height - 2) && (Players[op].Y_Velocity > 1.8))
        return 0x10;
#endif
    int next = Dijkstra(dnum, pnum);
    if (next == -1)
    {
#ifdef NEOBRAKECONTROL
        return NothingTodo(op);
#else
        return 0x10;
#endif
    }
    int nx = next % width;
    int ny = next / width;
#else
    int nx = dx;
    int ny = dy;
#endif
    if (px <= double(nx) + 0.2)
    {
        ret |= 0x1;
    }
    else if (px >= double(nx) + 0.8)
    {
        ret |= 0x2;
    }
    if (py <= double(ny) + 0.2)
    {
        ret |= 0x4;
    }
    else if (py >= double(ny) + 0.8)
    {
        ret |= 0x8;
    }
#ifdef NEOBRAKECONTROL
#ifndef SIMPLEBRAKECONTROL
    int ppx = floor(px);
    int ppy = floor(py);
#endif
    if ((Players[op].X_Velocity < -1) &&
        ((isupper(Map[ppy][ppx - 1]) ||
          (getTileKind(Map[ppy][ppx - 1]) != TileKind::Floor))))
        return (0x10 | (ret & 0xd));
    if ((Players[op].X_Velocity > 1) &&
        ((isupper(Map[ppy][ppx + 1]) ||
          (getTileKind(Map[ppy][ppx + 1]) != TileKind::Floor))))
        return (0x10 | (ret & 0xe));
    if ((Players[op].Y_Velocity < -1) &&
        ((isupper(Map[ppy - 1][ppx]) ||
          (getTileKind(Map[ppy - 1][ppx]) != TileKind::Floor))))
        return (0x10 | (ret & 0x7));
    if ((Players[op].Y_Velocity > 1) &&
        ((isupper(Map[ppy + 1][ppx]) ||
          (getTileKind(Map[ppy + 1][ppx]) != TileKind::Floor))))
        return (0x10 | (ret & 0xb));
    if ((Players[op].X_Velocity < -1) &&
        (Players[op].Y_Velocity < -1) &&
        ((isupper(Map[ppy - 1][ppx - 1])) ||
         (getTileKind(Map[ppy - 1][ppx - 1]) != TileKind::Floor)))
        return 0x10;
    if ((Players[op].X_Velocity > 1) &&
        (Players[op].Y_Velocity < -1) &&
        ((isupper(Map[ppy - 1][ppx + 1])) ||
         (getTileKind(Map[ppy - 1][ppx + 1]) != TileKind::Floor)))
        return 0x10;
    if ((Players[op].X_Velocity < -1) &&
        (Players[op].Y_Velocity > 1) &&
        ((isupper(Map[ppy + 1][ppx - 1])) ||
         (getTileKind(Map[ppy + 1][ppx - 1]) != TileKind::Floor)))
        return 0x10;
    if ((Players[op].X_Velocity > 1) &&
        (Players[op].Y_Velocity > 1) &&
        ((isupper(Map[ppy + 1][ppx + 1])) ||
         (getTileKind(Map[ppy + 1][ppx + 1]) != TileKind::Floor)))
        return 0x10;
#endif
#ifndef TRUEMOVE
    double fx = fabs(px - nx);
    double fy = fabs(py - ny);
    if (fabs(fx - fy) > 1.5)
    {
        if (fx > fy)
            ret &= 0x3;
        else
            ret &= 0xc;
    }
#endif
#ifdef NEOBRAKECONTROL
    if (ret == 0)
        return (0xf & NothingTodo(op));
#endif
    return ret;
}

// ret 低6位表示行动方案 低4位 0001-右 0010-左 0100-下 1000-上 低56位 00-Move 01-Interact 10-PutOrPick
int Action(const int op)
{
    assert(op < 2);
    Task &ct = ptask[op];
    Step &cs = ct.stp[ct.completed];

    int ret = Move(op, cs.desx, cs.desy);
    if (ret != 0)
        return (ret & 0xf);

    if (cs.ta == INTERACT)
        ret |= 0x10;
    else if (cs.ta == TAKE)
        ret |= 0x20;
    else
        assert(0);
    if (cs.d == RIGHT)
        ret |= 0x01;
    else if (cs.d == LEFT)
        ret |= 0x02;
    else if (cs.d == DOWN)
        ret |= 0x04;
    else if (cs.d == UP)
        ret |= 0x08;
    else
        assert(0);

    if (cs.ts == TAKE_UP_PLATE)
    // 释放使用的盘子的坐标
    {
        std::pair<int, int> pos;
        if (cs.d == RIGHT)
            pos = std::make_pair(cs.desx + 1, cs.desy);
        else if (cs.d == LEFT)
            pos = std::make_pair(cs.desx - 1, cs.desy);
        else if (cs.d == DOWN)
            pos = std::make_pair(cs.desx, cs.desy + 1);
        else if (cs.d == UP)
            pos = std::make_pair(cs.desx, cs.desy - 1);
        platefree[op] = pos;
    }
    else if (cs.ts == TAKING_PLATE_TO_PAN)
    // 加工完成前不可以进行拿取
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::Pan) &&
                (!Entity[i].entity.empty()))
            {
                for (auto it : Entity[i].entity)
                {
                    if (it == cs.product)
                        return ret;
                }
            }
        }
        return 0;
    }
    else if (cs.ts == TAKING_PLATE_TO_POT)
    // 加工完成前不可以进行拿取
    {
        for (int i = 0; i < entityCount; i++)
        {
            if ((Entity[i].containerKind == ContainerKind::Pot) &&
                (!Entity[i].entity.empty()))
            {
                for (auto it : Entity[i].entity)
                {
                    if (it == cs.product)
                        return ret;
                }
            }
        }
        return 0;
    }

    return ret;
}

// 碰撞检测
const double playercollisiondistance = 0.7 * 0.7 + 0.02;
bool CollisionDetection(const int fret)
{
    // #ifdef TRUEMOVE
    //     if (((fret & 0x30) == 0) &&
    //         (((fret >> 6) & 0x30) == 0) &&
    //         (Players[0].X_Velocity < 0.4) &&
    //         (Players[0].Y_Velocity < 0.4) &&
    //         (Players[1].X_Velocity < 0.4) &&
    //         (Players[1].Y_Velocity < 0.4) &&
    //         (PlayerPosCell(0) == PlayerPosCell(1)))
    //         return true;
    // #endif
    if ((ptask[0].stp[ptask[0].completed].ts == WASHING) && (ptask[1].stp[ptask[1].completed].ts == WASHING))
        return false;
    if (((fret & 0x30) == 0) &&
        (((fret >> 6) & 0x30) == 0) &&
        (Players[0].X_Velocity < 0.4) &&
        (Players[0].Y_Velocity < 0.4) &&
        (Players[1].X_Velocity < 0.4) &&
        (Players[1].Y_Velocity < 0.4) &&
        (((Players[0].x - Players[1].x) * (Players[0].x - Players[1].x) + (Players[0].y - Players[1].y) * (Players[0].y - Players[1].y)) < playercollisiondistance))
        return true;
    else
        return false;
}

// 碰撞应对
void CollisionAct(const int fret)
{
    int td = 0;
    const int cx0 = floor(Players[0].x);
    const int cx1 = floor(Players[1].x);
    const int cy0 = floor(Players[0].y);
    const int cy1 = floor(Players[1].y);
    const int d0 = fret & 0x0f;
    const int d1 = (fret >> 6) & 0x0f;
    if (((d1 & 0x01) == 0) && (!isupper(Map[cy0][cx0 - 1])) && (getTileKind(Map[cy0][cx0 - 1]) == TileKind::Floor))
        td |= 0x02;
    else if (((d1 & 0x02) == 0) && (!isupper(Map[cy0][cx0 + 1])) && (getTileKind(Map[cy0][cx0 + 1]) == TileKind::Floor))
        td |= 0x01;
    if (((d1 & 0x04) == 0) && (!isupper(Map[cy0 - 1][cx0])) && (getTileKind(Map[cy0 - 1][cx0]) == TileKind::Floor))
        td |= 0x08;
    else if (((d1 & 0x08) == 0) && (!isupper(Map[cy0 + 1][cx0])) && (getTileKind(Map[cy0 + 1][cx0]) == TileKind::Floor))
        td |= 0x04;

    if (((d0 & 0x01) == 0) && (!isupper(Map[cy1][cx1 - 1])) && (getTileKind(Map[cy1][cx1 - 1]) == TileKind::Floor))
        td |= (0x02 << 6);
    else if (((d0 & 0x02) == 0) && (!isupper(Map[cy1][cx1 + 1])) && (getTileKind(Map[cy1][cx1 + 1]) == TileKind::Floor))
        td |= (0x01 << 6);
    else if (((d0 & 0x08) == 0) && (!isupper(Map[cy1 + 1][cx1])) && (getTileKind(Map[cy1 + 1][cx1]) == TileKind::Floor))
        td |= (0x04 << 6);
    else if (((d0 & 0x04) == 0) && (!isupper(Map[cy1 - 1][cx1])) && (getTileKind(Map[cy1 - 1][cx1]) == TileKind::Floor))
        td |= (0x08 << 6);

    CollisionAvoidenceRet = td;
}

// 订单解析相关

// 料理方式解析
int CheckCookMethods(const std::string &s)
{
    if (s == "-chop->")
        return 1;
    else if (s == "-pot->")
        return 2;
    else if (s == "-pan->")
        return 3;
    else
        assert(0);
    return 0;
}

// 初始化菜品辅助数组相关信息
void InitIngredientAss()
{
    ingasscount = IngredientCount;
    for (int i = 0; i < IngredientCount; i++)
    {
        ingass[i].IngName = Ingredient[i].name;
        ingass[i].before = -1;
        ingass[i].cooktime = 0;
        ingass[i].cookmethod = 0;
        ingass[i].x = Ingredient[i].x;
        ingass[i].y = Ingredient[i].y;
        ingass[i].price = Ingredient[i].price;
    }
    for (int i = 0; i < recipeCount; i++)
    {
        for (int j = 0; j < ingasscount; j++)
        {
            if (ingass[j].IngName == Recipe[i].nameBefore)
            {
                ingass[ingasscount].IngName = Recipe[i].nameAfter;
                ingass[ingasscount].before = j;
                ingass[ingasscount].cooktime = ingass[j].cooktime + Recipe[i].time;
                ingass[ingasscount].cookmethod = CheckCookMethods(Recipe[i].kind);
                ingass[ingasscount].x = ingass[j].x;
                ingass[ingasscount].y = ingass[j].y;
                ingass[ingasscount].price = ingass[j].price;
                ingasscount++;
                break;
            }
        }
    }
}

// 初始化拿到菜品所需任务的数组的相关信息
void InitIngredientTask()
{
    std::deque<int> deq;
    int t1 = 0;
    for (int i = 0; i < ingasscount; i++)
    {
        // 初始化一个任务
        deq.clear();
        ingTask[i].completed = 0;
        ingTask[i].stpsum = 0;
        ingTask[i].cooktime = ingass[i].cooktime;
        ingTask[i].panused = false;
        ingTask[i].potused = false;

        // 将当前菜品和所有前驱菜品放入队列中
        deq.emplace_front(i);
        t1 = ingass[i].before;
        while (t1 >= 0)
        {
            deq.emplace_front(t1);
            t1 = ingass[t1].before;
        }

        // 将所有前置任务连起来得到完整的得到当前菜品的任务
        while (!deq.empty())
        {
            t1 = deq.front();
            deq.pop_front();
            switch (ingass[t1].cookmethod)
            {
            case 0:
                // 拿起食材
                CheckInteractPos(ingTask[i].stp[ingTask[i].stpsum], ingass[t1].x, ingass[t1].y);
                ingTask[i].stp[ingTask[i].stpsum].ta = TAKE;
                ingTask[i].stp[ingTask[i].stpsum].ts = GO_TO_INGREDIENT;
                ingTask[i].stpsum++;
                break;
            case 1:
                assert(ingTask[i].stpsum > 0);
                // 食材拿去切
                CheckInteractPos(ingTask[i].stp[ingTask[i].stpsum], xchoppingstation, ychoppingstation);
                ingTask[i].stp[ingTask[i].stpsum].ta = TAKE;
                ingTask[i].stp[ingTask[i].stpsum].ts = TAKING_INGREDIENT_TO_CHOP;
                ingTask[i].stpsum++;
                // 切
                ingTask[i].stp[ingTask[i].stpsum].d = ingTask[i].stp[ingTask[i].stpsum - 1].d;
                ingTask[i].stp[ingTask[i].stpsum].desx = ingTask[i].stp[ingTask[i].stpsum - 1].desx;
                ingTask[i].stp[ingTask[i].stpsum].desy = ingTask[i].stp[ingTask[i].stpsum - 1].desy;
                ingTask[i].stp[ingTask[i].stpsum].ta = INTERACT;
                ingTask[i].stp[ingTask[i].stpsum].ts = CHOPING;
                ingTask[i].stp[ingTask[i].stpsum].product = ingass[t1].IngName;
                ingTask[i].stpsum++;
                // 拿起切好的东西
                ingTask[i].stp[ingTask[i].stpsum].d = ingTask[i].stp[ingTask[i].stpsum - 1].d;
                ingTask[i].stp[ingTask[i].stpsum].desx = ingTask[i].stp[ingTask[i].stpsum - 1].desx;
                ingTask[i].stp[ingTask[i].stpsum].desy = ingTask[i].stp[ingTask[i].stpsum - 1].desy;
                ingTask[i].stp[ingTask[i].stpsum].ta = TAKE;
                ingTask[i].stp[ingTask[i].stpsum].ts = GO_TO_INGREDIENT;
                ingTask[i].stpsum++;
                break;
            case 2:
                assert(ingTask[i].stpsum > 0);
                // 将食材放入Pot
                CheckInteractPos(ingTask[i].stp[ingTask[i].stpsum], xpot, ypot);
                ingTask[i].stp[ingTask[i].stpsum].ta = TAKE;
                ingTask[i].stp[ingTask[i].stpsum].ts = TAKING_INGREDIENT_TO_POT;
                ingTask[i].stp[ingTask[i].stpsum].product = ingass[t1].IngName;
                ingTask[i].stpsum++;
                ingTask[i].potused = true;
                break;
            case 3:
                assert(ingTask[i].stpsum > 0);
                // 将食材放入Pan
                CheckInteractPos(ingTask[i].stp[ingTask[i].stpsum], xpan, ypan);
                ingTask[i].stp[ingTask[i].stpsum].ta = TAKE;
                ingTask[i].stp[ingTask[i].stpsum].ts = TAKING_INGREDIENT_TO_PAN;
                ingTask[i].stp[ingTask[i].stpsum].product = ingass[t1].IngName;
                ingTask[i].stpsum++;
                ingTask[i].panused = true;
                break;
            default:
                assert(0);
                break;
            }
        }
    }
}

// 对所有订单进行解析 解析的任务中含有未确定的目的地 例如盘子的位置是分配时才确定的
void ParseOrder()
{
    OrderTask otsk;
    Task stsk;
    std::vector<int> TaskNotUsePanOrPot;
    std::vector<int> TaskPan;
    std::vector<int> TaskPot;
#ifdef COOPERATIVEDISTRIBUTION
    int tc1 = 0;
#endif
    for (int i = 0; i < totalOrderCount; i++)
    {
        // 初始化
        TaskPan.clear();
        TaskPot.clear();
        TaskNotUsePanOrPot.clear();
        stsk.completed = 0;
        stsk.stpsum = 0;
        stsk.cooktime = 0;

        // 记录配方中的所有食材的编号
        for (auto it : totalOrder[i].recipe)
        {
            for (int j = 0; j < ingasscount; j++)
            {
                if (it == ingass[j].IngName)
                {
                    assert(!(ingTask[j].panused && ingTask[j].potused));
                    if (ingTask[j].panused)
                        TaskPan.emplace_back(j);
                    else if (ingTask[j].potused)
                        TaskPot.emplace_back(j);
                    else
                        TaskNotUsePanOrPot.emplace_back(j);
                    break;
                }
            }
        }

        if (TaskPan.empty() && TaskPot.empty())
        // 若不需要用到Pan以及Pot 则分配给一个人
        {
            for (auto it : TaskNotUsePanOrPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                stsk.stpsum++;
            }
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKE_UP_PLATE;
            stsk.stpsum++;
            CheckInteractPos(stsk.stp[stsk.stpsum], xservicewindows, yservicewindows);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
            stsk.stpsum++;
            totalOrderTaskParse[i].tsk[0] = stsk;
            totalOrderTaskParse[i].tsksum = 1;
            // totalOrderTaskParse[i].playerdistributed = 1;
        }
        else if (TaskPan.empty() && (!TaskPot.empty()))
        // 只需要用到Pot 则分配给一个人
        {
            for (auto it : TaskPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
            }
            for (auto it : TaskNotUsePanOrPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                stsk.stpsum++;
            }
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKE_UP_PLATE;
            stsk.stpsum++;
            CheckInteractPos(stsk.stp[stsk.stpsum], xpot, ypot);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_POT;
            stsk.stp[stsk.stpsum].product = ingTask[TaskPot[TaskPot.size() - 1]].stp[ingTask[TaskPot[TaskPot.size() - 1]].stpsum - 1].product;
            stsk.stpsum++;
            CheckInteractPos(stsk.stp[stsk.stpsum], xservicewindows, yservicewindows);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
            stsk.stpsum++;
            totalOrderTaskParse[i].tsk[0] = stsk;
            totalOrderTaskParse[i].tsksum = 1;
            // totalOrderTaskParse[i].playerdistributed = 1;
        }
        else if ((!TaskPan.empty()) && TaskPot.empty())
        // 只需要用到Pan 则分配给一个人
        {
            for (auto it : TaskPan)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
            }
            for (auto it : TaskNotUsePanOrPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                stsk.stpsum++;
            }
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKE_UP_PLATE;
            stsk.stpsum++;
            CheckInteractPos(stsk.stp[stsk.stpsum], xpan, ypan);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_PAN;
            stsk.stp[stsk.stpsum].product = ingTask[TaskPan[TaskPan.size() - 1]].stp[ingTask[TaskPan[TaskPan.size() - 1]].stpsum - 1].product;
            stsk.stpsum++;
            CheckInteractPos(stsk.stp[stsk.stpsum], xservicewindows, yservicewindows);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
            stsk.stpsum++;
            totalOrderTaskParse[i].tsk[0] = stsk;
            totalOrderTaskParse[i].tsksum = 1;
            // totalOrderTaskParse[i].playerdistributed = 1;
        }
        else if ((!TaskPan.empty()) && (!TaskPot.empty()))
        // 需要同时用到Pan和Pot 暂时仅分给一个人完成
        {
            int pantime = 0, pottime = 0;
            for (auto it : TaskPan)
            {
                pantime += ingTask[it].cooktime;
            }
            for (auto it : TaskPot)
            {
                pottime += ingTask[it].cooktime;
            }
#ifdef COOPERATIVEDISTRIBUTION
            if (pantime >= pottime)
            {
                for (auto it : TaskPan)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
                totalOrderTaskParse[i].tsk[0] = stsk;

                stsk.completed = 0;
                stsk.stpsum = 0;
                stsk.cooktime = 0;
                for (auto it : TaskPot)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
            }
            else
            {
                for (auto it : TaskPot)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
                totalOrderTaskParse[i].tsk[0] = stsk;

                stsk.completed = 0;
                stsk.stpsum = 0;
                stsk.cooktime = 0;
                for (auto it : TaskPan)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
            }
            for (auto it : TaskNotUsePanOrPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                stsk.stpsum++;
            }
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKE_UP_PLATE;
            stsk.stpsum++;
            if (pantime >= pottime)
            {
                CheckInteractPos(stsk.stp[stsk.stpsum], xpot, ypot);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_POT;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPot[TaskPot.size() - 1]].stp[ingTask[TaskPot[TaskPot.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
                CheckInteractPos(stsk.stp[stsk.stpsum], xpan, ypan);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_PAN;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPan[TaskPan.size() - 1]].stp[ingTask[TaskPan[TaskPan.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
            }
            else
            {
                CheckInteractPos(stsk.stp[stsk.stpsum], xpan, ypan);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_PAN;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPan[TaskPan.size() - 1]].stp[ingTask[TaskPan[TaskPan.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
                CheckInteractPos(stsk.stp[stsk.stpsum], xpot, ypot);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_POT;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPot[TaskPot.size() - 1]].stp[ingTask[TaskPot[TaskPot.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
            }
            CheckInteractPos(stsk.stp[stsk.stpsum], xservicewindows, yservicewindows);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
            stsk.stpsum++;
            totalOrderTaskParse[i].tsk[1] = stsk;
            totalOrderTaskParse[i].tsksum = 2;
            // totalOrderTaskParse[i].playerdistributed = 1;
#else
            if (pantime >= pottime)
            {
                for (auto it : TaskPan)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
                for (auto it : TaskPot)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
            }
            else
            {
                for (auto it : TaskPot)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
                for (auto it : TaskPan)
                {
                    stsk.cooktime += ingTask[it].cooktime;
                    for (int j = 0; j < ingTask[it].stpsum; j++)
                    {
                        stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                        stsk.stpsum++;
                    }
                }
            }
            for (auto it : TaskNotUsePanOrPot)
            {
                stsk.cooktime += ingTask[it].cooktime;
                for (int j = 0; j < ingTask[it].stpsum; j++)
                {
                    stsk.stp[stsk.stpsum] = ingTask[it].stp[j];
                    stsk.stpsum++;
                }
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                stsk.stpsum++;
            }
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKE_UP_PLATE;
            stsk.stpsum++;
            if (pantime >= pottime)
            {
                CheckInteractPos(stsk.stp[stsk.stpsum], xpan, ypan);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_PAN;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPan[TaskPan.size() - 1]].stp[ingTask[TaskPan[TaskPan.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
                CheckInteractPos(stsk.stp[stsk.stpsum], xpot, ypot);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_POT;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPot[TaskPot.size() - 1]].stp[ingTask[TaskPot[TaskPot.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
            }
            else
            {
                CheckInteractPos(stsk.stp[stsk.stpsum], xpot, ypot);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_POT;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPot[TaskPot.size() - 1]].stp[ingTask[TaskPot[TaskPot.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
                CheckInteractPos(stsk.stp[stsk.stpsum], xpan, ypan);
                stsk.stp[stsk.stpsum].ta = TAKE;
                stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_PAN;
                stsk.stp[stsk.stpsum].product = ingTask[TaskPan[TaskPan.size() - 1]].stp[ingTask[TaskPan[TaskPan.size() - 1]].stpsum - 1].product;
                stsk.stpsum++;
            }
            CheckInteractPos(stsk.stp[stsk.stpsum], xservicewindows, yservicewindows);
            stsk.stp[stsk.stpsum].ta = TAKE;
            stsk.stp[stsk.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
            stsk.stpsum++;
            totalOrderTaskParse[i].tsk[0] = stsk;
            totalOrderTaskParse[i].tsksum = 1;
            // totalOrderTaskParse[i].playerdistributed = 1;
#endif
        }
        else
            assert(0);
    }
}

// 任务调度和分配相关

// 初始化一些信息 确定盘子的总数和各自的坐标 确定服务台 水槽 盘子归还处坐标 然后确定洗碗事件的基本信息 原本是用来初始化寻路的但是还没写喵
void init_map()
{
    for (int i = 0; i < height * width; i++)
    {
        if ((!isupper(Map[i / width][i % width])) && (getTileKind(Map[i / width][i % width]) == TileKind::ServiceWindow))
        {
            xservicewindows = i % width;
            yservicewindows = i / width;
            break;
        }
    }
    for (int i = 0; i < height * width; i++)
    {
        if ((!isupper(Map[i / width][i % width])) && (getTileKind(Map[i / width][i % width]) == TileKind::Sink))
        {
            xsink = i % width;
            ysink = i / width;
            break;
        }
    }
    for (int i = 0; i < height * width; i++)
    {
        if ((!isupper(Map[i / width][i % width])) && (getTileKind(Map[i / width][i % width]) == TileKind::PlateReturn))
        {
            xplatereturn = i % width;
            yplatereturn = i / width;
            break;
        }
    }
    for (int i = 0; i < height * width; i++)
    {
        if ((!isupper(Map[i / width][i % width])) && (getTileKind(Map[i / width][i % width]) == TileKind::ChoppingStation))
        {
            xchoppingstation = i % width;
            ychoppingstation = i / width;
            break;
        }
    }
    for (int i = 0; i < entityCount; i++)
    {
        if (Entity[i].entity.front() == "Pan")
        {
            xpan = fround(Entity[i].x);
            ypan = fround(Entity[i].y);
            break;
        }
    }
    for (int i = 0; i < entityCount; i++)
    {
        if (Entity[i].entity.front() == "Pot")
        {
            xpot = fround(Entity[i].x);
            ypot = fround(Entity[i].y);
            break;
        }
    }
    WashDirtyPlate.completed = 0;
    WashDirtyPlate.cooktime = 180;
    CheckInteractPos(WashDirtyPlate.stp[0], xplatereturn, yplatereturn);
    WashDirtyPlate.stp[0].ta = TAKE;
    WashDirtyPlate.stp[0].ts = TAKE_UP_DIRTYPLATE;
    CheckInteractPos(WashDirtyPlate.stp[1], xsink, ysink);
    WashDirtyPlate.stp[1].ta = TAKE;
    WashDirtyPlate.stp[1].ts = TAKING_DIRTYPLATE_TO_SINK;
    WashDirtyPlate.stp[2].desx = WashDirtyPlate.stp[1].desx;
    WashDirtyPlate.stp[2].desy = WashDirtyPlate.stp[1].desy;
    WashDirtyPlate.stp[2].d = WashDirtyPlate.stp[1].d;
    WashDirtyPlate.stp[2].ta = INTERACT;
    WashDirtyPlate.stp[2].ts = WASHING;
    WashDirtyPlate.stpsum = 3;

#ifdef WASHPLATESHIFT
    for (int i = 0; i < height * width; i++)
    {
        if ((!isupper(Map[i / width][i % width])) && (getTileKind(Map[i / width][i % width]) == TileKind::PlateRack))
        {
            xplaterack = i % width;
            yplaterack = i / width;
            break;
        }
    }
    if ((xplaterack == 0) || (xplaterack == width - 1))
    {
        xplatewashshift = xplaterack;
        yplatewashshift = 2 * height;
        for (int i = 1; i < height - 1; i++)
        {
            if ((!isupper(Map[i][xplatewashshift])) &&
                (getTileKind(Map[i][xplatewashshift]) == TileKind::Table) &&
                (fabs(i - yplaterack) < fabs(yplatewashshift - yplaterack)))
            {
                if ((xplaterack == 0) &&
                    (!isupper(Map[i][1])) &&
                    (getTileKind(Map[i][1]) == TileKind::Floor))
                {
                    yplatewashshift = i;
                }
                else if ((xplaterack == width - 1) &&
                         (!isupper(Map[i][width - 2])) &&
                         (getTileKind(Map[i][width - 2]) == TileKind::Floor))
                {
                    yplatewashshift = i;
                }
            }
        }
        assert(yplatewashshift != 2 * height);
    }
    else if ((yplaterack == 0) || (yplaterack == height - 1))
    {
        yplatewashshift = yplaterack;
        xplatewashshift = 2 * width;
        for (int i = 1; i < width - 1; i++)
        {
            if (i == xplaterack)
                continue;
            if ((!isupper(Map[yplatewashshift][i])) &&
                (getTileKind(Map[yplatewashshift][i]) == TileKind::Table) &&
                (fabs(i - xplaterack) < fabs(xplatewashshift - xplaterack)))
            {
                if ((yplaterack == 0) &&
                    (!isupper(Map[1][i])) &&
                    (getTileKind(Map[1][i]) == TileKind::Floor))
                {
                    xplatewashshift = i;
                }
                else if ((yplaterack == height - 1) &&
                         (!isupper(Map[height - 2][i])) &&
                         (getTileKind(Map[height - 2][i]) == TileKind::Floor))
                {
                    xplatewashshift = i;
                }
            }
        }
        assert(xplatewashshift != 2 * width);
    }
    else
        assert(0);
    CheckInteractPos(WashDirtyPlate.stp[3], xplaterack, yplaterack);
    WashDirtyPlate.stp[3].ta = TAKE;
    WashDirtyPlate.stp[3].ts = CHECK_PLATE_STACK_TAKE_UP;
    CheckInteractPos(WashDirtyPlate.stp[4], xplatewashshift, yplatewashshift);
    WashDirtyPlate.stp[4].ta = TAKE;
    WashDirtyPlate.stp[4].ts = CHECK_PLATE_STACK_TAKE_DOWN;
    WashDirtyPlate.stpsum = 5;
#endif

    // double dis[(20 + 2) * (20 + 2)][(20 + 2) * (20 + 2)] = {};
    // int pre[(20 + 2) * (20 + 2)][(20 + 2) * (20 + 2)] = {};
    // for (int i = 0; i < height; i++)
    // {
    //     for (int j = 0; j < width; j++)
    //     {
    //         if ((!isupper(Map[i][j])) && (getTileKind(Map[i][j]) == TileKind::ServiceWindow))
    //         {
    //             xservicewindows = i;
    //             yservicewindows = j;
    //             break;
    //         }
    //     }
    // }
    //
    //     for (int i = 0; i < height; i++)
    //     {
    //         for (int j = 0; j < width; j++)
    //         {
    //             int cur = i * width + j;
    //             if ((!isupper(Map[i][j])) && (getTileKind(Map[i][j]) == TileKind::Floor))
    //             {
    //                 if ((i - 1 >= 0) && (j - 1 >= 0) &&
    //                     (getTileKind(Map[i - 1][j - 1]) == TileKind::Floor) &&
    //                     (getTileKind(Map[i - 1][j]) == TileKind::Floor) &&
    //                     (getTileKind(Map[i][j - 1]) == TileKind::Floor))
    //                 {
    //                     dis[cur][cur - width - 1] = sq2;
    //                 }
    //                 if ((i - 1 >= 0) &&
    //                     (getTileKind(Map[i - 1][j]) == TileKind::Floor))
    //                 {
    //                     dis[cur][cur - width] = 1;
    //                 }
    //             }
    //         }
    //     }
}

// 解析当前订单对应于初始订单的编号
int checkOrder(const struct Order &order)
{
    for (int i = 0; i < totalOrderCount; i++)
    {
        if (order.recipe.size() == totalOrder[i].recipe.size())
        {
            for (int j = 0; j < order.recipe.size(); j++)
            {
                checkorderass[j] = false;
            }
            for (int j = 0; j < order.recipe.size(); j++)
            {
                for (int ii = 0; ii < order.recipe.size(); ii++)
                {
                    if ((!checkorderass[j]) && (order.recipe[ii] == totalOrder[i].recipe[j]))
                    {
                        checkorderass[j] = true;
                        break;
                    }
                }
            }
            bool flag = true;
            for (int j = 0; j < order.recipe.size(); j++)
            {
                if (!checkorderass[j])
                {
                    flag = false;
                    break;
                }
            }
            if (flag)
                return i;
        }
    }
    assert(0);
    return -1;
}

// 订单数减少时读取新的订单
void NewOrderToTaskDeque()
{
    while (OrderInDeque < orderCount)
    {
        int t = checkOrder(Order[OrderInDeque]);
        NewdeqOrder.emplace_back(totalOrderTaskParse[t]);
        OrderInDeque++;
    }
}

// 确认归还盘子处是否有脏盘子并设置脏盘标志
void CheckDirtyPlate()
{
    if (dirtyplateflag != NONE)
        return;
    for (int i = 0; i < entityCount; i++)
    {
        if ((Entity[i].containerKind == ContainerKind::DirtyPlates) && (fabs(Entity[i].x - xplatereturn) < epsilon) && (fabs(Entity[i].y - yplatereturn) < epsilon))
        {
            dirtyplateflag = UNDISTRIBUTED;
            return;
        }
    }
}

// 确认场上盘子数量
void CheckPlateNum()
{
    platenum = 0;
    for (int i = 0; i < entityCount; i++)
    {
        if (Entity[i].containerKind == ContainerKind::Plate)
            platenum++;
        else if (Entity[i].containerKind == ContainerKind::DirtyPlates)
        {
            if (!((fabs(Entity[i].x - xplatereturn) < epsilon) && (fabs(Entity[i].y - yplatereturn) < epsilon)))
                platenum++;
        }
    }
    for (int i = 0; i < k; i++)
    {
        if (Players[i].containerKind == ContainerKind::Plate)
            platenum++;
        else if (Players[i].containerKind == ContainerKind::DirtyPlates)
            platenum += atoi(Players[i].entity.front().c_str());
    }
}

// 玩家到交互地点的距离
double DistancePlayerToInteract(const int op, const double ix, const double iy)
{
    double dx = fabs(Players[op].x - (ix + 0.5));
    double dy = fabs(Players[op].y - (iy + 0.5));
    if (dx < dy)
        std::swap(dx, dy);
    return (dy * sq2 + dx - dy);
}

// 确认空闲玩家中和任务交互地点距离以确定任务分配给更近的玩家
int CheckPlayerInteractDistance(Step &stp)
{
    int ret = -1;
    for (int i = 0; i < k; i++)
    {
        if (!FreePlayer[i])
            continue;
        if (ret == -1)
        {
            ret = i;
            continue;
        }
        if (DistancePlayerToInteract(i, double(stp.desx) + 0.5, double(stp.desy) + 0.5) < DistancePlayerToInteract(ret, double(stp.desx) + 0.5, double(stp.desy) + 0.5))
            ret = i;
    }
    assert(ret != -1);
    return ret;
}

// 具体分配
void PlayTaskDistribute()
{
#ifdef COOPERATIVEDISTRIBUTION
    if (FreePlayer[0] && (!PlayerTaskDeque[0].empty()))
    {
        ptask[0] = PlayerTaskDeque[0].front();
        PlayerTaskDeque[0].pop_front();
        FreePlayer[0] = false;
    }
    if (FreePlayer[1] && (!PlayerTaskDeque[1].empty()))
    {
        ptask[1] = PlayerTaskDeque[1].front();
        PlayerTaskDeque[1].pop_front();
        FreePlayer[1] = false;
    }
#endif
    OrderTask otsk;
    Task tsk;
    int nearplayer = 0;
    CheckPlateNum();
    while (FreePlayer[0] || FreePlayer[1])
    {
        if ((dirtyplateflag == UNDISTRIBUTED) && (platenum <= 1))
        {
            dirtyplateflag = DISTRIBUTED;
            nearplayer = CheckPlayerInteractDistance(WashDirtyPlate.stp[0]);
            FreePlayer[nearplayer] = false;
            ptask[nearplayer] = WashDirtyPlate;
            continue;
        }
        otsk = NewdeqOrder.front();
#ifdef COOPERATIVEDISTRIBUTION
        if (otsk.tsksum == 2)
            tsk = otsk.tsk[1];
        else if (otsk.tsksum == 1)
            tsk = otsk.tsk[0];
        else
            assert(0);
#else
        assert(otsk.tsksum == 1);
        tsk = otsk.tsk[0];
#endif
        int flag3 = -1;
        for (int j = 0; j < tsk.stpsum; j++)
        {
            if ((tsk.stp[j].ts == TAKING_INGREDIENT_TO_PLATE) || (tsk.stp[j].ts == TAKE_UP_PLATE))
            {
                if (flag3 == -1)
                {
                    if (CheckPlatePos(tsk.stp[j]))
                        flag3 = j;
                    else
                        break;
                }
                else
                {
                    tsk.stp[j].desx = tsk.stp[flag3].desx;
                    tsk.stp[j].desy = tsk.stp[flag3].desy;
                    tsk.stp[j].d = tsk.stp[flag3].d;
                }
            }
        }
        if (flag3 == -1)
        {
            if (dirtyplateflag == NONE)
            {
                nearplayer = CheckPlayerInteractDistance(WashDirtyPlate.stp[0]);
                FreePlayer[nearplayer] = false;
                ptask[nearplayer] = WashDirtyPlate;
                dirtyplateflag = DISTRIBUTED;
            }
            else if (dirtyplateflag == DISTRIBUTED)
            {
                nearplayer = CheckPlayerInteractDistance(WashDirtyPlate.stp[0]);
                FreePlayer[nearplayer] = false;
                ptask[nearplayer] = WashDirtyPlate;
                dirtyplateflag = TWODISTRIBUTED;
            }
            break;
        }
#ifdef COOPERATIVEDISTRIBUTION
        if (otsk.tsksum == 2)
        {
            nearplayer = CheckPlayerInteractDistance(otsk.tsk[0].stp[0]);
            FreePlayer[nearplayer] = false;
            ptask[nearplayer] = otsk.tsk[0];
            nearplayer = nearplayer ^ 1;
            if (FreePlayer[nearplayer])
            {
                FreePlayer[nearplayer] = false;
                ptask[nearplayer] = tsk;
            }
            else
                PlayerTaskDeque[nearplayer].emplace_back(tsk);
        }
        else
        {
            nearplayer = CheckPlayerInteractDistance(tsk.stp[0]);
            FreePlayer[nearplayer] = false;
            ptask[nearplayer] = tsk;
        }
#else
        nearplayer = CheckPlayerInteractDistance(tsk.stp[0]);
        FreePlayer[nearplayer] = false;
        ptask[nearplayer] = tsk;
#endif
        NewdeqOrder.pop_front();
    }
}

// 具体行动相关

// 初始读入后行为
void InitDo()
{
    init_map();
    InitIngredientAss();
    InitIngredientTask();
    ParseOrder();
    NewdeqOrder.clear();
    ptask[0].stpsum = 0;
    ptask[1].stpsum = 0;
    ptask[0].completed = 0;
    ptask[1].completed = 0;
    ptask[0].stp[0].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
    ptask[1].stp[0].ts = GO_TO_INGREDIENT;
    dirtyplateflag = NONE;
    plateused.clear();
    CollisionAvoidenceTime = 0;
    OrderInDeque = 1;
#ifdef WASHPLATESHIFT
    PlateRackNum = 0;
#endif
#ifdef TRUEMOVE
    initMapEdge();
#endif
#ifdef COOPERATIVEDISTRIBUTION
    PlayerTaskDeque[0].clear();
    PlayerTaskDeque[1].clear();
#endif
}

// 一帧的行为
int FrameDo()
{
    // 碰撞避免
    if (CollisionAvoidenceTime > 0)
    {
        CollisionAvoidenceTime--;
        return CollisionAvoidenceRet;
    }

    // 初始化操作
    int fret = 0;
    for (int i = 0; i < k; i++)
    {
        if (CheckInteractSuc(ptask[i].stp[ptask[i].completed], i))
        {
            if (ptask[i].stp[ptask[i].completed].ts == TAKING_PLATE_TO_SERVICEWINDOWS)
            {
                OrderInDeque--;
                NewOrderToTaskDeque();
            }
#ifdef WASHPLATESHIFT
            else if (ptask[i].stp[ptask[i].completed].ts == TAKING_DIRTYPLATE_TO_SINK)
            {
                PlateRackNum += atoi(Players[i].entity.front().c_str());
            }
            else if (ptask[i].stp[ptask[i].completed].ts == WASHING)
            {
                bool flag5 = false;
                if (PlateRackNum <= 1)
                    flag5 = true;
                for (int j = 0; (j < entityCount) && (!flag5); j++)
                {
                    if ((Entity[j].containerKind == ContainerKind::Plate) && (fabs(Entity[j].x - xplaterack) < epsilon) && (fabs(Entity[j].y - yplaterack) < epsilon))
                    {
                        if (!Entity[j].entity.empty())
                        {
                            flag5 = true;
                            break;
                        }
                    }
                }
                if (flag5)
                {
                    ptask[i].completed += 2;
                    flag5 = true;
                }
                if (!flag5)
                {
                    if ((DistancePlayerToInteract(i ^ 1, xplaterack, yplaterack) > 2) ||
                        (!((ptask[i ^ 1].stp[ptask[i ^ 1].completed].ts == TAKE_UP_PLATE) ||
                           ptask[i ^ 1].stp[ptask[i ^ 1].completed].ts == TAKING_INGREDIENT_TO_PLATE)))
                    {
                        for (int j = 0; j < entityCount; j++)
                        {
                            if ((Entity[j].containerKind == ContainerKind::Plate) && (fabs(Entity[j].x - xplatewashshift) < epsilon) && (fabs(Entity[j].y - yplaterack) < xplatewashshift))
                            {
                                flag5 = true;
                                ptask[i].completed += 2;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ptask[i].completed += 2;
                        flag5 = true;
                    }
                }
                // if (!flag5)
                // {
                //     plateused.emplace(std::make_pair(xplaterack, yplaterack));
                // }
            }
            if (ptask[i].stp[ptask[i].completed].ts == CHECK_PLATE_STACK_TAKE_UP)
            {
                PlateRackNum--;
                // plateused.erase(std::make_pair(xplaterack, yplaterack));
            }
#endif
            ptask[i].completed++;
        }
    }
    CheckDirtyPlate();
    for (int i = 0; i < k; i++)
    {
        platefree[i] = std::make_pair(-1, -1);
    }
    for (int i = 0; i < k; i++)
    {
        if (Players[i].live > 0)
            FreePlayer[i] = false;
        if (ptask[i].completed >= ptask[i].stpsum)
            FreePlayer[i] = true;
        else
            FreePlayer[i] = false;
    }

    // 具体任务分配
    PlayTaskDistribute();

    // 具体行动
    int ret = 0;
    for (int i = 0; i < k; i++)
    {
        if (FreePlayer[i])
            continue;
        ret = Action(i);
        fret |= (ret << (6 * i));
    }
    if (CollisionDetection(fret))
    {
        CollisionAvoidenceTime = 8;
        CollisionAct(fret);
        fret = CollisionAvoidenceRet;
    }

    // 结束操作
    for (int i = 0; i < k; i++)
    {
        if ((platefree[i].first != -1) && (platefree[i].second != -1))
        {
            plateused.erase(platefree[i]);
#ifdef WASHPLATESHIFT
            if ((platefree[i].first == xplaterack) && ((platefree[i].second == yplaterack)))
                PlateRackNum--;
#endif
        }
    }
    return fret;
}

void init_read()
{
    std::string s;
    std::stringstream ss;
    int frame;

    /* 读取初始地图信息 */
    std::getline(std::cin, s, '\0');
    ss << s;

    /* 若按照该读入，访问坐标(x, y)等价于访问Map[y][x],你可按照自己的习惯进行修改 */
    ss >> width >> height;
    std::cerr << "Map size: " << width << "x" << height << std::endl;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            ss >> Map[i][j];

    /* 读入原料箱：位置、名字、以及采购单价 */
    ss >> IngredientCount;
    for (int i = 0; i < IngredientCount; i++)
    {
        ss >> s;
        assert(s == "IngredientBox");
        ss >> Ingredient[i].x >> Ingredient[i].y >> Ingredient[i].name >> Ingredient[i].price;
    }

    /* 读入配方：加工时间、加工前的字符串表示、加工容器、加工后的字符串表示 */
    ss >> recipeCount;
    for (int i = 0; i < recipeCount; i++)
    {
        ss >> Recipe[i].time >> Recipe[i].nameBefore >> Recipe[i].kind >> Recipe[i].nameAfter;
    }

    /* 读入总帧数、当前采用的随机种子、一共可能出现的订单数量 */
    ss >> totalTime >> randomizeSeed >> totalOrderCount;

    /* 读入订单的有效帧数、价格、权重、订单组成 */
    for (int i = 0; i < totalOrderCount; i++)
    {
        ss >> totalOrder[i].validFrame >> totalOrder[i].price >> totalOrder[i].frequency;
        getline(ss, s);
        std::stringstream tmp(s);
        while (tmp >> s)
            totalOrder[i].recipe.push_back(s);
    }

    /* 读入玩家信息：初始坐标 */
    ss >> k;
    assert(k == 2);
    for (int i = 0; i < k; i++)
    {
        ss >> Players[i].x >> Players[i].y;
        Players[i].containerKind = ContainerKind::None;
        Players[i].entity.clear();
    }

    /* 读入实体信息：坐标、实体组成 */
    ss >> entityCount;
    for (int i = 0; i < entityCount; i++)
    {
        ss >> Entity[i].x >> Entity[i].y >> s;
        Entity[i].entity.push_back(s);
    }

    InitDo();
}

bool frame_read(int nowFrame, int &fret)
{
    std::string s;
    std::stringstream ss;
    int frame;
    std::getline(std::cin, s, '\0');
    ss.str(s);
    /*
      如果输入流中还有数据，说明游戏已经在请求下一帧了
      这时候我们应该跳过当前帧，以便能够及时响应游戏。
    */
    if (std::cin.rdbuf()->in_avail() > 0)
    {
        std::cerr << "Warning: skipping frame " << nowFrame
                  << " to catch up with the game" << std::endl;
        return true;
    }
    ss >> s;
    assert(s == "Frame");
    int currentFrame;
    ss >> currentFrame;
    assert(currentFrame == nowFrame);
    ss >> remainFrame >> Fund;
    /* 读入当前的订单剩余帧数、价格、以及配方 */
    ss >> orderCount;
    for (int i = 0; i < orderCount; i++)
    {
        ss >> Order[i].validFrame >> Order[i].price;
        Order[i].recipe.clear();
        getline(ss, s);
        std::stringstream tmp(s);
        while (tmp >> s)
        {
            Order[i].recipe.push_back(s);
        }
    }
    ss >> k;
    assert(k == 2);
    /* 读入玩家坐标、x方向速度、y方向速度、剩余复活时间 */
    for (int i = 0; i < k; i++)
    {
        ss >> Players[i].x >> Players[i].y >> Players[i].X_Velocity >> Players[i].Y_Velocity >> Players[i].live;
        getline(ss, s);
        std::stringstream tmp(s);
        Players[i].containerKind = ContainerKind::None;
        Players[i].entity.clear();
        while (tmp >> s)
        {
            /*
                若若该玩家手里有东西，则接下来一个分号，分号后一个空格，空格后为一个实体。
                以下是可能的输入（省略前面的输入）：
                 ;  : fish
                 ; @  : fish
                 ; @ Plate : fish
                 ; Plate
                 ; DirtyPlates 1
                ...
            */

            /* 若你不需要处理这些，可直接忽略 */
            if (s == ";" || s == ":" || s == "@" || s == "*")
                continue;

            /*
                Todo: 其他容器
            */
            if (s == "Plate")
                Players[i].containerKind = ContainerKind::Plate;
            else if (s == "DirtyPlates")
                Players[i].containerKind = ContainerKind::DirtyPlates;
            else if (s == "Pan")
                Players[i].containerKind = ContainerKind::Pan;
            else if (s == "Pot")
                Players[i].containerKind = ContainerKind::Pot;
            else
                Players[i].entity.push_back(s);
        }
    }

    ss >> entityCount;
    /* 读入实体坐标 */
    for (int i = 0; i < entityCount; i++)
    {
        ss >> Entity[i].x >> Entity[i].y;
        getline(ss, s);
        std::stringstream tmp(s);
        Entity[i].containerKind = ContainerKind::None;
        Entity[i].entity.clear();
        Entity[i].currentFrame = Entity[i].totalFrame = 0;
        Entity[i].sum = 1;
        while (tmp >> s)
        {
            /*
                读入一个实体，例子：
                DirtyPlates 2
                fish
                DirtyPlates 1 ; 15 / 180

            */

            /* 若你不需要处理这些，可直接忽略 */
            if (s == ":" || s == "@" || s == "*")
                continue;
            if (s == ";")
            {
                tmp >> Entity[i].currentFrame >> s >> Entity[i].totalFrame;
                assert(s == "/");
                break;
            }

            /*
                Todo: 其他容器
            */
            if (s == "Plate")
                Entity[i].containerKind = ContainerKind::Plate;
            else if (s == "DirtyPlates")
            {
                Entity[i].containerKind = ContainerKind::DirtyPlates;
                tmp >> Entity[i].sum;
            }
            else if (s == "Pan")
                Entity[i].containerKind = ContainerKind::Pan;
            else if (s == "Pot")
                Entity[i].containerKind = ContainerKind::Pot;
            else
                Entity[i].entity.push_back(s);
        }
    }

    fret = FrameDo();

    return false;
}