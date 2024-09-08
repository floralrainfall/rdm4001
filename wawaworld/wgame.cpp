#include "wgame.hpp"
#include "logging.hpp"
#include "input.hpp"
#include "filesystem.hpp"

namespace ww {
struct WGamePrivate {
  std::unique_ptr<gfx::BaseBuffer> pbuf;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

WGame::~WGame() {
  delete game;
}

void WGame::initialize() {  
  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this]{

  });

  {
    std::scoped_lock lock(gfxEngine->getContext()->getMutex()); // lock graphics context
    gfxEngine->getContext()->setCurrent();
    
    try {
      // initialize graphics here
      std::shared_ptr<gfx::Material> material = gfx::Material::create();
      material->addTechnique(gfx::Technique::create(gfxEngine->getDevice(), "dat1/test.vs", "dat1/test.fs"));

      float positions[] = {
        0.f, 0.f, 
        1.f, 0.f,
        0.f, 1.0f,
      };
      game->pbuf = gfxEngine->getDevice()->createBuffer(); 
      game->pbuf->upload(gfx::BaseBuffer::Array, gfx::BaseBuffer::StaticDraw, sizeof(positions), positions);
      
      auto arp = gfxEngine->getDevice()->createArrayPointers();
      arp->addAttrib(gfx::BaseArrayPointers::Attrib(gfx::DtFloat, 0, 2, 0, 0, game->pbuf.get()));
      arp->upload();

      unsigned char index[] = {
        0, 1, 2
      };
      auto buf = gfxEngine->getDevice()->createBuffer();
      buf->upload(gfx::BaseBuffer::Element, gfx::BaseBuffer::StaticDraw, sizeof(index), index);
      
      gfx::Entity* ent = gfxEngine->addEntity(std::unique_ptr<gfx::Entity>(new gfx::BufferEntity(std::move(buf), 3, std::move(arp), NULL)));
      ent->setMaterial(gfxEngine->getMaterialCache()->getOrLoad("Sprite").value());

    } catch(std::exception& e) {
      Log::printf(LOG_ERROR, "Error initializing graphics: %s", e.what());
    }

    gfxEngine->getContext()->unsetCurrent();
  }
}
}