/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include <gtest/gtest.h>

#include <vecmem/memory/cuda/device_memory_resource.hpp>
#include <vecmem/memory/cuda/managed_memory_resource.hpp>

#include "propagator_cuda_kernel.hpp"
#include "vecmem/utils/cuda/copy.hpp"

class CudaPropagatorWithRkStepper
    : public ::testing::TestWithParam<__plugin::vector3<scalar>> {};

TEST_P(CudaPropagatorWithRkStepper, propagator) {

    // Helper object for performing memory copies.
    vecmem::copy copy;

    // VecMem memory resource(s)
    vecmem::cuda::managed_memory_resource mng_mr;

    // Create the toy geometry
    detector_host_type det =
        create_toy_geometry<darray, thrust::tuple, vecmem::vector,
                            vecmem::jagged_vector>(mng_mr, n_brl_layers,
                                                   n_edc_layers);

    // Create the vector of initial track parameters
    vecmem::vector<free_track_parameters> tracks_host(&mng_mr);
    vecmem::vector<free_track_parameters> tracks_device(&mng_mr);

    // Set origin position of tracks
    const point3 ori{0., 0., 0.};

    // Loops of theta values ]0,pi[
    for (unsigned int itheta = 0; itheta < theta_steps; ++itheta) {
        scalar theta = 0.001 + itheta * (M_PI - 0.001) / theta_steps;
        scalar sin_theta = std::sin(theta);
        scalar cos_theta = std::cos(theta);

        // Loops of phi values [-pi, pi]
        for (unsigned int iphi = 0; iphi < phi_steps; ++iphi) {
            // The direction
            scalar phi = -M_PI + iphi * (2 * M_PI) / phi_steps;
            scalar sin_phi = std::sin(phi);
            scalar cos_phi = std::cos(phi);
            vector3 mom{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta};
            mom = 10. * mom;

            // intialize a track
            free_track_parameters traj(ori, 0, mom, -1);
            traj.set_overstep_tolerance(overstep_tolerance);

            // Put it into vector of trajectories
            tracks_host.push_back(traj);
            tracks_device.push_back(traj);
        }
    }

    /**
     * Host propagation
     */

    // Set the magnetic field
    const vector3 B = GetParam();
    field_type B_field(B);

    // Create RK stepper
    rk_stepper_type s(B_field);
    // Create navigator
    navigator_host_type n(det);
    // Create propagator
    propagator_host_type p(std::move(s), std::move(n));

    // Create vector for track recording
    vecmem::jagged_vector<scalar> host_path_lengths(&mng_mr);
    vecmem::jagged_vector<vector3> host_positions(&mng_mr);
    vecmem::jagged_vector<free_matrix> host_jac_transports(&mng_mr);

    for (unsigned int i = 0; i < theta_steps * phi_steps; i++) {

        // Create the propagator state
        inspector_host_t::state_type insp_state{mng_mr};
        pathlimit_aborter::state_type pathlimit_state{path_limit};

        propagator_host_type::state state(
            tracks_host[i], thrust::tie(insp_state, pathlimit_state));

        state._stepping.template set_constraint<step::constraint::e_accuracy>(
            constrainted_step_size);

        state._stepping.set_tolerance(rk_tolerance);

        // Run propagation
        p.propagate(state);

        // Record the step information
        host_path_lengths.push_back(insp_state._path_lengths);
        host_positions.push_back(insp_state._positions);
        host_jac_transports.push_back(insp_state._jac_transports);
    }

    /**
     * Device propagation
     */

    // Get detector data
    auto det_data = get_data(det);

    // Get tracks data
    auto tracks_data = vecmem::get_data(tracks_device);

    // Create navigator candidates buffer
    auto candidates_buffer =
        create_candidates_buffer(det, theta_steps * phi_steps, mng_mr);
    copy.setup(candidates_buffer);

    // Create vector buffer for track recording
    std::vector<std::size_t> sizes(theta_steps * phi_steps, 0);
    std::vector<std::size_t> capacities;
    for (auto& r : host_positions) {
        capacities.push_back(r.size());
    }

    vecmem::data::jagged_vector_buffer<scalar> path_lengths_buffer(
        sizes, capacities, mng_mr);
    vecmem::data::jagged_vector_buffer<vector3> positions_buffer(
        sizes, capacities, mng_mr);
    vecmem::data::jagged_vector_buffer<free_matrix> jac_transports_buffer(
        sizes, capacities, mng_mr);

    copy.setup(path_lengths_buffer);
    copy.setup(positions_buffer);
    copy.setup(jac_transports_buffer);

    // Run the propagator test for GPU device
    propagator_test(det_data, B, tracks_data, candidates_buffer,
                    path_lengths_buffer, positions_buffer,
                    jac_transports_buffer);

    vecmem::jagged_vector<scalar> device_path_lengths(&mng_mr);
    vecmem::jagged_vector<vector3> device_positions(&mng_mr);
    vecmem::jagged_vector<free_matrix> device_jac_transports(&mng_mr);

    copy(path_lengths_buffer, device_path_lengths);
    copy(positions_buffer, device_positions);
    copy(jac_transports_buffer, device_jac_transports);

    // Compare the positions
    for (unsigned int i = 0; i < host_positions.size(); i++) {
        ASSERT_TRUE(host_positions[i].size() > 0);

        for (unsigned int j = 0; j < host_positions[i].size(); j++) {

            auto host_pl = host_path_lengths[i][j];
            auto device_pl = device_path_lengths[i][j];

            // ASSERT_NEAR((host_pl - device_pl) / host_pl, 0, is_close);

            ASSERT_EQ(host_positions[i].size(), device_positions[i].size());

            ASSERT_NEAR(host_pl, device_pl, host_pl * is_close);

            auto& host_pos = host_positions[i][j];
            auto& device_pos = device_positions[i][j];

            auto relative_error =
                static_cast<point3>(1. / host_pl * (host_pos - device_pos));

            ASSERT_NEAR(getter::norm(relative_error), 0, is_close);
        }
    }

    // Compare the jacobian transports
    for (unsigned int i = 0; i < host_jac_transports.size(); i++) {
        for (unsigned int j = 0; j < host_jac_transports[i].size(); j++) {

            auto& host_J = host_jac_transports[i][j];
            auto& device_J = device_jac_transports[i][j];

            auto pl = host_path_lengths[i][j];

            for (__plugin::size_type row = 0; row < e_free_size; row++) {
                for (__plugin::size_type col = 0; col < e_free_size; col++) {

                    auto host_val = matrix_operator().element(host_J, row, col);

                    auto device_val =
                        matrix_operator().element(device_J, row, col);

                    ASSERT_NEAR((host_val - device_val) / pl, 0, is_close);
                }
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(CudaPropagatorValidation1, CudaPropagatorWithRkStepper,
                         ::testing::Values(__plugin::vector3<scalar>{
                             0. * unit_constants::T, 0. * unit_constants::T,
                             2. * unit_constants::T}));

INSTANTIATE_TEST_SUITE_P(CudaPropagatorValidation2, CudaPropagatorWithRkStepper,
                         ::testing::Values(__plugin::vector3<scalar>{
                             0. * unit_constants::T, 1. * unit_constants::T,
                             1. * unit_constants::T}));

INSTANTIATE_TEST_SUITE_P(CudaPropagatorValidation3, CudaPropagatorWithRkStepper,
                         ::testing::Values(__plugin::vector3<scalar>{
                             1. * unit_constants::T, 0. * unit_constants::T,
                             1. * unit_constants::T}));

INSTANTIATE_TEST_SUITE_P(CudaPropagatorValidation4, CudaPropagatorWithRkStepper,
                         ::testing::Values(__plugin::vector3<scalar>{
                             1. * unit_constants::T, 1. * unit_constants::T,
                             1. * unit_constants::T}));
