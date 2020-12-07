#pragma once

#include "utils/containers.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <any>
#include <cmath>
#include <iostream>

#ifdef DETRAY_CUSTOM_SCALARTYPE
using detray_scalar = DETRAY_CUSTOM_SCALARTYPE;
#else
using detray_scalar = double;
#endif

namespace detray
{
    using scalar = detray_scalar;

    // eigen getter methdos
    namespace getter
    {
        /** This method retrieves phi from a vector, vector base with rows > 2
         * 
         * @param v the input vector 
         **/
        template <typename vector_type>
        auto phi(const vector_type &v) noexcept
        {
            return std::atan2(v[0], v[1]);
        }

        /** This method retrieves theta from a vector, vector base with rows >= 3
         * 
         * @param v the input vector 
         **/
        template <typename vector_type>
        auto theta(const vector_type &v) noexcept
        {
            return std::atan2(std::sqrt(v[0] * v[0] + v[1] * v[1]), v[2]);
        }

        /** This method retrieves the perpenticular magnitude of a vector with rows >= 2
         * 
         * @param v the input vector 
         **/
        template <typename vector_type>
        auto perp(const vector_type &v) noexcept
        {
            return std::sqrt(v[0] * v[0] + v[1] * v[1]);
        }

        /** This method retrieves the norm of a vector, no dimension restriction
         * 
         * @param v the input vector 
         **/
        auto norm(const std::array<scalar, 2> &v)
        {
            return perp(v);
        }

        /** This method retrieves the norm of a vector, no dimension restriction
         * 
         * @param v the input vector 
         **/
        auto norm(const std::array<scalar, 3> &v)
        {
            return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        }

        /** This method retrieves the pseudo-rapidity from a vector or vector base with rows >= 3
         * 
         * @param v the input vector 
         **/
        template <typename vector_type>
        auto eta(const vector_type &v) noexcept
        {
            return std::atanh(v[2] / norm(v));
        }

        /** This method retrieves a column from a matrix
         * 
         * @param m the input matrix 
         **/
        template <unsigned int kROWS, unsigned int kCOLS, typename matrix_type>
        auto block(const Eigen::MatrixBase<derived_type> &m, unsigned int row, unsigned int col) noexcept
        {
            std::array< std::array<float, kROWS>, kCOLS> submatrix;
            for (unsigned int icol = col; icol < col+kCOLS; ++icol){
                for (unsigned int irow = row, irow < row +kCOLS; ++irow){
                    submatrix[icol-col][irow-row] = m[icol][irow];
                }
            }
            
        }

    } // namespace getter

    // eigen definitions
    namespace eigen
    {
        /** Transform wrapper class to ensure standard API within differnt plugins
         * 
         **/
        struct transform3
        {
            using vector3 = std::array<scalar, 3>;
            using point3 = vector3;
            using context = std::any;

            /** Contructor with arguments: t, z, x, ctx
             * 
             * @param t the translation (or origin of the new frame)
             * @param z the z axis of the new frame, normal vector for planes
             * @param x the x axis of the new frame
             * 
             **/
            transform3(const vector3 &t, const vector3 &z, const vector3 &x, const context & /*ctx*/)
            {
                auto y = z.cross(x);

                auto &matrix = _data.matrix();
                matrix.block<3, 1>(0, 0) = x;
                matrix.block<3, 1>(0, 1) = y;
                matrix.block<3, 1>(0, 2) = z;
                matrix.block<3, 1>(0, 3) = t;
            }

            /** Constructor with arguments: translation
             *
             * @param t is the transform
             **/
            transform3(const vector3 &t, const context & /*ctx*/)
            {
                auto &matrix = _data.matrix();
                matrix.block<3, 1>(0, 3) = t;
            }

            /** Constructor with arguments: matrix 
             * 
             * @param m is the full 4x4 matrix 
             **/
            transform3(const matrix44 &m, const context & /*ctx*/)
            {
                _data.matrix() = m;
            }

            /** Constructor with arguments: matrix as std::aray of scalar
             * 
             * @param ma is the full 4x4 matrix asa 16 array
             **/
            transform3(const std::array<scalar, 16> &ma, const context & /*ctx*/)
            {
                _data.matrix() << ma[0], ma[1], ma[2], ma[3], ma[4], ma[5], ma[6], ma[7],
                ma[8], ma[9], ma[10], ma[11], ma[12], ma[13], ma[14], ma[15];
            }

            /** Default contructors */
            transform3(const transform3& rhs) = default;
            ~transform3() = default;

            /** This method retrieves the rotation of a transform
             * 
             * @param ctx the context object
             * 
             * @note this is a contextual method
             **/
            auto rotation(const context & /*ctx*/) const
            {
                return _data.matrix().block<3, 3>(0, 0).eval();
            }

            /** This method retrieves the translation of a transform
             * 
             * @param ctx the context object
             * 
             * @note this is a contextual method
             **/
            auto translation(const context & /*ctx*/) const
            {
                return _data.matrix().block<3, 1>(0, 3).eval();
            }

            /** This method retrieves the 4x4 matrix of a transform
             * 
             * @param ctx the context object
             * 
             * @note this is a contextual method
             **/
            auto matrix(const context & /*ctx*/) const
            {
                return _data.matrix();
            }

            /** This method transform from a point from the local 3D cartesian frame to the global 3D cartesian frame
             * 
             * @note this is a contextual method 
             **/
            template <typename derived_type>
            const auto point_to_global(const Eigen::MatrixBase<derived_type> &v, const eigen::transform3::context & /*ctx*/) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::point_to_global(v) requires a (3,1) matrix");
                return (_data * v).eval();
            }

            /** This method transform from a vector from the global 3D cartesian frame into the local 3D cartesian frame
             * 
             * @note this is a contextual method 
             **/
            template <typename derived_type>
            const auto point_to_local(const Eigen::MatrixBase<derived_type> &v, const eigen::transform3::context & /*ctx*/) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::point_to_local(v) requires a (3,1) matrix");
                return (_data.inverse() * v).eval();
            }

            /** This method transform from a vector from the local 3D cartesian frame to the global 3D cartesian frame
             * 
             * @note this is a contextual method 
             **/
            template <typename derived_type>
            const auto vector_to_global(const Eigen::MatrixBase<derived_type> &v, const eigen::transform3::context & /*ctx*/) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::vector_to_global(v) requires a (3,1) matrix");
                return (_data.linear() * v).eval();
            }

            /** This method transform from a vector from the global 3D cartesian frame into the local 3D cartesian frame
             * 
             * @note this is a contextual method 
             **/
            template <typename derived_type>
            const auto vector_to_local(const Eigen::MatrixBase<derived_type> &v, const eigen::transform3::context & /*ctx*/) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::vector_to_local(v) requires a (3,1) matrix");
                return (_data.inverse().linear() * v).eval();
            }
        };

        /** Non-contextual local frame projection into a cartesian coordinate frame
         */
        struct cartesian2
        {
            using point2 = Eigen::Matrix<scalar, 2, 1>;

            /** This method transform from a point from the global 3D cartesian frame to the local 2D cartesian frame,
              * including the contextual transform into the local 3D frame
              * 
              * @tparam the type of the surface from which also point3 and context type can be deduced
              * 
              */
            template <typename surface_type>
            const auto operator()(const surface_type &s,
                                  const typename surface_type::transform3::point3 &p,
                                  const typename surface_type::transform3::context &ctx) const
            {
                return operator()(s.transform().point_to_local(p, ctx));
            }

            /** This method transform from a point from the global 3D cartesian frame to the local 2D cartesian frame
             */
            template <typename derived_type>
            const auto operator()(const Eigen::MatrixBase<derived_type> &v) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::point3_to_point2(v) requires a (3,1) matrix");
                return (v.template segment<2>(0)).eval();
            }
        };

        /** Non-contextual local frame projection into a polar coordinate frame
         **/
        struct polar2
        {
            using point2 = Eigen::Matrix<scalar, 2, 1>;

            /** This method transform from a point from the global 3D cartesian frame to the local 2D cartesian frame,
              * including the contextual transform into the local 3D frame
              * 
              * @tparam the type of the surface from which also point3 and context type can be deduced
              * 
              */
            template <typename surface_type>
            const auto operator()(const surface_type &s,
                                  const typename surface_type::transform3::point3 &p,
                                  const typename surface_type::transform3::context &ctx) const
            {
                return operator()(s.transform().point_to_local(p, ctx));
            }

            /** This method transform from a point from 2D or 3D cartesian frame to a 2D polar point */
            template <typename derived_type>
            const auto operator()(const Eigen::MatrixBase<derived_type> &v) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows >= 2 and cols == 1, "transform::point_to_point2pol(v) requires a (>2,1) matrix");
                return point2{getter::perp(v), getter::phi(v)};
            }
        };

        /** Non-contextual local frame projection into a polar coordinate frame
         **/
        struct cylindrical2
        {
            using point2 = Eigen::Matrix<scalar, 2, 1>;

            /** This method transform from a point from the global 3D cartesian frame to the local 2D cartesian frame,
              * including the contextual transform into the local 3D frame
              * 
              * @tparam the type of the surface from which also point3 and context type can be deduced
              * 
              */
            template <typename surface_type>
            const auto operator()(const surface_type &s,
                                  const typename surface_type::transform3::point3 &p,
                                  const typename surface_type::transform3::context &ctx) const
            {
                return operator()(s.transform().point_to_local(p, ctx));
            }

            /** This method transform from a point from 2 3D cartesian frame to a 2D cylindrical point */
            template <typename derived_type>
            const auto operator()(const Eigen::MatrixBase<derived_type> &v) const
            {
                constexpr int rows = Eigen::MatrixBase<derived_type>::RowsAtCompileTime;
                constexpr int cols = Eigen::MatrixBase<derived_type>::ColsAtCompileTime;
                static_assert(rows == 3 and cols == 1, "transform::point3_to_point2cyl(v) requires a a (3,1) matrix");
                return point2{getter::perp(v) * getter::phi(v), v[2]};
            }
        };

    } // namespace eigen

    // Non-contextual vector transfroms
    namespace vector
    {

        /** Get a normalized version of the input vector
         * 
         * @tparam derived_type is the matrix template
         * 
         * @param v the input vector
         **/
        template <typename derived_type>
        auto normalize(const Eigen::MatrixBase<derived_type> &v)
        {
            return v.normalized();
        }

        /** Dot product between two input vectors
         * 
         * @tparam derived_type_lhs is the first matrix (epresseion) template
         * @tparam derived_type_rhs is the second matrix (epresseion) template
         * 
         * @param a the first input vector
         * @param b the second input vector
         * 
         * @return the scalar dot product value 
         **/
        template <typename derived_type_lhs, typename derived_type_rhs>
        auto dot(const Eigen::MatrixBase<derived_type_lhs> &a, const Eigen::MatrixBase<derived_type_rhs> &b)
        {
            return a.dot(b);
        }

        /** Cross product between two input vectors
         * 
         * @tparam derived_type_lhs is the first matrix (epresseion) template
         * @tparam derived_type_rhs is the second matrix (epresseion) template
         *           
         * @param a the first input vector
         * @param b the second input vector
         * 
         * @return a vector (expression) representing the cross product
         **/
        template <typename derived_type_lhs, typename derived_type_rhs>
        auto cross(const Eigen::MatrixBase<derived_type_lhs> &a, const Eigen::MatrixBase<derived_type_rhs> &b)
        {
            return a.cross(b);
        }
    } // namespace vector

} // namespace detray