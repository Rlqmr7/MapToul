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
	:GameObject(), myMap_(MAP_WIDTH* MAP_HEIGHT, -1), //�����l��-1��20*20�̔z�������������
	isInMapEditArea_(false), //�}�b�v�G�f�B�^�̈���ɂ��邩�ǂ���
	drawAreaRect_{0, 0, 0, 0} // drawAreaRect_��������
{
	mapEditRect_ = { LEFT_MARGIN, TOP_MARGIN,
		MAP_WIDTH * MAP_IMAGE_SIZE, MAP_HEIGHT * MAP_IMAGE_SIZE };
}

MapEdit::~MapEdit()
{
}

void MapEdit::SetMap(Point p, int value)
{
	//�}�b�v�̍��Wp��value���Z�b�g����
	//p���A�z��͈̔͊O�̎���assert�ɂЂ�������
	assert(p.x >= 0 && p.x < MAP_WIDTH);
	assert(p.y >= 0 && p.y < MAP_HEIGHT);
	myMap_[p.y * MAP_WIDTH + p.x] = value; //y�sx���value���Z�b�g����

}

int MapEdit::GetMap(Point p) const
{
	//�}�b�v�̍��Wp�̒l���擾����
	//p���A�z��͈̔͊O�̎���assert�ɂЂ�������
	assert(p.x >= 0 && p.x < MAP_WIDTH);
	assert(p.y >= 0 && p.y < MAP_HEIGHT);
	return myMap_[p.y * MAP_WIDTH + p.x]; //y�sx��̒l���擾����

}

void MapEdit::Update()
{
	Point mousePos;
	if (GetMousePoint(&mousePos.x, &mousePos.y) == -1) {
		return;
	}
	// �}�E�X�̍��W���}�b�v�G�f�B�^�̈���ɂ��邩�ǂ����𔻒肷��
	isInMapEditArea_ = mousePos.x >= mapEditRect_.x && mousePos.x <= mapEditRect_.x + mapEditRect_.w &&
		mousePos.y >= mapEditRect_.y && mousePos.y <= mapEditRect_.y + mapEditRect_.h;

	//����@mapEditRect_.x, mapEditRect_.y
	//�E��@mapEditRect_.x + mapEditRect_.w, mapEditRect_.y
	//�E��  mapEditRect_.x + mapEditRect_.w, mapEditRect_.y + mapEditRect_.h
	//����  mapEditRect_.x, mapEditRect_.y + mapEditRect_.h
		// �O���b�h���W�ɕϊ�
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
				selectedForDelete_.push_back(p); // �I��
			} else {
				selectedForDelete_.erase(it); // ����
			}
		}
		// �폜���s�i��FEnter�L�[�ō폜�j
		if (Input::IsKeyDown(KEY_INPUT_RETURN)) {
			for (const auto& p : selectedForDelete_) {
				SetMap(p, -1);
			}
			selectedForDelete_.clear();
		}
		return; // �ʏ폈���̓X�L�b�v
	}

	if (Input::IsButtonKeep(MOUSE_INPUT_LEFT)) {
		MapChip* mapChip = FindGameObject<MapChip>();

		if (CheckHitKey(KEY_INPUT_LSHIFT)) //R�L�[�������Ă���Ȃ�
		{
			SetMap({ gridX, gridY }, -1); //�}�b�v�ɒl���Z�b�g�i-1�͉����Ȃ���ԁj
			return; //�}�b�v�`�b�v���폜�����炱���ŏI��
		}
		else if (mapChip && mapChip->IsHold()) //�}�b�v�`�b�v�������Ă���Ȃ�
		{
			SetMap({ gridX, gridY }, mapChip->GetHoldImage()); //�}�b�v�ɒl���Z�b�g
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

	// D�L�[�ŕ����폜���[�h�ؑ�
	if (Input::IsKeyDown(KEY_INPUT_D)) {
		multiDeleteMode_ = !multiDeleteMode_;
		selectedForDelete_.clear();
	}

}

void MapEdit::Draw()
{
	// �S�}�b�v��\��
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
	// �����폜���[�h�̑I��͈͕\��
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
	ofn.lpstrFilter = "�S�Ẵt�@�C�� (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 255;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		printfDx("�t�@�C�����I�����ꂽ\n");
		std::ofstream openfile(filename);
		if (!openfile.is_open()) {
			printfDx("�t�@�C���̃I�[�v���Ɏ��s���܂���\n");
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
		printfDx("�Z�[�u���L�����Z��\n");
	}
}

void MapEdit::LoadMapData()
{
    TCHAR filename[255] = "";
    OPENFILENAME ifn = { 0 };
    ifn.lStructSize = sizeof(ifn);
    ifn.hwndOwner = GetMainWindowHandle();
    ifn.lpstrFilter = "�S�Ẵt�@�C�� (*.*)\0*.*\0";
    ifn.lpstrFile = filename;
    ifn.nMaxFile = 255;

    if (GetOpenFileName(&ifn))
    {
        printfDx("�t�@�C�����I�����ꂽ��%s\n", filename);
        std::ifstream inputfile(filename);
        if (!inputfile.is_open()) {
            printfDx("�t�@�C���̃I�[�v���Ɏ��s���܂���\n");
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
        // �T�C�Y������Ȃ��ꍇ�͕␳
        if (newMap.size() != MAP_WIDTH * MAP_HEIGHT) {
            printfDx("�}�b�v�f�[�^�̃T�C�Y���s���ł�\n");
            newMap.resize(MAP_WIDTH * MAP_HEIGHT, -1);
        }
        myMap_ = newMap;
        printfDx("File Loaded!!!\n");
    }
    else
    {
        printfDx("���[�h���L�����Z��\n");
    }
}

