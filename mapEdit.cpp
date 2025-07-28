#include <windows.h>
#include "MapEdit.h"
#include <cassert>
#include "Input.h"
#include "DxLib.h"
#include "MapChip.h"
#include <fstream>
#include <codecvt>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>


MapEdit::MapEdit()
	:GameObject(), myMap_(MAP_WIDTH* MAP_HEIGHT, -1), //初期値を-1で20*20の配列を初期化する
	isInMapEditArea_(false), //マップエディタ領域内にいるかどうか
	drawAreaRect_{0, 0, 0, 0} // drawAreaRect_を初期化
{
	mapEditRect_ = { LEFT_MARGIN, TOP_MARGIN,
		MAP_WIDTH * MAP_IMAGE_SIZE, MAP_HEIGHT * MAP_IMAGE_SIZE };
}

MapEdit::~MapEdit()
{
}

void MapEdit::SetMap(Point p, int value)
{
	//マップの座標pにvalueをセットする
	//pが、配列の範囲外の時はassertにひっかかる
	assert(p.x >= 0 && p.x < MAP_WIDTH);
	assert(p.y >= 0 && p.y < MAP_HEIGHT);
	myMap_[p.y * MAP_WIDTH + p.x] = value; //y行x列にvalueをセットする

}

int MapEdit::GetMap(Point p) const
{
	//マップの座標pの値を取得する
	//pが、配列の範囲外の時はassertにひっかかる
	assert(p.x >= 0 && p.x < MAP_WIDTH);
	assert(p.y >= 0 && p.y < MAP_HEIGHT);
	return myMap_[p.y * MAP_WIDTH + p.x]; //y行x列の値を取得する

}

void MapEdit::Update()
{
	Point mousePos;
	if (GetMousePoint(&mousePos.x, &mousePos.y) == -1) {
		return;
	}
	// マウスの座標がマップエディタ領域内にいるかどうかを判定する
	isInMapEditArea_ = mousePos.x >= mapEditRect_.x && mousePos.x <= mapEditRect_.x + mapEditRect_.w &&
		mousePos.y >= mapEditRect_.y && mousePos.y <= mapEditRect_.y + mapEditRect_.h;

	//左上　mapEditRect_.x, mapEditRect_.y
	//右上　mapEditRect_.x + mapEditRect_.w, mapEditRect_.y
	//右下  mapEditRect_.x + mapEditRect_.w, mapEditRect_.y + mapEditRect_.h
	//左下  mapEditRect_.x, mapEditRect_.y + mapEditRect_.h
		// グリッド座標に変換
	if (!isInMapEditArea_) return;

	int gridX = (mousePos.x - LEFT_MARGIN) / MAP_IMAGE_SIZE;
	int gridY = (mousePos.y - TOP_MARGIN) / MAP_IMAGE_SIZE;

	if (gridX < 0 || gridX >= MAP_WIDTH || gridY < 0 || gridY >= MAP_HEIGHT) return;

	drawAreaRect_ = { LEFT_MARGIN + gridX * MAP_IMAGE_SIZE, TOP_MARGIN + gridY * MAP_IMAGE_SIZE,
		MAP_IMAGE_SIZE, MAP_IMAGE_SIZE };

	if (multiDeleteMode_) {
		if (Input::IsButtonDown(MOUSE_INPUT_LEFT)) {
			Point p{ gridX, gridY };
			auto it = std::find(selectedForDelete_.begin(), selectedForDelete_.end(), p);
			if (it == selectedForDelete_.end()) {
				selectedForDelete_.push_back(p); // 選択
			} else {
				selectedForDelete_.erase(it); // 解除
			}
		}
		// 削除実行（例：Enterキーで削除）
		if (Input::IsKeyDown(KEY_INPUT_RETURN)) {
			for (const auto& p : selectedForDelete_) {
				SetMap(p, -1);
			}
			selectedForDelete_.clear();
		}
		return; // 通常処理はスキップ
	}

	if (Input::IsButtonKeep(MOUSE_INPUT_LEFT)) {
		MapChip* mapChip = FindGameObject<MapChip>();

		if (CheckHitKey(KEY_INPUT_LSHIFT)) //Rキーを押しているなら
		{
			SetMap({ gridX, gridY }, -1); //マップに値をセット（-1は何もない状態）
			return; //マップチップを削除したらここで終了
		}
		else if (mapChip && mapChip->IsHold()) //マップチップを持っているなら
		{
			SetMap({ gridX, gridY }, mapChip->GetHoldImage()); //マップに値をセット
		}
	}
	if (Input::IsKeyDown(KEY_INPUT_S))
	{
		SaveMapData();
	}
	if (Input::IsKeyDown(KEY_INPUT_L))
	{
		LoadMapData();
	}

	// Dキーで複数削除モード切替
	if (Input::IsKeyDown(KEY_INPUT_D)) {
		multiDeleteMode_ = !multiDeleteMode_;
		selectedForDelete_.clear();
	}

}

void MapEdit::Draw()
{
	// 全マップを表示
	for (int j = 0; j < MAP_HEIGHT; j++)
	{
		for (int i = 0; i < MAP_WIDTH; i++)
		{
			int value = GetMap({ i, j });
			if (value != -1)
			{
				DrawGraph(LEFT_MARGIN + i * MAP_IMAGE_SIZE, TOP_MARGIN + j * MAP_IMAGE_SIZE,
						  value, TRUE);
			}
		}
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
	DrawBox(LEFT_MARGIN + 0, TOP_MARGIN + 0,
		LEFT_MARGIN + MAP_WIDTH * MAP_IMAGE_SIZE, TOP_MARGIN + MAP_HEIGHT * MAP_IMAGE_SIZE, GetColor(255, 255, 0), FALSE);
	for (int j = 0; j < MAP_HEIGHT; j++) {
		for (int i = 0; i < MAP_WIDTH; i++) {
			DrawLine(LEFT_MARGIN + i * MAP_IMAGE_SIZE, TOP_MARGIN + j * MAP_IMAGE_SIZE,
				LEFT_MARGIN + (i + 1) * MAP_IMAGE_SIZE, TOP_MARGIN + j * MAP_IMAGE_SIZE, GetColor(255, 255, 255), 1);
			DrawLine(LEFT_MARGIN + i * MAP_IMAGE_SIZE, TOP_MARGIN + j * MAP_IMAGE_SIZE,
				LEFT_MARGIN + i * MAP_IMAGE_SIZE, TOP_MARGIN + (j + 1) * MAP_IMAGE_SIZE, GetColor(255, 255, 255), 1);
		}
	}
	if (isInMapEditArea_) {

		DrawBox(drawAreaRect_.x, drawAreaRect_.y,
			drawAreaRect_.x + drawAreaRect_.w, drawAreaRect_.y + drawAreaRect_.h,
			GetColor(255, 255, 0), TRUE);
	}
	// 複数削除モードの選択範囲表示
	if (multiDeleteMode_) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
		for (const auto& p : selectedForDelete_) {
			int x = LEFT_MARGIN + p.x * MAP_IMAGE_SIZE;
			int y = TOP_MARGIN + p.y * MAP_IMAGE_SIZE;
			DrawBox(x, y, x + MAP_IMAGE_SIZE, y + MAP_IMAGE_SIZE, GetColor(255, 0, 0), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);



}

void MapEdit::SaveMapData()
{
	TCHAR filename[255] = "";
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetMainWindowHandle();
	ofn.lpstrFilter = "全てのファイル (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 255;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		printfDx("ファイルが選択された\n");
		std::ofstream openfile(filename);
		if (!openfile.is_open()) {
			printfDx("ファイルのオープンに失敗しました\n");
			return;
		}
		openfile << "#TinyMapData\n";
		MapChip* mc = FindGameObject<MapChip>();
		for (int j = 0; j < MAP_HEIGHT; j++) {
			for (int i = 0; i < MAP_WIDTH; i++) {
				int index = (myMap_[j * MAP_WIDTH + i] != -1) ? mc->GetChipIndex(myMap_[j * MAP_WIDTH + i]) : -1;
				openfile << index;
				if (i != MAP_WIDTH - 1) openfile << ",";
			}
			openfile << std::endl;
		}
		openfile.close();
		printfDx("File Saved!!!\n");
	}
	else
	{
		printfDx("セーブがキャンセル\n");
	}
}

void MapEdit::LoadMapData()
{
    TCHAR filename[255] = "";
    OPENFILENAME ifn = { 0 };
    ifn.lStructSize = sizeof(ifn);
    ifn.hwndOwner = GetMainWindowHandle();
    ifn.lpstrFilter = "全てのファイル (*.*)\0*.*\0";
    ifn.lpstrFile = filename;
    ifn.nMaxFile = 255;

    if (GetOpenFileName(&ifn))
    {
        printfDx("ファイルが選択された→%s\n", filename);
        std::ifstream inputfile(filename);
        if (!inputfile.is_open()) {
            printfDx("ファイルのオープンに失敗しました\n");
            return;
        }
        std::string line;
        MapChip* mc = FindGameObject<MapChip>();
        std::vector<int> newMap;
        while (std::getline(inputfile, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            std::string tmp;
            while (getline(iss, tmp, ',')) {
                if (tmp == "-1")
                    newMap.push_back(-1);
                else {
                    int index = std::stoi(tmp);
                    int handle = mc->GetHandle(index);
                    newMap.push_back(handle);
                }
            }
        }
        // サイズが合わない場合は補正
        if (newMap.size() != MAP_WIDTH * MAP_HEIGHT) {
            printfDx("マップデータのサイズが不正です\n");
            newMap.resize(MAP_WIDTH * MAP_HEIGHT, -1);
        }
        myMap_ = newMap;
        printfDx("File Loaded!!!\n");
    }
    else
    {
        printfDx("ロードがキャンセル\n");
    }
}

