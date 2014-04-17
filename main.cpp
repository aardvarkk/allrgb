#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "lodepng.h"

static const int kAttempts = 1000;
static const int kWorkingW = 4096;
static const int kWorkingH = 4096;

struct Color
{
  Color() : r(0), g(0), b(0) {}
  Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}
  unsigned char r, g, b;
};

struct Point
{
  Point() : x(0), y(0) {}
  Point(short x, short y, Color color) : x(x), y(y), color(color) {}
  short x, y;
  Color color;
};

vector<unsigned char> color_to_lodepng(vector<Color> color)
{
  vector<unsigned char> lodepng(color.size()*4);
  size_t j = 0;
  for (auto c : color) {
    lodepng[j++] = c.r;
    lodepng[j++] = c.g;
    lodepng[j++] = c.b;
    lodepng[j++] = 255;
  }
  return lodepng;
}

vector<Point> lodepng_to_point(vector<unsigned char> lodepng, unsigned int w, unsigned int h)
{
  vector<Point> point(lodepng.size()/4);
  size_t j = 0;
  for (unsigned int y = 0; y < h; ++y) {
    for (unsigned int x = 0; x < w; ++x) {
      point[j] = Point(x, y, Color(lodepng[4*j+0], lodepng[4*j+1], lodepng[4*j+2]));
      ++j;
    }
  }
  return point;
}

deque<Color> all_colors()
{
  deque<Color> all_colors;
  for (int r = 0; r < 256; ++r)
    for (int g = 0; g < 256; ++g)
      for (int b = 0; b < 256; ++b)
        all_colors.push_back(Color(r, g, b));
  return all_colors;
}

int color_diff(Color const& a, Color const& b)
{
  int dr = a.r - b.r;
  int dg = a.g - b.g;
  int db = a.b - b.g;
  return dr * dr + dg * dg + db * db;
}

int main(int argc, char* argv[])
{
  vector<unsigned char> input;
  unsigned int width, height;
  lodepng::decode(input, width, height, argv[1]);
  cout << "Decoded image" << endl;
  
  assert(width == kWorkingW);
  assert(height == kWorkingH);

  // TEST DATA
  //width = 2;
  //height = 1;
  //input.push_back(255);
  //input.push_back(0);
  //input.push_back(0);
  //input.push_back(255);
  //input.push_back(0);
  //input.push_back(255);
  //input.push_back(0);
  //input.push_back(255);

  vector<Point> orig = lodepng_to_point(input, width, height);
  cout << "Converted to points" << endl;

  deque<Color> todo = all_colors();
  cout << "Generated all colors" << endl;

  // TEST DATA
  //deque<Color> todo;
  //todo.push_back(Color(0, 254, 0));
  //todo.push_back(Color(254, 0, 0));

  random_shuffle(orig.begin(), orig.end());
  random_shuffle(todo.begin(), todo.end());
  cout << "Shuffled everything" << endl;

  vector<Color> output(width * height);

  int px = 0;
  for (auto p : orig) {
    auto chosen_it = todo.begin();
    auto todo_it   = todo.begin();
    int  min_diff  = INT_MAX;
    for (int i = 0; i < kAttempts; ++i) {
      if (todo_it == todo.end()) {
        break;
      }
      auto diff = color_diff(p.color, *todo_it);
      if (diff < min_diff) {
        min_diff = diff;
        chosen_it = todo_it;
      }
      ++todo_it;
    }

    output[width * p.y + p.x] = *chosen_it;
    todo.erase(chosen_it);

    if (++px % 100000 == 0) {
      cout << "Finished " << px << " of " << width * height << " pixels" << endl;
    }
  }
  cout << "Filled output" << endl;

  lodepng::encode(argv[2], color_to_lodepng(output), width, height);
  cout << "Encoded output" << endl;
  
  return EXIT_SUCCESS;
}