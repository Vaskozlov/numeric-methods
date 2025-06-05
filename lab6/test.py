def milne_method(f, xs, y0, eps):
    n = len(xs)
    h = xs[1] - xs[0]
    y = [y0]
    for i in range(1, 4):
        k1 = h * f(xs[i - 1], y[i - 1])
        k2 = h * f(xs[i - 1] + h / 2, y[i - 1] + k1 / 2)
        k3 = h * f(xs[i - 1] + h / 2, y[i - 1] + k2 / 2)
        k4 = h * f(xs[i - 1] + h, y[i - 1] + k3)
        y.append(y[i - 1] + (k1 + 2 * k2 + 2 * k3 + k4) / 6)

    for i in range(4, n):
        # Предиктор
        yp = y[i - 4] + 4 * h * (2 * f(xs[i - 3], y[i - 3]) - f(xs[i - 2], y[i - 2]) + 2 * f(xs[i - 1], y[i - 1])) / 3
        print(yp)

        # Корректор
        y_next = yp
        while True:
            yc = y[i - 2] + h * (f(xs[i - 2], y[i - 2]) + 4 * f(xs[i - 1], y[i - 1]) + f(xs[i], y_next)) / 3
            print(f'c: {yc}')
            if abs(yc - y_next) < eps:
                y_next = yc
                break
            y_next = yc

        y.append(y_next)

    return y

xs = [0 + i * (1 - 0) / 5 for i in range(6)]
print(milne_method(lambda x, y: x + y, xs, 1, 1e-4))