# Examples

## Input

### Input axis

Creating an input axis is simple.

	//                               name,     pos,    neg
	rdm::Input::singleton()->newAxis("MyAxis", SDLK_a, SDLK_b);

Then, you can read the value of the axis using
	
	Input::Axis* theAxis = Input::singleton()->getAxis("MyAxis");
	float val = theAxis->value;

rdm::putil::FpsController for example, requires you define 2 axis' named ForwardBackward, and LeftRight.

## Network

### Simple entity

Creating an entity is a little bit of an involved process.

#### Class definition

	#include <network/network.hpp>
	class MyEntity : public rdm::network::Entity {
	  // BOILERPLATE
	 public:
	  MyEntity(NetworkManager* manager, EntityId id) : rdm::network::Entity(manager, id) {
	  
	  }
	  
	  virtual const char* getTypeName() { return "MyEntity"; }; // You want this to match
	  
	  // Serializes the entities state.
	  virtual void serialize(BitStream& stream) {
	  
	  }
	  
	  // Deserializes an incoming entity update.
	  virtual void deserialize(BitStream& stream) {	  
	  
	  }
	  
	  // Called by the NetworkManager after servicing the ENet connection.
	  virtual void tick() {
	    if(getManager()->isBackend()) {
		  // code that will only be executed by the server...
		  
		  // this will tell the network manager that there are updates,
		  // this will cause the network manager to serialize this entity
		  getManager()->addPendingUpdate(getEntityId());
		} else {
		  // code that will only be executed by the client...
		}
	  }
	};
	
#### Adding constructor

Usually you will have a function in your Game class called something like `addEntityConstructors(rdm::network::NetworkManager* manager)`. You can then add the constructors with the following:

	void MyGame::addEntityConstructors(nrdm::network::NetworkManager* manager) {
		manager->setPassword("MYGAMEMYGAME"); // This is to prevent other games 
		// from connecting to your game, that might not support the protocol. 
		// This is not a "user password", which should be set by the user to 
		// lock down their own server.
		
		// The name should match what you put in getTypeName().
		manager->registerConstructor(rdm::network::EntityConstructor<MyEntity>, "MyEntity");
		
		// Eventually you will have an Entity that inherits rdm::network::Player,
		// you can tell the NetworkManager to instantiate that type using
		// manager->setPlayerType.
	}

You can call this in your initializeClient and initializeServer functions.

#### Instantiating

Instantiating an entity after you've added the constructor is simple. Just use

	(your network manager)->instantiate("MyEntity");
	
which will return a pointer to Entity*. You can then dynamic_cast it to your type of choice.

## Rendering

### Simple mesh

Requires the Mesh material to be defined in dat1/materials.json.

	std::shared_ptr<gfx::Material> material =
		gfxEngine->getMaterialCache()->getOrLoad("Mesh").value();
	gfx::BaseProgram* program =
		material->prepareDevice(gfxEngine->getDevice(), 0);
	program->setParameter("model", gfx::DtMat4,
		gfx::BaseProgram::Parameter{
			.matrix4x4 = glm::identity()});
	program->bind();
	gfx::Model* model = gfxEngine->getMeshCache()
		->get("dat5/baseq3/models/andi_rig.obj").value()
	model->render(getGfxEngine()->getDevice());
		
This can be called using the gfx::Engine::renderStepped signal. The listen function can be used for this purpose.

### Creating a material

Creating a material is slightly overcomplicated.

You need to add this to materials.json:

	"Materials": {
		"YourMaterial": {
			"Techniques": [
				{"ProgramName": "YourProgram"}
			]
	 ...
	 
Then, to add the program referenced by the technique

	"Programs": {
		"YourProgram": {"VSName": "dat1/yourprogram.vs.glsl", 
		                "FSName": "dat1/yourprogram.fs.glsl"}
	...
	
Then it can be referenced using rdm::gfx::MaterialCache.

