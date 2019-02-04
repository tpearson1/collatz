// MIT License

// Copyright (c) 2019 Thomas Pearson

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <Magick++.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <vector>

using NodesMap = std::unordered_map<int, std::vector<int>>;

std::vector<int> &processValue(NodesMap &nodes, int value) {
  auto it = nodes.find(value);
  if (it != std::end(nodes)) {
    // Done - already processed
    return it->second;
  }

  nodes[value] = std::vector<int>{};
  int next = value % 2 == 0 ? value / 2 : 3 * value + 1;
  processValue(nodes, next).push_back(value);
  return nodes[value];
}

NodesMap buildUpTo(int high) {
  NodesMap nodes;

  if (high <= 0) {
    throw std::runtime_error{"Cannot create tree with integers zero or lower"};
  }

  // Automatically process the special case of 1
  nodes[1] = std::vector<int>{};

  // Process all the other numbers
  for (int i = 2; i < high; i++) processValue(nodes, i);

  return nodes;
}

constexpr const auto nodeHeight = 20;
constexpr const auto nodeSpacing = 10;

class Node {
  int value_;
  std::vector<Node> above_;
  int positionX = 0;
  int positionY = 0;
  int width;

  static inline int highestY_ = 0; // The most negative Y value
  static inline int highestX_ = 0; // The most positive X value

  int calculateWidth() {
    int textWidthVal = textWidth();
    if (above_.empty()) return width = textWidthVal;

    width = (above_.size() - 1) * nodeSpacing;
    for (Node &x : above_) width += x.calculateWidth();
    if (textWidthVal > width) width = textWidthVal;
    return width;
  }

public:
  Node(int value, const NodesMap &nodes) : value_(value) {
    auto it = nodes.find(value);
    if (it == std::end(nodes)) {
      throw std::runtime_error{"Invalid data generated"};
    }

    auto &above = it->second;
    above_.reserve(above.size());
    for (const auto &v : above) { above_.push_back(Node{v, nodes}); }

    calculateWidth();
  }

  int textWidth() const {
    std::ostringstream s;
    s << value_;
    auto text = s.str();
    return 8 * text.size();
  }

  static int highestX() noexcept { return highestX_; }
  static int highestY() noexcept { return highestY_; }

  int getWidth() const noexcept { return width; }
  int value() const noexcept { return value_; }
  const std::vector<Node> &above() const noexcept { return above_; }

  int getPositionX() const noexcept { return positionX; }
  int getPositionY() const noexcept { return positionY; }

  void setPositions(int x, int y) {
    positionX = x;
    positionY = y;

    int offsetX = -width / 2;
    for (std::size_t i = 0; i < above_.size(); i++) {
      // Offsets relative to this node
      const int posX = x + offsetX + above_[i].width / 2,
                posY = y - nodeSpacing - nodeHeight;
      offsetX += above_[i].width + nodeSpacing;
      highestX_ = std::max(highestX_, posX);
      highestY_ = std::min(highestY_, posY);
      above_[i].setPositions(posX, posY);
    }
  }

  template <typename DrawFunc>
  void draw(DrawFunc drawFunc, const Node *below = nullptr) const {
    drawFunc(below, *this);
    for (const auto &x : above_) x.draw(drawFunc, this);
  }
};

void generateImage(const NodesMap &nodes, const std::string &outFileName) {
  auto rootNode = Node{1, nodes};
  rootNode.setPositions(0, 0);
  constexpr const int imagePadding = 80;
  const int imageWidth = rootNode.getWidth() + imagePadding * 2;
  const int imageHeight = imagePadding * 2 - Node::highestY();

  const int globalOffsetX = imageWidth / 2;
  const int globalOffsetY = imageHeight - imagePadding;

  auto image =
      Magick::Image{Magick::Geometry{static_cast<unsigned int>(imageWidth),
                                     static_cast<unsigned int>(imageHeight)},
                    Magick::Color{"white"}};

  std::vector<Magick::Drawable> textDrawList;
  textDrawList.push_back(Magick::DrawableFont{
      "Work-Sans-Light", Magick::NormalStyle, 400, Magick::NormalStretch});
  textDrawList.push_back(Magick::DrawableText{0, 0, "."});
  textDrawList.push_back(Magick::DrawableStrokeColor{Magick::Color{"black"}});
  textDrawList.push_back(Magick::DrawableFillOpacity{0});

  std::vector<Magick::Drawable> lineDrawList;
  lineDrawList.push_back(Magick::DrawableStrokeColor{Magick::Color{"black"}});
  lineDrawList.push_back(Magick::DrawableStrokeWidth{2});
  lineDrawList.push_back(Magick::DrawableFillOpacity{0});
  lineDrawList.push_back(Magick::DrawableLine{10, 10, 50, 50});

  rootNode.draw([&](const Node *below, const Node &current) {
    std::ostringstream s;
    s << current.value();
    auto text = s.str();
    textDrawList[1] = Magick::DrawableText{
        static_cast<double>(current.getPositionX() + globalOffsetX - current.textWidth() / 2),
        static_cast<double>(current.getPositionY() + globalOffsetY), s.str()};
    image.draw(textDrawList);

    if (below) {
      // Values 8, 15 prevent the line from intersecting the text
      double startX = static_cast<double>(current.getPositionX() + globalOffsetX);
      double startY = static_cast<double>(current.getPositionY() + globalOffsetY + 8);
      double endX = static_cast<double>(below->getPositionX() + globalOffsetX);
      double endY = static_cast<double>(below->getPositionY() + globalOffsetY - 15);

      lineDrawList[3] = Magick::DrawableLine{startX, startY, endX, endY};
      image.draw(lineDrawList);
    }
  });

  image.write(outFileName);
}

template <typename Container>
void commaSeparateValues(std::ostream &os, const Container &c) {
  if (c.empty()) return;

  auto end = --std::end(c);
  for (auto it = std::begin(c); it != end; it++) { os << *it << ", "; }

  os << *end;
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    std::cerr << "Expected: collatz <up-to> <out-file>\n";
    return 1;
  }

  std::istringstream iss{argv[1]};
  int upTo;
  if (!(iss >> upTo) || upTo <= 0) {
    std::cerr << "Expected positive integer argument\n";
    return 1;
  }

  auto outFile = argv[2];

  std::cout << "Calculating...\n";
  auto nodes = buildUpTo(upTo);
  std::cout << "Calculated\n";

  std::cout << "Generating image...\n";
  generateImage(nodes, outFile);
  std::cout << "Done\n";

  return 0;
}
