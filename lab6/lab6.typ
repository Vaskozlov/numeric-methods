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
    №6

    Решение ОДУ
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
  Решить задачу Коши для обыкновенных дифференциальных уравнений численными методами.
]

#pagebreak()

= Решение ОДУ методом Эйлера
#par(first-line-indent: (amount: 1.25em, all: true),justify: false)[
  Метод Эйлера относится к одношаговым итерационным методам численного решение ОДУ.
]

#par(first-line-indent: (amount: 1.25em, all: true), justify: false)[
  Рабочая формула метода: $y_n = y_(n - 1) + h f(x_(n-1), y_(n-1))$. Рассмотрим численное решение задачи Коши: $y' = x + y", " y(0) = 1, x in [0, 1]$. Точное решение задачи: $y = 2 e^x - x - 1$.
]


#table(
  columns: (1fr, 1fr, 1fr, 1fr, 1fr, 1fr),
  align: horizon + center,
  rows: (3em, 2em),
  [i], [$x_i$], [$y_i$], [$f(x_i, y_i)$], [Точное решение], [Погрешность],
  [0], [0], [1], [1], [1], [0],
  [1], [0.2], [1.2], [1.4], [1.24], [0.04],
  [2], [0.4], [1.48], [1.88], [1.58], [0.1],
  [3], [0.6], [1.86], [2.46], [2.04], [0.18],
  [4], [0.8], [2.35], [3.15], [2.65], [0.3],
  [5], [1.0], [2.98], [3.98], [3.44], [0.46],
)

#pagebreak()
= Решение ОДУ методом Рунге-Кутта четвертого порядка
#par(first-line-indent: (amount: 1.25em, all: true), justify: false)[
  Метод Рунге-Кутта также относится к одношаговым итерационным методам численного решение ОДУ.
]

#par(first-line-indent: (amount: 1.25em, all: true), justify: false)[
  Рабочая формула метода:
]

$
  y_(i + 1) = y_i + 1/6 (k_1 + 2k_2 + 2k_3 + k_4) \
  k_1 = h dot f(x_i, y_i) \
  k_2 = h dot f(x_i + h/2, y_i + k_1/2) \
  k_3 = h dot f(x_i + h/2, y_i + k_2/2) \
  k_4 = h dot f(x_i + h, y_i + k_3) \
$

#table(
  columns: (1fr, 1fr, 1fr, 1fr),
  align: horizon + center,
  rows: (3em, 2em),
  [i], [$x_i$], [$y_i$],[Точное решение], 
  [0], [0], [1], [1],
  [1], [0.2], [1.24], [1.24], 
  [2], [0.4], [1.58],  [1.58],
  [3], [0.6], [2.04],  [2.04],
  [4], [0.8], [2.65],  [2.65],
  [5], [1.0], [3.44],  [3.44],
)

#pagebreak()

= Решение ОДУ методом Милна
#par(first-line-indent: (amount: 1.25em, all: true), justify: false)[
  Метод Милна относится к многошаговым методам решение ОДУ. В начале требуется задать решение в трех первых точках, которые можно получить одношаговым методом Рунге-Кутта.
]

Вычислительные формулы:
$
  y_i^"прогн" = y_(i - 4) + (4h)/3 (2f_(i - 3) - f_(i - 2) + 2f_(i - 1))
$

$
  y_i^"корр" = y_(i_2) + h/3 dot (f_(i - 2) + 4 f_(i - 1) + f_i^"прогн") \
  f_i^"прогн" = f(x_i, y_i^"прогн")
$

#par(first-line-indent: (amount: 1.25em, all: true), justify: false)[
На этапе коррекции высчитывается $y_i^"корр"$, если $abs(y_i^"корр" - y_i^"прогн") < epsilon$, то коррекция выполнена, переходим к следующему $x_i$, иначе $y_i^"прогн" = y_i^"корр"$ и этап коррекции начинается заново.
]

#table(
  columns: (1fr, 1fr, 1fr, 1fr),
  align: horizon + center,
  rows: (3em, 2em),
  [i], [$x_i$], [$y_i$],[Точное решение], 
  [0], [0], [1], [1],
  [1], [0.2], [1.24], [1.24], 
  [2], [0.4], [1.58],  [1.58],
  [3], [0.6], [2.04],  [2.04],
  [4], [0.8], [2.65],  [2.65],
  [5], [1.0], [3.44],  [3.44],
)

#pagebreak()

= Код программы

#pagebreak()

= Вывод