/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

#if defined(array)
#include "detray/plugins/algebra/array_definitions.hpp"
#elif defined(eigen)
#include "detray/plugins/algebra/eigen_definitions.hpp"
#elif defined(smatrix)
#include "detray/plugins/algebra/smatrix_definitions.hpp"
#elif defined(vc_array)
#include "detray/plugins/algebra/vc_array_definitions.hpp"
#endif

#include "detray/definitions/qualifiers.hpp"
#include "detray/definitions/units.hpp"
#include "detray/field/constant_magnetic_field.hpp"
#include "detray/propagator/rk_stepper.hpp"
#include "detray/propagator/track.hpp"

using namespace detray;

namespace {

// type definitions
using size_type = __plugin::size_type;
using vector3 = __plugin::vector3<scalar>;
using point3 = __plugin::point3<scalar>;
using size_type = __plugin::size_type;
using mag_field_t = constant_magnetic_field<>;
using rk_stepper_t = rk_stepper<mag_field_t, free_track_parameters>;
using crk_stepper_t =
    rk_stepper<mag_field_t, free_track_parameters, constrained_step<>>;
using transform3 = __plugin::transform3<scalar>;
using matrix_operator = standard_matrix_operator<scalar>;

// geomery navigation configurations
constexpr unsigned int theta_steps = 100;
constexpr unsigned int phi_steps = 100;
constexpr unsigned int rk_steps = 100;

constexpr scalar epsilon = 1e-3;
constexpr scalar rk_tolerance = 1e-4;

// dummy navigation struct
struct nav_state {

    DETRAY_HOST_DEVICE
    scalar operator()() const { return _step_size; }

    DETRAY_HOST_DEVICE
    inline auto current_object() const -> dindex { return dindex_invalid; }

    DETRAY_HOST_DEVICE
    inline void set_full_trust() {}

    DETRAY_HOST_DEVICE
    inline void set_high_trust() {}

    DETRAY_HOST_DEVICE
    inline void set_fair_trust() {}

    DETRAY_HOST_DEVICE
    inline void set_no_trust() {}

    DETRAY_HOST_DEVICE
    inline bool abort() { return false; }

    scalar _step_size = 1. * unit_constants::mm;
};

// dummy propagator state
template <typename stepping_t, typename navigation_t>
struct prop_state {
    stepping_t _stepping;
    navigation_t _navigation;
};

}  // anonymous namespace

namespace detray {

// Test function for Runge-Kutta stepper bound state
// This test investigates only one track
void bound_state_test(
    vecmem::data::vector_view<bound_track_parameters> out_param,
    const bound_track_parameters in_param, const vector3 B,
    const transform3 trf);

}  // namespace detray