//
// Created by goksu on 2/25/20.
//

#include <algorithm>
#include <atomic>
#include <fstream>
#include <mutex>
#include <thread>
#include "Scene.hpp"
#include "Renderer.hpp"


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    // change the spp value to change sample ammount
    int spp = 16;
    std::cout << "SPP: " << spp << "\n";
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0)
        thread_count = 4;
    thread_count = std::min(thread_count, static_cast<unsigned int>(scene.height));
    std::cout << "Threads: " << thread_count << "\n";

    std::atomic<int> next_row(0);
    std::atomic<int> finished_rows(0);
    std::mutex progress_mutex;
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    auto render_worker = [&]() {
        while (true) {
            int j = next_row.fetch_add(1);
            if (j >= scene.height)
                break;

            for (int i = 0; i < scene.width; ++i) {
                // generate primary ray direction
                float x = (2 * (i + 0.5f) / (float)scene.width - 1) *
                          imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5f) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                int index = j * scene.width + i;
                for (int k = 0; k < spp; k++) {
                    framebuffer[index] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
                }
            }

            int done = finished_rows.fetch_add(1) + 1;
            {
                std::lock_guard<std::mutex> lock(progress_mutex);
                UpdateProgress(done / (float)scene.height);
            }
        }
    };

    for (unsigned int t = 0; t < thread_count; ++t) {
        workers.emplace_back(render_worker);
    }

    for (auto& worker : workers) {
        worker.join();
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
