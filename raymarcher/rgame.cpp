#include "rgame.hpp"

#include "gfx/gui/gui.hpp"
#include "input.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace rm {
struct RGamePrivate {
  float cameraPitch;
  float cameraYaw;

  std::shared_ptr<gfx::Material> rayMarchMaterial;
};

RGame::RGame() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new RGamePrivate();
}

RGame::~RGame() { delete game; }

void RGame::initialize() {
  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    glm::vec2 mouseDelta = Input::singleton()->getMouseDelta();

    Input::Axis* fbA = Input::singleton()->getAxis("ForwardBackward");
    Input::Axis* lrA = Input::singleton()->getAxis("LeftRight");

    game->cameraPitch += mouseDelta.x * (M_PI / 180.0);
    game->cameraYaw += mouseDelta.y * (M_PI / 180.0);

    glm::quat yawQuat =
        glm::angleAxis(game->cameraYaw, glm::vec3(1.f, 0.f, 0.f));
    glm::quat pitchQuat =
        glm::angleAxis(game->cameraPitch, glm::vec3(0.f, 1.f, 0.f));
    glm::mat3 vm = glm::toMat3(pitchQuat * yawQuat);
    glm::vec3 forward = glm::vec3(0, 0, 1);
    gfx::Camera& cam = gfxEngine->getCamera();
    float speed = 10.0;
    cam.setPosition(
        cam.getPosition() +
        (vm * glm::vec3(lrA->value, 0.0, fbA->value) * speed * (1.f / 60.f)));
    cam.setTarget(cam.getPosition() + vm * forward);
  });

  gfxEngine->initialized.listen([this] {
    game->rayMarchMaterial = gfxEngine->getMaterialCache()->getOrLoad("RayMarch").value_or(nullptr);
  });

  gfxEngine->renderStepped.listen([this] {
    gfxEngine->renderFullscreenQuad(NULL, game->rayMarchMaterial.get());
  });
}
};  // namespace rm
