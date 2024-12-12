#include "state.hpp"

#include <glm/ext/matrix_transform.hpp>

#include "fun.hpp"
#include "game.hpp"
#include "gfx/base_types.hpp"
#include "gfx/gui/gui.hpp"
namespace rdm {
GameState::GameState(Game* game) {
  this->game = game;
  state = Intro;
  timer = 10.f;

  game->getGfxEngine()->renderStepped.listen([this, game] {
    auto mainMenu =
        game->getGfxEngine()->getGuiManager()->getComponentByName("MainMenu");
    auto mainMenuButtons =
        game->getGfxEngine()->getGuiManager()->getComponentByName(
            "MainMenuButtons");
    if (!mainMenu) {
      Log::printf(LOG_ERROR,
                  "Define component MainMenu in the file for dat3/%s.xml",
                  Fun::getModuleName().c_str());
      return;
    }
    if (!mainMenuButtons) {
      Log::printf(
          LOG_ERROR,
          "Define component MainMenuButtons in the file for dat3/%s.xml",
          Fun::getModuleName().c_str());
      return;
    }
    switch (state) {
      case MainMenu: {
        mainMenuButtons.value()->domRoot.visible = true;
        mainMenu.value()->domRoot.visible = true;
      } break;
      case Intro: {
        mainMenuButtons.value()->domRoot.visible = false;
        mainMenu.value()->domRoot.visible = false;
        Graph::Node node;
        node.basis = glm::identity<glm::mat3>();
        node.origin = glm::vec3(0);
        std::shared_ptr<gfx::Material> material =
            game->getGfxEngine()->getMaterialCache()->getOrLoad("Mesh").value();
        gfx::Camera& camera = game->getGfxEngine()->getCamera();
        float time = game->getGfxEngine()->getTime();
        game->getGfxEngine()->setClearColor(glm::vec3(0.0, 0.0, 0.0));
        if (time < 1.0) {
          camera.setTarget(
              glm::vec3(2.f - (time * 2.f), 0.0, 2.f - (time * 2.f)));
        } else if (time > 9.0) {
          camera.setTarget(
              glm::vec3(0.0, glm::mix(0.0, 10.0, 9.f - time), 0.0));
          game->getGfxEngine()->setClearColor(
              glm::mix(glm::vec3(0.0), glm::vec3(1), time - 9.f));
        } else {
        }
        camera.setUp(glm::vec3(0.0, 1.0, 0.0));
        camera.setNear(0.01f);
        camera.setFar(100.0f);
        camera.setFOV(30.f + (time * 4.f));
        float distance = 4.0 + time;
        camera.setPosition(glm::vec3(0.0, 0.0, distance));
        gfx::BaseProgram* program =
            material->prepareDevice(game->getGfxEngine()->getDevice(), 0);
        program->setParameter(
            "model", gfx::DtMat4,
            gfx::BaseProgram::Parameter{.matrix4x4 = node.worldTransform()});
        gfx::Model* model = game->getGfxEngine()
                                ->getMeshCache()
                                ->get("dat0/entropy.obj")
                                .value();
        model->render(game->getGfxEngine()->getDevice());
      } break;
    }
  });

  game->getWorld()->stepped.listen([this, game] {
    switch (state) {
      case Intro:

        timer -= 1.0 / 60.0;
        if (timer <= 0.0) state = MainMenu;
        break;
      case MainMenu:
        break;
    }
  });
}

}  // namespace rdm
