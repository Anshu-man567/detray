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

using field_type = constant_magnetic_field<>;
using rk_stepper_type = rk_stepper<field_type, free_track_parameters>;
using propagator_host_type =
    propagator<rk_stepper_type, navigator_host_type, actor_chain<>>;
using propagator_device_type =
    propagator<rk_stepper_type, navigator_device_type, actor_chain<>>;

namespace detray {

/// test function for propagator with single state
void propagator_benchmark(
    detector_view<detector_host_type> det_data,
    vecmem::data::vector_view<free_track_parameters>& tracks_data,
    vecmem::data::jagged_vector_view<intersection_t>& candidates_data);

}  // namespace detray