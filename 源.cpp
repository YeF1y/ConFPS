#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
using namespace std;

#include <Windows.h>

int nScreenWidth = 120;//屏幕宽度
int nScreenHeight = 40;//端幕高度

//玩家位置和视角角度，不能用整数，防止其走路的时候一顿一顿，起始位置在房间中间，而不是在墙里
float fPlayerX = 8.0f;//左右
float fPlayerY = 8.0f;//上下
float fPlayerA = 0.0f;

//小地图，二维数组
int nMapHeight = 16;
int nMapWidth = 16;

//现场变量
float fFOV = 3.14159 / 4.0;

//变量深度
float fDepth = 16.0f;

int main() {
	//创建WR类型屏幕,控制台缓冲区
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];//屏幕数组
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;

	map += L"################";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#          #   #";
	map += L"#          #   #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#        #######";
	map += L"#              #";
	map += L"################";


	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	//游戏循环
	while (1) {
		//抓住时间控制帧率
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		//让视野旋转
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (0.8f) * fElapsedTime;

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (0.8f) * fElapsedTime;

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
			//当我们撞到墙了就撤回操作，加就减掉
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sin(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY -= cos(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sin(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY += cos(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		//横向移动视野时，相当于屏幕宽度的120次视野投影，作为玩家所看到的地图
		for (int x = 0; x < nScreenWidth; x++) {
			//玩家视野是视野角的中线，所以减去一半的视野角再加上视野变量
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			//单位变量代表玩家看的方向
			//注意：sin 函数对应的参数是弧度值，并非角度值，弧度和角度相互转换原理：弧度 = 角度 * 圆周率 / 180.0
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				//测试视野是否超出边界
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else {
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;
						//利用一面墙的四个角的视线投影的矢量算法测试碰壁
						vector<pair<float, float>> p;//存储距离和点积
						for(int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						//lambda排序向量
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) { return left.first < right.first; });

						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						//if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}
			//计算天花板和地板的数量，因为越远天花板和地板越大也越少
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';
			short cShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;//最暗
			else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
			else if (fDistanceToWall < fDepth)				nShade = 0x2591;
			else                                            nShade = ' ';   //最远

			if (bBoundary)									nShade = ' ';

			for (int y = 0; y < nScreenHeight; y++) {
				if (y < nCeiling)
					screen[y * nScreenWidth + x] = ' ';
				else if(y > nCeiling && y <= nFloor)
					screen[y * nScreenWidth + x] = nShade;
				else {
					//地板的阴影以区分远近
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)			cShade = '#';
					else if (b < 0.5)		cShade = 'x';
					else if (b < 0.75)		cShade = '.';
					else if (b < 0.9)		cShade = '-';
					else					cShade = ' ';
					screen[y * nScreenWidth + x] = cShade;
				}
			}
		}

		//数据统计
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		//小地图
		for(int nx = 0; nx < nMapWidth; nx++)
			for (int ny = 0; ny < nMapWidth; ny++) {
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}

		//角色位置P
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0';//末尾字符，让他知道什么时候停止
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}


	return 0;
}