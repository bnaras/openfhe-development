// Diagnostic logging channel for OpenFHE.
//
// Call sites that previously wrote directly to std::cerr / std::cout
// should use OPENFHE_LOG_ERR / OPENFHE_LOG_OUT so the output stream
// can be swapped per embedding without touching every call site.
//
// When OPENFHE_R_BUILD is defined (as it is by the `openfhe` R
// package's inst/build_openfhe.sh), the macros expand to an
// ostream-like temporary whose destructor flushes the accumulated
// buffer through R's REprintf / Rprintf entry points. Compiled
// objects then contain NO references to std::cerr / std::cout /
// std::clog, which satisfies R CMD check's "compiled code should
// not write to stdout/stderr instead of to the console" rule
// (Writing R Extensions §1.1.3.1, check step 16).
//
// Otherwise — the upstream OpenFHE build without OPENFHE_R_BUILD —
// the macros expand to the ordinary std::cerr / std::cout streams,
// preserving the pre-existing behavior for non-R embeddings.

#ifndef _LBCRYPTO_UTILS_OPENFHE_LOG_H_
#define _LBCRYPTO_UTILS_OPENFHE_LOG_H_

#ifdef OPENFHE_R_BUILD

#include <sstream>
#include <utility>

#include <R_ext/Print.h>

namespace lbcrypto {

// Accumulates into an ostringstream; flushes on destruction via the
// Rf_printf-shaped function passed in at construction (REprintf or
// Rprintf). Instantiated once per OPENFHE_LOG_ERR / OPENFHE_LOG_OUT
// statement as a temporary, so the destructor (and the single call
// into R's console) fires at the `;` ending the expression.
class ROStream {
    std::ostringstream buf;
    void (*flush_fn)(const char*, ...);
 public:
    explicit ROStream(void (*fn)(const char*, ...)) : flush_fn(fn) {}
    ROStream(const ROStream&) = delete;
    ROStream& operator=(const ROStream&) = delete;
    ~ROStream() {
        const std::string& s = buf.str();
        if (!s.empty()) flush_fn("%s", s.c_str());
    }

    // Forward plain values into the underlying std::ostringstream.
    template <typename T>
    ROStream& operator<<(T&& x) {
        buf << std::forward<T>(x);
        return *this;
    }

    // Explicit overloads for ostream manipulators (std::endl, std::flush,
    // std::hex, std::dec, etc.). Without these, template deduction on
    // `operator<<(T&& x)` fails because these manipulators are overloaded
    // function templates and the compiler can't pick an instantiation.
    ROStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        buf << manip;
        return *this;
    }
    ROStream& operator<<(std::ios& (*manip)(std::ios&)) {
        buf << manip;
        return *this;
    }
    ROStream& operator<<(std::ios_base& (*manip)(std::ios_base&)) {
        buf << manip;
        return *this;
    }
};

}  // namespace lbcrypto

#define OPENFHE_LOG_ERR (::lbcrypto::ROStream(&REprintf))
#define OPENFHE_LOG_OUT (::lbcrypto::ROStream(&Rprintf))

#else  // OPENFHE_R_BUILD not defined — ordinary build

#include <iostream>

#define OPENFHE_LOG_ERR (std::cerr)
#define OPENFHE_LOG_OUT (std::cout)

#endif  // OPENFHE_R_BUILD

#endif  // _LBCRYPTO_UTILS_OPENFHE_LOG_H_
