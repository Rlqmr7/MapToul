#pragma once

struct Point {
    int x;
    int y;
    // == ���Z�q�̃I�[�o�[���[�h
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
};
struct Rect
{
	int x, y;
	int w, h;
};
