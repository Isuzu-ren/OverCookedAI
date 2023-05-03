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

// enum Status
// {
//     FREE,
//     GO_TO_INGREDIENT,
//     TAKING_INGREDIENT_TO_COOK,
//     COOKING,
//     TAKING_INGREDIENT_TO_PLATE,
//     TAKING_PLATE_TO_SERVICEWINDOWS,
//     TAKING_DIRTYPLATE_TO_SINK,
//     WASHING,
//     COLLISION_AVOIDENCE
// };
// static Status p0status, p1status;

enum DirectionInteract
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
    CHECKED
};
enum TypeInteract
{
    TAKE,
    INTERACT
};

enum TypeStep
{
    GO_TO_INGREDIENT,
    TAKING_INGREDIENT_TO_COOK,
    COOKING,
    TAKING_INGREDIENT_TO_PLATE,
    TAKE_UP_PLATE,
    TAKING_PLATE_TO_SERVICEWINDOWS,
    TAKE_UP_DIRTYPLATE,
    TAKING_DIRTYPLATE_TO_SINK,
    WASHING,
    COLLISION_AVOIDENCE
};
struct Step
{
    int desx, desy;      // 终点坐标
    bool descheck;       // 终点是否已确定
    DirectionInteract d; // 交互方向
    TypeInteract ta;     // 交互方式
    TypeStep ts;         // 任务内容
};

struct Task
{
    Step stp[10];   // 每步需要完成的任务
    int stpsum;     // 总步数
    int completed;  // 完成数量
    int plateindex; // 使用的盘子编号
};

std::string IngredientName[5] =
    {"fish", "rice", "kelp"};

std::string ContainerName[5] =
    {"Pan", "Pot", "Plate", "DirtyPlates"};

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

struct Plate
{
    double x, y;
    bool used;          // false表示未被使用
    bool curframecheck; // 当前帧的确认，用于识别
};
int platenum = 0;
Plate platearr[20];

const double epsilon = 1e-4;
void checkplate()
{
    for (int i = 0; i < 20; i++)
    {
        platearr[i].curframecheck = false;
    }
    bool flag1 = false;
    int add = 0;
    for (int i = 0; i < entityCount; i++)
    {
        for (auto it : Entity[i].entity)
        {
            if (it == "Plate")
            {
                flag1 = false;
                for (int j = 0; j < 20; j++)
                {
                    if ((platearr[j].curframecheck == false) &&
                        (fabs(Entity[i].x - platearr[j].x) < epsilon) &&
                        (fabs(Entity[i].y - platearr[j].y) < epsilon))
                    {
                        platearr[j].curframecheck = true;
                        flag1 = true;
                        break;
                    }
                }
                if (!flag1)
                {
                    add++;
                    for (int i = 0; i < 20; i++)
                    {
                        if ((platearr[i].x == -1) || (platearr[i].y == -1))
                        {
                            platearr[i].x = Entity[i].x;
                            platearr[i].y = Entity[i].y;
                            platearr[i].curframecheck = true;
                            platearr[i].used = false;
                            break;
                        }
                    }
                }
            }
        }
    }
    platenum += add;
}

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

// double dis[(20 + 2) * (20 + 2)][(20 + 2) * (20 + 2)] = {};
// int pre[(20 + 2) * (20 + 2)][(20 + 2) * (20 + 2)] = {};

int xservicewindows, yservicewindows;
int xsink, ysink;
int xplatereturn, yplatereturn;
Task WashDirtyPlate;

// 初始化一些信息，确定盘子的总数和各自的坐标，确定服务台坐标和洗盘子水槽坐标
void init_map()
{
    for (int i = 0; i < entityCount; i++)
    {
        for (auto it : Entity[i].entity)
        {
            if (it == "Plate")
            {
                platearr[platenum].used = false;
                platearr[platenum].x = Entity[i].x;
                platearr[platenum].y = Entity[i].y;
                platenum++;
                break;
            }
        }
    }
    for (int i = platenum; i < 20; i++)
    {
        platearr[i].x = -1;
        platearr[i].y = -1;
    }
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
    WashDirtyPlate.completed = 0;
    CheckInteractPos(WashDirtyPlate.stp[0], xplatereturn, yplatereturn);
    WashDirtyPlate.stp[0].descheck = true;
    WashDirtyPlate.stp[0].ta = TAKE;
    WashDirtyPlate.stp[0].ts = TAKE_UP_DIRTYPLATE;
    CheckInteractPos(WashDirtyPlate.stp[1], xsink, ysink);
    WashDirtyPlate.stp[1].descheck = true;
    WashDirtyPlate.stp[1].ta = TAKE;
    WashDirtyPlate.stp[1].ts = TAKING_DIRTYPLATE_TO_SINK;
    WashDirtyPlate.stp[2].desx = WashDirtyPlate.stp[1].desx;
    WashDirtyPlate.stp[2].desy = WashDirtyPlate.stp[1].desy;
    WashDirtyPlate.stp[2].d = WashDirtyPlate.stp[1].d;
    WashDirtyPlate.stp[2].descheck = true;
    WashDirtyPlate.stp[2].ta = INTERACT;
    WashDirtyPlate.stp[2].ts = WASHING;
    WashDirtyPlate.stpsum = 3;
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

    //     const double sq2 = sqrt(2);
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

Task totalOrderParseTask[20 + 5];
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
                task.stpsum++;
                task.stp[task.stpsum].descheck = false;
                task.stp[task.stpsum].ta = TAKE;
                task.stp[task.stpsum].ts = TAKING_INGREDIENT_TO_PLATE;
                task.stpsum++;
                break;
            }
        }
    }
    // 拿起盘子
    task.stp[task.stpsum].descheck = false;
    task.stp[task.stpsum].ta = TAKE;
    task.stp[task.stpsum].ts = TAKE_UP_PLATE;
    task.stpsum++;
    // 将盘子送往服务台
    CheckInteractPos(task.stp[task.stpsum], xservicewindows, yservicewindows);
    task.stp[task.stpsum].descheck = true;
    task.stp[task.stpsum].ta = TAKE;
    task.stp[task.stpsum].ts = TAKING_PLATE_TO_SERVICEWINDOWS;
    task.stpsum++;
    task.completed = 0;
    return task;
}

// ret 低四位表示移动方向 0001-右 0010-左 0100-下 1000-上
int Move(const double px, const double py, const int dx, const int dy)
{
    int ret = 0;
    if (px <= double(dx + 0.35))
    {
        ret |= 0x1;
    }
    else if (px >= double(dx + 0.65))
    {
        ret |= 0x2;
    }
    if (py <= double(dy + 0.35))
    {
        ret |= 0x4;
    }
    else if (py >= double(dy + 0.65))
    {
        ret |= 0x8;
    }
    return ret;
}

int RunningTaskSum = 0;
std::deque<Task> deqOrder;
Task ptask[2 + 2];

bool checkplatepos(Step &stp, const int op)
{
    assert(stp.ts == TAKING_INGREDIENT_TO_PLATE);
    for (int i = 0; i < 20; i++)
    {
        if ((platearr[i].x >= 0) && (platearr[i].y >= 0) && (platearr[i].used = false))
        {
            platearr[i].used = true;
            CheckInteractPos(stp, int(platearr[i].x), int(platearr[i].y));
            stp.descheck = true;
            ptask[op].plateindex = i;
            return true;
        }
    }
    return false;
}

// ret 低6位表示行动方案 低4位 0001-右 0010-左 0100-下 1000-上 低56位 00-Move 01-Interact 10-PutOrPick
int Action(const int op)
{
    assert(op < 2);
    Task &ct = ptask[op];
    int ret = Move(Players[op].x, Players[op].y, ct.stp[ct.completed].desx, ct.stp[ct.completed].desy);
    if (ret != 0)
        return ret;
    if (ct.stp[ct.completed].ta == INTERACT)
        ret |= 0x10;
    else if (ct.stp[ct.completed].ta == TAKE)
        ret |= 0x20;
    else
        assert(0);
    if (ct.stp[ct.completed].d == RIGHT)
        ret |= 0x01;
    else if (ct.stp[ct.completed].d == LEFT)
        ret |= 0x02;
    else if (ct.stp[ct.completed].d == DOWN)
        ret |= 0x04;
    else if (ct.stp[ct.completed].d == UP)
        ret |= 0x08;
    else
        assert(0);
    ct.completed++;
    if (ct.completed == ct.stpsum)
        RunningTaskSum--;
    return ret;
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

void OrderToTaskDeque()
{
    if (RunningTaskSum + deqOrder.size() >= orderCount)
        return;
    for (int i = deqOrder.size() + RunningTaskSum; i < orderCount; i++)
    {
        int t = checkOrder(Order[i]);
        deqOrder.emplace_back(totalOrderParseTask[t]);
    }
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
    RunningTaskSum = 0;
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

    OrderToTaskDeque();
    // std::cout << deqOrder.size() << std::endl;
    // for (auto it : deqOrder)
    // {
    //     for (int i = 0; i < it.stpsum; i++)
    //     {
    //         std::cout << it.stp[i].desx << ' ' << it.stp[i].desy << std::endl;
    //     }
    // }
    fret = 0;
    checkplate();
    Task temptask;
    for (int i = 0; i < k; i++)
    {
        if (Players[i].live > 0)
            continue;
        assert(0);
        if (ptask[i].completed >= ptask[i].stpsum)
        {
            temptask = deqOrder.front();
            bool flag3 = false;
            double curplatex, curplatey;
            for (int j = 0; j < temptask.stpsum; j++)
            {
                if (temptask.stp[j].ts == TAKING_INGREDIENT_TO_PLATE)
                {
                    if (!flag3)
                    {
                        if (checkplatepos(temptask.stp[j], k))
                            flag3 = true;
                        else
                            break;
                    }
                    else
                        CheckInteractPos(temptask.stp[j], platearr[temptask.plateindex].x, platearr[temptask.plateindex].y);
                }
            }
            if (!flag3)
                continue;
            deqOrder.pop_front();
            ptask[i] = temptask;
            RunningTaskSum++;
        }

        std::cout << i << " : " << ptask[i].plateindex << " " << ptask[i].completed << " " << ptask[i].stpsum << std::endl;
        for (int j = 0; j < ptask[i].stpsum; j++)
        {
            std::cout << ptask[i].stp[j].desx << " " << ptask[i].stp[j].desy;
        }

        int ret = Action(i);
        fret |= (ret << (6 * i));
    }

    return false;
}