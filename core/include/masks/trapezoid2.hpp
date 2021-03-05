/** Detray library, part of the ACTS project (R&D line)
 * 
 * (c) 2020 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 */
#pragma once

#include "masks/mask_identifier.hpp"
#include "core/intersection.hpp"
#include "utils/containers.hpp"
#include "tools/planar_intersector.hpp"

#include <cmath>
#include <climits>

namespace detray
{
    /** This is a simple 2-dimensional mask for a regular trapezoid
     * 
     * @tparam intersector_type is a struct used for intersecting this cylinder
     * @tparam links_type is an object where the mask can link to 
     * @tparam kMaskIdentifier is a unique mask identifier in a program context
     *  
     * It is defined by half lengths in local0 coordinate _values[0] and _values[1] at
     * -/+ half length in the local1 coordinate _values[2]
     **/
    template <typename intersector_type = planar_intersector,
              typename links_type = bool,
              unsigned int kMaskIdentifier = e_trapezoid2>
    struct trapezoid2
    {
        using mask_tolerance = darray<scalar, 2>;

        using mask_values = darray<scalar, 3>;

        mask_values _values =
            {std::numeric_limits<scalar>::infinity(),
             std::numeric_limits<scalar>::infinity(),
             std::numeric_limits<scalar>::infinity()};

        links_type _links;

        static constexpr unsigned int mask_identifier = kMaskIdentifier;

        static constexpr mask_tolerance within_epsilon 
            = { std::numeric_limits<scalar>::epsilon(), 
                std::numeric_limits<scalar>::epsilon() };

        /** Assignment operator from an array, convenience function
         * 
         * @param rhs is the right hand side object
         **/
        trapezoid2<intersector_type, links_type, kMaskIdentifier> &
        operator=(const darray<scalar, 3> &rhs)
        {
            _values = rhs;
            return (*this);
        }

        /** Mask operation 
         * 
         * @tparam point2_type is the type of the point to be checked w.r.t. to
         * the mask bounds
         * 
         * @param p the point to be checked
         * @param t us the tolerance tuple (l0,l1)
         * 
         * @return an intersection status e_inside / e_outside
         **/
        template <typename local_type>
        intersection_status is_inside(const typename local_type::point2 &p,
                                      const mask_tolerance& t = within_epsilon) const
        {
            scalar rel_y = (_values[2] + p[1]) / (2 * _values[2]);
            return 
                (std::abs(p[0]) <= _values[0] + rel_y * (_values[1] - _values[0]) + t[0] and std::abs(p[1]) <= _values[2] + t[1]) ? 
                    e_inside : e_outside;
        }

        /** Equality operator from an array, convenience function
         * 
         * @param rhs is the rectangle to be compared with
         * 
         * checks identity within epsilon and @return s a boolean*
         **/
        bool operator==(const darray<scalar, 3> &rhs)
        {
            return (_values == rhs);
        }

        /** Equality operator 
         * 
         * @param rhs is the rectangle to be compared with
         * 
         * checks identity within epsilon and @return s a boolean*
         **/
        bool operator==(const trapezoid2<> &rhs)
        {
            return operator==(rhs._values);
        }

        /** Access operator - non-const
         * @return the reference to the member variable
         */
        scalar &operator[](unsigned int value_index)
        {
            return _values[value_index];
        }

        /** Access operator - non-const
         * @return a copy of the member variable
         */
        scalar operator[](unsigned int value_index) const
        {
            return _values[value_index];
        }

        /** Return an associated intersector type */
        intersector_type intersector() const { return intersector_type{}; };

        /** Return the volume link - const reference */
        const links_type &links() const { return _links; }

        /** Return the volume link - non-const access */
        links_type &links() { return _links; }

    };

} // namespace detray
