#include "svg.h"


namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, Rgb rgb) {
    out << "rgb(" << unsigned(rgb.red) << ',' << unsigned(rgb.green) << ',' << unsigned(rgb.blue) << ')';
    return out;
}

std::ostream& operator<<(std::ostream& out, Rgba rgba) {
    out << "rgba(" << unsigned(rgba.red) << ',' << unsigned(rgba.green) << ',' << unsigned(rgba.blue) << ',' << rgba.opacity << ')';
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
  using namespace std::literals;

  std::string result;

  switch (line_join) {
    case StrokeLineJoin::ARCS:
      result = "arcs"sv;
      break;
    case StrokeLineJoin::BEVEL:
      result = "bevel"sv;
      break;
    case StrokeLineJoin::MITER:
      result = "miter"sv;
      break;
    case StrokeLineJoin::MITER_CLIP:
      result = "miter-clip"sv;
      break;
    case StrokeLineJoin::ROUND:
      result = "round"sv;
      break;
    default:
      break;
  }
  out << result;

  return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
  using namespace std::literals;

  std::string result;

  switch (line_cap) {
    case StrokeLineCap::BUTT:
      result = "butt"sv;
      break;
    case StrokeLineCap::ROUND:
      result = "round"sv;
      break;
    case StrokeLineCap::SQUARE:
      result = "square"sv;
      break;
    default:
      break;
  }
  out << result;

  return out;
}

// ---------- Document ----------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
  objects_.emplace_back(std::move(obj));
}

//<?xml version="1.0" encoding="UTF-8" ?>
//<svg xmlns="http://www.w3.org/2000/svg" version="1.1">
//Объекты, добавленные с помощью svg::Document::Add, в порядке их добавления
//</svg>


void Document::Render(std::ostream& out) const {
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
  for (const auto& o : objects_) {
    out << "  "sv;
    o->Render(out);
  }
  out << "</svg>"sv << std::endl;

}

// ---------- Object ------------------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

//<circle cx="30" cy="70" r="20" fill="rgb(240,240,240)" stroke="black"/>

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
  if (points_.empty()) {
    points_ += ToString(point.x) + ","s + ToString(point.y);
  } else {
    points_ += " "s + ToString(point.x) + ","s + ToString(point.y);
  }

  return *this;
}

//<polyline points="50,10 52.3511,16.7639 4894,16.9098 47.6489,16.7639 50,10" fill="red" stroke="black"/>

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv << points_ << "\""s;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& Text::SetPosition(Point pos) {
      pos_ = pos;

      return *this;
    }

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& Text::SetOffset(Point offset) {
      offset_ = offset;

      return *this;
    }

    // Задаёт размеры шрифта (атрибут font-size)
    Text& Text::SetFontSize(uint32_t size) {
      size_ = size;
      return *this;
    }

    // Задаёт название шрифта (атрибут font-family)
    Text& Text::SetFontFamily(std::string font_family) {
      font_family_ = font_family;
      return *this;
    }

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& Text::SetFontWeight(std::string font_weight) {
      font_weight_ = font_weight;
      return *this;
    }

    std::string Text::Shielding(std::string str) {
      std::string result;

      std::string quot = "&quot;"s;
      std::string apos = "&apos;"s;
      std::string lt = "&lt;"s;
      std::string gt = "&gt;"s;
      std::string amp = "&amp;"s;

      for (const auto& c : str) {
        switch (c) {
          case '"':
            result += quot;
            break;
          case '\'':
            result += apos;
            break;
          case '<':
            result += lt;
            break;
          case '>':
            result += gt;
            break;
          case '&':
            result += amp;
            break;

          default:
            result += c;
            break;
        }
      }



      //      Двойная кавычка " заменяется на &quot;. Точка с запятой в представлении этого и следующих спецсимволов — обязательная часть экранирующей последовательности.
      //      Одинарная кавычка или апостроф ' заменяется на &apos;.
      //      Символы < и > заменяются на &lt; и &gt; соответственно.
      //      Амперсанд & заменяется на &amp;.

      return result;
    }

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& Text::SetData(std::string data) {
      data_ = Shielding(data);
      return *this;
    }

//    <text fill="yellow" stroke="yellow" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" x="10" y="100" dx="0" dy="0" font-size="12" font-family="Verdana">Happy New Year!</text>


    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(out);
        out <<  " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv; // pos
        out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv; // offset
        out << "font-size=\""sv << size_ << "\""sv; // size
        if (!font_family_.empty())
          out << " font-family=\""sv << font_family_ << "\""sv; // font fam
        if (!font_weight_.empty())
          out << " font-weight=\""sv << font_weight_ << "\""sv; // font-w
        out << ">"sv;

        out << data_;

        out << "</text>"sv;
    }

}  // namespace svg



namespace shapes {


void Triangle::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
}


svg::Polyline Star::CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) const {
    using namespace std::literals;


    svg::Polyline polyline;
    polyline.SetFillColor("red"s).SetStrokeColor("black"s);
    for (int i = 0; i <= num_rays; ++i) {
        double angle = 2 * M_PI * (i % num_rays) / num_rays;
        polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
        if (i == num_rays) {
            break;
        }
        angle += M_PI / num_rays;
        polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
    }
    return polyline;
}

void Star::Draw(svg::ObjectContainer& container) const {
    container.Add(CreateStar(center_, outer_rad_, inner_rad_, num_rays_));
}

std::vector<svg::Circle> Snowman::CreateSnowman(svg::Point center, double rad) const {   // порядок рисования снизу-вверх
  using namespace std::literals;

  std::vector<svg::Circle> snowman;
  snowman.push_back(svg::Circle().SetCenter({center.x, center.y + 5*rad}).SetRadius(2*rad).SetFillColor("rgb(240,240,240)"s).SetStrokeColor("black"s));
  snowman.push_back(svg::Circle().SetCenter({center.x, center.y + 2*rad}).SetRadius(1.5*rad).SetFillColor("rgb(240,240,240)"s).SetStrokeColor("black"s));
  snowman.push_back(svg::Circle().SetCenter({center.x, center.y}).SetRadius(rad).SetFillColor("rgb(240,240,240)"s).SetStrokeColor("black"s));

  return snowman;
}

void Snowman::Draw(svg::ObjectContainer& container) const {
  for (const auto& c : CreateSnowman(center_, radius_)) {
    container.Add(c);
  }
}


}  // namespace shapes
