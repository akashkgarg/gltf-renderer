#include <iostream>
#include <fstream>
#include <set>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/TextureProvider.h>
#include <gltfio/materials/uberarchive.h>

#include <filament/Camera.h>
#include <filament/ColorGrading.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <math/mat4.h>
#include <math/mat3.h>
#include <math/vec3.h>
#include <mathio/ostream.h>

#include <utils/Path.h>
#include <utils/EntityManager.h>
#include <utils/NameComponentManager.h>
#include <utils/Entity.h>
#include <utils/Slice.h>

#include <chrono>
#include <ctime>
#include <thread>

#include "IBL.h"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace filament;

static std::ifstream::pos_type getFileSize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

struct App {

    Engine *engine;
    SwapChain *swap_chain;
    Renderer *renderer;
    View *view;
    Scene *scene;

    utils::NameComponentManager *names;

    utils::Entity cam_entity;
    Camera *camera;

    utils::Entity rootTransformEntity;

    std::unique_ptr<IBL> ibl;

    gltfio::MaterialProvider* materials;

    gltfio::AssetLoader* assetLoader;
    gltfio::FilamentAsset* asset = nullptr;
    gltfio::FilamentInstance* instance = nullptr;

    gltfio::ResourceLoader* resourceLoader = nullptr;
    gltfio::TextureProvider* stbDecoder = nullptr;
    gltfio::TextureProvider* ktxDecoder = nullptr;

};

void loadAsset(App &app, const utils::Path &filename) {
    // Peek at the file size to allow pre-allocation.
    long const contentSize = static_cast<long>(getFileSize(filename.c_str()));
    if (contentSize <= 0) {
        std::cerr << "Unable to open " << filename << std::endl;
        exit(1);
    }

    // Consume the glTF file.
    std::ifstream in(filename.c_str(), std::ifstream::binary | std::ifstream::in);
    std::vector<uint8_t> buffer(static_cast<unsigned long>(contentSize));
    if (!in.read((char*) buffer.data(), contentSize)) {
        std::cerr << "Unable to read " << filename << std::endl;
        exit(1);
    }

    std::cout << buffer.size() << std::endl;

    // Parse the glTF file and create Filament entities.
    app.asset = app.assetLoader->createAsset(buffer.data(), buffer.size());
    if (!app.asset) {
        std::cerr << "Unable to parse " << filename << std::endl;
        exit(1);
    }

    // Load the textures, compute tangents, upload vertex/index buffers
    // Load external textures and buffers.
    std::string const gltfPath = filename.getAbsolutePath();
    gltfio::ResourceConfiguration configuration = {};
    configuration.engine = app.engine;
    configuration.gltfPath = gltfPath.c_str();
    configuration.normalizeSkinningWeights = true;
    app.resourceLoader = new gltfio::ResourceLoader(configuration);
    app.stbDecoder = gltfio::createStbProvider(app.engine);
    app.ktxDecoder = gltfio::createKtx2Provider(app.engine);
    app.resourceLoader->addTextureProvider("image/png", app.stbDecoder);
    app.resourceLoader->addTextureProvider("image/jpeg", app.stbDecoder);
    app.resourceLoader->addTextureProvider("image/ktx2", app.ktxDecoder);
    bool loaded_resources = app.resourceLoader->loadResources(app.asset);
    if (!loaded_resources) {
        std::cerr << "Unable to load gltf resources" << std::endl;
        exit(1);
    }

    // pre-compile all material variants
    std::set<Material*> materials;
    RenderableManager const& rcm = app.engine->getRenderableManager();
    utils::Slice<utils::Entity> const renderables{
            app.asset->getRenderableEntities(), app.asset->getRenderableEntityCount() };
    for (utils::Entity const e: renderables) {
        auto ri = rcm.getInstance(e);
        size_t const c = rcm.getPrimitiveCount(ri);
        for (size_t i = 0; i < c; i++) {
            MaterialInstance* const mi = rcm.getMaterialInstanceAt(ri, i);
            Material* ma = const_cast<Material *>(mi->getMaterial());
            materials.insert(ma);
        }
        app.scene->addEntity(e);
    }

    for (Material* ma : materials) {
        // First compile high priority variants
        ma->compile(Material::CompilerPriorityQueue::HIGH,
                UserVariantFilterBit::DIRECTIONAL_LIGHTING |
                UserVariantFilterBit::DYNAMIC_LIGHTING |
                UserVariantFilterBit::SHADOW_RECEIVER);

        // and then, everything else at low priority, except STE, which is very uncommon.
        ma->compile(Material::CompilerPriorityQueue::LOW,
                UserVariantFilterBit::FOG |
                UserVariantFilterBit::SKINNING |
                UserVariantFilterBit::SSR |
                UserVariantFilterBit::VSM);
    }

    app.instance = app.asset->getInstance();
    buffer.clear();
    buffer.shrink_to_fit();
}

void loadIBL(App &app, const std::string &path) {
    utils::Path iblPath(path);
    if (!iblPath.exists()) {
        std::cerr << "The specified IBL path does not exist: " << iblPath << std::endl;
        return;
    }

    // Note that IBL holds a skybox, and Scene also holds a reference.  We cannot release IBL's
    // skybox until after new skybox has been set in the scene.
    app.ibl = std::make_unique<IBL>(*app.engine);

    if (!iblPath.isDirectory()) {
        if (!app.ibl->loadFromEquirect(iblPath)) {
            std::cerr << "Could not load the specified IBL: " << iblPath << std::endl;
            app.ibl.reset(nullptr);
            return;
        }
    } else {
        if (!app.ibl->loadFromDirectory(iblPath)) {
            std::cerr << "Could not load the specified IBL: " << iblPath << std::endl;
            app.ibl.reset(nullptr);
            return;
        }
    }

    if (app.ibl != nullptr) {
        //app.ibl->getSkybox()->setLayerMask(0x7, 0x4);
        // app.scene->setSkybox(app.ibl->getSkybox());
        app.scene->setIndirectLight(app.ibl->getIndirectLight());
    }
}

void setup(App &app, uint32_t res) {

#ifdef TARGET_OS_MACOS
    Engine *engine = Engine::Builder().backend(backend::Backend::METAL).build();
#else 
    Engine *engine = Engine::Builder().backend(backend::Backend::VULKAN).build();
#endif // MACOS 
    if (!engine) {
        std::cerr << "Could not create filament engine" << std::endl;
        return;
    }
    app.engine = engine;

    app.names = new utils::NameComponentManager(utils::EntityManager::get());
      
    auto swap_chain = engine->createSwapChain(res, res, 0);
    if (!swap_chain) {
        std::cerr << "Could not get swapchain" << std::endl;
        engine->destroy(&engine);
        return;
    }
    app.swap_chain = swap_chain;

    app.renderer = engine->createRenderer();
    app.scene = engine->createScene();
    app.view = engine->createView();
    app.view->setViewport({ 0, 0, res, res });
    
    app.cam_entity = utils::EntityManager::get().create();
    app.camera = engine->createCamera(app.cam_entity);
    app.camera->setExposure(16.0f, 1 / 125.0f, 100.0f);

    app.view->setCamera(app.camera);
    app.view->setScene(app.scene);

    engine->enableAccurateTranslations();
    auto& tcm = engine->getTransformManager();
    app.rootTransformEntity = engine->getEntityManager().create();
    tcm.create(app.rootTransformEntity);

    app.materials = gltfio::createUbershaderProvider(engine, UBERARCHIVE_DEFAULT_DATA, UBERARCHIVE_DEFAULT_SIZE);

    app.assetLoader = gltfio::AssetLoader::create({ app.engine, app.materials, app.names });

    // set so that pixels untouched by view are fully transparent allowing the
    // renderer's clear color to show through.
    app.view->setBlendMode(BlendMode::TRANSLUCENT);

    // enable AO
    app.view->setAmbientOcclusionOptions({ .enabled = true });
}

void preRender(App &app) {
    using namespace filament::math;

    auto& rcm = app.engine->getRenderableManager();

    app.camera->setLensProjection(40.0, 1.0, 0.1, 100.0);

    app.view->setCamera(app.camera);

    // Override the aspect ratio in the glTF file and adjust the aspect ratio of this
    // camera to the viewport.
    const Viewport& vp = app.view->getViewport();
    double const aspectRatio = (double) vp.width / vp.height;
    app.camera->setScaling({1.0 / aspectRatio, 1.0});

    // technically we don't need to do this each frame
    auto& tcm = app.engine->getTransformManager();
    TransformManager::Instance const& root = tcm.getInstance(app.rootTransformEntity);
    tcm.setParent(tcm.getInstance(app.cam_entity), root);
    tcm.setParent(tcm.getInstance(app.asset->getRoot()), root);

    app.view->setColorGrading(nullptr);

    app.renderer->setClearOptions({ .clearColor = { 1.0, 1.0, 1.0, 0.0 }, .clear = true });
}

struct ScreenshotState {
    View* view;
    std::string filename;
    bool done;
};

static void convertRGBAtoRGB(void* buffer, uint32_t width, uint32_t height) {
    uint8_t* writePtr = static_cast<uint8_t*>(buffer);
    uint8_t const* readPtr = static_cast<uint8_t const*>(buffer);
    for (uint32_t i = 0, n = width * height; i < n; ++i) {
        writePtr[0] = readPtr[0];
        writePtr[1] = readPtr[1];
        writePtr[2] = readPtr[2];
        writePtr += 3;
        readPtr += 4;
    }
}

void renderToFile(App &app, std::string filename) {
    auto view = app.view;
    auto renderer = app.renderer;
    const Viewport& vp = view->getViewport();
    const size_t byteCount = vp.width * vp.height * 4;

    ScreenshotState state{ .view = view, .filename = filename, .done = false };

    // Create a buffer descriptor that writes the PPM after the data becomes ready on the CPU.
    backend::PixelBufferDescriptor buffer(
        new uint8_t[byteCount], byteCount,
        backend::PixelBufferDescriptor::PixelDataFormat::RGBA,
        backend::PixelBufferDescriptor::PixelDataType::UBYTE,
        [](void* buffer, size_t size, void* user) {
            ScreenshotState* state = static_cast<ScreenshotState*>(user);
            const Viewport& vp = state->view->getViewport();

            // ReadPixels on Metal only supports RGBA, but the PPM format only supports RGB.
            // So, manually perform a quick transformation here.
            // convertRGBAtoRGB(buffer, vp.width, vp.height);

            utils::Path out(state->filename);
            // std::ofstream ppmStream(out);
            // ppmStream << "P6 " << vp.width << " " << vp.height << " " << 255 << std::endl;
            // ppmStream.write(static_cast<char*>(buffer), vp.width * vp.height * 3);
            // stbi_write_image_png(state->filename, vp.width, vp.height, 4, static_cast<char*>(buffer));
            int len;
            auto png = stbi_write_png_to_mem(static_cast<unsigned char*>(buffer), 0, vp.width, vp.height, 4, &len);
            if (png == NULL) {
                std::cerr << "Could not write image to png mem" << std::endl;
            }
            std::ofstream png_stream(out);
            png_stream.write(reinterpret_cast<char*>(png), len);
            STBIW_FREE(png);
            delete[] static_cast<uint8_t*>(buffer);
            state->done = true;
        },
        &state
    );

    while (!app.renderer->beginFrame(app.swap_chain))
        ;
    app.renderer->render(app.view);
    // Invoke readPixels asynchronously.
    renderer->readPixels((uint32_t) vp.left, (uint32_t) vp.bottom, vp.width, vp.height,
            std::move(buffer));
    app.renderer->endFrame();

    // filament expects to fill the command stream while waiting for CPU actions
    // to complete.
    while (!state.done) {
        if (app.renderer->beginFrame(app.swap_chain)) {
            app.renderer->render(app.view);
            app.renderer->endFrame();
        }
    }
}

void render(App &app) {
    using namespace filament::math;
    std::array<float3, 9> views = { 
        float3(0, -1, 0),
        float3(1, 0, 0),
        float3(0, 1, 0),
        float3(-1, 0, 0),
        float3(1, -1, 1),
        float3(1, 1, 1),
        float3(-1, 1, 1),
        float3(-1, -1, 1),
        float3(0, 0, 1) };

    int i = 0;
    for (auto vdir : views)  {
        // z-up since we match blender where z is up.
        float3 up = norm(vdir - float3(0, 0, 1)) < 1.e-6 ? float3(0, 1, 0) : float3(0, 0, 1);
        app.camera->lookAt(normalize(vdir) * 2.0, {0, 0, 0}, up);

        std::stringstream filename;
        filename << "render0" << i << ".png";
        std::cout << "rendering " << filename.str() << std::endl;

        auto tic = std::chrono::high_resolution_clock::now();
        renderToFile(app, filename.str());
        auto toc = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
        std::cout << "Render + Image write time (ms): " << duration.count() << std::endl;
        ++i;
    }
    
}

void cleanup(App &app) {
    app.assetLoader->destroyAsset(app.asset);
    app.materials->destroyMaterials();

    delete app.materials;
    delete app.names;

    gltfio::AssetLoader::destroy(&app.assetLoader);

    app.ibl.reset();
    app.engine->destroy(app.scene);
    Engine::destroy(app.engine);
}

math::mat4f fitIntoUnitCube(const Aabb& bounds, float zoffset) {
    using namespace filament::math;
    float3 minpt = bounds.min;
    float3 maxpt = bounds.max;
    float maxExtent;
    maxExtent = std::max(maxpt.x - minpt.x, maxpt.y - minpt.y);
    maxExtent = std::max(maxExtent, maxpt.z - minpt.z);
    std::cout << "max extent: " << maxExtent << std::endl;
    float scaleFactor = 1.0f / maxExtent;
    float3 center = (minpt + maxpt) / 2.0f;
    center.z += zoffset / scaleFactor;
    return mat4f::scaling(float3(scaleFactor)) * mat4f::translation(-center);
}

void setupCamera(App &app) {
    using namespace filament::math;
    Aabb aabb = app.asset->getBoundingBox();
    auto transform = fitIntoUnitCube(aabb, 0);
    aabb = aabb.transform(transform);

    std::cout << transform << std::endl;

    auto root = app.asset->getRoot();
    auto &tcm = app.engine->getTransformManager();

    // convert from y-up to z-up to match blender.
    mat3f rot(float3(1, 0, 0), float3(0, 0, -1), float3(0, 1, 0));
    rot = transpose(rot);
    mat4f coord_xform(rot);
    std::cout << "rotation " << coord_xform << std::endl;
    transform = coord_xform * transform;

    tcm.setTransform(tcm.getInstance(root), transform);

    app.camera->lookAt({4, 0, -4}, {0, 0, -4}, {0, 1, 0});
}

int main(int argc, char **argv) {

    App app;

    // utils::Path filename("./assets/FlightHelmet/FlightHelmet.gltf");
    // utils::Path filename("/Users/akashgarg/scripts/LB_Reed_Diffuser.glb");
    utils::Path filename(argv[1]);

    setup(app, 512);

    auto tic = std::chrono::high_resolution_clock::now();
    loadIBL(app, "assets/lightroom_14b.hdr");
    loadAsset(app, filename);
    auto toc = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic);
    std::cout << "Load glTF time (ms): " << duration.count() << std::endl;

    setupCamera(app);

    preRender(app);
    render(app);
    cleanup(app);
}
