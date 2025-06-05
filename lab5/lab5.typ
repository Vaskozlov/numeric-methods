#import "@preview/oxifmt:0.2.1": strfmt

#set text(size: 12pt)
#set text(lang: "ru")

#let title(
  subject,
  lab_number,
  variant,
  student,
  group,
  teacher,
  city: "Санкт-Петербург",
) = [
  #set align(center)
  #set block(above: 20pt)

  #let inline(body) = box(baseline: 12pt)[#body]
  #let undertitle(title, width: auto, body) = {
    layout(size => {
      stack(
        dir: ttb,
        body,
        block(
          inset: (y: 3pt),
          line(
            length: if width == auto {
              measure(body).width
            } else {
              width
            },
            stroke: .5pt,
          ),
        ),
        text(size: 9pt)[(#title)],
      )
    })
  }

  #grid(
    rows: (1fr, auto),
    [
      #text(size: 12pt)[Федеральное государственное автономное образовательное учреждение высшего образования «Национальный исследовательский университет ИТМО»]

      #block(above: 30pt, below: 40pt)[
        #text(size: 12pt)[Факультет программной инженерии и компьютерной техники]
      ]

      #text(size: 12pt)[#subject]

      #text(size: 12pt)[Отчет по лабораторной работе #lab_number]

      #block(above: 40pt)[
        #text(size: 12pt)[Вариант #variant]
      ]
    ],
    [
      #align(right)[
        #text(size: 12pt)[Выполнил:]

        #text(size: 12pt)[_ #student _ ]

        #text(size: 12pt)[#group]

        #text(size: 12pt)[Проверила:]

        #text(size: 12pt)[_ #teacher _]

        #block(above: 40pt)[]
      ]
    ],
    [#city, #datetime.today().year()]
  )
]

#title(
  "Вычислительная математика",
  text[
    №5

    Интерполяция
  ],
  10,
  "Козлов Василий Сергеевич",
  "P3215",
  "Малышева Татьяна Алексеевна",
)

#set heading(numbering: "1.")
// #show heading: it => text(it.body)
#outline()

#pagebreak()

#set page(
  numbering: (..x) => numbering("1", x.pos().at(0)),
  number-align: center,
)

= Цель лабораторной работы

#par(first-line-indent: (amount: 1.25em, all: true),justify: false)[
  Решить задачу построения кубической сплайн интерполяции функции, заданной на неравномерной сетке.
]

#pagebreak()

= Рабочие формулы
#image("images/formulas.png")

$h_i = x_(i + 1) - x_i$

#pagebreak()

= Код Программы

== Решение системы уравнений методом прогонки
```Cpp
template <std::floating_point T>
auto solveThomas3(
    const std::vector<T> &a, std::vector<T> b, const std::vector<T> &c, std::vector<T> d)
    -> std::vector<T>
{
    assert(a.size() == b.size());
    assert(a.size() == c.size());
    assert(a.size() == d.size());

    const auto n = a.size();
    std::vector<T> x(n);

    for (std::size_t i = 1; i < n; ++i) {
        const auto m = a[i] / b[i - 1];
        b[i] -= m * c[i - 1];
        d[i] -= m * d[i - 1];
    }

    x.back() = d.back() / b.back();

    for (ssize_t i = static_cast<ssize_t>(n) - 2; i >= 0; --i) {
        const auto idx = static_cast<std::size_t>(i);
        x[idx] = (d[idx] - c[idx] * x[idx + 1]) / b[idx];
    }

    return x;
}
```

#pagebreak()

== Сплайн интерполяция

```Cpp
template <std::floating_point T>
struct CubicSplineCoefficients
{
    T a;
    T b;
    T c;
    T d;
};

template <std::floating_point T>
auto createCubicSpline(const std::span<const T> x, const std::span<const T> y)
    -> std::vector<CubicSplineCoefficients<T>>
{
    const auto n = x.size();
    std::vector<T> m_a(n);
    std::vector<T> m_b(n);
    std::vector<T> m_c(n);
    std::vector<T> m_d(n);

    m_b.front() = m_b.back() = 1;

    for (std::size_t i = 1; i < n - 1; ++i) {
        m_a[i] = x[i] - x[i - 1];
        m_b[i] = 2 * (x[i + 1] - x[i - 1]);
        m_c[i] = x[i + 1] - x[i];

        m_d[i] = static_cast<T>(3) * ((y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]));
    }

    auto c = linalg::solveThomas3(
        m_a, std::move(m_b), m_c, std::move(m_d));

    std::vector<CubicSplineCoefficients<T>> result(n - 1);

    for (std::size_t i = 0; i < n - 1; ++i) {
        const auto h = x[i + 1] - x[i];

        result[i].a = y[i];
        result[i].b = (y[i + 1] - y[i]) / h
                      - h * (c[i + 1] + static_cast<T>(2) * c[i]) / static_cast<T>(3);

        result[i].c = c[i];
        result[i].d = (c[i + 1] - c[i]) / (static_cast<T>(3) * h);
    }

    return result;
}
```

== Интерполяция Лагранжа

```Cpp
template <std::floating_point T>
auto lagrange(const std::span<const T> x, const std::span<const T> y, const T x_point) -> T
{
    assert(x.size() == y.size());

    const auto n = x.size();
    T result{};

    for (std::size_t i = 0; i < n; ++i) {
        auto term = y[i];

        for (std::size_t j = 0; j < n; ++j) {
            if (i != j) {
                term *= (x_point - x[j]) / (x[i] - x[j]);
            }
        }

        result += term;
    }

    return result;
}
```

#pagebreak()

= Примеры работы

== Одномерная Интерполяция

- Оранжевым цветом показана функция, которая получилась в результате сплайн-интерполяции

- Синим цветом показана функция, которая получилась в результате интерполяции Лагранжа

#image("images/spline1d.png")

== Двумерная интерполяция

#image("images/spline2d.png")

#pagebreak()

= Выводы

#par(first-line-indent: (amount: 1.25em, all: true),justify: false)[
  В ходе выполнения лабораторной работы была реализована программа по построению сплайн-интерполяции. Получившийся сплайн был сравнен со сплайном из библиотеки scipy, для проверки корректности реализации. Также сплайн был использован для интерполяции 2D, путем интерполирования в начале по одной оси, потом по другой.
]
