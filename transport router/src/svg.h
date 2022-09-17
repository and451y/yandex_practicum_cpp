#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <optional>
#include <variant>

namespace svg {

using namespace std::literals;

struct Rgb {

  friend std::ostream& operator<<(std::ostream& out, Rgb rgb);


  Rgb(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)
    : red(r)
    , green(g)
    , blue(b) {
  }


  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct Rgba {
  Rgba(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, double o = 1)
    : red(r)
    , green(g)
    , blue(b)
    , opacity(o) {
  }

  friend std::ostream& operator<<(std::ostream& out, Rgba rgba);


  uint8_t red;
  uint8_t green;
  uint8_t blue;
  double opacity;
};

using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

inline const Color NoneColor{"none"s};

struct Visiter {

    void operator()(std::monostate) const {
        out << "none";
    }
    void operator()(Rgb rgb) const {
        out << rgb;
    }
    void operator()(Rgba rgba) const {
        out << rgba;
    }
    void operator()(std::string color) const {
        out << color;
    }

    std::ostream& out;
};

// Объявив в заголовочном файле константу со спецификатором inline,
// мы сделаем так, что она будет одной на все единицы трансляции,
// которые подключают этот заголовок.
// В противном случае каждая единица трансляции будет использовать свою копию этой константы
enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
      width_ = std::move(width);
      return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
      line_cap_ = std::move(line_cap);
      return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
      line_join_ = std::move(line_join);
      return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;
//        visit(OstreamSolutionPrinter{strm}, solution);

        if (!(std::holds_alternative<std::monostate>(fill_color_))) {
            if(!std::holds_alternative<std::monostate>(fill_color_)){
              out << " fill=\""sv;
              std::visit(Visiter{ out }, fill_color_);
              out << "\""sv;
            }
        }

        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
              out << " stroke=\""sv;
              std::visit(Visiter{ out }, stroke_color_);
              out << "\""sv;
        }
        if (width_) {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_) {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    Color fill_color_;
    Color stroke_color_;
    std::optional<StrokeLineJoin> line_join_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<double> width_;

};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
  template <typename Obj>
  void Add(Obj obj) {
    AddPtr(std::make_unique<Obj>(std::move(obj)));
  }

  // Добавляет в svg-документ объект-наследник svg::Object
  virtual void AddPtr(std::unique_ptr<Object>&&) = 0;

  virtual ~ObjectContainer() = default;

private:
};


class Document : public ObjectContainer{
public:
  Document() =  default;

    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj);

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
  Circle() = default;

    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:

  Polyline() = default;
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::string points_ = {};
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
  Text() = default;

    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    std::string Shielding(std::string str);
    Point pos_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t size_ = 1;
    std::string data_ = {};
    std::string font_weight_;
    std::string font_family_;
};



template <typename T>
std::string ToString(T const& value) {
    std::stringstream sstr;
    sstr << value;
    return sstr.str();
}

class Drawable {
public:
  virtual void Draw(svg::ObjectContainer& container) const = 0;

  virtual ~Drawable() = default;
private:
};




}  // namespace svg


namespace shapes {

class Triangle final : public svg::Drawable {
public:
    Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
        : p1_(p1)
        , p2_(p2)
        , p3_(p3) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point p1_, p2_, p3_;
};

class Star final : public svg::Drawable {
 public:

  Star() = default;

  Star(svg::Point center, double outer_rad, double inner_rad, int num_rays)
         : center_(center)
         , outer_rad_(outer_rad)
         , inner_rad_(inner_rad)
         , num_rays_(num_rays) {
     }

     // Реализует метод Draw интерфейса svg::Drawable
     void Draw(svg::ObjectContainer& container) const override;

 private:
     svg::Point center_ = {0, 0};
     double outer_rad_ = 0;
     double inner_rad_ = 0;
     int num_rays_ = 0;


     svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) const;

};


class Snowman final : public svg::Drawable {
 public:
  Snowman(svg::Point center, double radius)
         : center_(center)
         , radius_(radius) {
     }

     // Реализует метод Draw интерфейса svg::Drawable
     void Draw(svg::ObjectContainer& container) const override;

 private:
     svg::Point center_ = {0, 0};
     double radius_ = 0;

     std::vector<svg::Circle> CreateSnowman(svg::Point center, double rad) const; // порядок рисования снизу-вверх


};

} // namespace shapes
