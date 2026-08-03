#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cfenv>
#include <climits>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mpfr.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

// Every theorem-path operation below names its MPFR rounding direction.  The
// only binary64 conversion occurs while writing outward-rounded endpoints.
static mpfr_prec_t interval_precision = 192;

class Interval {
public:
    Interval() {
        mpfr_init2(lo_, interval_precision);
        mpfr_init2(hi_, interval_precision);
        mpfr_set_zero(lo_, 0);
        mpfr_set_zero(hi_, 0);
    }
    Interval(const Interval& other) {
        mpfr_init2(lo_, interval_precision);
        mpfr_init2(hi_, interval_precision);
        mpfr_set(lo_, other.lo_, MPFR_RNDD);
        mpfr_set(hi_, other.hi_, MPFR_RNDU);
    }
    Interval(Interval&& other) noexcept {
        mpfr_init2(lo_, mpfr_get_prec(other.lo_));
        mpfr_init2(hi_, mpfr_get_prec(other.hi_));
        mpfr_swap(lo_, other.lo_);
        mpfr_swap(hi_, other.hi_);
    }
    Interval& operator=(const Interval& other) {
        if (this != &other) {
            mpfr_set(lo_, other.lo_, MPFR_RNDD);
            mpfr_set(hi_, other.hi_, MPFR_RNDU);
        }
        return *this;
    }
    Interval& operator=(Interval&& other) noexcept {
        if (this != &other) {
            mpfr_swap(lo_, other.lo_);
            mpfr_swap(hi_, other.hi_);
        }
        return *this;
    }
    ~Interval() {
        mpfr_clear(lo_);
        mpfr_clear(hi_);
    }

    mpfr_ptr lower() { return lo_; }
    mpfr_ptr upper() { return hi_; }
    mpfr_srcptr lower() const { return lo_; }
    mpfr_srcptr upper() const { return hi_; }

private:
    mpfr_t lo_;
    mpfr_t hi_;
};

static inline bool exact_zero(const Interval& x) {
    return mpfr_zero_p(x.lower()) && mpfr_zero_p(x.upper());
}

static Interval point(double value) {
    if (!std::isfinite(value)) throw std::runtime_error("non-finite point interval");
    Interval out;
    mpfr_set_d(out.lower(), value, MPFR_RNDD);
    mpfr_set_d(out.upper(), value, MPFR_RNDU);
    return out;
}

static Interval sub(const Interval& a, const Interval& b) {
    Interval out;
    mpfr_sub(out.lower(), a.lower(), b.upper(), MPFR_RNDD);
    mpfr_sub(out.upper(), a.upper(), b.lower(), MPFR_RNDU);
    return out;
}

enum class SignClass { nonnegative, nonpositive, mixed };

static SignClass sign_class(const Interval& x) {
    if (mpfr_sgn(x.lower()) >= 0) return SignClass::nonnegative;
    if (mpfr_sgn(x.upper()) <= 0) return SignClass::nonpositive;
    return SignClass::mixed;
}

static Interval mul(const Interval& a, const Interval& b) {
    if (exact_zero(a) || exact_zero(b)) return point(0.0);
    const auto sa = sign_class(a);
    const auto sb = sign_class(b);
    mpfr_srcptr lx = nullptr, ly = nullptr, hx = nullptr, hy = nullptr;
    if (sa == SignClass::nonnegative && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.lower(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::nonnegative && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.lower(); hx = a.lower(); hy = b.upper();
    } else if (sa == SignClass::nonnegative && sb == SignClass::mixed) {
        lx = a.upper(); ly = b.lower(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::nonpositive && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.upper(); hx = a.upper(); hy = b.lower();
    } else if (sa == SignClass::nonpositive && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.upper(); hx = a.lower(); hy = b.lower();
    } else if (sa == SignClass::nonpositive && sb == SignClass::mixed) {
        lx = a.lower(); ly = b.upper(); hx = a.lower(); hy = b.lower();
    } else if (sa == SignClass::mixed && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.upper(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::mixed && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.lower(); hx = a.lower(); hy = b.lower();
    }
    Interval out;
    if (sa != SignClass::mixed || sb != SignClass::mixed) {
        mpfr_mul(out.lower(), lx, ly, MPFR_RNDD);
        mpfr_mul(out.upper(), hx, hy, MPFR_RNDU);
        return out;
    }
    mpfr_mul(out.lower(), a.lower(), b.upper(), MPFR_RNDD);
    mpfr_mul(out.upper(), a.lower(), b.lower(), MPFR_RNDU);
    mpfr_t candidate;
    mpfr_init2(candidate, interval_precision);
    mpfr_mul(candidate, a.upper(), b.lower(), MPFR_RNDD);
    if (mpfr_less_p(candidate, out.lower())) mpfr_set(out.lower(), candidate, MPFR_RNDD);
    mpfr_mul(candidate, a.upper(), b.upper(), MPFR_RNDU);
    if (mpfr_greater_p(candidate, out.upper())) mpfr_set(out.upper(), candidate, MPFR_RNDU);
    mpfr_clear(candidate);
    return out;
}

static Interval rational(std::int64_t numerator, std::int64_t denominator) {
    static_assert(sizeof(long) >= sizeof(std::int64_t), "MPFR integer conversion needs 64-bit long");
    if (denominator <= 0) throw std::runtime_error("nonpositive rational denominator");
    Interval out;
    mpfr_set_si(out.lower(), static_cast<long>(numerator), MPFR_RNDD);
    mpfr_set_si(out.upper(), static_cast<long>(numerator), MPFR_RNDU);
    mpfr_div_si(out.lower(), out.lower(), static_cast<long>(denominator), MPFR_RNDD);
    mpfr_div_si(out.upper(), out.upper(), static_cast<long>(denominator), MPFR_RNDU);
    return out;
}

static inline void accumulate(Interval& a, const Interval& b) {
    if (exact_zero(b)) return;
    mpfr_add(a.lower(), a.lower(), b.lower(), MPFR_RNDD);
    mpfr_add(a.upper(), a.upper(), b.upper(), MPFR_RNDU);
}

static void accumulate_product(Interval& sum, const Interval& a, const Interval& b) {
    if (exact_zero(a) || exact_zero(b)) return;
    const auto sa = sign_class(a);
    const auto sb = sign_class(b);
    mpfr_srcptr lx = nullptr, ly = nullptr, hx = nullptr, hy = nullptr;
    if (sa == SignClass::nonnegative && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.lower(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::nonnegative && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.lower(); hx = a.lower(); hy = b.upper();
    } else if (sa == SignClass::nonnegative && sb == SignClass::mixed) {
        lx = a.upper(); ly = b.lower(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::nonpositive && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.upper(); hx = a.upper(); hy = b.lower();
    } else if (sa == SignClass::nonpositive && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.upper(); hx = a.lower(); hy = b.lower();
    } else if (sa == SignClass::nonpositive && sb == SignClass::mixed) {
        lx = a.lower(); ly = b.upper(); hx = a.lower(); hy = b.lower();
    } else if (sa == SignClass::mixed && sb == SignClass::nonnegative) {
        lx = a.lower(); ly = b.upper(); hx = a.upper(); hy = b.upper();
    } else if (sa == SignClass::mixed && sb == SignClass::nonpositive) {
        lx = a.upper(); ly = b.lower(); hx = a.lower(); hy = b.lower();
    } else {
        const auto product = mul(a, b);
        accumulate(sum, product);
        return;
    }
    mpfr_fma(sum.lower(), lx, ly, sum.lower(), MPFR_RNDD);
    mpfr_fma(sum.upper(), hx, hy, sum.upper(), MPFR_RNDU);
}

static std::size_t checked_product(std::size_t a, std::size_t b, const char* label) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
        throw std::runtime_error(std::string("size overflow in ") + label);
    }
    return a * b;
}

static std::size_t checked_sum(std::size_t a, std::size_t b, const char* label) {
    if (b > std::numeric_limits<std::size_t>::max() - a) {
        throw std::runtime_error(std::string("size overflow in ") + label);
    }
    return a + b;
}

static std::size_t matrix_size(int rows, int cols) {
    if (rows < 0 || cols < 0) throw std::runtime_error("negative matrix dimension");
    return checked_product(static_cast<std::size_t>(rows), static_cast<std::size_t>(cols),
                           "interval matrix");
}

// Later raw signed expressions are at most 82 times a cutoff; recurrence
// rational products use D < 42 times a cutoff.  Dividing INT_MAX by 128
// therefore keeps the former in int and 4*D*(D+2) in signed int64.
static constexpr int supported_cutoff_max = INT_MAX / 128;

struct DimensionInfo {
    int g_count = 0, N = 0, shift_start = 0, maximum_mode = 0;
    int radial = 0, angular = 0, far_D = 0, shape_boundary = 0;
};

static int checked_index(std::int64_t value, const char* label) {
    if (value < 0 || value > INT_MAX) {
        throw std::runtime_error(std::string("signed index overflow in ") + label);
    }
    return static_cast<int>(value);
}

static DimensionInfo checked_dimensions(int L, int S, int R, int J) {
    if (L < 0 || S < 1 || R < 0 || J < 0 || R > L || J > L
        || L > supported_cutoff_max || S > supported_cutoff_max
        || R > supported_cutoff_max || J > supported_cutoff_max) {
        throw std::runtime_error("invalid or unsupported centre dimensions");
    }
    const std::size_t g = checked_product(static_cast<std::size_t>(L) + 1,
                                          static_cast<std::size_t>(S), "centre g dimension");
    const std::size_t n = checked_sum(g, static_cast<std::size_t>(J) + 1,
                                      "centre total dimension");
    if (g > static_cast<std::size_t>(INT_MAX) || n > static_cast<std::size_t>(INT_MAX)) {
        throw std::runtime_error("centre dimension exceeds signed index range");
    }
    DimensionInfo out;
    out.g_count = static_cast<int>(g);
    out.N = static_cast<int>(n);
    out.shift_start = checked_index(2LL * L + R + 1, "shape shift start");
    out.maximum_mode = checked_index(3LL * L + 2LL * R + 1, "maximum tail mode");
    out.radial = checked_index(static_cast<std::int64_t>(S) + 10LL * R + 1,
                               "radial support");
    out.angular = checked_index(static_cast<std::int64_t>(L) + R, "angular support");
    out.far_D = checked_index(2LL * (out.radial + 1LL), "far-g degree");
    out.shape_boundary = checked_index(static_cast<std::int64_t>(out.shift_start) - J - 1,
                                       "shape boundary count");
    return out;
}

static mpfr_prec_t parse_precision_bits(const char* token) {
    if (token == nullptr || *token == '\0') {
        throw std::runtime_error("precision must be ASCII digits in [64,4096]");
    }
    unsigned value = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(token); *p; ++p) {
        if (*p < '0' || *p > '9') {
            throw std::runtime_error("precision must be ASCII digits in [64,4096]");
        }
        value = 10 * value + (*p - '0');
        if (value > 4096) throw std::runtime_error("precision must be ASCII digits in [64,4096]");
    }
    if (value < 64) throw std::runtime_error("precision must be ASCII digits in [64,4096]");
    return static_cast<mpfr_prec_t>(value);
}

static std::streamsize checked_stream_bytes(std::size_t count, const char* label) {
    const std::size_t bytes = checked_product(count, sizeof(double), label);
    if (bytes > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error(std::string("stream size overflow in ") + label);
    }
    return static_cast<std::streamsize>(bytes);
}


static void require_remaining_bytes(std::ifstream& input, std::streamsize expected,
                                    const char* label) {
    const std::streampos current = input.tellg();
    input.seekg(0, std::ios::end);
    const std::streampos end = input.tellg();
    if (current < 0 || end < current || end - current != expected) {
        throw std::runtime_error(std::string("wrong binary payload size in ") + label);
    }
    input.seekg(current);
    if (!input) throw std::runtime_error(std::string("cannot seek binary payload in ") + label);
}

struct Matrix {
    int rows = 0;
    int cols = 0;
    std::vector<Interval> values;
    Matrix() = default;
    Matrix(int r, int c) : rows(r), cols(c), values(matrix_size(r, c)) {}
    Interval& operator()(int i, int j) { return values[static_cast<std::size_t>(i) * cols + j]; }
    const Interval& operator()(int i, int j) const { return values[static_cast<std::size_t>(i) * cols + j]; }
};
static Matrix x_apply(int n, const Matrix& in) {
    Matrix out(in.rows + 1, in.cols);
    for (int s = 0; s < in.rows; ++s) {
        const auto a = rational(static_cast<std::int64_t>(s + 1) * (s + n + 1),
                                static_cast<std::int64_t>(2 * s + n + 1) * (2 * s + n + 2));
        Interval c = point(0.0);
        if (s > 0) {
            c = rational(static_cast<std::int64_t>(s) * (s + n),
                         static_cast<std::int64_t>(2 * s + n) * (2 * s + n + 1));
        }
        const auto b = sub(sub(point(1.0), a), c);
        for (int j = 0; j < in.cols; ++j) {
            accumulate_product(out(s + 1, j), a, in(s, j));
            accumulate_product(out(s, j), b, in(s, j));
            if (s > 0) accumulate_product(out(s - 1, j), c, in(s, j));
        }
    }
    return out;
}

static std::pair<int, Matrix> multiply_step(int mode, const Matrix& in, bool by_z) {
    Matrix out(in.rows + 1, in.cols);
    for (int s = 0; s < in.rows; ++s) {
        if (by_z) {
            if (mode >= 0) {
                const std::int64_t den = 2LL * s + mode + 1;
                const auto a = rational(s + mode + 1, den);
                for (int j = 0; j < in.cols; ++j) accumulate_product(out(s, j), a, in(s, j));
                if (s > 0) {
                    const auto b = rational(s, den);
                    for (int j = 0; j < in.cols; ++j) accumulate_product(out(s - 1, j), b, in(s, j));
                }
            } else {
                const int q = -mode;
                const std::int64_t den = 2LL * s + q + 1;
                const auto a = rational(s + 1, den);
                const auto b = rational(s + q, den);
                for (int j = 0; j < in.cols; ++j) {
                    accumulate_product(out(s + 1, j), a, in(s, j));
                    accumulate_product(out(s, j), b, in(s, j));
                }
            }
        } else {
            if (mode <= 0) {
                const int q = -mode;
                const std::int64_t den = 2LL * s + q + 1;
                const auto a = rational(s + q + 1, den);
                for (int j = 0; j < in.cols; ++j) accumulate_product(out(s, j), a, in(s, j));
                if (s > 0) {
                    const auto b = rational(s, den);
                    for (int j = 0; j < in.cols; ++j) accumulate_product(out(s - 1, j), b, in(s, j));
                }
            } else {
                const int q = mode;
                const std::int64_t den = 2LL * s + q + 1;
                const auto a = rational(s + 1, den);
                const auto b = rational(s + q, den);
                for (int j = 0; j < in.cols; ++j) {
                    accumulate_product(out(s + 1, j), a, in(s, j));
                    accumulate_product(out(s, j), b, in(s, j));
                }
            }
        }
    }
    return {mode + (by_z ? 1 : -1), std::move(out)};
}

static std::pair<int, Matrix> shift_mode(int mode, Matrix in, int quotient_shift) {
    const bool by_z = quotient_shift >= 0;
    if (quotient_shift == INT_MIN || std::abs(quotient_shift) > INT_MAX / 10) {
        throw std::runtime_error("unsupported angular shift");
    }
    const int steps = 10 * std::abs(quotient_shift);
    for (int k = 0; k < steps; ++k) {
        auto next = multiply_step(mode, in, by_z);
        mode = next.first;
        in = std::move(next.second);
    }
    return {mode, std::move(in)};
}

static std::vector<Matrix> radial_powers(int n, int input_rows, int degree) {
    std::vector<Matrix> powers;
    powers.reserve(degree + 1);
    Matrix current(input_rows, input_rows);
    for (int i = 0; i < input_rows; ++i) current(i, i) = point(1.0);
    powers.push_back(current);
    for (int r = 1; r <= degree; ++r) {
        for (int k = 0; k < 10; ++k) current = x_apply(n, current);
        powers.push_back(current);
    }
    return powers;
}

static Matrix clamped_inverse_block(int quotient_mode, int radial_count) {
    const int n = 10 * quotient_mode;
    Matrix K(radial_count + 2, radial_count);
    for (int s = 1; s <= radial_count; ++s) {
        const std::int64_t D = n + 2LL * s;
        K(s - 1, s - 1) = rational(1, 4 * D * (D + 1));
        K(s, s - 1) = rational(-1, 2 * D * (D + 2));
        K(s + 1, s - 1) = rational(1, 4 * (D + 1) * (D + 2));
    }
    return K;
}

struct Header {
    char magic[8];
    std::uint64_t version;
    std::uint64_t header_bytes;
    std::uint64_t L;
    std::uint64_t S;
    std::uint64_t R;
    std::uint64_t J;
    std::uint64_t N;
    std::uint64_t precision_bits;
    std::uint64_t backend;
    std::uint64_t rounding;
    std::uint64_t mpfr_major;
    std::uint64_t mpfr_minor;
    std::uint64_t mpfr_patch;
};

static_assert(sizeof(Header) == 8 + 13 * sizeof(std::uint64_t), "unexpected interval header padding");
static constexpr std::uint64_t format_version = 2;
static constexpr std::uint64_t backend_mpfr = 1;
static constexpr std::uint64_t rounding_directed_endpoints = 1;

static bool hex_binary64_syntax(const std::string& token) {
    std::size_t i = 0;
    if (i < token.size() && (token[i] == '+' || token[i] == '-')) ++i;
    if (i + 2 > token.size() || token[i] != '0' || (token[i + 1] != 'x' && token[i + 1] != 'X')) return false;
    i += 2;
    bool digit = false;
    bool dot = false;
    while (i < token.size() && token[i] != 'p' && token[i] != 'P') {
        const unsigned char ch = static_cast<unsigned char>(token[i]);
        if (std::isxdigit(ch)) {
            digit = true;
        } else if (token[i] == '.' && !dot) {
            dot = true;
        } else {
            return false;
        }
        ++i;
    }
    if (!digit || i == token.size()) return false;
    ++i;
    if (i < token.size() && (token[i] == '+' || token[i] == '-')) ++i;
    const std::size_t exponent_start = i;
    while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) ++i;
    return i == token.size() && i > exponent_start;
}

static Interval parse_hex_binary64(const std::string& token) {
    if (!hex_binary64_syntax(token)) throw std::runtime_error("bad hexadecimal binary64 syntax: " + token);
    Interval out;
    char* end = nullptr;
    const int ternary = mpfr_strtofr(out.lower(), token.c_str(), &end, 0, MPFR_RNDN);
    if (end != token.c_str() + token.size() || ternary != 0 || !mpfr_number_p(out.lower())) {
        throw std::runtime_error("inexact or non-finite hexadecimal token: " + token);
    }
    const double value = mpfr_get_d(out.lower(), MPFR_RNDN);
    if (!std::isfinite(value)) throw std::runtime_error("hexadecimal token is outside binary64 range: " + token);
    mpfr_set_d(out.upper(), value, MPFR_RNDN);
    if (!mpfr_equal_p(out.lower(), out.upper())) {
        throw std::runtime_error("hexadecimal token is not an exact binary64 value: " + token);
    }
    mpfr_set(out.upper(), out.lower(), MPFR_RNDN);
    return out;
}

static std::array<double, 2> outward_binary64(const Interval& value) {
    if (!mpfr_number_p(value.lower()) || !mpfr_number_p(value.upper())
        || !mpfr_lessequal_p(value.lower(), value.upper())) {
        throw std::runtime_error("invalid MPFR interval before binary64 conversion");
    }
    const double lo = mpfr_get_d(value.lower(), MPFR_RNDD);
    const double hi = mpfr_get_d(value.upper(), MPFR_RNDU);
    if (!std::isfinite(lo) || !std::isfinite(hi) || lo > hi) {
        throw std::runtime_error("finite interval cannot be represented by binary64 endpoints");
    }
    return {lo, hi};
}

static Interval endpoint_interval(double lo, double hi) {
    if (!std::isfinite(lo) || !std::isfinite(hi) || lo > hi) {
        throw std::runtime_error("invalid binary64 interval endpoint");
    }
    Interval out;
    mpfr_set_d(out.lower(), lo, MPFR_RNDN);
    mpfr_set_d(out.upper(), hi, MPFR_RNDN);
    return out;
}

static Interval positive_reciprocal(const Interval& x) {
    if (mpfr_sgn(x.lower()) <= 0) throw std::runtime_error("reciprocal is not provably positive");
    Interval out;
    mpfr_ui_div(out.lower(), 1, x.upper(), MPFR_RNDD);
    mpfr_ui_div(out.upper(), 1, x.lower(), MPFR_RNDU);
    return out;
}

static std::pair<double, double> midpoint_radius64(const Interval& x) {
    mpfr_t mid, left, right;
    mpfr_inits2(interval_precision, mid, left, right, static_cast<mpfr_ptr>(nullptr));
    mpfr_add(mid, x.lower(), x.upper(), MPFR_RNDN);
    mpfr_div_2ui(mid, mid, 1, MPFR_RNDN);
    const double midpoint = mpfr_get_d(mid, MPFR_RNDN);
    mpfr_set_d(mid, midpoint, MPFR_RNDN);
    mpfr_sub(left, mid, x.lower(), MPFR_RNDU);
    mpfr_sub(right, x.upper(), mid, MPFR_RNDU);
    if (mpfr_less_p(left, right)) mpfr_set(left, right, MPFR_RNDU);
    const double radius = mpfr_get_d(left, MPFR_RNDU);
    mpfr_clears(mid, left, right, static_cast<mpfr_ptr>(nullptr));
    if (!std::isfinite(midpoint) || !std::isfinite(radius) || radius < 0.0) {
        throw std::runtime_error("midpoint/radius is not finite binary64");
    }
    return {midpoint, radius};
}

struct InverseHeader {
    char magic[8];
    std::uint64_t version;
    std::uint64_t N;
};

struct SparseColumn {
    std::vector<int> row;
    std::vector<double> midpoint;
    std::vector<double> radius;
};

static bool subnormal(double x) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &x, sizeof(bits));
    return (bits & 0x7ff0000000000000ULL) == 0 && (bits & 0x000fffffffffffffULL) != 0;
}

static bool normal_or_zero(double x) { return std::isfinite(x) && !subnormal(x); }

static std::string upper_decimal(double x) {
    if (!(x >= 0.0) || !std::isfinite(x)) throw std::runtime_error("invalid upper bound");
    mpfr_t value;
    mpfr_init2(value, 64);
    mpfr_set_d(value, x, MPFR_RNDN);
    char* text = nullptr;
    if (mpfr_asprintf(&text, "%.25RUf", value) < 0) throw std::runtime_error("MPFR formatting failed");
    std::string result(text);
    mpfr_free_str(text);
    mpfr_clear(value);
    if (result.find('.') != std::string::npos) {
        while (!result.empty() && result.back() == '0') result.pop_back();
        if (!result.empty() && result.back() == '.') result.pop_back();
    }
    return result.empty() ? "0" : result;
}

static void positive_dot_upper(mpfr_ptr out, double computed, std::size_t terms,
                               const std::vector<double>& gamma,
                               const std::vector<double>& tau, mpfr_ptr scratch) {
    if (!(computed >= 0.0) || !std::isfinite(computed)) throw std::runtime_error("invalid positive FMA dot");
    mpfr_set_d(out, computed, MPFR_RNDU);
    mpfr_add_d(out, out, tau[terms], MPFR_RNDU);
    mpfr_set_ui(scratch, 1, MPFR_RNDD);
    mpfr_sub_d(scratch, scratch, gamma[terms], MPFR_RNDD);
    mpfr_div(out, out, scratch, MPFR_RNDU);
}

static void signed_dot_error(mpfr_ptr out, double absolute_dot, std::size_t terms,
                             const std::vector<double>& gamma,
                             const std::vector<double>& tau, mpfr_ptr scratch) {
    positive_dot_upper(out, absolute_dot, terms, gamma, tau, scratch);
    mpfr_mul_d(out, out, gamma[terms], MPFR_RNDU);
    mpfr_add_d(out, out, tau[terms], MPFR_RNDU);
}

static double overflow_guard(std::size_t terms, double amax, double bmax,
                             const std::vector<double>& gamma,
                             const std::vector<double>& tau) {
    if (terms == 0 || amax == 0.0 || bmax == 0.0) return 0.0;
    mpfr_t bound, limit, factor;
    mpfr_inits2(interval_precision, bound, limit, factor, static_cast<mpfr_ptr>(nullptr));
    mpfr_set_d(bound, amax, MPFR_RNDU);
    mpfr_mul_d(bound, bound, bmax, MPFR_RNDU);
    mpfr_mul_ui(bound, bound, terms, MPFR_RNDU);
    mpfr_set_ui(factor, 1, MPFR_RNDU);
    mpfr_add_d(factor, factor, gamma[terms], MPFR_RNDU);
    mpfr_mul(bound, bound, factor, MPFR_RNDU);
    mpfr_add_d(bound, bound, tau[terms], MPFR_RNDU);
    mpfr_set_d(limit, std::numeric_limits<double>::max(), MPFR_RNDD);
    mpfr_div_2ui(limit, limit, 1, MPFR_RNDD);
    if (!mpfr_less_p(bound, limit)) throw std::runtime_error("binary64 FMA overflow guard failed");
    mpfr_div(bound, bound, limit, MPFR_RNDU);
    const double ratio = mpfr_get_d(bound, MPFR_RNDU);
    mpfr_clears(bound, limit, factor, static_cast<mpfr_ptr>(nullptr));
    return ratio;
}

static int verify_finite(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: interval_assemble --verify-finite CENTER.hex INTERVALS.bin INVERSE.bin OUTPUT.json\n";
        return 2;
    }
    std::ifstream intervals(argv[3], std::ios::binary);
    Header header{};
    intervals.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!intervals || std::memcmp(header.magic, "POMINT02", 8) != 0 || header.version != 2
        || header.header_bytes != sizeof(Header) || header.backend != backend_mpfr
        || header.rounding != rounding_directed_endpoints || header.precision_bits < 64
        || header.precision_bits > 4096) {
        throw std::runtime_error("invalid v2 MPFR interval payload header");
    }
    interval_precision = static_cast<mpfr_prec_t>(header.precision_bits);
    if (header.L > static_cast<std::uint64_t>(supported_cutoff_max)
        || header.S > static_cast<std::uint64_t>(supported_cutoff_max)
        || header.R > static_cast<std::uint64_t>(supported_cutoff_max)
        || header.J > static_cast<std::uint64_t>(supported_cutoff_max)) {
        throw std::runtime_error("unsupported interval payload dimensions");
    }
    const DimensionInfo dimensions = checked_dimensions(
        static_cast<int>(header.L), static_cast<int>(header.S),
        static_cast<int>(header.R), static_cast<int>(header.J));
    if (header.N != static_cast<std::uint64_t>(dimensions.N)) {
        throw std::runtime_error("inconsistent interval payload dimension");
    }
    const std::size_t N = static_cast<std::size_t>(dimensions.N);
    const std::size_t matrix_count = checked_product(N, N, "finite verification matrix");
    const std::size_t payload_count = checked_sum(checked_product(2, N, "residual payload"),
        checked_product(2, matrix_count, "interval payload"), "complete interval payload");
    const std::streamsize payload_bytes = checked_stream_bytes(payload_count, "interval payload");
    require_remaining_bytes(intervals, payload_bytes, "interval payload");
    std::vector<double> payload(payload_count);
    intervals.read(reinterpret_cast<char*>(payload.data()), payload_bytes);
    if (!intervals) throw std::runtime_error("cannot read interval payload");

    std::ifstream center(argv[2]);
    int L = 0, S = 0, Rdegree = 0, Jmax = 0;
    if (!(center >> L >> S >> Rdegree >> Jmax) || L != static_cast<int>(header.L)
        || S != static_cast<int>(header.S) || Rdegree != static_cast<int>(header.R)
        || Jmax != static_cast<int>(header.J)) throw std::runtime_error("centre/header mismatch");
    std::string token;
    const DimensionInfo centre_dimensions = checked_dimensions(L, S, Rdegree, Jmax);
    if (centre_dimensions.N != dimensions.N) throw std::runtime_error("centre/header dimension mismatch");
    std::vector<Interval> p(static_cast<std::size_t>(Rdegree + 1));
    for (Interval& x : p) {
        if (!(center >> token)) throw std::runtime_error("truncated centre p coefficients");
        x = parse_hex_binary64(token);
    }
    Interval discarded;
    const std::size_t g_count = static_cast<std::size_t>(centre_dimensions.g_count);
    for (std::size_t i = 0; i < g_count; ++i) {
        if (!(center >> token)) throw std::runtime_error("truncated centre g coefficients");
        discarded = parse_hex_binary64(token);
    }
    if (center >> token) throw std::runtime_error("extra finite centre coefficient");
    if (!center.eof()) throw std::runtime_error("I/O error after finite centre coefficients");
    if (N != g_count + static_cast<std::size_t>(Jmax + 1)) {
        throw std::runtime_error("inconsistent finite centre dimension");
    }
    if (mpfr_sgn(p[0].lower()) <= 0) throw std::runtime_error("p0 is not positive");

    std::ifstream inverse_file(argv[4], std::ios::binary);
    InverseHeader inverse_header{};
    inverse_file.read(reinterpret_cast<char*>(&inverse_header), sizeof(inverse_header));
    if (!inverse_file || std::memcmp(inverse_header.magic, "POMINV1", 8) != 0
        || inverse_header.version != 1 || inverse_header.N != N) {
        throw std::runtime_error("invalid point inverse header");
    }
    const std::streamsize inverse_bytes = checked_stream_bytes(matrix_count, "point inverse");
    require_remaining_bytes(inverse_file, inverse_bytes, "point inverse");
    std::vector<double> inverse(matrix_count);
    inverse_file.read(reinterpret_cast<char*>(inverse.data()), inverse_bytes);
    if (!inverse_file) throw std::runtime_error("cannot read point inverse");
    double inverse_max = 0.0;
    for (double x : inverse) {
        if (!normal_or_zero(x)) throw std::runtime_error("inverse has non-finite/subnormal entry");
        inverse_max = std::max(inverse_max, std::abs(x));
    }

    const int max_mode = std::max(L, Jmax);
    std::vector<Interval> row_mode(static_cast<std::size_t>(max_mode + 1));
    Interval rho = rational(21, 20), power = point(1.0);
    for (int mode = 0; mode <= max_mode; ++mode) {
        row_mode[mode] = mode == 0 ? power : mul(point(2.0), power);
        power = mul(power, rho);
    }
    std::vector<Interval> column_factor(N);
    for (std::size_t col = 0; col < g_count; ++col) {
        column_factor[col] = positive_reciprocal(row_mode[col / static_cast<std::size_t>(S)]);
    }
    power = point(1.0);
    for (int j = 0; j <= Jmax; ++j) {
        column_factor[g_count + j] = positive_reciprocal(mul(mul(point(2.0), p[0]), power));
        power = mul(power, rho);
    }

    std::vector<double> fmid(N), frad(N);
    #pragma omp parallel for schedule(static)
    for (std::int64_t row = 0; row < static_cast<std::int64_t>(N); ++row) {
        const int mode = row < static_cast<std::int64_t>(g_count) ? static_cast<int>(row / S)
                                                                  : static_cast<int>(row - static_cast<std::int64_t>(g_count));
        const Interval raw = endpoint_interval(payload[2 * row], payload[2 * row + 1]);
        const auto pair = midpoint_radius64(mul(raw, row_mode[mode]));
        fmid[row] = pair.first;
        frad[row] = pair.second;
    }
    std::vector<SparseColumn> columns(N);
    #pragma omp parallel for schedule(static)
    for (std::int64_t col = 0; col < static_cast<std::int64_t>(N); ++col) {
        SparseColumn& output = columns[col];
        for (std::size_t row = 0; row < N; ++row) {
            const std::size_t offset = 2 * N + 2 * (row * N + static_cast<std::size_t>(col));
            if (payload[offset] == 0.0 && payload[offset + 1] == 0.0) continue;
            const int mode = row < g_count ? static_cast<int>(row / S) : static_cast<int>(row - g_count);
            const Interval raw = endpoint_interval(payload[offset], payload[offset + 1]);
            const auto pair = midpoint_radius64(mul(mul(raw, row_mode[mode]), column_factor[col]));
            if (pair.first != 0.0 || pair.second != 0.0) {
                output.row.push_back(static_cast<int>(row));
                output.midpoint.push_back(pair.first);
                output.radius.push_back(pair.second);
            }
        }
    }
    double matrix_mid_max = 0.0, matrix_rad_max = 0.0, fmid_max = 0.0, frad_max = 0.0;
    std::size_t matrix_terms = 0, residual_terms = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (subnormal(fmid[i]) || subnormal(frad[i])) throw std::runtime_error("scaled residual has subnormal multiplicand");
        fmid_max = std::max(fmid_max, std::abs(fmid[i]));
        frad_max = std::max(frad_max, frad[i]);
        if (fmid[i] != 0.0 || frad[i] != 0.0) ++residual_terms;
        for (std::size_t k = 0; k < columns[i].row.size(); ++k) {
            if (subnormal(columns[i].midpoint[k]) || subnormal(columns[i].radius[k])) {
                throw std::runtime_error("scaled matrix has subnormal multiplicand");
            }
            matrix_mid_max = std::max(matrix_mid_max, std::abs(columns[i].midpoint[k]));
            matrix_rad_max = std::max(matrix_rad_max, columns[i].radius[k]);
        }
        matrix_terms = std::max(matrix_terms, columns[i].row.size());
    }

    const double unit = std::ldexp(1.0, -53), beta = std::numeric_limits<double>::min();
    std::vector<double> gamma(N + 1), tau(N + 1);
    mpfr_t exact, denominator;
    mpfr_inits2(interval_precision, exact, denominator, static_cast<mpfr_ptr>(nullptr));
    for (std::size_t terms = 1; terms <= N; ++terms) {
        mpfr_set_d(exact, unit, MPFR_RNDU);
        mpfr_mul_ui(exact, exact, terms, MPFR_RNDU);
        mpfr_ui_sub(denominator, 1, exact, MPFR_RNDD);
        mpfr_div(exact, exact, denominator, MPFR_RNDU);
        gamma[terms] = mpfr_get_d(exact, MPFR_RNDU);
        mpfr_set_d(exact, beta, MPFR_RNDU);
        mpfr_mul_ui(exact, exact, terms, MPFR_RNDU);
        mpfr_div(exact, exact, denominator, MPFR_RNDU);
        tau[terms] = mpfr_get_d(exact, MPFR_RNDU);
    }
    mpfr_clears(exact, denominator, static_cast<mpfr_ptr>(nullptr));
    const double guard_mid = overflow_guard(matrix_terms, inverse_max, matrix_mid_max, gamma, tau);
    const double guard_rad = overflow_guard(matrix_terms, inverse_max, matrix_rad_max, gamma, tau);
    const double guard_fmid = overflow_guard(residual_terms, inverse_max, fmid_max, gamma, tau);
    const double guard_frad = overflow_guard(residual_terms, inverse_max, frad_max, gamma, tau);

    if (std::fesetround(FE_TONEAREST) != 0) throw std::runtime_error("cannot select round-to-nearest");
#ifdef _OPENMP
    omp_set_dynamic(0);
#endif
    std::atomic<int> bad_round{0};
    #pragma omp parallel
    {
        if (std::fesetround(FE_TONEAREST) != 0 || std::fegetround() != FE_TONEAREST) bad_round.store(1);
    }
    if (bad_round.load()) throw std::runtime_error("worker floating-point rounding mode is not nearest");

    std::vector<double> z_columns(N);
    #pragma omp parallel for schedule(static)
    for (std::int64_t col = 0; col < static_cast<std::int64_t>(N); ++col) {
        const SparseColumn& source = columns[col];
        const std::size_t terms = source.row.size();
        mpfr_t column_sum, error, tail, value, scratch;
        mpfr_inits2(interval_precision, column_sum, error, tail, value, scratch,
                    static_cast<mpfr_ptr>(nullptr));
        mpfr_set_zero(column_sum, 0);
        for (std::size_t row = 0; row < N; ++row) {
            double product = 0.0, absolute_product = 0.0, radius_product = 0.0;
            const std::size_t base = row * N;
            for (std::size_t k = 0; k < terms; ++k) {
                const double a = inverse[base + static_cast<std::size_t>(source.row[k])];
                product = std::fma(a, source.midpoint[k], product);
                absolute_product = std::fma(std::abs(a), std::abs(source.midpoint[k]), absolute_product);
                radius_product = std::fma(std::abs(a), source.radius[k], radius_product);
            }
            if (!std::isfinite(product)) throw std::runtime_error("non-finite signed FMA dot");
            signed_dot_error(error, absolute_product, terms, gamma, tau, scratch);
            positive_dot_upper(tail, radius_product, terms, gamma, tau, scratch);
            mpfr_set_si(value, row == static_cast<std::size_t>(col) ? 1 : 0, MPFR_RNDN);
            if (mpfr_sub_d(value, value, product, MPFR_RNDN) != 0) {
                throw std::runtime_error("unexpected inexact binary64 defect subtraction");
            }
            mpfr_abs(value, value, MPFR_RNDU);
            mpfr_add(value, value, error, MPFR_RNDU);
            mpfr_add(value, value, tail, MPFR_RNDU);
            mpfr_add(column_sum, column_sum, value, MPFR_RNDU);
        }
        z_columns[col] = mpfr_get_d(column_sum, MPFR_RNDU);
        mpfr_clears(column_sum, error, tail, value, scratch, static_cast<mpfr_ptr>(nullptr));
    }

    std::vector<int> residual_index;
    for (std::size_t k = 0; k < N; ++k) if (fmid[k] != 0.0 || frad[k] != 0.0) residual_index.push_back(k);
    std::vector<double> y_rows(N);
    #pragma omp parallel for schedule(static)
    for (std::int64_t row = 0; row < static_cast<std::int64_t>(N); ++row) {
        double product = 0.0, absolute_product = 0.0, radius_product = 0.0;
        for (int k : residual_index) {
            const double a = inverse[static_cast<std::size_t>(row) * N + k];
            product = std::fma(a, fmid[k], product);
            absolute_product = std::fma(std::abs(a), std::abs(fmid[k]), absolute_product);
            radius_product = std::fma(std::abs(a), frad[k], radius_product);
        }
        if (!std::isfinite(product)) throw std::runtime_error("non-finite residual FMA dot");
        mpfr_t error, tail, value, scratch;
        mpfr_inits2(interval_precision, error, tail, value, scratch, static_cast<mpfr_ptr>(nullptr));
        signed_dot_error(error, absolute_product, residual_index.size(), gamma, tau, scratch);
        positive_dot_upper(tail, radius_product, residual_index.size(), gamma, tau, scratch);
        mpfr_set_d(value, product, MPFR_RNDN);
        mpfr_abs(value, value, MPFR_RNDU);
        mpfr_add(value, value, error, MPFR_RNDU);
        mpfr_add(value, value, tail, MPFR_RNDU);
        y_rows[row] = mpfr_get_d(value, MPFR_RNDU);
        mpfr_clears(error, tail, value, scratch, static_cast<mpfr_ptr>(nullptr));
    }

    mpfr_t y_total, inverse_sum;
    mpfr_inits2(interval_precision, y_total, inverse_sum, static_cast<mpfr_ptr>(nullptr));
    mpfr_set_zero(y_total, 0);
    for (double x : y_rows) mpfr_add_d(y_total, y_total, x, MPFR_RNDU);
    double z_upper = 0.0, inverse_norm = 0.0;
    std::size_t z_index = 0;
    for (std::size_t col = 0; col < N; ++col) {
        if (z_columns[col] > z_upper) { z_upper = z_columns[col]; z_index = col; }
        mpfr_set_zero(inverse_sum, 0);
        for (std::size_t row = 0; row < N; ++row) {
            mpfr_add_d(inverse_sum, inverse_sum, std::abs(inverse[row * N + col]), MPFR_RNDU);
        }
        inverse_norm = std::max(inverse_norm, mpfr_get_d(inverse_sum, MPFR_RNDU));
    }
    const double y_upper = mpfr_get_d(y_total, MPFR_RNDU);
    mpfr_clears(y_total, inverse_sum, static_cast<mpfr_ptr>(nullptr));
    const double guard = std::max(std::max(guard_mid, guard_rad), std::max(guard_fmid, guard_frad));

    std::ofstream output(argv[5]);
    if (!output) throw std::runtime_error("cannot create finite verification output");
    output << "{\n"
           << "  \"N\": " << N << ",\n"
           << "  \"precision_bits\": " << interval_precision << ",\n"
           << "  \"finite_Y\": {\"lo\": \"0\", \"hi\": \"" << upper_decimal(y_upper) << "\"},\n"
           << "  \"finite_Z\": {\"lo\": \"0\", \"hi\": \"" << upper_decimal(z_upper) << "\"},\n"
           << "  \"finite_Z_columns\": [";
    for (std::size_t col = 0; col < N; ++col) {
        if (col) output << ',';
        output << "\"" << upper_decimal(z_columns[col]) << "\"";
    }
    output << "],\n"
           << "  \"max_finite_Z_column\": " << z_index << ",\n"
           << "  \"approx_inverse_l1\": {\"lo\": \"0\", \"hi\": \"" << upper_decimal(inverse_norm) << "\"},\n"
           << "  \"matrix_max_terms\": " << matrix_terms << ",\n"
           << "  \"residual_terms\": " << residual_terms << ",\n"
           << "  \"gamma_max\": \"" << upper_decimal(gamma[N]) << "\",\n"
           << "  \"underflow_beta\": \"0x1p-1022\",\n"
           << "  \"overflow_guard_ratio\": \"" << upper_decimal(guard) << "\",\n"
           << "  \"dot_model\": \"fixed-order std::fma; MPFR RNDU bounds; nonzero subnormal multiplicands rejected\"\n"
           << "}\n";
    if (!output) throw std::runtime_error("failed while writing finite verification output");
    return 0;
}

using Series = std::map<int, Matrix>;

static void add_scaled(Matrix& target, const Matrix& source, const Interval& scale) {
    if (source.cols < 1) throw std::runtime_error("empty tail kernel batch");
    if (target.cols == 0) target = Matrix(source.rows, source.cols);
    if (target.cols != source.cols) throw std::runtime_error("bad tail accumulator");
    if (target.rows < source.rows) {
        target.values.resize(matrix_size(source.rows, source.cols));
        target.rows = source.rows;
    }
    for (int s = 0; s < source.rows; ++s) for (int col = 0; col < source.cols; ++col) {
        accumulate_product(target(s, col), source(s, col), scale);
    }
}

static void add_series(Series& target, int mode, const Matrix& source, const Interval& scale) {
    add_scaled(target[mode], source, scale);
}

// Exact coefficient action of |p|^2 K on one real-symmetric g source.
static Series g_action(int ell, const Matrix& source, const std::vector<Interval>& p) {
    const int R = static_cast<int>(p.size()) - 1;
    std::vector<Matrix> difference(static_cast<std::size_t>(R + 1));
    Matrix power = source;
    for (int r = 0; r <= R; ++r) {
        for (int d = 0; d <= R - r; ++d) {
            add_scaled(difference[d], power, mul(p[r + d], p[r]));
        }
        if (r < R) for (int k = 0; k < 10; ++k) power = x_apply(10 * ell, power);
    }
    Series output;
    add_series(output, ell, difference[0], point(1.0));
    for (int d = 1; d <= R; ++d) {
        auto plus = shift_mode(10 * ell, difference[d], d);
        add_series(output, ell + d, plus.second, point(1.0));
        if (d <= ell) {
            auto lower = shift_mode(10 * ell, difference[d], -d);
            add_series(output, ell - d, lower.second, point(1.0));
        }
        if (ell > 0 && d >= ell) {
            auto mirror = shift_mode(-10 * ell, difference[d], d);
            add_series(output, d - ell, mirror.second, point(1.0));
        }
    }
    return output;
}

static int core_row(int h, int s, int L, int S, int J) {
    if (h >= 0 && h <= L && s >= 1 && s <= S) return h * S + s - 1;
    if (h >= 0 && h <= J && s == 0) return (L + 1) * S + h;
    return -1;
}

static void add_abs(mpfr_ptr sum, const Interval& value, mpfr_ptr scratch) {
    mpfr_abs(scratch, value.lower(), MPFR_RNDU);
    if (mpfr_cmpabs(value.upper(), value.lower()) > 0) mpfr_abs(scratch, value.upper(), MPFR_RNDU);
    mpfr_add(sum, sum, scratch, MPFR_RNDU);
}

static double mpfr_upper_double(mpfr_srcptr value) {
    const double out = mpfr_get_d(value, MPFR_RNDU);
    if (!(out >= 0.0) || !std::isfinite(out)) throw std::runtime_error("invalid tail norm");
    return out;
}

struct SplitColumn {
    std::vector<Interval> core;
    double tail = 0.0;
};

static SplitColumn split_real(const Series& source, int L, int S, int J,
                              const std::vector<Interval>& weights, const Interval& input_inverse,
                              int column = 0) {
    const int N = checked_dimensions(L, S, 0, J).N;
    SplitColumn out{std::vector<Interval>(static_cast<std::size_t>(N)), 0.0};
    mpfr_t tail, scratch;
    mpfr_inits2(interval_precision, tail, scratch, static_cast<mpfr_ptr>(nullptr));
    mpfr_set_zero(tail, 0);
    for (const auto& [h, block] : source) {
        if (h < 0 || h >= static_cast<int>(weights.size())) throw std::runtime_error("real mode outside weight table");
        const Interval scale = mul(weights[h], input_inverse);
        for (int s = 0; s < block.rows; ++s) {
            if (column >= block.cols || exact_zero(block(s, column))) continue;
            const Interval value = mul(block(s, column), scale);
            const int row = core_row(h, s, L, S, J);
            if (row >= 0) accumulate(out.core[row], value); else add_abs(tail, value, scratch);
        }
    }
    out.tail = mpfr_upper_double(tail);
    mpfr_clears(tail, scratch, static_cast<mpfr_ptr>(nullptr));
    return out;
}

static SplitColumn split_shape(const Series& source, int j, int L, int S, int J,
                               const std::vector<Interval>& powers) {
    const int N = checked_dimensions(L, S, 0, J).N;
    SplitColumn out{std::vector<Interval>(static_cast<std::size_t>(N)), 0.0};
    int maximum = 0;
    for (const auto& item : source) maximum = std::max(maximum, std::abs(item.first));
    if (maximum >= static_cast<int>(powers.size())) throw std::runtime_error("shape mode outside weight table");
    const Interval inverse_input = positive_reciprocal(powers[j]);
    mpfr_t tail, scratch;
    mpfr_inits2(interval_precision, tail, scratch, static_cast<mpfr_ptr>(nullptr));
    mpfr_set_zero(tail, 0);
    for (int h = 0; h <= maximum; ++h) {
        auto positive = source.find(h), negative = source.find(-h);
        if (positive == source.end() && negative == source.end()) continue;
        const int rows = std::max(positive == source.end() ? 0 : positive->second.rows,
                                  negative == source.end() ? 0 : negative->second.rows);
        const Interval scale = mul(powers[h], inverse_input);
        for (int s = 0; s < rows; ++s) {
            Interval coefficient;
            if (positive != source.end() && s < positive->second.rows) accumulate(coefficient, positive->second(s, 0));
            if (h > 0 && negative != source.end() && s < negative->second.rows) accumulate(coefficient, negative->second(s, 0));
            if (exact_zero(coefficient)) continue;
            const Interval value = mul(coefficient, scale);
            const int row = core_row(h, s, L, S, J);
            if (row >= 0) accumulate(out.core[row], value); else add_abs(tail, value, scratch);
        }
    }
    out.tail = mpfr_upper_double(tail);
    mpfr_clears(tail, scratch, static_cast<mpfr_ptr>(nullptr));
    return out;
}

struct RoundModel {
    std::vector<double> gamma;
    double beta = std::numeric_limits<double>::min();
};

struct SourceNormalizationAudit {
    std::size_t count = 0, row = 0; std::string key; double midpoint = 0.0, radius = 0.0;
};

static bool enclose_subnormal_source(std::pair<double, double>& pair) {
    const bool changed = subnormal(pair.first) || subnormal(pair.second);
    if (subnormal(pair.first)) {
        mpfr_t bound; mpfr_init2(bound, interval_precision);
        mpfr_set_d(bound, pair.first, MPFR_RNDN); mpfr_abs(bound, bound, MPFR_RNDU);
        mpfr_add_d(bound, bound, pair.second, MPFR_RNDU);
        pair = {0.0, mpfr_get_d(bound, MPFR_RNDU)};
        mpfr_clear(bound);
    }
    if (subnormal(pair.second)) pair.second = std::numeric_limits<double>::min();
    if (!normal_or_zero(pair.first) || !normal_or_zero(pair.second) || pair.second < 0.0) throw std::runtime_error("invalid normal source enclosure");
    return changed;
}

static void numeric_selftest() {
    const double d = std::numeric_limits<double>::denorm_min(), b = std::numeric_limits<double>::min();
    if (!subnormal(d) || subnormal(b) || normal_or_zero(d)) throw std::runtime_error("binary64 classification self-test failed");
    for (double m : {d, -d}) { std::pair<double, double> p{m, 0.0}; if (!enclose_subnormal_source(p) || p.first != 0.0 || p.second < b) throw std::runtime_error("midpoint enclosure self-test failed"); }
    std::pair<double, double> r{1.0, d}, n{1.0, b};
    if (!enclose_subnormal_source(r) || r.second != b || enclose_subnormal_source(n)) throw std::runtime_error("radius/inverse rejection self-test failed");
}

static RoundModel round_model(std::size_t maximum) {
    RoundModel model;
    model.gamma.resize(maximum + 1);
    mpfr_t value, denominator;
    mpfr_inits2(interval_precision, value, denominator, static_cast<mpfr_ptr>(nullptr));
    for (std::size_t n = 1; n <= maximum; ++n) {
        mpfr_set_ui_2exp(value, 1, -53, MPFR_RNDU);
        mpfr_mul_ui(value, value, n, MPFR_RNDU);
        mpfr_ui_sub(denominator, 1, value, MPFR_RNDD);
        if (mpfr_sgn(denominator) <= 0) throw std::runtime_error("roundoff model does not close");
        mpfr_div(value, value, denominator, MPFR_RNDU);
        model.gamma[n] = mpfr_get_d(value, MPFR_RNDU);
    }
    mpfr_clears(value, denominator, static_cast<mpfr_ptr>(nullptr));
    return model;
}

static void dag_positive_upper(mpfr_ptr out, double computed, std::size_t depth,
                               std::size_t operations, const RoundModel& model, mpfr_ptr scratch) {
    if (!(computed >= 0.0) || !std::isfinite(computed) || depth >= model.gamma.size()) {
        throw std::runtime_error("invalid positive reduction");
    }
    mpfr_set_d(out, computed, MPFR_RNDU);
    mpfr_set_d(scratch, model.beta, MPFR_RNDU);
    mpfr_mul_ui(scratch, scratch, operations, MPFR_RNDU);
    mpfr_add(out, out, scratch, MPFR_RNDU);
    mpfr_set_ui(scratch, 1, MPFR_RNDD);
    mpfr_sub_d(scratch, scratch, model.gamma[depth], MPFR_RNDD);
    mpfr_div(out, out, scratch, MPFR_RNDU);
}

// R is exact binary64.  This bounds the full l1 image with one theorem for
// the fixed FMA tree, including interval radii and a normal-underflow reserve.
static double inverse_image(const std::vector<double>& inverse, std::size_t N,
                            const std::vector<Interval>& vector, const RoundModel& model,
                            const std::string& key, SourceNormalizationAudit& audit) {
    SparseColumn source;
    for (std::size_t k = 0; k < N; ++k) {
        if (exact_zero(vector[k])) continue;
        auto pair = midpoint_radius64(vector[k]);
        const auto original = pair;
        if (enclose_subnormal_source(pair)) {
            #pragma omp critical(tail_source_audit)
            {
                ++audit.count;
                if (audit.key.empty() || key < audit.key || (key == audit.key && k < audit.row)) {
                    audit.key = key; audit.row = k;
                    audit.midpoint = original.first; audit.radius = original.second;
                }
            }
        }
        if (subnormal(pair.first) || subnormal(pair.second)) throw std::runtime_error("subnormal source enclosure failed");
        source.row.push_back(static_cast<int>(k));
        source.midpoint.push_back(pair.first);
        source.radius.push_back(pair.second);
    }
    const std::size_t terms = source.row.size();
    if (terms == 0) return 0.0;
    double sum_value = 0.0, sum_absolute = 0.0, sum_radius = 0.0;
    for (std::size_t row = 0; row < N; ++row) {
        double value = 0.0, absolute = 0.0, radius = 0.0;
        for (std::size_t k = 0; k < terms; ++k) {
            const double a = inverse[row * N + static_cast<std::size_t>(source.row[k])];
            value = std::fma(a, source.midpoint[k], value);
            absolute = std::fma(std::abs(a), std::abs(source.midpoint[k]), absolute);
            radius = std::fma(std::abs(a), source.radius[k], radius);
        }
        sum_value = std::fma(1.0, std::abs(value), sum_value);
        sum_absolute = std::fma(1.0, absolute, sum_absolute);
        sum_radius = std::fma(1.0, radius, sum_radius);
    }
    if (!std::isfinite(sum_value) || !std::isfinite(sum_absolute) || !std::isfinite(sum_radius)) {
        throw std::runtime_error("tail inverse product overflowed");
    }
    mpfr_t a, b, c, reserve, result, scratch;
    mpfr_inits2(interval_precision, a, b, c, reserve, result, scratch, static_cast<mpfr_ptr>(nullptr));
    dag_positive_upper(a, sum_value, N, N, model, scratch);
    const std::size_t depth = checked_sum(N, terms, "tail reduction depth");
    const std::size_t operations = checked_product(N, checked_sum(terms, 1, "tail term count"),
                                                   "tail reduction operations");
    const std::size_t reserve_operations = checked_product(N, terms, "tail reserve operations");
    dag_positive_upper(b, sum_absolute, depth, operations, model, scratch);
    dag_positive_upper(c, sum_radius, depth, operations, model, scratch);
    mpfr_mul_d(b, b, model.gamma[terms], MPFR_RNDU);
    mpfr_set_d(reserve, model.beta, MPFR_RNDU);
    mpfr_set_ui(scratch, 1, MPFR_RNDD);
    mpfr_mul_ui(reserve, reserve, reserve_operations, MPFR_RNDU);
    mpfr_sub_d(scratch, scratch, model.gamma[terms], MPFR_RNDD);
    mpfr_div(reserve, reserve, scratch, MPFR_RNDU);
    mpfr_add(result, a, b, MPFR_RNDU);
    mpfr_add(result, result, reserve, MPFR_RNDU);
    mpfr_add(result, result, c, MPFR_RNDU);
    const double output = mpfr_upper_double(result);
    mpfr_clears(a, b, c, reserve, result, scratch, static_cast<mpfr_ptr>(nullptr));
    return output;
}

struct TailCenter {
    int L = 0, S = 0, R = 0, J = 0;
    std::vector<Interval> p, g;
};

static TailCenter read_tail_center(const char* path) {
    std::ifstream input(path);
    TailCenter c;
    if (!(input >> c.L >> c.S >> c.R >> c.J)) {
        throw std::runtime_error("invalid tail centre header");
    }
    const DimensionInfo dimensions = checked_dimensions(c.L, c.S, c.R, c.J);
    c.p.resize(static_cast<std::size_t>(c.R + 1));
    c.g.resize(static_cast<std::size_t>(dimensions.g_count));
    std::string token;
    for (Interval& x : c.p) { if (!(input >> token)) throw std::runtime_error("truncated p"); x = parse_hex_binary64(token); }
    for (Interval& x : c.g) { if (!(input >> token)) throw std::runtime_error("truncated g"); x = parse_hex_binary64(token); }
    if (input >> token) throw std::runtime_error("extra centre coefficient");
    if (!input.eof()) throw std::runtime_error("I/O error after tail centre coefficients");
    if (mpfr_sgn(c.p[0].lower()) <= 0) throw std::runtime_error("p0 is not positive");
    return c;
}

static std::vector<double> read_tail_inverse(const char* path, std::size_t N) {
    std::ifstream input(path, std::ios::binary);
    InverseHeader header{};
    input.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!input || std::memcmp(header.magic, "POMINV1", 8) || header.version != 1 || header.N != N) {
        throw std::runtime_error("invalid tail inverse");
    }
    const std::size_t count = checked_product(N, N, "tail inverse");
    const std::streamsize bytes = checked_stream_bytes(count, "tail inverse");
    require_remaining_bytes(input, bytes, "tail inverse");
    std::vector<double> out(count);
    input.read(reinterpret_cast<char*>(out.data()), bytes);
    if (!input) throw std::runtime_error("cannot read tail inverse");
    for (double x : out) if (!normal_or_zero(x)) throw std::runtime_error("bad tail inverse entry");
    return out;
}

struct ColumnBound { int first = 0, second = 0; double finite = 0.0, tail = 0.0; };

static int verify_tails(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: interval_assemble --verify-tails CENTER.hex INVERSE.bin OUTPUT.json PRECISION_BITS\n";
        return 2;
    }
    const mpfr_prec_t bits = parse_precision_bits(argv[5]);
    interval_precision = bits;
    const std::uint16_t endian_probe = 1;
    if (*reinterpret_cast<const unsigned char*>(&endian_probe) != 1 || sizeof(double) != 8
        || !std::numeric_limits<double>::is_iec559 || std::numeric_limits<double>::digits != 53) {
        throw std::runtime_error("tail verifier requires little-endian IEEE-754 binary64");
    }
#ifdef _OPENMP
    omp_set_dynamic(0);
#endif
    if (std::fesetround(FE_TONEAREST) != 0) throw std::runtime_error("cannot select round-to-nearest");
    std::atomic<int> bad_round{0};
    #pragma omp parallel
    { if (std::fesetround(FE_TONEAREST) != 0 || std::fegetround() != FE_TONEAREST) bad_round.store(1); }
    if (bad_round.load()) throw std::runtime_error("tail worker rounding mode is not nearest");
    const TailCenter c = read_tail_center(argv[2]);
    const DimensionInfo dimensions = checked_dimensions(c.L, c.S, c.R, c.J);
    const int L = c.L, S = c.S, R = c.R, J = c.J;
    const int gcount = dimensions.g_count, N = dimensions.N;
    const auto inverse = read_tail_inverse(argv[3], static_cast<std::size_t>(N));
    const RoundModel model = round_model(checked_product(2, static_cast<std::size_t>(N), "round model"));
    SourceNormalizationAudit source_audit;
    const int shift_start = dimensions.shift_start, maximum_mode = dimensions.maximum_mode;
    std::vector<Interval> powers(static_cast<std::size_t>(maximum_mode + 1));
    powers[0] = point(1.0);
    const Interval rho = rational(21, 20);
    for (int k = 1; k <= maximum_mode; ++k) powers[k] = mul(powers[k - 1], rho);
    std::vector<Interval> weights = powers;
    for (int k = 1; k <= maximum_mode; ++k) weights[k] = mul(point(2.0), powers[k]);

    mpfr_t pnorm, unorm, scratch;
    mpfr_inits2(interval_precision, pnorm, unorm, scratch, static_cast<mpfr_ptr>(nullptr));
    mpfr_set_zero(pnorm, 0);
    mpfr_set_zero(unorm, 0);
    for (int r = 0; r <= R; ++r) add_abs(pnorm, mul(c.p[r], powers[r]), scratch);

    Series residual, vseries;
    for (int ell = 0; ell <= L; ++ell) {
        const Matrix K = clamped_inverse_block(ell, S);
        Matrix v(S + 2, 1);
        for (int row = 0; row < S + 2; ++row) {
            for (int s = 0; s < S; ++s) accumulate_product(v(row, 0), K(row, s), c.g[ell * S + s]);
        }
        Matrix u = v;
        if (ell == 0) accumulate(u(0, 0), point(1.0));
        for (int s = 0; s < u.rows; ++s) add_abs(unorm, mul(u(s, 0), weights[ell]), scratch);
        const Series part = g_action(ell, u, c.p);
        for (const auto& [h, block] : part) add_series(residual, h, block, point(1.0));
        add_series(vseries, ell, v, point(1.0));
        if (ell > 0) add_series(vseries, -ell, v, point(1.0));
    }
    const int radial = dimensions.radial, angular = dimensions.angular;
    for (const auto& [h, block] : residual) {
        const int maximum_s = h <= L ? radial : S + 1 + 10 * (angular - h);
        if (h < 0 || h > angular) throw std::runtime_error("omitted residual angular support escaped");
        for (int s = maximum_s + 1; s < block.rows; ++s) {
            if (!exact_zero(block(s, 0))) throw std::runtime_error("omitted residual radial support escaped");
        }
    }
    const double omitted_y = split_real(residual, L, S, J, weights, point(1.0)).tail;
    std::vector<ColumnBound> omitted_rows;
    auto record_residual = [&](int h, int s) {
        double value = 0.0;
        const auto mode = residual.find(h);
        if (mode != residual.end() && s < mode->second.rows) {
            mpfr_t sum, temporary;
            mpfr_inits2(interval_precision, sum, temporary, static_cast<mpfr_ptr>(nullptr));
            mpfr_set_zero(sum, 0);
            add_abs(sum, mul(mode->second(s, 0), weights[h]), temporary);
            value = mpfr_upper_double(sum);
            mpfr_clears(sum, temporary, static_cast<mpfr_ptr>(nullptr));
        }
        omitted_rows.push_back({h, s, 0.0, value});
    };
    for (int h = 0; h <= L; ++h) {
        if (h > J) record_residual(h, 0);
        for (int s = S + 1; s <= radial; ++s) record_residual(h, s);
    }
    for (int h = L + 1; h <= angular; ++h) {
        for (int s = 0; s <= S + 1 + 10 * (angular - h); ++s) record_residual(h, s);
    }

    std::vector<ColumnBound> tasks;
    for (int ell = 0; ell <= L; ++ell) for (int s = 1; s <= S; ++s) tasks.push_back({ell, s});
    const std::size_t finite_g_tasks = tasks.size();
    for (int ell = 0; ell <= L; ++ell) for (int s = S + 1; s <= radial; ++s) tasks.push_back({ell, s});
    for (int ell = L + 1; ell <= angular; ++ell) {
        for (int s = 1; s <= S + 1 + 10 * (angular - ell); ++s) tasks.push_back({ell, s});
    }
    const std::size_t boundary_count = tasks.size() - finite_g_tasks;
    std::vector<double> finite_tail(static_cast<std::size_t>(N));
    #pragma omp parallel for schedule(dynamic,1)
    for (int ell = 0; ell <= angular; ++ell) {
        std::vector<std::size_t> indices;
        int maximum_s = 0;
        for (std::size_t i = 0; i < tasks.size(); ++i) if (tasks[i].first == ell) {
            indices.push_back(i);
            maximum_s = std::max(maximum_s, tasks[i].second);
        }
        if (indices.empty()) continue;
        Matrix batch(maximum_s + 2, static_cast<int>(indices.size()));
        for (std::size_t col = 0; col < indices.size(); ++col) {
            const int s = tasks[indices[col]].second;
            const std::int64_t D = 10LL * ell + 2LL * s;
            batch(s - 1, col) = rational(1, 4 * D * (D + 1));
            batch(s, col) = rational(-1, 2 * D * (D + 2));
            batch(s + 1, col) = rational(1, 4 * (D + 1) * (D + 2));
        }
        const Series action = g_action(ell, batch, c.p);
        const Interval input_inverse = positive_reciprocal(weights[ell]);
        for (std::size_t col = 0; col < indices.size(); ++col) {
            ColumnBound& task = tasks[indices[col]];
            const SplitColumn split = split_real(action, L, S, J, weights, input_inverse,
                                                  static_cast<int>(col));
            task.tail = split.tail;
            if (indices[col] < finite_g_tasks) {
                finite_tail[static_cast<std::size_t>(ell * S + task.second - 1)] = task.tail;
            } else {
                const std::string key = "g:" + std::to_string(ell) + ':' + std::to_string(task.second);
                task.finite = inverse_image(inverse, static_cast<std::size_t>(N), split.core, model,
                                            key, source_audit);
            }
        }
    }

    const Interval inverse_p0 = positive_reciprocal(c.p[0]);
    std::vector<Interval> ratio(static_cast<std::size_t>(R + 1));
    ratio[0] = point(1.0);
    for (int r = 1; r <= R; ++r) ratio[r] = mul(c.p[r], inverse_p0);
    Series shape;
    for (const auto& [mode, block] : vseries) {
        for (int r = 0; r <= R; ++r) {
            auto shifted = shift_mode(10 * mode, block, -r);
            add_series(shape, mode - r, shifted.second, ratio[r]);
        }
    }
    for (int r = 1; r <= R; ++r) {
        Matrix pure(1, 1);
        pure(0, 0) = ratio[r];
        add_series(shape, -r, pure, point(1.0));
    }
    std::vector<ColumnBound> shape_boundary(static_cast<std::size_t>(dimensions.shape_boundary));
    std::vector<std::vector<Interval>> shape_core(shape_boundary.size());
    std::vector<double> far_p_modes(static_cast<std::size_t>(shift_start));
    double far_p = 0.0;
    for (int j = 0; j <= shift_start; ++j) {
        SplitColumn split = split_shape(shape, j, L, S, J, powers);
        if (j <= J) finite_tail[static_cast<std::size_t>(gcount + j)] = split.tail;
        else if (j < shift_start) {
            ColumnBound& bound = shape_boundary[static_cast<std::size_t>(j - J - 1)];
            bound.first = j;
            bound.tail = split.tail;
            shape_core[static_cast<std::size_t>(j - J - 1)] = std::move(split.core);
        } else {
            for (const auto& item : shape) if (item.first <= L) throw std::runtime_error("shape shift cutoff did not clear core modes");
            far_p = split.tail;
            for (int h = L + 1; h <= shift_start + L; ++h) {
                const auto mode = shape.find(h);
                if (mode == shape.end()) continue;
                mpfr_t sum, temporary;
                mpfr_inits2(interval_precision, sum, temporary, static_cast<mpfr_ptr>(nullptr));
                mpfr_set_zero(sum, 0);
                const Interval scale = mul(powers[h], positive_reciprocal(powers[j]));
                for (int s = 0; s < mode->second.rows; ++s) add_abs(sum, mul(mode->second(s, 0), scale), temporary);
                far_p_modes[static_cast<std::size_t>(h - L - 1)] = mpfr_upper_double(sum);
                mpfr_clears(sum, temporary, static_cast<mpfr_ptr>(nullptr));
            }
        }
        if (j < shift_start) {
            Series next;
            for (const auto& [mode, block] : shape) {
                auto shifted = shift_mode(10 * mode, block, 1);
                if (shifted.first % 10) throw std::runtime_error("nonintegral shifted shape mode");
                add_series(next, shifted.first / 10, shifted.second, point(1.0));
            }
            shape = std::move(next);
        }
    }
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < shape_boundary.size(); ++i)
        shape_boundary[i].finite = inverse_image(inverse, static_cast<std::size_t>(N), shape_core[i], model, "p:" + std::to_string(shape_boundary[i].first), source_audit);

    Interval pbound;
    mpfr_set_zero(pbound.lower(), 0);
    mpfr_set(pbound.upper(), pnorm, MPFR_RNDU);
    const int far_D = dimensions.far_D;
    Interval kappa = rational(1, 4LL * far_D * (far_D + 1));
    accumulate(kappa, rational(1, 2LL * far_D * (far_D + 2)));
    accumulate(kappa, rational(1, 4LL * (far_D + 1) * (far_D + 2)));
    const double far_g = mpfr_get_d(mul(mul(pbound, pbound), kappa).upper(), MPFR_RNDU);
    if (boundary_count != tasks.size() - finite_g_tasks
        || static_cast<int>(shape_boundary.size()) != shift_start - J - 1) {
        throw std::runtime_error("tail support count mismatch");
    }

    std::ofstream output(argv[4]);
    if (!output) throw std::runtime_error("cannot create tail output");
    output << "{\n  \"precision_bits\": " << bits
           << ",\n  \"mpfr_version\": \"" << mpfr_get_version() << "\""
           << ",\n  \"mpfr_buildopt_tls_p\": true"
           << ",\n  \"compiler\": \"" << __VERSION__ << "\""
#ifdef _OPENMP
           << ",\n  \"threads\": " << omp_get_max_threads()
#else
           << ",\n  \"threads\": 1"
#endif
           << ",\n  \"subnormal_source_enclosures\": {\"count\": " << source_audit.count << ", \"minimum_key\": \"" << source_audit.key << "\", \"row\": " << source_audit.row
           << ", \"midpoint_hex\": \"" << std::hexfloat << source_audit.midpoint
           << "\", \"radius_hex\": \"" << source_audit.radius << std::defaultfloat << "\"}"
           << ",\n  \"support\": {\"finite_columns\": " << N
           << ", \"g_boundary_columns\": " << boundary_count
           << ", \"p_boundary_first\": " << J + 1
           << ", \"p_boundary_last\": " << shift_start - 1
           << ", \"far_g_min_D\": " << far_D
           << ", \"shape_shift_start\": " << shift_start << "},\n"
           << "  \"y_omitted\": \"" << upper_decimal(omitted_y) << "\",\n"
           << "  \"p_norm\": \"" << upper_decimal(mpfr_upper_double(pnorm)) << "\",\n"
           << "  \"u_norm\": \"" << upper_decimal(mpfr_upper_double(unorm)) << "\",\n"
           << "  \"far_g\": \"" << upper_decimal(far_g) << "\",\n"
           << "  \"far_p\": \"" << upper_decimal(far_p) << "\",\n  \"omitted_rows\": [";
    for (std::size_t i = 0; i < omitted_rows.size(); ++i) {
        if (i) output << ',';
        output << "[\"" << omitted_rows[i].first << ':' << omitted_rows[i].second
               << "\",\"" << upper_decimal(omitted_rows[i].tail) << "\"]";
    }
    output << "],\n  \"far_p_modes\": [";
    for (std::size_t i = 0; i < far_p_modes.size(); ++i) {
        if (i) output << ',';
        output << "[\"" << L + 1 + static_cast<int>(i) << "\",\"" << upper_decimal(far_p_modes[i]) << "\"]";
    }
    output << "],\n  \"finite_tail\": [";
    for (std::size_t i = 0; i < finite_tail.size(); ++i) {
        if (i) output << ',';
        output << "\"" << upper_decimal(finite_tail[i]) << "\"";
    }
    output << "],\n  \"g_boundary\": [";
    for (std::size_t i = finite_g_tasks; i < tasks.size(); ++i) {
        if (i != finite_g_tasks) output << ',';
        output << "[\"" << tasks[i].first << ':' << tasks[i].second << "\",\""
               << upper_decimal(tasks[i].finite) << "\",\"" << upper_decimal(tasks[i].tail) << "\"]";
    }
    output << "],\n  \"p_boundary\": [";
    for (std::size_t i = 0; i < shape_boundary.size(); ++i) {
        if (i) output << ',';
        output << "[\"" << shape_boundary[i].first << "\",\"" << upper_decimal(shape_boundary[i].finite)
               << "\",\"" << upper_decimal(shape_boundary[i].tail) << "\"]";
    }
    output << "]\n}\n";
    if (!output) throw std::runtime_error("failed while writing tail output");
    mpfr_clears(pnorm, unorm, scratch, static_cast<mpfr_ptr>(nullptr));
    return 0;
}

int main(int argc, char** argv) {
    if (mpfr_buildopt_tls_p() == 0) {
        std::cerr << "fatal: MPFR was built without thread-safe TLS support\n";
        return 1;
    }
    numeric_selftest();
    if (argc > 1 && std::strcmp(argv[1], "--verify-finite") == 0) {
        try {
            return verify_finite(argc, argv);
        } catch (const std::exception& error) {
            std::cerr << "fatal: " << error.what() << "\n";
            return 1;
        }
    }
    if (argc > 1 && std::strcmp(argv[1], "--verify-tails") == 0) {
        try {
            return verify_tails(argc, argv);
        } catch (const std::exception& error) {
            std::cerr << "fatal: " << error.what() << "\n";
            return 1;
        }
    }
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: interval_assemble CENTER.hex OUTPUT.bin [PRECISION_BITS]\n";
        return 2;
    }
    try {
        if (argc == 4) {
            interval_precision = parse_precision_bits(argv[3]);
        }
        const std::uint16_t endian_probe = 1;
        if (*reinterpret_cast<const unsigned char*>(&endian_probe) != 1
            || sizeof(double) != 8 || !std::numeric_limits<double>::is_iec559
            || std::numeric_limits<double>::digits != 53) {
            throw std::runtime_error("compact interval format requires little-endian IEEE-754 binary64");
        }
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open centre file");
        int L = 0, S = 0, R = 0, Jmax = 0;
        if (!(input >> L >> S >> R >> Jmax)) throw std::runtime_error("bad centre header");
        const DimensionInfo dimensions = checked_dimensions(L, S, R, Jmax);
        const int g_count = dimensions.g_count, N = dimensions.N;
        const auto matrix_count = checked_product(static_cast<std::size_t>(N), static_cast<std::size_t>(N), "Jacobian");
        const auto residual_count = checked_product(static_cast<std::size_t>(L + 1), static_cast<std::size_t>(N), "residual parts");
        const auto shape_count = checked_product(
            residual_count, static_cast<std::size_t>(Jmax + 1), "shape parts");
        std::vector<Interval> pi(static_cast<std::size_t>(R + 1));
        std::vector<Interval> gi(static_cast<std::size_t>(g_count));
        std::string token;
        for (Interval& x : pi) {
            if (!(input >> token)) throw std::runtime_error("truncated p coefficients");
            x = parse_hex_binary64(token);
        }
        for (Interval& x : gi) {
            if (!(input >> token)) throw std::runtime_error("truncated g coefficients");
            x = parse_hex_binary64(token);
        }
        if (input >> token) throw std::runtime_error("extra token after centre coefficients: " + token);
        if (!input.eof()) throw std::runtime_error("I/O error after centre coefficients");
        if (mpfr_sgn(pi[0].lower()) <= 0) throw std::runtime_error("p0 is not positive");

        const int input_rows = S + 2;
        std::vector<Interval> jacobian(matrix_count);
        std::vector<Interval> residual_parts(residual_count);
        std::vector<Interval> shape_parts(shape_count);

        auto residual_row = [=](int h, int s) -> int {
            if (h >= 0 && h <= L && s >= 1 && s <= S) return h * S + s - 1;
            if (h >= 0 && h <= Jmax && s == 0) return g_count + h;
            return -1;
        };
        auto g_column = [=](int l, int s) -> int { return l * S + s - 1; };
        auto& J = jacobian;
        const auto start = std::chrono::steady_clock::now();

        #pragma omp parallel for schedule(static,1)
        for (int l = 0; l <= L; ++l) {
            auto fpart = [&](int row) -> Interval& {
                return residual_parts[static_cast<std::size_t>(l) * N + row];
            };
            auto cpart = [&](int row, int j) -> Interval& {
                return shape_parts[(static_cast<std::size_t>(l) * N + row) * (Jmax + 1) + j];
            };
            const auto powers = radial_powers(10 * l, input_rows, R);
            const Matrix K = clamped_inverse_block(l, S);
            std::vector<Interval> U(static_cast<std::size_t>(input_rows));
            for (int i = 0; i < input_rows; ++i) {
                for (int j = 0; j < S; ++j) accumulate_product(U[i], K(i, j), gi[l * S + j]);
            }
            if (l == 0) accumulate(U[0], point(1.0));

            std::vector<Matrix> radial_difference;
            radial_difference.reserve(R + 1);
            for (int d = 0; d <= R; ++d) {
                Matrix M(powers[R - d].rows, input_rows);
                for (int r = 0; r <= R - d; ++r) {
                    const auto coefficient = mul(pi[r + d], pi[r]);
                    for (int i = 0; i < powers[r].rows; ++i) {
                        for (int j = 0; j < input_rows; ++j) {
                            accumulate_product(M(i, j), coefficient, powers[r](i, j));
                        }
                    }
                }
                radial_difference.push_back(std::move(M));
            }

            auto add_g_block = [&](int h, const Matrix& B) {
                if (l == 0) {
                    for (int s = 0; s <= S; ++s) {
                        const int row = residual_row(h, s);
                        if (row >= 0) accumulate(fpart(row), B(s, 0));
                    }
                }
                for (int s = 0; s <= S; ++s) {
                    const int row = residual_row(h, s);
                    if (row < 0) continue;
                    for (int j = 0; j < S; ++j) {
                        Interval value = point(0.0);
                        for (int a = 0; a < input_rows; ++a) accumulate_product(value, B(s, a), K(a, j));
                        // Each l owns its g columns, so this write is race-free.
                        accumulate(J[static_cast<std::size_t>(row) * N + g_column(l, j + 1)], value);
                        accumulate_product(fpart(row), value, gi[l * S + j]);
                    }
                }
            };

            for (int ds = -R; ds <= R; ++ds) {
                const int h = l + ds;
                if (h < 0 || h > L) continue;
                auto shifted = shift_mode(10 * l, radial_difference[std::abs(ds)], ds);
                if (shifted.first != 10 * h) throw std::runtime_error("mode-shift invariant failed");
                add_g_block(h, shifted.second);
            }
            if (l > 0) {
                for (int h = 0; h <= std::min(L, R - l); ++h) {
                    const int d = h + l;
                    auto shifted = shift_mode(-10 * l, radial_difference[d], d);
                    if (shifted.first != 10 * h) throw std::runtime_error("mirror-shift invariant failed");
                    add_g_block(h, shifted.second);
                }
            }

            Matrix Pmatrix(powers[R].rows, R + 1);
            for (int r = 0; r <= R; ++r) {
                for (int i = 0; i < powers[r].rows; ++i) {
                    Interval value = point(0.0);
                    for (int a = 0; a < input_rows; ++a) accumulate_product(value, powers[r](i, a), U[a]);
                    Pmatrix(i, r) = value;
                }
            }
            auto add_shape_block = [&](int h, int d, const Matrix& V) {
                for (int s = 0; s <= S; ++s) {
                    const int row = residual_row(h, s);
                    if (row < 0) continue;
                    for (int r = 0; r <= R; ++r) {
                        const int high = r + d;
                        if (high <= Jmax) accumulate_product(cpart(row, high), pi[r], V(s, r));
                        if (r <= Jmax && r + d <= R) accumulate_product(cpart(row, r), pi[r + d], V(s, r));
                    }
                }
            };
            const int shape_shift = std::max(R, Jmax);
            for (int h = std::max(0, l - shape_shift); h <= std::min(L, l + shape_shift); ++h) {
                const int ds = h - l;
                auto shifted = shift_mode(10 * l, Pmatrix, ds);
                if (shifted.first != 10 * h) throw std::runtime_error("shape-shift invariant failed");
                add_shape_block(h, std::abs(ds), shifted.second);
            }
            if (l > 0) {
                for (int h = 0; h <= std::max(0, shape_shift - l); ++h) {
                    const int d = h + l;
                    auto shifted = shift_mode(-10 * l, Pmatrix, d);
                    if (shifted.first != 10 * h) throw std::runtime_error("shape-mirror invariant failed");
                    add_shape_block(h, d, shifted.second);
                }
            }
        }

        // Deterministic reductions over source angular modes.
        #pragma omp parallel for schedule(static)
        for (int row = 0; row < N; ++row) {
            Interval value = point(0.0);
            for (int l = 0; l <= L; ++l) accumulate(value, residual_parts[static_cast<std::size_t>(l) * N + row]);
            residual_parts[row] = value;  // reuse the first block as final residual storage
            for (int j = 0; j <= Jmax; ++j) {
                Interval entry = point(0.0);
                for (int l = 0; l <= L; ++l) {
                    accumulate(entry, shape_parts[(static_cast<std::size_t>(l) * N + row) * (Jmax + 1) + j]);
                }
                jacobian[static_cast<std::size_t>(row) * N + g_count + j] = entry;
            }
        }
        for (int l = 0; l <= L; ++l) {
            for (int s = 1; s <= S; ++s) {
                const int row = residual_row(l, s);
                const int col = g_column(l, s);
                accumulate(jacobian[static_cast<std::size_t>(row) * N + col], point(1.0));
                accumulate(residual_parts[row], gi[col]);
            }
        }

        Header header{};
        std::memcpy(header.magic, "POMINT02", 8);
        header.version = format_version;
        header.header_bytes = sizeof(Header);
        header.L = L;
        header.S = S;
        header.R = R;
        header.J = Jmax;
        header.N = N;
        header.precision_bits = static_cast<std::uint64_t>(interval_precision);
        header.backend = backend_mpfr;
        header.rounding = rounding_directed_endpoints;
        header.mpfr_major = MPFR_VERSION_MAJOR;
        header.mpfr_minor = MPFR_VERSION_MINOR;
        header.mpfr_patch = MPFR_VERSION_PATCHLEVEL;
        std::ofstream output(argv[2], std::ios::binary);
        if (!output) throw std::runtime_error("cannot create output file");
        output.write(reinterpret_cast<const char*>(&header), sizeof(header));
        auto write_intervals = [&](const std::vector<Interval>& v, std::size_t count) {
            std::vector<double> buffer;
            buffer.reserve(8192);
            auto flush = [&]() {
                output.write(reinterpret_cast<const char*>(buffer.data()),
                             static_cast<std::streamsize>(buffer.size() * sizeof(double)));
                buffer.clear();
            };
            for (std::size_t i = 0; i < count; ++i) {
                const auto endpoints = outward_binary64(v[i]);
                buffer.push_back(endpoints[0]);
                buffer.push_back(endpoints[1]);
                if (buffer.size() == buffer.capacity()) flush();
            }
            if (!buffer.empty()) flush();
        };
        write_intervals(residual_parts, N);
        write_intervals(jacobian, matrix_count);
        output.close();
        if (!output) throw std::runtime_error("failed while writing compact interval payload");

        double residual_max = 0.0;
        double residual_width = 0.0;
        for (int i = 0; i < N; ++i) {
            const auto endpoints = outward_binary64(residual_parts[i]);
            residual_max = std::max(residual_max, std::max(std::abs(endpoints[0]), std::abs(endpoints[1])));
            residual_width = std::max(residual_width, endpoints[1] - endpoints[0]);
        }
        const auto stop = std::chrono::steady_clock::now();
        std::cerr << std::setprecision(17)
                  << "backend=MPFR version=" << mpfr_get_version()
                  << " precision_bits=" << interval_precision
                  << " mpfr_buildopt_tls_p=1"
                  << " rounding=RNDD/RNDU\n"
                  << "assembly_seconds=" << std::chrono::duration<double>(stop - start).count() << "\n"
                  << "N=" << N << " residual_abs_max=" << residual_max
                  << " residual_width_max=" << residual_width << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << "\n";
        return 1;
    }
}
