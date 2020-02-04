/**
 * This file is part of liberrc.
 *
 *  liberrc is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  liberrc is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  and GNU Lesser General Public License along with liberrc.  If not,
 *  see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBERRC_ERRC_H
#define LIBERRC_ERRC_H

#include <type_traits>
#include <ostream>
#include <cmath>
#include <compare>

template <typename T = long double , typename E = long double>
class ErrorValue {

    static_assert(std::is_arithmetic<T>::value && !std::is_same<T, bool>::value,
            "Type of ErrorValue value must be arithmetic, but not bool");
    static_assert(std::is_floating_point<E>::value,
                  "Type of ErrorValue error value must be float, double or long double");

public:

    const int DEF_ERROR_ZERO = 0;
    const int DEF_ERROR_HALF = 1;
    const int DEF_ERROR_FUNC = 2;

    T value;
    E error;

    //------- CONSTRUCTORS -------

    [[nodiscard]] ErrorValue() = default;
    [[nodiscard]] ErrorValue(const ErrorValue &ev) = default;
    [[nodiscard]] ErrorValue(T value_, E error_) : value(value_), error(error_) {};

    //------- ASSIGMENT OPERATORS -------

    ErrorValue& operator=(const ErrorValue &ev) {
        if (*ev != this) {
            value = ev.value, error = ev.error;
            return *this;
        }
    }

    ErrorValue& operator=(T value_) {
        value = value_;
    }

    //------- COMPOUND ASSIGMENT OPERATORS -------

    ErrorValue operator+=(const ErrorValue &ev) {
        value += ev.value;
        error += ev.error;
    }

    ErrorValue operator+=(T value_) {
        *this += ErrorValue(value_, defaultNumberError(value_));
    }

    ErrorValue operator-=(const ErrorValue &ev) {
        value -= ev.value;
        error += ev.error;
    }

    ErrorValue operator-=(T value_) {
        *this -= ErrorValue(value_, defaultNumberError(value_));
    }

    ErrorValue operator*=(const ErrorValue &ev) {
        T oldV = value;
        value *= ev.value;
        error = value*(error/oldV + ev.error/ev.value);
    }

    ErrorValue operator*=(T value_) {
        *this *= ErrorValue(value_, defaultNumberError(value_));
    }

    ErrorValue operator/=(const ErrorValue &ev) {
        T oldV = value;
        value /= ev.value;
        error = value*(error/oldV + ev.error/ev.value);
    }

    ErrorValue operator/=(T value_) {
        *this /= ErrorValue(value_, defaultNumberError(value_));
    }

    //------- ARITHMETIC OPERATORS -------

    ErrorValue operator+(const ErrorValue &ev) const {
        ErrorValue res = *this;
        res += ev;
        return res;
    }

    ErrorValue operator+(const T &value_) const {
        ErrorValue res = *this;
        res += value_;
        return res;
    }

    ErrorValue operator-(const ErrorValue &ev) const {
        ErrorValue res = *this;
        res -= ev;
        return res;
    }

    ErrorValue operator-(const T &value_) const {
        ErrorValue res = *this;
        res -= value_;
        return res;
    }

    ErrorValue operator*(const ErrorValue &ev) const {
        ErrorValue res = *this;
        res *= ev;
        return res;
    }

    ErrorValue operator*(const T &value_) const {
        ErrorValue res = *this;
        res *= value_;
        return res;
    }

    ErrorValue operator/(const ErrorValue &ev) const {
        ErrorValue res = *this;
        res /= ev;
        return res;
    }

    ErrorValue operator/(const T &value_) const {
        ErrorValue res = *this;
        res /= value_;
        return res;
    }

    ErrorValue operator+() const {
        return ErrorValue(*this);
    }

    ErrorValue operator-() const {
        return ErrorValue(-value, error);
    }

    ErrorValue& operator++() {
        return (*this += 1);
    }

    const ErrorValue operator++(int) {
        ErrorValue tmp(*this);
        ++(*this);
        return tmp;
    }

    ErrorValue& operator--() {
        return (*this -= 1);
    }

    const ErrorValue operator--(int) {
        ErrorValue tmp(*this);
        --(*this);
        return tmp;
    }

    //------- COMPARISON OPERATORS -------

    std::weak_ordering operator<=>(const ErrorValue  &ev) const {
        return (value <=> ev.value);
    }

    //------- MEMBER OPERATORS -------

    E operator[](int i) const {
        switch(i) {
            case 0:
                return value;
            case 1:
                return error;
            default:
                throw std::range_error("ErrorValue index must be 0 or 1");
        }
    }

    //------- STATIC_CAST CONVERSION OPERATORS -------

    explicit operator T() const {
        return value;
    };

    //------- VOID METHODS -------

    void set(T value_, E error_) {
        value = value_;
        error = error_;
    }

    void setDefaultErrorCalculationMethod(int code, std::function<E(T)> fun = nullptr) {
        switch(code) {
            case DEF_ERROR_FUNC:
                defaultErrorCalcFunction = fun;
                [[fallthrough]];
            case DEF_ERROR_ZERO:
                [[fallthrough]];
            case DEF_ERROR_HALF:
               numberDefaultErrorCode = code;
            default:
                throw std::range_error("Invalid default error function code: " + std::to_string(code));
        }
    }

    //------- NON-VOID METHODS -------

    [[nodiscard]] E min() const {
        return value - error;
    }

    [[nodiscard]] E max() const {
        return value + error;
    }

    [[nodiscard]] int getDefaultErrorCalculationMethod() const {
        return numberDefaultErrorCode;
    }

    [[nodiscard]] std::function<E(T)> getDefaultErrorCalcFunction() const {
        if (numberDefaultErrorCode != DEF_ERROR_FUNC)
            return nullptr;
        return defaultErrorCalcFunction;
    }

protected:

    int numberDefaultErrorCode = DEF_ERROR_ZERO;
    std::function<E(T)> defaultErrorCalcFunction = nullptr;

    E defaultNumberError(T x) {
        switch (numberDefaultErrorCode) {
            case DEF_ERROR_ZERO:
                return 0;
                break;
            case DEF_ERROR_HALF:
                return halfErrorCalcFunction(x);
                break;
                [[unlikely]] case DEF_ERROR_FUNC:
                return defaultErrorCalcFunction(x);
                break;
        }
    }

    E halfErrorCalcFunction(T x) {
        if (floor(x) == x) {
            long double c = 0;
            while (x % 10 == 0) {
                c++;
                x /= 10;
            }
            return 5*pow(10, --c);
        } else {
            long double c = 0;
            while (floor(x) != x) {
                c++;
                x *= 10;
            }
            return 5*pow(10, -c-1);
        }
    }

};

template <typename T, typename E>
std::ostream& operator<<(std::ostream& os, const ErrorValue<T,E> &ev) {
    os << ev.value << " ± " + ev.error;
    return os;
}

// CMath functions

#ifndef LIBERRC_ADD_ERRMATH

namespace errmath {

    template <typename T, typename E>
    auto sin(const ErrorValue<T, E> &x) {
        return ErrorValue(sin(x.value), abs(cos(x.value))*x.error);
    }

    template <typename T, typename E>
    auto cos(const ErrorValue<T, E> &x) {
        return ErrorValue(cos(x.value), abs(sin(x.value)*x.error));
    }

    template <typename T, typename E>
    auto tan(const ErrorValue<T, E> &x) {
        return ErrorValue(tan(x.value), x.error/pow(cos(x.value, 2)));
    }

    template <typename T, typename E>
    auto asin(const ErrorValue<T, E> &x) {
        return ErrorValue(asin(x.value), x.error/sqrt(1 - x.value*x.value));
    }

    template <typename T, typename E>
    auto acos(const ErrorValue<T, E> &x) {
        return ErrorValue(acos(x.value), x.error/sqrt(1 - x.value*x.value));
    }

    template <typename T, typename E>
    auto atan(const ErrorValue<T, E> &x) {
        return ErrorValue(atan(x.value), x.error/sqrt(1 + x.value*x.value));
    }

    template <typename T, typename E, typename T1, typename E1>
    auto atan2(const ErrorValue<T, E> &y, const ErrorValue<T1, E1> &x) {
        return atan(y/x);
    }

    template <typename T, typename E>
    auto sinh(const ErrorValue<T, E> &x) {
        return ErrorValue(sinh(x.value), cosh(x.value)*x.error);
    }

    template <typename T, typename E>
    auto cosh(const ErrorValue<T, E> &x) {
        return ErrorValue(cosh(x.value), abs(sinh(x.value))*x.error);
    }

    template <typename T, typename E>
    auto tanh(const ErrorValue<T, E> &x) {
        return ErrorValue(tanh(x.value), x.error/pow(cosh(x.value, 2)));
    }

    template <typename T, typename E>
    auto asinh(const ErrorValue<T, E> &x) {
        return ErrorValue(asinh(x.value), x.error/sqrt(1 + x.value*x.value));
    }

    template <typename T, typename E>
    auto acosh(const ErrorValue<T, E> &x) {
        return ErrorValue(acosh(x.value), x.error/sqrt(x.value*x.value - 1));
    }

    template <typename T, typename E>
    auto atanh(const ErrorValue<T, E> &x) {
        return ErrorValue(atanh(x.value), x.error/(1 - x.value*x.value));
    }

    template <typename T, typename E>
    auto exp(const ErrorValue<T, E> &x) {
        return ErrorValue(exp(x.value), exp(x.value)*x.error);
    }

    template <typename T, typename E>
    auto log10(const ErrorValue<T, E> &x) {
        return ErrorValue(log10(x.value), x.error/(x.value * log(static_cast<E>(10))));
    }

    template <typename T, typename E>
    auto exp2(const ErrorValue<T, E> &x) {
        return ErrorValue(exp2(x.value), exp2(x.value)*log(static_cast<E>(2))*x.error);
    }

    template <typename T, typename E>
    auto log2(const ErrorValue<T, E> &x) {
        return ErrorValue(log2(x.value), x.error/(x.value*log(static_cast<E>(2))));
    }

    template <typename T, typename E>
    auto log(const ErrorValue<T, E> &x) {
        return ErrorValue(log(x.value), x.error/x.value);
    }

    template <typename T, typename E>
    auto expm1(const ErrorValue<T, E> &x) {
        return ErrorValue(expm1(x.value), exp(x.value)*x.error);
    }

    template <typename T, typename E>
    auto log1p(const ErrorValue<T, E> &x) {
        return ErrorValue(log1p(x.value), x.error/log(1 + x.value));
    }

    template <typename T, typename E, typename N>
    typename std::enable_if<std::is_floating_point<T>::value, ErrorValue<T, E>>::type
    logn(ErrorValue<T , E> x, N n) {
        static_assert(std::is_integral<N>::value,
                      "Type of logn base value must be integral");
        return ErrorValue<T, E>(
                log(x.value)/log(n),
                x.error/(x.value*log(n))
                );
    }

    template <typename T, typename E, typename N>
    typename std::enable_if<std::is_integral<T>::value, ErrorValue<double , E>>::type
    logn(ErrorValue<T , E> x, N n) {
        static_assert(std::is_integral<N>::value,
                      "Type of logn base value must be integral");
        return ErrorValue<double , E>(
                log(x.value)/log(n),
                x.error/(x.value*log(n))
        );
    }

    template <typename T, typename E, typename T1, typename E1>
    auto pow(const ErrorValue<T , E>& base, const ErrorValue<T1, E1>& exponent) {
        T x = base.value, y = exponent.value;
        T dx = base.error, dy = exponent.error;
        return ErrorValue(
                pow(x, y),
                abs(y*pow(x, y - 1))*dx + abs(pow(x, y)*log(x))*dy
                );
    }

    template <typename T, typename E, typename N>
    auto pow(const ErrorValue<T , E>& base, N exponent) {
        static_assert(std::is_arithmetic<N>::value && !std::is_same<N, bool>::value,
                      "Type of exponent base value must be arithmetic, but not bool");
        return ErrorValue(
                pow(base.value, exponent),
                abs(exponent*pow(base.value, exponent - 1))*base.error
        );
    }

    template <typename T, typename E>
    auto sqrt(const ErrorValue<T, E> &x) {
        return ErrorValue(sqrt(x.value), x.error/2*sqrt(x.value));
    }

    template <typename T, typename E>
    auto cbrt(const ErrorValue<T, E> &x) {
        return ErrorValue(cbrt(x.value), x.error/3*cbrt(x.value));
    }

    template <typename T, typename E, typename T1, typename E1>
    auto hypot(const ErrorValue<T , E>& x_, const ErrorValue<T1, E1>& y_) {
        T x = x_.value, y = y_.value;
        T dx = x_.error, dy = y_.error;
        return ErrorValue(
                hypot(x, y),
                (x*dx + y*dy)/sqrt(x*x + y*y)
                );
    }

    template <typename T, typename E>
    auto erf(const ErrorValue<T, E> &x);

    template <typename T, typename E>
    auto erfc(const ErrorValue<T, E> &x);

    template <typename T, typename E>
    auto tgamma(const ErrorValue<T, E> &x);

    template <typename T, typename E>
    auto lgamma(const ErrorValue<T, E> &x);

    template <typename T, typename E>
    auto abs(const ErrorValue<T, E> &x);

    template <typename T, typename E, typename T1, typename E1, typename T2, typename E2>
    auto fma(const ErrorValue<T, E> &x, const ErrorValue<T1, E1> &y, const ErrorValue<T2, E2> &z);
}

#endif //LIBERRC_ADD_ERRMATH

#endif //LIBERRC_ERRC_H
