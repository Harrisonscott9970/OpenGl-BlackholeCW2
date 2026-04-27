#include "helper/scene.h"
#include "helper/scenerunner.h"
#include "scenebasic_uniform.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "[Startup] Black Hole Mission starting...\n";

    SceneRunner runner("Shader_Basics");
    std::unique_ptr<Scene> scene;
    scene = std::unique_ptr<Scene>(new SceneBasic_Uniform());

    int result = runner.run(*scene);

    std::cout << "[Shutdown] Application closed with code " << result << "\n";
    return result;
}