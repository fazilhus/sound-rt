//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include <chrono>
#include <thread>

#include "audio/audio_manager.h"
#include "fx/gltf.h"

#include "core/filesystem.h"
#include "core/maths.h"
#include "physics/phy.h"
#include "physics/physicsmesh.h"

#include <cstdio>

using namespace Display;
using namespace Render;


namespace Game {

    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::SpaceGameApp()
        : window(nullptr), camera(nullptr), deltaTime(1.0f / 60.0f) {
        // empty
    }

    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::~SpaceGameApp() {
        // empty
        delete camera;
    }

    //------------------------------------------------------------------------------
    /**
    */
    bool SpaceGameApp::Open() {
        App::Open();
        this->window = new Window;
        this->window->SetSize(1920, 1080);

        if (this->window->Open()) {
            // set clear color to gray
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            RenderDevice::Init();

            // set ui rendering function
            this->window->SetUiRender([this]() { this->RenderUI(); });

            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::Run() {
        int w;
        int h;
        this->window->GetSize(w, h);
        glm::mat4 projection = glm::perspective(
            glm::radians(90.0f), static_cast<float>(w) / static_cast<float>(h), 0.01f, 1000.f
            );
        Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
        cam->projection = projection;

        auto cam_pos = glm::vec3(0, 1.0f, -5.0f);
        auto t = glm::mat4(1.0f);
        cam->view = lookAt(cam_pos, cam_pos + glm::vec3(t[2]), glm::vec3(t[1]));

        camera = new DebugCamera(5.0f, 1.5f);
        camera->pos = cam_pos;

        Physics::init_debug();

        // load all resources
        // TODO scale vertices transform scaling no worki
        const auto cubemesh = LoadModel(fs::create_path_from_rel_s("assets/system/cube.glb"));
        const auto cathedral = LoadModel(fs::create_path_from_rel_s("assets/audio/cathedral.glb"));

        const auto emitter_position = glm::vec3(6.0f, -6.0f, 0.0f);
        const auto emitter_transform = glm::translate(emitter_position) * glm::scale(glm::vec3(0.5f));

        Audio::AudioManager::get().load_scene(fs::create_path_from_rel_s("assets/audio/cathedral.glb"), Audio::bvh::SAH);

        // Setup skybox
        std::vector<std::string> skybox
        {
            fs::create_path_from_rel_s("assets/space/bg.png"),
            fs::create_path_from_rel_s("assets/space/bg.png"),
            fs::create_path_from_rel_s("assets/space/bg.png"),
            fs::create_path_from_rel_s("assets/space/bg.png"),
            fs::create_path_from_rel_s("assets/space/bg.png"),
            fs::create_path_from_rel_s("assets/space/bg.png")
        };
        TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
        RenderDevice::SetSkybox(skyboxId);

        Input::Keyboard* kbd = Input::GetDefaultKeyboard();
        Input::Mouse* mouse = Input::GetDefaultMouse();

        constexpr auto numLights = 40;
        // Setup lights
        for (PointLightId lights[numLights];
             auto& light: lights) {
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f
                );
            glm::vec3 color = glm::vec3(
                Core::RandomFloat(),
                Core::RandomFloat(),
                Core::RandomFloat()
                );
            light = LightServer::CreatePointLight(
                translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f)
                );
        }

        // Audio::AudioManager::get().set_emitter_collider(sound_cube);
        Audio::AudioManager::get().update_emitter_position(emitter_position);

        Physics::Ray r(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0));
        Physics::HitInfo hit;
        // auto dt = 1.0f / 60.0f;

        // game loop
        while (this->window->IsOpen()) {
            auto timeStart = std::chrono::steady_clock::now();
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            this->window->Update();
            this->camera->Update(this->deltaTime);

            if (kbd->pressed[Input::Key::Code::End]) { ShaderResource::ReloadShaders(); }

            if (kbd->held[Input::Key::LeftControl] && mouse->held[Input::Mouse::Button::LeftButton]) {
                r = DebugCamera::SpawnRay();
                const auto aabb = Core::CVarGet("r_draw_aabb");
                const auto aabb_id = Core::CVarGet("r_draw_aabb_id");
                const auto cm_id = Core::CVarGet("r_draw_cm_id");
                if (Physics::cast_ray(r, hit)) {
                    Core::CVarWriteInt(aabb, 1);
                    Core::CVarWriteInt(aabb_id, hit.collider.index);
                    Core::CVarWriteInt(cm_id, hit.collider.index);
                    Physics::add_impulse(hit.collider, hit.pos, 1.0f * r.dir);
                }
                else {
                    Core::CVarWriteInt(aabb, 0);
                    Core::CVarWriteInt(aabb_id, -1);
                    Core::CVarWriteInt(cm_id, -1);
                }
            }
            Debug::DrawRay(r, glm::vec4(1, 0, 1, 1));
            hit = {};
            if (hit.hit()) {
                Debug::DrawBox(
                    hit.pos,
                    glm::quat(),
                    0.1f,
                    glm::vec4(1, 0, 1, 1)
                    );
            }

            Physics::step(this->deltaTime);

            Audio::AudioManager::get().update_listener_pos_and_at(camera->pos, camera->ort);
            Audio::AudioManager::get().update();

            // Store all drawcalls in the render device
            RenderDevice::Draw(cubemesh, emitter_transform);
            RenderDevice::Draw(cathedral, glm::mat4(1.0f));

            Audio::AudioManager::get().debug_draw();

            Debug::DrawGrid();
            // Debug::DrawSelectedAABB();
            // Debug::DrawSelectedCMesh();
            // Debug::DrawAABBs();
            // Debug::DrawCMeshes();

            // Execute the entire rendering pipeline
            RenderDevice::Render(this->window, this->deltaTime);

            // transfer new frame to window
            this->window->SwapBuffers();

            auto timeEnd = std::chrono::steady_clock::now();
            this->deltaTime = Math::min(0.04f, std::chrono::duration<float>(timeEnd - timeStart).count());

            if (kbd->pressed[Input::Key::Code::Escape])
                this->Exit();
        }
    }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::Exit() { this->window->Close(); }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::RenderUI() const {
        if (this->window->IsOpen()) {
            ImGui::Begin("Debug");

            ImGui::Text(std::to_string(this->deltaTime).c_str());

            ImGui::Text("Debug Camera");
            ImGui::InputFloat3("Pos", &camera->pos[0]);

            ImGui::SeparatorText("Mouse");
            auto ml = Math::norm_screen_pos(Input::GetDefaultMouse()->position, glm::vec2(1920.0f, 1080.0f));
            ImGui::InputFloat2("Loc", &ml[0]);

            ImGui::SeparatorText("Debug Draw");

            Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
            int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
            if (ImGui::Checkbox("Draw Light Spheres", reinterpret_cast<bool*>(&drawLightSpheres))) {
                Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
            }

            Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
            int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
            if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId)){
                Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);
            }

            ImGui::SeparatorText("Audio");

            Core::CVar* a_enable_direct = Core::CVarGet("a_enable_direct");
            auto direct_enabled = Core::CVarReadInt(a_enable_direct);
            if (ImGui::Checkbox("Enable Direct", reinterpret_cast<bool*>(&direct_enabled))) {
                Core::CVarWriteInt(a_enable_direct, direct_enabled);
            }

            Core::CVar* a_enable_indirect = Core::CVarGet("a_enable_indirect");
            auto indirect_enabled = Core::CVarReadInt(a_enable_indirect);
            if (ImGui::Checkbox("Enable Indirect", reinterpret_cast<bool*>(&indirect_enabled))) {
                Core::CVarWriteInt(a_enable_indirect, indirect_enabled);
            }

            ImGui::End();

            Debug::DispatchDebugTextDrawing();
        }
    }

} // namespace Game
