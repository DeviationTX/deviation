#ifndef _LAYOUT_H_
#define _LAYOUT_H_

typedef struct _grid_layout_data {
    int w, h, x, y;
}GridData;

#define BeginGridLayout(row, col) BeginGridLayout2(0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT, row, col)

#define BeginGridLayout2(startx, starty, width, height, row, col) \
        for (GridData _grid = { (width)/(col), (height)/(row), (startx), (starty)}; _grid.w > 0; _grid.w = 0)

#define Grid_XY(row, col) \
        (_grid.x + _grid.w * ((col) - 1)), (_grid.y + _grid.h * ((row) - 1))

#define Grid_WH() \
        _grid.w, _grid.h

#define Grid_WH_Span(span_w, span_h) \
        _grid.w * span_w, _grid.h * span_h

#endif  // _LAYOUT_H_