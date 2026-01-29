#include "config.h"
#include "particlesystem.h"
#include "shaderresource.h"
#include "core/filesystem.h"


namespace Render {
    void ParticleSystem::Initialize() {
        auto vs = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::VERTEXSHADER,
            fs::create_path_from_rel_s("shd/vs_particles_bufstorage.glsl").c_str()
        );
        auto fs = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::FRAGMENTSHADER,
            fs::create_path_from_rel_s("shd/fs_particles_bufstorage.glsl").c_str()
        );
        this->particleShaderId = Render::ShaderResource::CompileShaderProgram({vs, fs});
        auto cs = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::COMPUTESHADER,
            fs::create_path_from_rel_s("shd/cs_particle_sim_bufstorage.glsl").c_str()
        );
        this->particleSimComputeShaderId = Render::ShaderResource::CompileShaderProgram({cs});
        glGenBuffers(1, &this->emitterBlockUBO);
    }

    ParticleEmitter::ParticleEmitter(uint32_t numParticles) {
        data.numParticles = numParticles;
        glGenBuffers(2, this->bufPositions);
        glGenBuffers(2, this->bufVelocities);
        glGenBuffers(2, this->bufColors);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufPositions[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufPositions[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufVelocities[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufVelocities[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufColors[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->bufColors[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, this->data.numParticles * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    }

    ParticleEmitter::~ParticleEmitter() {
        glDeleteBuffers(2, this->bufPositions);
        glDeleteBuffers(2, this->bufVelocities);
        glDeleteBuffers(2, this->bufColors);
    }
}