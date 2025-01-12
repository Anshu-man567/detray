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

#include "detray/definitions/units.hpp"
#include "detray/field/constant_magnetic_field.hpp"
#include "detray/propagator/aborters.hpp"
#include "detray/propagator/actor_chain.hpp"
#include "detray/propagator/base_actor.hpp"
#include "detray/propagator/navigator.hpp"
#include "detray/propagator/propagator.hpp"
#include "detray/propagator/rk_stepper.hpp"
#include "detray/propagator/track.hpp"
#include "tests/common/tools/create_toy_geometry.hpp"

using namespace detray;

using intersection_t = line_plane_intersection;

using detector_host_type =
    detector<detector_registry::toy_detector, darray, thrust::tuple,
             vecmem::vector, vecmem::jagged_vector>;
using detector_device_type =
    detector<detector_registry::toy_detector, darray, thrust::tuple,
             vecmem::device_vector, vecmem::jagged_device_vector>;

using navigator_host_type = navigator<detector_host_type>;
using navigator_device_type = navigator<detector_device_type>;

using constraints_t = constrained_step<>;
using field_type = constant_magnetic_field<>;
using rk_stepper_type =
    rk_stepper<field_type, free_track_parameters, constraints_t>;

using matrix_operator = standard_matrix_operator<scalar>;

// detector configuration
constexpr std::size_t n_brl_layers = 4;
constexpr std::size_t n_edc_layers = 3;

// geomery navigation configurations
constexpr unsigned int theta_steps = 10;
constexpr unsigned int phi_steps = 10;

constexpr scalar rk_tolerance = 1e-4;
constexpr scalar overstep_tolerance = -1e-4;
constexpr scalar constrainted_step_size = 1. * unit_constants::mm;
constexpr scalar is_close = 1e-4;
constexpr scalar path_limit = 2 * unit_constants::m;

namespace detray {

template <template <typename...> class vector_t>
struct track_inspector : actor {

    struct track_inspector_state {

        track_inspector_state(vecmem::memory_resource &resource)
            : _path_lengths(&resource),
              _positions(&resource),
              _jac_transports(&resource) {}

        DETRAY_HOST_DEVICE
        track_inspector_state(vector_t<scalar> path_lengths,
                              vector_t<vector3> positions,
                              vector_t<free_matrix> jac_transports)
            : _path_lengths(path_lengths),
              _positions(positions),
              _jac_transports(jac_transports) {}

        vector_t<scalar> _path_lengths;
        vector_t<vector3> _positions;
        vector_t<free_matrix> _jac_transports;
    };

    using state_type = track_inspector_state;

    template <typename propagator_state_t>
    DETRAY_HOST_DEVICE void operator()(
        state_type &inspector_state,
        const propagator_state_t &prop_state) const {

        const auto &stepping = prop_state._stepping;

        // Record only on the object
        inspector_state._path_lengths.push_back(stepping.path_length());
        inspector_state._positions.push_back(stepping().pos());
        inspector_state._jac_transports.push_back(stepping._jac_transport);
    }
};

// Assemble propagator type
using inspector_host_t = track_inspector<vecmem::vector>;
using inspector_device_t = track_inspector<vecmem::device_vector>;
using actor_chain_host_t =
    actor_chain<thrust::tuple, inspector_host_t, pathlimit_aborter>;
using actor_chain_device_t =
    actor_chain<thrust::tuple, inspector_device_t, pathlimit_aborter>;
using propagator_host_type =
    propagator<rk_stepper_type, navigator_host_type, actor_chain_host_t>;
using propagator_device_type =
    propagator<rk_stepper_type, navigator_device_type, actor_chain_device_t>;

/// test function for propagator with single state
void propagator_test(
    detector_view<detector_host_type> det_data, const vector3 B,
    vecmem::data::vector_view<free_track_parameters> &tracks_data,
    vecmem::data::jagged_vector_view<intersection_t> &candidates_data,
    vecmem::data::jagged_vector_view<scalar> &path_lengths_data,
    vecmem::data::jagged_vector_view<vector3> &positions_data,
    vecmem::data::jagged_vector_view<free_matrix> &jac_transports_data);

}  // namespace detray
