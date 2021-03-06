#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

#include "lodepng.h"

static const int kAttempts = 100;
static const int kWorkingW = 4096;
static const int kWorkingH = 4096;

struct ColorRGB
{
  ColorRGB() : r(0), g(0), b(0) {}
  ColorRGB(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}
  unsigned char r, g, b;
};

struct ColorXYZ
{
  ColorXYZ() : X(0.f), Y(0.f), Z(0.f) {}
  ColorXYZ(double X, double Y, double Z) : X(X), Y(Y), Z(Z) {}
  double X, Y, Z;
};

struct ColorLab
{
  ColorLab() : L(0.f), a(0.f), b(0.f) {}
  ColorLab(double L, double a, double b) : L(L), a(a), b(b) {}
  double L, a, b;
};

struct Point
{
  Point() : x(0), y(0) {}
  Point(short x, short y, ColorRGB color) : x(x), y(y), color(color) {}
  short x, y;
  ColorRGB color;
};

vector<unsigned char> color_to_lodepng(vector<ColorRGB> ColorRGB)
{
  vector<unsigned char> lodepng(ColorRGB.size()*4);
  size_t j = 0;
  for (auto c : ColorRGB) {
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
      point[j] = Point(x, y, ColorRGB(lodepng[4*j+0], lodepng[4*j+1], lodepng[4*j+2]));
      ++j;
    }
  }
  return point;
}

deque<ColorRGB> all_colors()
{
  deque<ColorRGB> all_colors;
  for (int r = 0; r < 256; ++r)
    for (int g = 0; g < 256; ++g)
      for (int b = 0; b < 256; ++b)
        all_colors.push_back(ColorRGB(r, g, b));
  return all_colors;
}

// http://www.easyrgb.com/index.php?X=MATH
ColorXYZ rgb_to_xyz(ColorRGB const& rgb)
{
  ColorXYZ xyz;

  auto var_R = rgb.r / 255.;        //R from 0 to 255
  auto var_G = rgb.g / 255.;        //G from 0 to 255
  auto var_B = rgb.b / 255.;        //B from 0 to 255

  if (var_R > 0.04045)
    var_R = pow((var_R + 0.055) / 1.055, 2.4);
  else                   
    var_R = var_R / 12.92;

  if (var_G > 0.04045)
    var_G = pow((var_G + 0.055) / 1.055, 2.4);
  else                   
    var_G = var_G / 12.92;

  if (var_B > 0.04045) 
    var_B = pow((var_B + 0.055) / 1.055, 2.4);
  else                   
    var_B = var_B / 12.92;

  var_R = var_R * 100;
  var_G = var_G * 100;
  var_B = var_B * 100;

  //Observer. = 2�, Illuminant = D65
  xyz.X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
  xyz.Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
  xyz.Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

  return xyz;
}

// http://www.easyrgb.com/index.php?X=MATH
ColorLab xyz_to_lab(ColorXYZ const& xyz)
{
  ColorLab lab;

  auto ref_X =  95.047;
  auto ref_Y = 100.000;
  auto ref_Z = 108.883;

  auto var_X = xyz.X / ref_X;          //ref_X =  95.047   Observer= 2�, Illuminant= D65
  auto var_Y = xyz.Y / ref_Y;          //ref_Y = 100.000
  auto var_Z = xyz.Z / ref_Z;          //ref_Z = 108.883

  auto c1 = 1.  / 3;
  auto c2 = 16. / 116;

  if (var_X > 0.008856)
    var_X = pow(var_X, c1);
  else
    var_X = 7.787 * var_X + c2;

  if (var_Y > 0.008856)
    var_Y = pow(var_Y, c1);
  else                    
    var_Y = (7.787 * var_Y) + c2;

  if (var_Z > 0.008856)
    var_Z = pow(var_Z, c1);
  else                    
    var_Z = (7.787 * var_Z) + c2;

  lab.L = (116 * var_Y) - 16;
  lab.a = 500 * (var_X - var_Y);
  lab.b = 200 * (var_Y - var_Z);

  return lab;
}

double color_diff_CIE76(ColorRGB const& a, ColorRGB const& b)
{
  auto lab_a = xyz_to_lab(rgb_to_xyz(a));
  auto lab_b = xyz_to_lab(rgb_to_xyz(b));
  auto dL = lab_a.L - lab_b.L;
  auto da = lab_a.a - lab_b.a;
  auto db = lab_a.b - lab_b.b;
  return dL * dL + da * da + db * db;
}

double color_diff_CIE76_speedy(ColorLab const& lab_a, ColorRGB const& b)
{
  auto lab_b = xyz_to_lab(rgb_to_xyz(b));
  auto dL = lab_a.L - lab_b.L;
  auto da = lab_a.a - lab_b.a;
  auto db = lab_a.b - lab_b.b;
  return dL * dL + da * da + db * db;
  //return dL * dL;
  //return da * da;
  //return db * db;
}

double color_diff_CIE94_speedy(ColorLab const& lab_a, ColorRGB const& b)
{
  //WHT-L, WHT-C, WHT-H                //Weighting factors depending
                                     //on the application (1 = default)

  auto lab_b = xyz_to_lab(rgb_to_xyz(b));

  auto xC1 = sqrt(lab_a.a * lab_a.a + lab_a.b * lab_a.b);
  auto xC2 = sqrt(lab_b.a * lab_b.a + lab_b.b * lab_b.b);
  auto xDL = lab_b.L - lab_a.L;
  auto xDC = xC2 - xC1;
  auto xDE = sqrt( ( ( lab_a.L - lab_b.L ) * ( lab_a.L - lab_b.L ) )
            + ( ( lab_a.a - lab_b.a ) * ( lab_a.a - lab_b.a ) )
            + ( ( lab_a.b - lab_b.b ) * ( lab_a.b - lab_b.b ) ) );
  double xDH = 0;
  if ( sqrt( xDE ) > ( sqrt( abs( xDL ) ) + sqrt( abs( xDC ) ) ) ) {
     xDH = sqrt( ( xDE * xDE ) - ( xDL * xDL ) - ( xDC * xDC ) );
  }
  auto xSC = 1 + ( 0.045 * xC1 );
  auto xSH = 1 + ( 0.015 * xC1 );
  //xDL /= WHT-L
  xDC /= /*WHT-C * */ xSC;
  xDH /= /*WHT-H * */ xSH;
  return sqrt(xDL * xDL + xDC * xDC + xDH *xDH);
}

int color_diff(ColorRGB const& a, ColorRGB const& b)
{
  int dr = a.r - b.r;
  int dg = a.g - b.g;
  int db = a.b - b.g;
  return dr * dr + dg * dg + db * db;
}

struct ThreadArgs
{
  vector<Point>* orig;
  deque<ColorRGB>* todo_all;
  vector<ColorRGB>* output;
  unsigned int width;
};

void choose_colors(
  unsigned int tid,
  unsigned int num_threads,
  ThreadArgs const& args
  )
{
  auto orig = args.orig;
  auto todo_all = args.todo_all;
  auto output = args.output;
  auto width = args.width;

  auto batch_size = orig->size() / num_threads;
  auto offset = tid * batch_size;
  auto orig_it = orig->begin() + offset;
  cout << "Thread " << tid << " has batch size " << batch_size << " and offset " << offset << endl;

  // Create a todo batch by copying from all
  deque<ColorRGB> todo(batch_size);
  int j = 0;
  for (unsigned int i = offset; i < offset + batch_size; ++i) {
    todo[j++] = todo_all->at(i);
  }
  cout << "Thread " << tid << " created todo list" << endl;

  for (unsigned int px = 0; px < batch_size; ++px, ++orig_it) {

    auto chosen_it = todo.begin();
    auto todo_it   = todo.begin();
    //int  min_diff  = INT_MAX;
    double min_diff = DBL_MAX;
    auto lab_p = xyz_to_lab(rgb_to_xyz(orig_it->color));
    for (int i = 0; i < kAttempts; ++i) {
      if (todo_it == todo.end()) {
        break;
      }
      //auto diff = color_diff(orig_it->color, *todo_it);
      //auto diff = color_diff_CIE76_speedy(lab_p, *todo_it);
      auto diff = color_diff_CIE94_speedy(lab_p, *todo_it);
      //auto diff = color_diff_CIE76(
      //  orig_it->color, 
      //  *todo_it
      //  );
      if (diff < min_diff) {
        min_diff = diff;
        chosen_it = todo_it;
      }
      ++todo_it;
    }

    if (px % 100000 == 0) {
      cout << "Thread " << tid << " finished " << px << " of " << batch_size << " pixels" << endl;
    }

    output->at(width * orig_it->y + orig_it->x) = *chosen_it;
    todo.erase(chosen_it);
  }

  cout << "Thread " << tid << " done filling output" << endl;
  return;
}

int main(int argc, char* argv[])
{
  vector<unsigned char> input;
  unsigned int width, height;

  // TEST DATA
  ColorRGB testRGB(123, 45, 67);
  auto testXYZ = rgb_to_xyz(testRGB);
  auto testLab = xyz_to_lab(testXYZ);
  ColorRGB testRGB2(90, 101, 121);
  auto testXYZ2 = rgb_to_xyz(testRGB2);
  auto testLab2 = xyz_to_lab(testXYZ2);
  auto diff = color_diff_CIE76_speedy(testLab, testRGB2);

  lodepng::decode(input, width, height, argv[1]);
  cout << "Decoded image" << endl;
  //assert(width == kWorkingW);
  //assert(height == kWorkingH);

  //// TEST DATA
  //width  = 256;
  //height = 256;
  //input = vector<unsigned char>(4 * width * height);
  //for (size_t i = 0; i < input.size(); ++i) {
  //  input[i] = rand() % 256;
  //}
  //// Full alpha
  //for (size_t i = 3; i < input.size(); i += 4) {
  //  input[i] = 255;
  //}
  //lodepng::encode("test.png", input, width, height);

  vector<Point> orig = lodepng_to_point(input, width, height);
  cout << "Converted to points" << endl;

  deque<ColorRGB> todo = all_colors();
  cout << "Generated all colors" << endl;

  //// TEST DATA
  //deque<ColorRGB> todo(width * height);
  //for (size_t i = 0; i < todo.size(); ++i) {
  //  todo[i] = ColorRGB(rand() % 256, rand() % 256, rand() % 256);
  //}

  random_shuffle(orig.begin(), orig.end());
  random_shuffle(todo.begin(), todo.end());
  cout << "Shuffled everything" << endl;

  vector<ColorRGB> output(width * height);

  // Package to send to thread
  ThreadArgs args;
  args.orig     = &orig;
  args.output   = &output;
  args.todo_all = &todo;
  args.width    = width;

  auto num_threads = thread::hardware_concurrency();
  vector<thread> threads;
  for (unsigned int tid = 0; tid < num_threads; ++tid) {
    threads.push_back(thread(choose_colors, tid, num_threads, args));
  }
  for (auto& t : threads) {
    t.join();
  }
  cout << "Threads joined" << endl;
 
  lodepng::encode(argv[2], color_to_lodepng(output), width, height);
  cout << "Encoded output" << endl;
  
  return EXIT_SUCCESS;
}