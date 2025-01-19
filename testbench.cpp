#include "information.h"
#include <hls_stream.h>
#include <complex>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 8 // Assuming N is 8 for an 8-point FFT

void fft(hls::stream<axis_data> &input_vector, hls::stream<axis_data> &output_vector);
void fft_sw(std::complex<float> input_array[N], std::complex<float> FFT_output[N]);

int main()
{
    hls::stream<axis_data> input_stream;
    hls::stream<axis_data> output_stream;
    std::complex<float> FFT_output_hw[N];
    std::complex<float> FFT_input[N] = {
        std::complex<float>(11, 23),
        std::complex<float>(32,10),
        std::complex<float>(91,94),
        std::complex<float>(15,69),
        std::complex<float>(47,96),
        std::complex<float> (44,12),
        std::complex<float>(96,17),
        std::complex<float>(49,58)
    };
    std::complex<float> fft_output_sw[N];

    fft_sw(FFT_input, fft_output_sw);

    for (int i = 0; i < N; i++)
    {
        float real_part = FFT_input[i].real();
        float imag_part = FFT_input[i].imag();

        uint32_t real_part_int = *reinterpret_cast<uint32_t *>(&real_part);
        uint32_t imag_part_int = *reinterpret_cast<uint32_t *>(&imag_part);
        uint64_t packed_data = (static_cast<uint64_t>(imag_part_int) << 32) | real_part_int;
        axis_data output_data;
        output_data.data = (ap_uint<64>)packed_data;
        output_data.last = (ap_uint<1>)(i == N - 1); // Set last signal for the last element

        // Write to input of fft
        input_stream.write(output_data);
    }

    fft(input_stream, output_stream);

    for (int i = 0; i < N; i++)
    {
        axis_data input_data = output_stream.read();
        uint64_t raw_data = input_data.data.to_uint64();

        uint32_t real_part = static_cast<uint32_t>(raw_data & 0xFFFFFFFF);        // Lower 32 bits
        uint32_t imag_part = static_cast<uint32_t>((raw_data >> 32) & 0xFFFFFFFF); // Upper 32 bits

        float real = *reinterpret_cast<float *>(&real_part);
        float imag = *reinterpret_cast<float *>(&imag_part);

        if (fft_output_sw[i].real() != real || fft_output_sw[i].imag() != imag)
        {
            printf("Error at index %d: Expected (%f, %f), got (%f, %f)\n", i, fft_output_sw[i].real(), fft_output_sw[i].imag(), real, imag);
            return 1;
        }
    }

    printf("Successful\n");
    return 0;
}

void fft_sw(std::complex<float> input_array[N], std::complex<float> FFT_output[N])
{
    std::complex<float> reversed_array[N];

    const std::complex<float> W[4] = {
        std::complex<float>(1, 0),
        std::complex<float>(std::cos(-2 * M_PI / 8), std::sin(-2 * M_PI / 8)),
        std::complex<float>(std::cos(-4 * M_PI / 8), std::sin(-4 * M_PI / 8)),
        std::complex<float>(std::cos(-6 * M_PI / 8), std::sin(-6 * M_PI / 8))
    };

    const int rev8[N] = {0, 4, 2, 6, 1, 5, 3, 7}; // Permutation for bit-reversed order

    for (int i = 0; i < N; i++) {
        reversed_array[i] = input_array[rev8[i]]; // Rearrange data based on the reverse bit order
    }

    std::complex<float> temp1[N];
    std::complex<float> temp2[N];

    // Stage 1: Butterfly computation with stride 2
    for (int i = 0; i < N; i += 2) {
        temp1[i] = reversed_array[i] + reversed_array[i + 1];      // Sum
        temp1[i + 1] = reversed_array[i] - reversed_array[i + 1];  // Difference
    }

    // Stage 2: Butterfly computation with stride 4
    for (int i = 0; i < N; i += 4) {
        for (int j = 0; j < 2; ++j) {
            temp2[i + j] = temp1[i + j] + W[2 * j] * temp1[i + j + 2];      // Sum with twiddle factor
            temp2[i + 2 + j] = temp1[i + j] - W[2 * j] * temp1[i + j + 2];  // Difference with twiddle factor
        }
    }

    // Stage 3: Final butterfly computation with stride 8
    for (int i = 0; i < N / 2; i++) {
        FFT_output[i] = temp2[i] + W[i] * temp2[i + 4];      // Sum with twiddle factor
        FFT_output[i + 4] = temp2[i] - W[i] * temp2[i + 4];  // Difference with twiddle factor
    }
}
