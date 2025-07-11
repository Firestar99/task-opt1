// Extract from basisu_transcoder.cpp
// Copyright (C) 2019-2021 Binomial LLC. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

//************************** Helpers and Boilerplate **************************/

#include "basisu_headers.h"

/**
 * Helper to return the current time in milliseconds.
 */
static unsigned millis() {
	return static_cast<unsigned>((clock() * 1000LL) / CLOCKS_PER_SEC);
}

/**
 * Prebuilt table with known results.
 */
static const etc1_to_dxt1_56_solution known[32 * 8 * NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS * NUM_ETC1_TO_DXT1_SELECTOR_RANGES] = {
#include "basisu_transcoder_tables_dxt1_6.inc"
};

/**
 * Helper to compare two tables to see if they match.
 */
static bool verifyTable(const etc1_to_dxt1_56_solution* a, const etc1_to_dxt1_56_solution* b) {
	for (unsigned n = 0; n < 32 * 8 * NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS * NUM_ETC1_TO_DXT1_SELECTOR_RANGES; n++) {
		if (a->m_hi != b->m_hi || a->m_lo != b->m_lo || a->m_err != b->m_err) {
			printf("Failed with n = %d\n", n);
			return false;
		}
		a++;
		b++;
	}
	return true;
}

//************************ Optimisation Task Goes Here ************************/

/**
 * Results stored here.
 */
static etc1_to_dxt1_56_solution result[32 * 8 * NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS * NUM_ETC1_TO_DXT1_SELECTOR_RANGES];

static void write(uint32_t inten, uint32_t g, uint32_t sr, uint32_t m, uint32_t lo, uint32_t hi, uint32_t total_err)
{
	size_t index = m + NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS * (sr + NUM_ETC1_TO_DXT1_SELECTOR_RANGES * (g + inten * 32));
	if (total_err < result[index].m_err) {
		result[index] = (etc1_to_dxt1_56_solution){ (uint8_t)lo, (uint8_t)hi, (uint16_t)total_err };
	}
}

/**
 * Function to optimise.
 */
static void create_etc1_to_dxt1_6_conversion_table() {
	for (int inten = 0; inten < 8; inten++) {
		for (uint32_t g = 0; g < 32; g++) {
			for (uint32_t sr = 0; sr < NUM_ETC1_TO_DXT1_SELECTOR_RANGES; sr++) {
				for (uint32_t m = 0; m < NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS; m++) {
					size_t index = m + NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS * (sr + NUM_ETC1_TO_DXT1_SELECTOR_RANGES * (g + inten * 32));
					result[index] = (etc1_to_dxt1_56_solution){ 0, 0, UINT16_MAX };
				} // m
			} // sr
		} // g
	} // inten

	for (int inten = 0; inten < 8; inten++) {
		for (uint32_t g = 0; g < 32; g++) {
			color32 block_colors[4];
			decoder_etc_block::get_diff_subblock_colors(block_colors, decoder_etc_block::pack_color5(color32(g, g, g, 255), false), inten);

			for (uint32_t hi = 0; hi <= 63; hi++) {
				for (uint32_t lo = 0; lo <= 63; lo++) {

					uint32_t colors[4];
					colors[0] = (lo << 2) | (lo >> 4);
					colors[3] = (hi << 2) | (hi >> 4);
					colors[1] = (colors[0] * 2 + colors[3]) / 3;
					colors[2] = (colors[3] * 2 + colors[0]) / 3;

					for (uint32_t m = 0; m < NUM_ETC1_TO_DXT1_SELECTOR_MAPPINGS; m++) {
						int errors[4];
						for (int s = 0; s < 4; ++s)
						{
							uint32_t err = block_colors[s].g - colors[g_etc1_to_dxt1_selector_mappings[m][s]];
							errors[s] = err * err;
						}

						int errors_0123 = errors[0] + errors[1] + errors[2] + errors[3];
						write(inten, g, 0, m, lo, hi, errors_0123);

						int errors_123 = errors[1] + errors[2] + errors[3];
						write(inten, g, 1, m, lo, hi, errors_123);

						int errors_012 = errors[0] + errors[1] + errors[2];
						write(inten, g, 2, m, lo, hi, errors_012);

						int errors_12 = errors[1] + errors[2];
						write(inten, g, 3, m, lo, hi, errors_12);

						int errors_23 = errors[2] + errors[3];
						write(inten, g, 4, m, lo, hi, errors_23);

						int errors_01 = errors[0] + errors[1];
						write(inten, g, 5, m, lo, hi, errors_01);
					} // m
				} // lo
			} // hi
		} // g
	} // inten
}

//******************************** Entry Point ********************************/

/**
 * Tests the generation and benchmarks it.
 */
int main(int /*argc*/, char* /*argv*/[]) {
	// Run this once and compare the result to the known table
	create_etc1_to_dxt1_6_conversion_table();
	if (!verifyTable(result, known)) {
		printf("Generated results don't match known values\n");
	}

    // Perform multiple runs and take the best time
    unsigned best = UINT32_MAX;
    for (int n = 10; n > 0; n--) {
    	unsigned time = millis();
    	create_etc1_to_dxt1_6_conversion_table();
    	time = millis() - time;
    	if (time < best) {
    		best = time;
    	}
    }

    printf("Best run took %dms\n", best);
    return 0;
}
