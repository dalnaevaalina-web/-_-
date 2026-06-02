#include <chrono>
#include <iostream>
#include <random>
#include <sycl/sycl.hpp>
#include <vector>

using namespace sycl;

// ================= CPU =================

void thresholdCPU(const std::vector<unsigned char>& input,
    std::vector<unsigned char>& output, int width, int height,
    unsigned char T) {
    for (int y = 0; y < height; y++) {

        for (int x = 0; x < width; x++) {

            int idx = y * width + x;

            output[idx] = (input[idx] > T) ? 255 : 0;
        }
    }
}

int main() {

    try {

        queue q(default_selector_v);

        std::cout << "Device: " << q.get_device().get_info<info::device::name>()
            << "\n";

        const int width = 1024;
        const int height = 1024;

        const int SIZE = width * height;

        const unsigned char T = 128;

        std::vector<unsigned char> input(SIZE);
        std::vector<unsigned char> cpu_result(SIZE);
        std::vector<unsigned char> gpu_result(SIZE);

        // ================= Генерация изображения =================

        std::mt19937 gen(42);

        std::uniform_int_distribution<int> dist(0, 255);

        for (int i = 0; i < SIZE; i++) {

            input[i] = static_cast<unsigned char>(dist(gen));
        }

        // ================= CPU =================

        auto cpu_start = std::chrono::high_resolution_clock::now();

        thresholdCPU(input, cpu_result, width, height, T);

        auto cpu_end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> cpu_time = cpu_end - cpu_start;

        // ================= SYCL =================

        {
            buffer<unsigned char> bufInput(input.data(), range<1>(SIZE));

            buffer<unsigned char> bufOutput(gpu_result.data(), range<1>(SIZE));

            auto gpu_start = std::chrono::high_resolution_clock::now();

            q.submit([&](handler& h) {
                accessor in(bufInput, h, read_only);

                accessor out(bufOutput, h, write_only);

                // 2D ND-range
                h.parallel_for(nd_range<2>(range<2>(height, width), range<2>(16, 16)),
                    [=](nd_item<2> item) {
                        int y = item.get_global_id(0);
                        int x = item.get_global_id(1);

                        int idx = y * width + x;

                        out[idx] = (in[idx] > T) ? 255 : 0;
                    });
                });

            q.wait();

            auto gpu_end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> gpu_time = gpu_end - gpu_start;

            // ================= Проверка =================

            bool correct = true;

            for (int i = 0; i < SIZE; i++) {

                if (cpu_result[i] != gpu_result[i]) {

                    correct = false;
                    break;
                }
            }

            std::cout << "Results: " << (correct ? "correct" : "incorrect") << "\n";

            std::cout << "CPU time: " << cpu_time.count() << " s\n";

            std::cout << "SYCL time: " << gpu_time.count() << " s\n";

            std::cout << "Acceleration: " << cpu_time.count() / gpu_time.count()
                << "x\n";
        }

    }
    catch (exception const& e) {

        std::cout << "SYCL exception: " << e.what() << "\n";

        return 1;
    }

    return 0;
}