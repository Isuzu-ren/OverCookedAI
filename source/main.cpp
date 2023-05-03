#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <string>
#include <framework.h>

int main()
{
    std::ios::sync_with_stdio(false);
    std::cerr.tie(nullptr);
    std::cerr << std::nounitbuf;
    std::string s;
    std::stringstream ss;
    int frame;

    init_read();

    /*
        你可以在读入后进行一些相关预处理，时间限制：5秒钟
        init();
    */
    std::string action;
    int fret;
    int totalFrame = 14400;
    for (int i = 0; i < totalFrame; i++)
    {
        bool skip = frame_read(i, fret);
        if (skip)
            continue;

        /* 输出当前帧的操作，此处仅作示例 */
        std::cout << "Frame " << i << "\n";

        action = "";
        if ((fret & 0x30) == 0)
            action = action + "Move ";
        else if ((fret & 0x30) == 0x10)
            action = action + "Interact ";
        else if ((fret & 0x30) == 0x20)
            action = action + "PutOrPick ";
        if ((fret & 0x0f) == 0x01)
            action = action + "R";
        if ((fret & 0x0f) == 0x02)
            action = action + "L";
        if ((fret & 0x0f) == 0x04)
            action = action + "D";
        if ((fret & 0x0f) == 0x08)
            action = action + "U";
        action = action + "\n";
        fret >>= 6;
        if ((fret & 0x30) == 0)
            action = action + "Move ";
        else if ((fret & 0x30) == 0x10)
            action = action + "Interact ";
        else if ((fret & 0x30) == 0x20)
            action = action + "PutOrPick ";
        if ((fret & 0x0f) == 0x01)
            action = action + "R";
        if ((fret & 0x0f) == 0x02)
            action = action + "L";
        if ((fret & 0x0f) == 0x04)
            action = action + "D";
        if ((fret & 0x0f) == 0x08)
            action = action + "U";
        action = action + "\n";

        /* 合成一个字符串再输出，否则输出有可能会被打断 */
        std::cout << action << std::endl;

        /* 不要忘记刷新输出流，否则游戏将无法及时收到响应 */
        std::cout.flush();
    }
}
