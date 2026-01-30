//------------------------------------------------------------------------------
//  model.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "model.h"
#include "fx/gltf.h"
#include "textureresource.h"

namespace Render {
    static uint nameCounter = 0;
    static std::vector<Model> modelAllocator;
    static std::unordered_map<std::string, ModelId> modelRegistry;

    //------------------------------------------------------------------------------
    /**
*/
    int SlotFromGltf(std::string const& attr) {
        if (attr == "POSITION")
            return 0;
        if (attr == "NORMAL")
            return 1;
        if (attr == "TANGENT")
            return 2;
        if (attr == "TEXCOORD_0")
            return 3;
        if (attr == "COLOR_0")
            return 4;
        if (attr == "JOINTS_0")
            return 5;
        if (attr == "WEIGHTS_0")
            return 6;
        if (attr == "TEXCOORD_1")
            return 7;
        if (attr == "TEXCOORD_2")
            return 8;

        n_error("Attribute location not set!\n");
        return 0;
    }

    //------------------------------------------------------------------------------
    /**
*/
    void InferBufferTargets(fx::gltf::Document& model) {
        auto infer = false;
        for (const auto & bufferView : model.bufferViews) {
            if (bufferView.target == fx::gltf::BufferView::TargetType::None) {
                infer = true;
                break;
            }
        }

        if (infer) {
            // TODO: could be better
            for (auto const& mesh: model.meshes) {
                for (auto const& primitive: mesh.primitives) {
                    for (const auto& val: primitive.attributes | std::views::values) {
                        auto const& accessor = model.accessors[val];
                        model.bufferViews[accessor.bufferView].target = static_cast<fx::gltf::BufferView::TargetType>(GL_ARRAY_BUFFER);
                    }

                    auto const& accessor = model.accessors[primitive.indices];
                    model.bufferViews[accessor.bufferView].target = static_cast<fx::gltf::BufferView::TargetType>(GL_ELEMENT_ARRAY_BUFFER);
                }
            }
        }
    }

    //------------------------------------------------------------------------------
    /**
*/
    Model LoadGLTF(std::string const& uri, fx::gltf::ReadQuotas q) {
        fx::gltf::Document doc;

        try {
            if (uri.substr(uri.find_last_of('.') + 1) == "glb")
                doc = fx::gltf::LoadFromBinary(uri, q);
            else
                doc = fx::gltf::LoadFromText(uri, q);
        }
        catch (const std::exception& err) {
            printf("%s\n", err.what());
            assert(false);
            return Model();
        }

        InferBufferTargets(doc); // fix up buffertargets if necessary

        auto numBuffers = 0;
        //count number of buffers necessary
        size_t const numBufferViews = doc.bufferViews.size();
        for (unsigned i = 0; i < numBufferViews; i++) {
            numBuffers += (doc.bufferViews[i].target != fx::gltf::BufferView::TargetType::None);
        }

        std::vector<fx::gltf::Primitive const*> proxyPrimitives;

        Model model;
        model.buffers.resize(numBuffers, -1);
        glGenBuffers(numBuffers, &model.buffers[0]);

        std::vector<unsigned> bufferViewMap(numBufferViews);

        //TODO: We should use less buffers and possibly rebind less often.
        //		Put everything in one buffer and just use the offsets in the attribute pointers

        unsigned bufferIndex = 0;
        for (unsigned i = 0; i < numBufferViews; i++) {
            if (auto const& bufferView = doc.bufferViews[i];
                bufferView.target != fx::gltf::BufferView::TargetType::None) {
                auto const target = static_cast<GLenum>(bufferView.target);
                glBindBuffer(target, model.buffers[bufferIndex]);
                glBufferData(
                    target,
                    bufferView.byteLength,
                    &(doc.buffers[bufferView.buffer].data[0]) + bufferView.byteOffset,
                    GL_STATIC_DRAW
                );
                bufferViewMap[i] = bufferIndex;
                bufferIndex++;
            }
        }

        std::vector<TextureResourceId> textures;
        textures.resize(doc.textures.size(), InvalidResourceId);

        auto LoadTexture = [&doc, &uri, &textures](
            int textureIndex, fx::gltf::Texture const& texture, bool sRGB = false
        ) {
            fx::gltf::Image const& image = doc.images[texture.source];
            fx::gltf::Sampler sampler;
            if (texture.sampler != -1) { sampler = doc.samplers[texture.sampler]; }

            // set invalid samplers to default values for GL
            if (sampler.magFilter == fx::gltf::Sampler::MagFilter::None)
                sampler.magFilter = fx::gltf::Sampler::MagFilter::Linear;
            if (sampler.minFilter == fx::gltf::Sampler::MinFilter::None)
                sampler.minFilter = fx::gltf::Sampler::MinFilter::NearestMipMapLinear;

            std::string name;
            ImageId id = InvalidResourceId;
            if (image.IsEmbeddedResource()) {
                // Make up some random name
                uint uid = nameCounter++;
                name = "embedded_image_";
                name += std::to_string(uid);
                std::vector<uint8_t> data;

                image.MaterializeData(data);

                TextureLoadInfo texInfo = {
                    .name = name,
                    .buffer = &data[0],
                    .bytes = data.size(),
                    .magFilter = static_cast<MagFilter>(sampler.magFilter),
                    .minFilter = static_cast<MinFilter>(sampler.minFilter),
                    .wrappingModeS = static_cast<WrappingMode>(sampler.wrapS),
                    .wrappingModeT = static_cast<WrappingMode>(sampler.wrapT),
                    .sRGB = sRGB,
                    .placeholder = TextureResource::GetWhiteTexture()
                };

                id = TextureResource::LoadTextureFromMemory(texInfo);
            }
            else if (image.uri.empty()) {
                // this mean the image is in a buffer view, and this needs to be handled as well
                fx::gltf::BufferView const& bufferView = doc.bufferViews[image.bufferView];
                fx::gltf::Buffer const& buffer = doc.buffers[bufferView.buffer];

                // Make up some random name
                uint uid = nameCounter++;
                name = "embedded_image_";
                name += std::to_string(uid);

                TextureLoadInfo texInfo = {
                    .name = name,
                    .buffer = (void*)&buffer.data[bufferView.byteOffset],
                    .bytes = bufferView.byteLength,
                    .magFilter = static_cast<MagFilter>(sampler.magFilter),
                    .minFilter = static_cast<MinFilter>(sampler.minFilter),
                    .wrappingModeS = static_cast<WrappingMode>(sampler.wrapS),
                    .wrappingModeT = static_cast<WrappingMode>(sampler.wrapT),
                    .sRGB = sRGB,
                    .placeholder = TextureResource::GetWhiteTexture()
                };

                id = TextureResource::LoadTextureFromMemory(texInfo);
            }
            else // external image
            {
                name = image.uri;
                id = TextureResource::GetImageId(name);
                if (id == InvalidImageId) {
                    // get base path to file
                    std::filesystem::path p(uri);
                    std::string imagePath = p.parent_path().string() + "/" + name;
                    id = TextureResource::LoadTexture(
                        imagePath.c_str(),
                        (MagFilter)sampler.magFilter,
                        (MinFilter)sampler.minFilter,
                        (WrappingMode)sampler.wrapS,
                        (WrappingMode)sampler.wrapT,
                        sRGB
                    );
                }
            }

            textures[textureIndex] = id;
        };

        // load all basecolor textures
        for (auto const& material: doc.materials) {
            int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
            if (textureIndex != -1) {
                // Load basecolor texture as sRGB because it's required by GLTF to be in sRGB space
                LoadTexture(textureIndex, doc.textures[textureIndex], true);
            }
        }

        // load the rest of the textures
        for (int i = 0; i < doc.textures.size(); i++) {
            if (textures[i] != InvalidResourceId)
                continue;

            auto const& texture = doc.textures[i];
            LoadTexture(i, texture);
        }

        for (auto const& mesh: doc.meshes) {
            Model::Mesh m;
            for (auto const& primitive: mesh.primitives) {
                Model::Mesh::Primitive p;

                glGenVertexArrays(1, &p.vao);
                glBindVertexArray(p.vao);
                for (const auto& [fst, snd]: primitive.attributes) {
                    auto const& accessor = doc.accessors[snd];

                    Model::VertexAttribute attr;
                    attr.offset = accessor.byteOffset;
                    attr.stride = doc.bufferViews[accessor.bufferView].byteStride;
                    attr.normalized = accessor.normalized;

                    attr.slot = SlotFromGltf(fst);
                    attr.type = static_cast<GLenum>(accessor.componentType);

                    n_assert(static_cast<int>(accessor.type) > 0 && static_cast<int>(accessor.type) <= 4);
                    attr.components = static_cast<GLint>(accessor.type);

                    glBindBuffer(
                        static_cast<GLenum>(doc.bufferViews[accessor.bufferView].target),
                        model.buffers[bufferViewMap[accessor.bufferView]]
                    );
                    glEnableVertexArrayAttrib(p.vao, attr.slot);
                    glVertexAttribPointer(
                        attr.slot,
                        attr.components,
                        attr.type,
                        attr.normalized,
                        attr.stride,
                        reinterpret_cast<void*>(static_cast<intptr_t>(attr.offset))
                    );
                }

                auto const& ibAccessor = doc.accessors[primitive.indices];
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.buffers[bufferViewMap[ibAccessor.bufferView]]);
                p.numIndices = ibAccessor.count;
                p.offset = ibAccessor.byteOffset;
                p.indexType = static_cast<GLenum>(ibAccessor.componentType);

                if (primitive.material != -1) {
                    // TODO: cutout materials
                    fx::gltf::Material const& gltfMaterial = doc.materials[primitive.material];

                    auto const& bcf = gltfMaterial.pbrMetallicRoughness.baseColorFactor;
                    p.material.baseColorFactor = glm::vec4(bcf[0], bcf[1], bcf[2], bcf[3]);

                    p.material.textures[Model::Material::TEXTURE_BASECOLOR] =
                        gltfMaterial.pbrMetallicRoughness.baseColorTexture.index > -1
                            ? textures[gltfMaterial.pbrMetallicRoughness.baseColorTexture.index]
                            : TextureResource::GetWhiteTexture();

                    p.material.textures[Model::Material::TEXTURE_NORMAL] = gltfMaterial.normalTexture.index > -1
                                                                               ? textures[gltfMaterial.normalTexture.
                                                                                   index]
                                                                               : TextureResource::GetDefaultNormalTexture();

                    p.material.textures[Model::Material::TEXTURE_METALLICROUGHNESS] =
                        gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index > -1
                            ? textures[gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index]
                            : TextureResource::GetDefaultMetallicRoughnessTexture();

                    p.material.textures[Model::Material::TEXTURE_EMISSIVE] = gltfMaterial.emissiveTexture.index > -1
                                                                                 ? textures[gltfMaterial.emissiveTexture
                                                                                     .index]
                                                                                 : TextureResource::GetBlackTexture();

                    p.material.textures[Model::Material::TEXTURE_OCCLUSION] = gltfMaterial.occlusionTexture.index > -1
                                                                                  ? textures[gltfMaterial.
                                                                                      occlusionTexture.index]
                                                                                  : TextureResource::GetBlackTexture();

                    p.material.alphaMode = static_cast<Model::Material::AlphaMode>(gltfMaterial.alphaMode);
                    p.material.alphaCutoff = gltfMaterial.alphaCutoff;
                    p.material.doubleSided = gltfMaterial.doubleSided;
                    if (p.material.alphaMode == Model::Material::AlphaMode::Blend)
                        m.blendPrimitives.push_back(static_cast<uint16_t>(m.primitives.size()));
                    else
                        m.opaquePrimitives.push_back(static_cast<uint16_t>(m.primitives.size()));
                }

                glBindVertexArray(0);

                m.primitives.push_back(p);
            }
            model.meshes.push_back(std::move(m));
        }

        return model;
    }

    //------------------------------------------------------------------------------
    /**
*/
    ModelId LoadModel(const std::string& name, const fx::gltf::ReadQuotas& q) {
        if (const auto iter = modelRegistry.find(name);
            iter != modelRegistry.end()) {
            ModelId const mid = iter->second;
            modelAllocator[mid].refcount++; // increment refcount
            return mid;
        }

        if (std::filesystem::exists(name)) {
            Model mdl = LoadGLTF(name, q);
            mdl.refcount = 1;
            const auto mid = static_cast<ModelId>(modelAllocator.size());
            modelAllocator.push_back(mdl);
            modelRegistry.emplace(name, mid);
            return mid;
        }

        n_warning("Trying to load invalid model named '%s'!\n", name.c_str());
        return LoadModel("assets/error.glb");
    }

    //------------------------------------------------------------------------------
    /**
*/
    void UnloadModel(const ModelId mid) {
        n_assert(IsModelValid(mid));
        modelAllocator[mid].refcount--;
        if (modelAllocator[mid].refcount <= 0) {
            // TODO: Fully unload model
        }
    }

    //------------------------------------------------------------------------------
    /**
*/
    bool IsModelValid(ModelId) {
        // FIXME: we should use a generational id for the models.
        return true;
    }

    //------------------------------------------------------------------------------
    /**
*/
    Model const& GetModel(const ModelId id) { return modelAllocator[id]; }
} // namespace Render