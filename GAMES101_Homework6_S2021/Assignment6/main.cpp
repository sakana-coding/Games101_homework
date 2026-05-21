#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
    std::filesystem::path models_dir = "../models";
    if (!std::filesystem::exists(models_dir))
    {
        models_dir = "models";
    }
    std::vector<std::filesystem::path> model_paths;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(models_dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".obj")
        {
            model_paths.push_back(entry.path());
        }
    }
    std::sort(model_paths.begin(), model_paths.end());

    Renderer r;
    for (const auto& model_path : model_paths)
    {
        Scene scene(1280, 960);

        std::cout << "Loading model: " << model_path << std::endl;
        MeshTriangle model(model_path.string());
        scene.Add(&model);

        scene.Add(std::make_unique<Light>(Vector3f(-20, 70, 20), 1));
        scene.Add(std::make_unique<Light>(Vector3f(20, 70, 20), 1));
        scene.buildBVH();

        std::string output_filename = model_path.stem().string() + ".ppm";
        std::cout << "Rendering to: " << output_filename << std::endl;

        auto start = std::chrono::system_clock::now();
        r.Render(scene, output_filename);
        auto stop = std::chrono::system_clock::now();

        std::cout << "\nRender complete: " << output_filename << "\n";
        std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count()
                  << " hours\n";
        std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count()
                  << " minutes\n";
        std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count()
                  << " seconds\n";
    }

    return 0;
}
