/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2020 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include <benchmark/benchmark.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vecmem/memory/host_memory_resource.hpp>

#include "core/detector.hpp"
#include "core/transform_store.hpp"
#include "io/csv_io.hpp"
#include "tests/common/read_geometry.hpp"
#include "tools/intersection_kernel.hpp"

using namespace detray;

#ifdef DETRAY_BENCHMARKS_REP
unsigned int gbench_repetitions = DETRAY_BENCHMARKS_REP;
#else
unsigned int gbench_repetitions = 0;
#endif

unsigned int theta_steps = 100;
unsigned int phi_steps = 100;
bool stream_file = false;

vecmem::host_memory_resource host_mr;
auto [d, name_map] = read_from_csv(tml_files, host_mr);

using geometry = decltype(d)::geometry;
using links_type = typename geometry::surface_links;
constexpr auto k_surfaces = geometry::e_surface;

const auto &surfaces = d.template objects<k_surfaces>();
const auto &masks = d.masks();

namespace __plugin {
// This test runs intersection with all surfaces of the TrackML detector
static void BM_INTERSECT_ALL(benchmark::State &state) {

    /*std::ofstream hit_out;
    if (stream_file)
    {
        hit_out.open("tml_hits.csv");
    }*/
    unsigned int hits = 0;
    unsigned int missed = 0;

    // point3 ori = {0., 0., 0.};

    using detray_context = decltype(d)::transform_store::context;
    detray_context default_context;

    for (auto _ : state) {
        track<static_transform_store<>::context> track;
        track.pos = point3{0., 0., 0.};

        // Loops of theta values
        for (unsigned int itheta = 0; itheta < theta_steps; ++itheta) {
            scalar theta = 0.1 + itheta * (M_PI - 0.1) / theta_steps;
            scalar sin_theta = std::sin(theta);
            scalar cos_theta = std::cos(theta);

            // Loops of phi values
            for (unsigned int iphi = 0; iphi < phi_steps; ++iphi) {
                // The direction
                scalar phi = -M_PI + iphi * (2 * M_PI) / phi_steps;
                scalar sin_phi = std::sin(phi);
                scalar cos_phi = std::cos(phi);
                track.dir = {cos_phi * sin_theta, sin_phi * sin_theta,
                             cos_theta};

                // Loop over volumes
                for (const auto &v : d.volumes()) {
                    // Loop over surfaces
                    for (size_t si = v.template range<k_surfaces>()[0];
                         si < v.template range<k_surfaces>()[1]; si++) {
                        links_type links{};
                        auto sfi = intersect(track, surfaces[si],
                                             d.transforms(default_context),
                                             masks, links);

                        benchmark::DoNotOptimize(hits);
                        benchmark::DoNotOptimize(missed);
                        if (sfi.status == intersection_status::e_inside) {
                            /* state.PauseTiming();
                            if (stream_file)
                            {
                                hit_out << sfi.p3[0] << "," << sfi.p3[1] << ","
                            << sfi.p3[2]
                            << "\n";
                            }
                            state.ResumeTiming();*/
                            ++hits;
                        } else {
                            ++missed;
                        }
                        benchmark::ClobberMemory();
                    }
                }
            }
        }
    }

#ifndef DETRAY_BENCHMARKS_MULTITHREAD
    std::cout << "[detray] hits / missed / total = " << hits << " / " << missed
              << " / " << hits + missed << std::endl;
#endif
    /**if (stream_file)
    {
        hit_out.close();
    }*/
}

BENCHMARK(BM_INTERSECT_ALL)
#ifdef DETRAY_BENCHMARKS_MULTITHREAD
    ->ThreadPerCpu()
#endif
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(gbench_repetitions)
    ->DisplayAggregatesOnly(true);

}  // namespace __plugin

BENCHMARK_MAIN();
