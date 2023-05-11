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
#include <cctype>
#include <cmath>
#include <set>

struct Step
{
    int desx, desy;      // 终点坐标
    bool descheck;       // 终点是否已确定
    bool stay;           // 是否需要停在终点
    DirectionInteract d; // 交互方向
    TypeInteract ta;     // 交互方式
    TypeStep ts;         // 任务内容
};

struct Task
{
    Step stp[10];                 // 每步需要完成的任务
    int stpsum;                   // 总步数
    int completed;                // 完成数量
    std::pair<int, int> platepos; // 使用的盘子的坐标
};

struct Plate
{
    double x, y;
    bool used;          // false表示未被使用
    bool curframecheck; // 当前帧的确认，用于识别
};

// std::string IngredientName[5] =
//     {"fish", "rice", "kelp"};

// std::string ContainerName[5] =
//     {"Pan", "Pot", "Plate", "DirtyPlates"};

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
Task WashDirtyPlate;                     // 洗盘子任务
std::set<std::pair<int, int>> plateused; // 已分配的盘子的坐标
std::pair<int, int> platefree[2];        // 当前帧需要释放的分配盘子坐标
PlateFlag dirtyplateflag;                // 脏盘标志
int platenum = 0;                        // 盘子总数
std::deque<Task> deqOrder;               // 订单任务队列
Task ptask[2 + 2];                       // 玩家正在执行的任务
Task ptaskbackup[2 + 2];                 // 死亡时重新分配的备份
Task totalOrderParseTask[20 + 5];        // 订单解析数组
bool FreePlayer[2] = {};                 // 当前帧玩家是否空闲

// 抛弃或暂无用的全局变量
// int RunningTaskSum = 0;
Plate platearr[20];

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
    {
        assert(0);
    }
    stp.descheck = true;
}

// 确认交互是否成功
bool CheckInteractSuc(Step &stp, const int op)
{
    if (stp.ts == GO_TO_INGREDIENT)
        return (!Players[op].entity.empty());
    else if ((stp.ts == TAKING_INGREDIENT_TO_PLATE) || (stp.ts == TAKING_INGREDIENT_TO_COOK_OR_CUT))
        return Players[op].entity.empty();
    else if ((stp.ts == TAKE_UP_PLATE) || (stp.ts == TAKING_PLATE_TO_PAN_OR_POT))
        return (Players[op].containerKind == ContainerKind::Plate);
    else if (stp.ts == TAKING_PLATE_TO_SERVICEWINDOWS)
        return (Players[op].containerKind == ContainerKind::None);
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
    else
        return false;
}

// 分配一个可用的盘子
bool CheckPlatePos(Step &stp)
{
    assert(stp.ts == TAKING_INGREDIENT_TO_PLATE);
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
                stp.descheck = true;
                plateused.emplace(pos);
                return true;
            }
        }
    }
    return false;
}

// 移动和行动相关

// ret 低四位表示移动方向 0001-右 0010-左 0100-下 1000-上
int Move(const double px, const double py, const int dx, const int dy)
{
    int ret = 0;
    if (px <= double(dx + 0.15))
    {
        ret |= 0x1;
    }
    else if (px >= double(dx + 0.85))
    {
        ret |= 0x2;
    }
    if (py <= double(dy + 0.15))
    {
        ret |= 0x4;
    }
    else if (py >= double(dy + 0.85))
    {
        ret |= 0x8;
    }
    return ret;
}

// ret 低6位表示行动方案 低4位 0001-右 0010-左 0100-下 1000-上 低56位 00-Move 01-Interact 10-PutOrPick
int Action(const int op)
{
    assert(op < 2);
    Task &ct = ptask[op];
    Step &cs = ct.stp[ct.completed];
    int ret = Move(Players[op].x, Players[op].y, cs.desx, cs.desy);
    // std :: cout << op << " " << Players[op].x << " " << Players[op].y << " " << ct.stp[ct.completed].desx << " " << ct.stp[ct.completed].desy << " ret= " << ret << "\n";
    if (ret != 0)
        return ret;

    bool flag4;
    if (cs.ts == WASHING)
    {
        if (cs.descheck)
            cs.descheck = false;
        else
        {
            flag4 = false;
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
                // ct.completed++;
                dirtyplateflag = NONE;
            }
        }
    }
    // else if (cs.ts == TAKE_UP_DIRTYPLATE)
    // {
    // }

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
    // if (cs.ts != WASHING)
    //     ct.completed++;
    if (cs.ts == TAKING_PLATE_TO_SERVICEWINDOWS)
        deqOrder.pop_front();
    // if (ct.completed == ct.stpsum)
    //     RunningTaskSum--;
    return ret;
}

// 订单解析相关

// 该函数只负责最初解析订单所需要做的事，目前一个订单对应一个任务，但之后可能修改为更适合合作完成的分解小任务，其解析的任务中含有未确定的目标，例如并未确定盘子要在何处取，当任务真正被分配然后执行的时候才会确定
Task ParseOrder(const struct Order &order)
{
    Task task;
    task.stpsum = 0;
    // 取食材，并放到盘子里
    for (auto it : order.recipe)
    {
        for (int i = 0; i < IngredientCount; i++)
        {
            if (Ingredient[i].name == it)
            {
                CheckInteractPos(task.stp[task.stpsum], Ingredient[i].x, Ingredient[i].y);
                task.stp[task.stpsum].descheck = true;
                task.stp[task.stpsum].ta = TAKE;
                task.stp[task.stpsum].ts = GO_TO_INGREDIENT;
                task.stp[task.stpsum].stay = false;
                task.stpsum++;
                task.stp[task.stpsum].descheck = false;
                task.stp[task.stpsum].ta = TAKE;
                task.stp[task.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                task.stp[task.stpsum].stay = false;
                task.stpsum++;
                break;
            }
        }
    }
    // 拿起盘子
    task.stp[task.stpsum].descheck = false;
    task.stp[task.stpsum].ta = TAKE;
    task.stp[task.stpsum].ts = TAKE_UP_PLATE;
    task.stp[task.stpsum].stay = false;
    task.stpsum++;
    // 将盘子送往服务台
    CheckInteractPos(task.stp[task.stpsum], xservicewindows, yservicewindows);
    task.stp[task.stpsum].descheck = true;
    task.stp[task.stpsum].ta = TAKE;
    task.stp[task.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
    task.stp[task.stpsum].stay = false;
    task.stpsum++;
    task.completed = 0;
    return task;
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
    for (int i = 0; i < entityCount; i++)
    {
        if (Entity[i].containerKind == ContainerKind::Plate)
        {
            platenum++;
        }
    }
    WashDirtyPlate.completed = 0;
    CheckInteractPos(WashDirtyPlate.stp[0], xplatereturn, yplatereturn);
    WashDirtyPlate.stp[0].descheck = true;
    WashDirtyPlate.stp[0].ta = TAKE;
    WashDirtyPlate.stp[0].ts = TAKE_UP_DIRTYPLATE;
    WashDirtyPlate.stp[0].stay = false;
    CheckInteractPos(WashDirtyPlate.stp[1], xsink, ysink);
    WashDirtyPlate.stp[1].descheck = true;
    WashDirtyPlate.stp[1].ta = TAKE;
    WashDirtyPlate.stp[1].ts = TAKING_DIRTYPLATE_TO_SINK;
    WashDirtyPlate.stp[0].stay = false;
    WashDirtyPlate.stp[2].desx = WashDirtyPlate.stp[1].desx;
    WashDirtyPlate.stp[2].desy = WashDirtyPlate.stp[1].desy;
    WashDirtyPlate.stp[2].d = WashDirtyPlate.stp[1].d;
    WashDirtyPlate.stp[2].descheck = true;
    WashDirtyPlate.stp[2].ta = INTERACT;
    WashDirtyPlate.stp[2].ts = WASHING;
    WashDirtyPlate.stp[2].stay = true;
    WashDirtyPlate.stpsum = 3;

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

    // for (int i = 0; i < entityCount; i++)
    // {
    //     if (Entity[i].containerKind == ContainerKind::Plate)
    //     {
    //         platearr[platenum].used = false;
    //         platearr[platenum].x = Entity[i].x;
    //         platearr[platenum].y = Entity[i].y;
    //         platenum++;
    //     }
    // }
    // for (int i = platenum; i < 20; i++)
    // {
    //     platearr[i].x = -1;
    //     platearr[i].y = -1;
    // }
}

int checkOrder(const struct Order &order)
{
    for (int i = 0; i < totalOrderCount; i++)
    {
        if (order.recipe == totalOrder[i].recipe)
            return i;
    }
    assert(0);
    return -1;
}

// 订单数减少时读取新的订单
void OrderToTaskDeque()
{
    if (deqOrder.size() < orderCount)
    {
        for (int i = deqOrder.size(); i < orderCount; i++)
        {
            int t = checkOrder(Order[i]);
            deqOrder.emplace_back(totalOrderParseTask[t]);
        }
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

// 玩家到交互地点的距离
double DistancePlayerToInteract(const int op, const double ix, const double iy)
{
    double dx = fabs(Players[op].x - ix);
    double dy = fabs(Players[op].y - iy);
    if (dx < dy)
        std::swap(dx, dy);
    return (dy * sq2 + dx - dy);
}

// 确认玩家和第一个交互地点距离以确定任务分配给更近的玩家
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

// 具体行动相关

void InitDo()
{
    init_map();
    for (int i = 0; i < totalOrderCount; i++)
    {
        totalOrderParseTask[i] = ParseOrder(totalOrder[i]);
    }
    deqOrder.clear();
    ptask[0].stpsum = 0;
    ptask[1].stpsum = 0;
    ptask[0].completed = 0;
    ptask[1].completed = 0;
    // RunningTaskSum = 0;
    dirtyplateflag = NONE;
    plateused.clear();
}

int FrameDo()
{
    // 初始化操作
    int fret = 0;
    OrderToTaskDeque();
    for (int i = 0; i < k; i++)
    {
        if (CheckInteractSuc(ptask[i].stp[ptask[i].completed], i))
            ptask[i].completed++;
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

    // 具体分配
    Task temptask;
    int nearplayer = 0;
    while (true)
    {
        if (!(FreePlayer[0] || FreePlayer[1]))
            break;
        if (dirtyplateflag == UNDISTRIBUTED)
        {
            dirtyplateflag = DISTRIBUTED;
            nearplayer = CheckPlayerInteractDistance(WashDirtyPlate.stp[0]);
            FreePlayer[nearplayer] = false;
            ptask[nearplayer] = WashDirtyPlate;
            continue;
        }
        temptask = deqOrder.front();
        int flag3 = -1;
        for (int j = 0; j < temptask.stpsum; j++)
        {
            if ((temptask.stp[j].ts == TAKING_INGREDIENT_TO_PLATE) || (temptask.stp[j].ts == TAKE_UP_PLATE))
            {
                if (flag3 == -1)
                {
                    if (CheckPlatePos(temptask.stp[j]))
                        flag3 = j;
                    else
                        break;
                }
                else
                {
                    temptask.stp[j].descheck = true;
                    temptask.stp[j].desx = temptask.stp[flag3].desx;
                    temptask.stp[j].desy = temptask.stp[flag3].desy;
                    temptask.stp[j].d = temptask.stp[flag3].d;
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
        nearplayer = CheckPlayerInteractDistance(temptask.stp[0]);
        FreePlayer[nearplayer] = false;
        ptask[nearplayer] = temptask;
    }

    // 具体行动
    int ret = 0;
    for (int i = 0; i < k; i++)
    {
        if (FreePlayer[i])
            continue;
        ret = Action(i);
        fret |= (ret << (6 * i));
    }

    // 结束操作
    for (int i = 0; i < k; i++)
    {
        if ((platefree[i].first != -1) && (platefree[i].second != -1))
            plateused.erase(platefree[i]);
    }
    return fret;

    // for (int i = 0; i < k; i++)
    // {
    //     if (Players[i].live > 0)
    //         continue;
    //     if (ptask[i].completed >= ptask[i].stpsum)
    //     {
    //         if (dirtyplateflag == UNDISTRIBUTED)
    //         {
    //             dirtyplateflag = DISTRIBUTED;
    //             ptask[i] = WashDirtyPlate;
    //             // RunningTaskSum++;
    //             continue;
    //         }
    //         temptask = deqOrder.front();
    //         int flag3 = -1;
    //         // double curplatex, curplatey;
    //         for (int j = 0; j < temptask.stpsum; j++)
    //         {
    //             if ((temptask.stp[j].ts == TAKING_INGREDIENT_TO_PLATE) || (temptask.stp[j].ts == TAKE_UP_PLATE))
    //             {
    //                 if (flag3 == -1)
    //                 {
    //                     if (CheckPlatePos(temptask.stp[j], i))
    //                         flag3 = j;
    //                     else
    //                         break;
    //                 }
    //                 else
    //                 {
    //                     temptask.stp[j].descheck = true;
    //                     temptask.stp[j].desx = temptask.stp[flag3].desx;
    //                     temptask.stp[j].desy = temptask.stp[flag3].desy;
    //                     temptask.stp[j].d = temptask.stp[flag3].d;
    //                 }
    //             }
    //         }
    //         if (flag3 == -1)
    //             continue;
    //         ptask[i] = temptask;
    //         // RunningTaskSum++;
    //     }
    //     int ret = Action(i);
    //     fret |= (ret << (6 * i));
    // }
    // for (int i = 0; i < k; i++)
    // {
    //     if ((platefree[i].first != -1) && (platefree[i].second != -1))
    //         plateused.erase(platefree[i]);
    // }
    // return fret;

    // std::cout << i << " : " << ptask[i].plateindex << " " << ptask[i].completed << " " << ptask[i].stpsum << std::endl;
    // for (int j = 0; j < ptask[i].stpsum; j++)
    // {
    //     std::cout << ptask[i].stp[j].desx << " " << ptask[i].stp[j].desy << std::endl;
    // }
}

// void checkplate()
// {
//     for (int i = 0; i < 20; i++)
//     {
//         platearr[i].curframecheck = false;
//     }
//     bool flag1 = false;
//     int add = 0;
//     for (int i = 0; i < entityCount; i++)
//     {
//         if (Entity[i].containerKind == ContainerKind::Plate)
//         {
//             flag1 = false;
//             for (int j = 0; j < 20; j++)
//             {
//                 if ((platearr[j].curframecheck == false) &&
//                     (fabs(Entity[i].x - platearr[j].x) < epsilon) &&
//                     (fabs(Entity[i].y - platearr[j].y) < epsilon))
//                 {
//                     platearr[j].curframecheck = true;
//                     flag1 = true;
//                     break;
//                 }
//             }
//             if (!flag1)
//             {
//                 add++;
//                 for (int i = 0; i < 20; i++)
//                 {
//                     if ((platearr[i].x == -1) || (platearr[i].y == -1))
//                     {
//                         platearr[i].x = Entity[i].x;
//                         platearr[i].y = Entity[i].y;
//                         platearr[i].curframecheck = true;
//                         platearr[i].used = false;
//                         break;
//                     }
//                 }
//             }
//         }
//     }
//     platenum = 0;
//     for (int i = 0; i < 20; i++)
//     {
//         if (platearr[i].curframecheck)
//             platenum++;
//         else
//         {
//             platearr[i].x = -1;
//             platearr[i].y = -1;
//         }
//     }
//     // platenum += add;
// }

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
            else
                Entity[i].entity.push_back(s);
        }
    }

    fret = FrameDo();
    // std::cout << deqOrder.size() << std::endl;
    // for (auto it : deqOrder)
    // {
    //     for (int i = 0; i < it.stpsum; i++)
    //     {
    //         std::cout << it.stp[i].desx << ' ' << it.stp[i].desy << std::endl;
    //     }
    // }
    // std::cout << "Frame " << nowFrame << "\n";
    // std::cout << platenum << std::endl;
    // for (int i = 0; i < platenum; i++)
    // {
    //     std::cout << platearr[i].x << " " << platearr[i].y << std::endl;
    // }

    // checkplate();

    // std::cout << "Frame " << nowFrame << "\n";
    // std::cout << platenum << std::endl;
    // for (int i = 0; i < platenum; i++)
    // {
    //     std::cout << platearr[i].x << " " << platearr[i].y << std::endl;
    // }

    // std::cout << "fret=" << fret << std::endl;

    return false;
}