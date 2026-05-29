#pragma once

#include <cmath>
#include <cstdint>
#include <QString>

struct PdfMatrix {
    double a = 1.0, b = 0.0;
    double c = 0.0, d = 1.0;
    double e = 0.0, f = 0.0;

    static PdfMatrix identity() { return {}; }

    static PdfMatrix translation(double tx, double ty) {
        PdfMatrix m;
        m.e = tx; m.f = ty;
        return m;
    }

    static PdfMatrix fromCm(double a, double b, double c, double d, double e, double f) {
        PdfMatrix m;
        m.a = a; m.b = b;
        m.c = c; m.d = d;
        m.e = e; m.f = f;
        return m;
    }

    PdfMatrix operator*(const PdfMatrix &rhs) const {
        PdfMatrix r;
        r.a = a * rhs.a + b * rhs.c;
        r.b = a * rhs.b + b * rhs.d;
        r.c = c * rhs.a + d * rhs.c;
        r.d = c * rhs.b + d * rhs.d;
        r.e = e * rhs.a + f * rhs.c + rhs.e;
        r.f = e * rhs.b + f * rhs.d + rhs.f;
        return r;
    }

    bool nearlyEquals(const PdfMatrix &other, double eps = 1e-9) const {
        return std::abs(a - other.a) < eps && std::abs(b - other.b) < eps
            && std::abs(c - other.c) < eps && std::abs(d - other.d) < eps
            && std::abs(e - other.e) < eps && std::abs(f - other.f) < eps;
    }

    QString toString() const {
        return QStringLiteral("[%1 %2 %3 %4 %5 %6]").arg(a).arg(b).arg(c).arg(d).arg(e).arg(f);
    }
};
