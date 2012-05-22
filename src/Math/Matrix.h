#ifndef Magnum_Math_Matrix_h
#define Magnum_Math_Matrix_h
/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Magnum::Math::Matrix
 */

#include "Vector.h"

namespace Magnum { namespace Math {

#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Implementation {
    template<size_t size, class T> class MatrixDeterminant;

    template<size_t ...> struct Sequence {};

    /* E.g. GenerateSequence<3>::Type is Sequence<0, 1, 2> */
    template<size_t N, size_t ...sequence> struct GenerateSequence:
        GenerateSequence<N-1, N-1, sequence...> {};

    template<size_t ...sequence> struct GenerateSequence<0, sequence...> {
        typedef Sequence<sequence...> Type;
    };
}
#endif

/**
 * @brief %Matrix
 *
 * @todo @c PERFORMANCE - loop unrolling for Matrix<3, T> and Matrix<4, T>
 * @todo first col, then row (cache adjacency)
 */
template<size_t size, class T> class Matrix {
    friend class Matrix<size+1, T>; /* for ij() */

    public:
        /**
         * @brief %Matrix from array
         * @return Reference to the data as if it was Matrix, thus doesn't
         *      perform any copying.
         *
         * @attention Use with caution, the function doesn't check whether the
         *      array is long enough.
         */
        inline constexpr static Matrix<size, T>& from(T* data) {
            return *reinterpret_cast<Matrix<size, T>*>(data);
        }
        /** @overload */
        inline constexpr static const Matrix<size, T>& from(const T* data) {
            return *reinterpret_cast<const Matrix<size, T>*>(data);
        }

        /**
         * @brief %Matrix from column vectors
         * @param first First column vector
         * @param next  Next column vectors
         */
        template<class ...U> inline constexpr static Matrix<size, T> from(const Vector<size, T>& first, const U&... next) {
            static_assert(sizeof...(next)+1 == size, "Improper number of arguments passed to Matrix from Vector constructor");
            return from(typename Implementation::GenerateSequence<size>::Type(), first, next...);
        }

        /** @brief Pass to constructor to create zero-filled matrix */
        enum ZeroType { Zero };

        /**
         * @brief Zero-filled matrix constructor
         *
         * Use this constructor by calling `Matrix m(Matrix::Zero);`.
         */
        inline constexpr explicit Matrix(ZeroType): _data() {}

        /** @brief Pass to constructor to create identity matrix */
        enum IdentityType { Identity };

        /**
         * @brief Default constructor
         *
         * You can also explicitly call this constructor with
         * `Matrix m(Matrix::Identity);`. Optional parameter @p value allows
         * you to specify value on diagonal.
         */
        inline explicit Matrix(IdentityType = Identity, T value = T(1)): _data() {
            for(size_t i = 0; i != size; ++i)
                _data[size*i+i] = value;
        }

        /**
         * @brief Initializer-list constructor
         * @param first First value
         * @param next  Next values
         *
         * Note that the values are in column-major order.
         * @todoc Remove workaround when Doxygen supports uniform initialization
         */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        template<class ...U> inline constexpr Matrix(T first, U... next): _data{first, next...} {
            static_assert(sizeof...(next)+1 == size*size, "Improper number of arguments passed to Matrix constructor");
        }
        #else
        template<class ...U> inline constexpr Matrix(T first, U... next);
        #endif

        /** @brief Copy constructor */
        inline constexpr Matrix(const Matrix<size, T>& other) = default;

        /** @brief Assignment operator */
        inline Matrix<size, T>& operator=(const Matrix<size, T>& other) = default;

        /**
         * @brief Raw data
         * @return One-dimensional array of `size*size` length in column-major
         *      order.
         */
        inline T* data() { return _data; }
        inline constexpr const T* data() const { return _data; } /**< @overload */

        /** @brief %Matrix column */
        inline Vector<size, T>& operator[](size_t col) {
            return Vector<size, T>::from(_data+col*size);
        }
        /** @overload */
        inline constexpr const Vector<size, T>& operator[](size_t col) const {
            return Vector<size, T>::from(_data+col*size);
        }

        /** @brief Equality operator */
        inline bool operator==(const Matrix<size, T>& other) const {
            for(size_t i = 0; i != size*size; ++i)
                if(!MathTypeTraits<T>::equals(_data[i], other._data[i])) return false;

            return true;
        }

        /** @brief Non-equality operator */
        inline constexpr bool operator!=(const Matrix<size, T>& other) const {
            return !operator==(other);
        }

        /** @brief Multiply matrix operator */
        Matrix<size, T> operator*(const Matrix<size, T>& other) const {
            Matrix<size, T> out(Zero);

            for(size_t row = 0; row != size; ++row)
                for(size_t col = 0; col != size; ++col)
                    for(size_t pos = 0; pos != size; ++pos)
                        out(col, row) += (*this)(pos, row)*other(col, pos);

            return out;
        }

        /** @brief Multiply and assign matrix operator */
        inline Matrix<size, T>& operator*=(const Matrix<size, T>& other) {
            return (*this = *this*other);
        }

        /** @brief Multiply vector operator */
        Vector<size, T> operator*(const Vector<size, T>& other) const {
            Vector<size, T> out;

            for(size_t row = 0; row != size; ++row)
                for(size_t pos = 0; pos != size; ++pos)
                    out[row] += (*this)(pos, row)*other[pos];

            return out;
        }

        /** @brief Transposed matrix */
        Matrix<size, T> transposed() const {
            Matrix<size, T> out(Zero);

            for(size_t row = 0; row != size; ++row)
                for(size_t col = 0; col != size; ++col)
                    out(row, col) = (*this)(col, row);

            return out;
        }

        /** @brief %Matrix without given column and row */
        Matrix<size-1, T> ij(size_t skipCol, size_t skipRow) const {
            Matrix<size-1, T> out(Matrix<size-1, T>::Zero);

            for(size_t row = 0; row != size-1; ++row)
                for(size_t col = 0; col != size-1; ++col)
                    out(col, row) = (*this)(col + (col >= skipCol),
                                            row + (row >= skipRow));

            return out;
        }

        /**
         * @brief Determinant
         *
         * Computed recursively using Laplace's formula:
         * @f[
         * \det(A) = \sum_{j=1}^n (-1)^{i+j} a_{i,j} \det(A^{i,j})
         * @f]
         * @f$ A^{i, j} @f$ is matrix without i-th row and j-th column, see
         * ij(). The formula is expanded down to 2x2 matrix, where the
         * determinant is computed directly:
         * @f[
         * \det(A) = a_{0, 0} a_{1, 1} - a_{1, 0} a_{0, 1}
         * @f]
         */
        inline T determinant() const { return Implementation::MatrixDeterminant<size, T>()(*this); }

        /**
         * @brief Inverted matrix
         *
         * Computed using Cramer's rule:
         * @f[
         * A^{-1} = \frac{1}{\det(A)} Adj(A)
         * @f]
         */
        Matrix<size, T> inverted() const {
            Matrix<size, T> out(Zero);

            T _determinant = determinant();

            for(size_t row = 0; row != size; ++row)
                for(size_t col = 0; col != size; ++col)
                    out(col, row) = (((row+col) & 1) ? -1 : 1)*ij(row, col).determinant()/_determinant;

            return out;
        }

    private:
        template<size_t ...sequence, class ...U> inline constexpr static Matrix<size, T> from(Implementation::Sequence<sequence...> s, const Vector<size, T>& first, U... next) {
            return from(s, next..., first[sequence]...);
        }

        template<size_t ...sequence, class ...U> inline constexpr static Matrix<size, T> from(Implementation::Sequence<sequence...> s, T first, U... next) {
            return Matrix<size, T>(first, next...);
        }

        /* Used internally instead of [][], because GCC does some heavy
           optimalization in release mode which breaks it */
        inline T& operator()(size_t col, size_t row) {
            return _data[col*size+row];
        }
        inline constexpr const T& operator()(size_t col, size_t row) const {
            return _data[col*size+row];
        }

        T _data[size*size];
};

#ifndef DOXYGEN_GENERATING_OUTPUT
#define MAGNUM_MATRIX_SUBCLASS_IMPLEMENTATION(Type, VectorType, size)       \
    inline constexpr static Type<T>& from(T* data) {                        \
        return *reinterpret_cast<Type<T>*>(data);                           \
    }                                                                       \
    inline constexpr static const Type<T>& from(const T* data) {            \
        return *reinterpret_cast<const Type<T>*>(data);                     \
    }                                                                       \
    template<class ...U> inline constexpr static Type<T> from(const Vector<size, T>& first, const U&... next) { \
        return Matrix<size, T>::from(first, next...);                       \
    }                                                                       \
                                                                            \
    inline Type<T>& operator=(const Type<T>& other) {                       \
        Matrix<size, T>::operator=(other);                                  \
        return *this;                                                       \
    }                                                                       \
                                                                            \
    inline VectorType<T>& operator[](size_t col) {                          \
        return VectorType<T>::from(Matrix<size, T>::data()+col*size);       \
    }                                                                       \
    inline constexpr const VectorType<T>& operator[](size_t col) const {    \
        return VectorType<T>::from(Matrix<size, T>::data()+col*size);       \
    }                                                                       \
                                                                            \
    inline Type<T> operator*(const Matrix<size, T>& other) const {          \
        return Matrix<size, T>::operator*(other);                           \
    }                                                                       \
    inline Type<T>& operator*=(const Matrix<size, T>& other) {              \
        Matrix<size, T>::operator*=(other);                                 \
        return *this;                                                       \
    }                                                                       \
    inline VectorType<T> operator*(const Vector<size, T>& other) const {    \
        return Matrix<size, T>::operator*(other);                           \
    }                                                                       \
                                                                            \
    inline Type<T> transposed() const { return Matrix<size, T>::transposed(); } \
    inline Type<T> inverted() const { return Matrix<size, T>::inverted(); }

namespace Implementation {

template<size_t size, class T> class MatrixDeterminant {
    public:
        /** @brief Functor */
        T operator()(const Matrix<size, T>& m) {
            T out(0);

            for(size_t col = 0; col != size; ++col)
                out += ((col & 1) ? -1 : 1)*m[col][0]*m.ij(col, 0).determinant();

            return out;
        }
};

template<class T> class MatrixDeterminant<2, T> {
    public:
        /** @brief Functor */
        inline constexpr T operator()(const Matrix<2, T>& m) {
            return m[0][0]*m[1][1] - m[1][0]*m[0][1];
        }
};

template<class T> class MatrixDeterminant<1, T> {
    public:
        /** @brief Functor */
        inline constexpr T operator()(const Matrix<1, T>& m) {
            return m[0][0];
        }
};

}

template<class T, size_t size> Corrade::Utility::Debug operator<<(Corrade::Utility::Debug debug, const Magnum::Math::Matrix<size, T>& value) {
    debug << "Matrix(";
    debug.setFlag(Corrade::Utility::Debug::SpaceAfterEachValue, false);
    for(size_t row = 0; row != size; ++row) {
        if(row != 0) debug << ",\n       ";
        for(size_t col = 0; col != size; ++col) {
            if(col != 0) debug << ", ";
            debug << value[col][row];
        }
    }
    debug << ')';
    debug.setFlag(Corrade::Utility::Debug::SpaceAfterEachValue, true);
    return debug;
}
#endif

}}

#endif
