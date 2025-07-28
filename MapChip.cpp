#define NOMINMAX

#include "MapChip.h"
#include "globals.h"
#include "Source\Screen.h"
#include "Input.h"
#include "ImGui/imgui.h"
#include <algorithm>


namespace
{
	////同じ名前の変数はあとでまとめましょう
	//const int IMAGE_SIZE = { 32 }; // 画像のサイズ
	//const int MAP_CHIP_WIDTH = { 16 };//チップの横並び数
	//const int MAP_CHIP_HEIGHT = { 12 };//チップの縦並び数
	//const int MAP_CHIP_NUM_X = { 8 };//マップチップウィンドウの横並び数
	//const int MAP_CHIP_NUM_Y = { 24 };//マップチップウィンドウの縦並び数
	//const int MAP_CHIP_WIN_WIDTH = { IMAGE_SIZE * MAP_CHIP_NUM_X };//ウィンドウの横幅
	//const int MAP_CHIP_WIN_HEIGHT = { IMAGE_SIZE * MAP_CHIP_NUM_Y };//ウィンドウの縦幅

}

MapChip::MapChip()
	: GameObject(), isUpdate_(false), isInMapChipArea_(false), selectedIndex_(-1)
	, cfg_(GetMapChipConfig()), selected_({ 0,0 }), isHold_(false), ScrollOffset_({ 0,0 })
{


	bgHandle.resize(cfg_.TILE_PIX_SIZE * cfg_.TILES_X * cfg_.TILES_Y, -1);


	LoadDivGraph("./bg.png", cfg_.TILES_X * cfg_.TILES_Y,
		cfg_.TILES_X, cfg_.TILES_Y,
		cfg_.TILE_PIX_SIZE, cfg_.TILE_PIX_SIZE, bgHandle.data());


	//LUT(Look Up Table) 作成
	for (int i = 0; i < bgHandle.size(); i++)
	{
		HandleToIndex[bgHandle[i]] = i;
	}
}


MapChip::~MapChip()
{
	for (int i = 0; i < cfg_.TILES_X * cfg_.TILES_Y; i++) {
		if (bgHandle[i] != -1) {
			DeleteGraph(bgHandle[i]);
			bgHandle[i] = -1;
		}
	}
}

Point MapChip::GetViewOrigin() const {
	return { Screen::WIDTH - cfg_.MAPCHIP_WIN_WIDTH,0 };
}

bool MapChip::IsInChipArea(const Point& mouse) const
{
	return (mouse.x >= GetViewOrigin().x &&
		mouse.x < Screen::WIDTH &&
		mouse.y >= 0 &&
		mouse.y < cfg_.MAPCHIP_WIN_HEIGHT);
}

Point MapChip::ScreenToChipIndex(const Point& mouse) const
{
	int localX = (mouse.x - GetViewOrigin().x) / cfg_.TILE_PIX_SIZE;
	int localY = mouse.y / cfg_.TILE_PIX_SIZE;
	return { localX, localY };
}

void MapChip::Update()
{

	 // スクロール操作（上下キーでY、左右キーでX）
    if (Input::IsKeyDown(KEY_INPUT_UP))    scrollY_ -= cfg_.TILE_PIX_SIZE;
    if (Input::IsKeyDown(KEY_INPUT_DOWN))  scrollY_ += cfg_.TILE_PIX_SIZE;
    if (Input::IsKeyDown(KEY_INPUT_LEFT))  scrollX_ -= cfg_.TILE_PIX_SIZE;
    if (Input::IsKeyDown(KEY_INPUT_RIGHT)) scrollX_ += cfg_.TILE_PIX_SIZE;

    // スクロール範囲制限
    int maxScrollY = std::max(0, cfg_.TILE_PIX_SIZE * cfg_.TILES_Y - cfg_.MAPCHIP_WIN_HEIGHT);
    int maxScrollX = std::max(0, cfg_.TILE_PIX_SIZE * cfg_.TILES_X - cfg_.MAPCHIP_WIN_WIDTH);
    if (scrollY_ < 0) scrollY_ = 0;
    if (scrollY_ > maxScrollY) scrollY_ = maxScrollY;
    if (scrollX_ < 0) scrollX_ = 0;
    if (scrollX_ > maxScrollX) scrollX_ = maxScrollX;

    // --- 既存の選択処理 ---
    Point mousePos;
    if (GetMousePoint(&mousePos.x, &mousePos.y) == -1) return;

    isInMapChipArea_ = IsInChipArea(mousePos);

    if (isInMapChipArea_) {
        // スクロール分だけ座標補正
        Point scrolledMouse = mousePos;
        scrolledMouse.x += scrollX_;
        scrolledMouse.y += scrollY_;
        selected_ = ScreenToChipIndex(scrolledMouse);

        int gx = selected_.x;
        int gy = selected_.y;
        int index = gy * cfg_.TILES_X + gx;
        if (index >= 0 && index < bgHandle.size() && Input::IsButtonDown(MOUSE_INPUT_LEFT)) {
            selectedIndex_ = bgHandle[index];
            isHold_ = true;
        }
    } else {
        isInMapChipArea_ = false;
    }
}

void MapChip::Draw()
{
    const int originX = Screen::WIDTH - cfg_.MAPCHIP_WIN_WIDTH;
    const int originY = 0;

    for (int y = 0; y < cfg_.TILES_Y; y++) {
        for (int x = 0; x < cfg_.TILES_X; x++) {
            int index = y * cfg_.TILES_X + x;
            int drawX = originX + x * cfg_.TILE_PIX_SIZE - scrollX_;
            int drawY = originY + y * cfg_.TILE_PIX_SIZE - scrollY_;
            // ウィンドウ内のみ描画
            if (drawX + cfg_.TILE_PIX_SIZE > originX && drawX < originX + cfg_.MAPCHIP_WIN_WIDTH &&
                drawY + cfg_.TILE_PIX_SIZE > originY && drawY < originY + cfg_.MAPCHIP_WIN_HEIGHT) {
                DrawGraph(drawX, drawY, bgHandle[index], TRUE);
            }
        }
    }

    // 選択中チップのハイライト
    if (isInMapChipArea_) {
        int x = originX + selected_.x * cfg_.TILE_PIX_SIZE - scrollX_;
        int y = selected_.y * cfg_.TILE_PIX_SIZE - scrollY_;
        int size = cfg_.TILE_PIX_SIZE;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawBox(x + 1, y - 1, x + size - 1, y + size + 1, GetColor(255, 255, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(x, y, x + size, y + size, GetColor(255, 0, 0), FALSE);
    }

    // ホールド中チップの描画（変更なし）
    if (isHold_) {
        Point mousePos;
        if (GetMousePoint(&mousePos.x, &mousePos.y) != -1) {
            DrawExtendGraph(mousePos.x, mousePos.y,
                            mousePos.x + cfg_.TILE_PIX_SIZE,
                            mousePos.y + cfg_.TILE_PIX_SIZE,
                            selectedIndex_, TRUE);
        }
        if (Input::IsButtonUP(MOUSE_INPUT_RIGHT)) {
            isHold_ = false;
            selectedIndex_ = -1;
        }
    }
}

bool MapChip::IsHold()
{
	return isHold_;
}

int MapChip::GetHoldImage()
{
	if (isHold_)
	{
		return selectedIndex_;
	}
	else
	{
		return -1; //持っていない場合は-1を返す
	}
}

int MapChip::GetChipIndex(int handle)
{

	return HandleToIndex[handle];

	//for (int i = 0;i < bgHandle.size();i++)
	//{
	//	if (handle == bgHandle[i])
	//		return i;
	//}
	//int a = HandleToIndex[handle];
	//if(HandleToIndex[handle])

}