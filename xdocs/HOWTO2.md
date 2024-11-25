# Examples

## Logging

Logging is simple.

	#include <logging.hpp>
	
	...
	
	rdm::Log::printf(rdm::LOG_DEBUG, "%s", "Uses printf format");

## Scheduler

### Adding job

Adding a scheduler job (see rdm::SchedulerJob) is simple.

#### Defining job class

Typically, when you have a class that requires a scheduler job, you pass a pointer to the owner class within the job constructor.

Here is an example:

	#include <world.hpp>
	
	class MyClass;
	class MyJob : public rdm::SchedulerJob {
		MyClass* myClass;
	public:
		MyJob(MyClass* me) : SchedulerJob("MyJob"), myClass(me) {
		
		}
		
		virtual double getFrameRate() {
			return 1.0 / 60.0; // This is the default, if you dont define getFrameRate()
		}
		
		virtual Result step() {
			// your code goes here
			
			return Stepped;
		}
	};
	
	class MyClass {
	public:
		MyClass(rdm::World* world) {
			world->getScheduler()->addJob(new MyJob(this));
		};
	};

Upon initializing MyClass, it will add MyJob to the World's scheduler. It will begin executing only before the main loop starts, any scheduler jobs added after (at the moment) will not be executed.

Passing your MyClass to the MyJob is not necessary either, you have the freedom to make it run independently from all your other code.

## Input

See rdm::Input

### Input axis

Creating an input axis is simple.

	//                               name,     pos,    neg
	rdm::Input::singleton()->newAxis("MyAxis", SDLK_a, SDLK_b);

Then, you can read the value of the axis using
	
	Input::Axis* theAxis = Input::singleton()->getAxis("MyAxis");
	float val = theAxis->value;

rdm::putil::FpsController for example, requires you define 2 axis' named ForwardBackward, and LeftRight.

## Network

See rdm::network::NetworkManager, and rdm::network::Entity

### Architecture

RDM4001 uses a simple client-server architecture, where the clients receive the game state from the server, and have one class that they send their specific updates (rdm::network::Player or any inherited class) to.

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

This creates a class that inherits rdm::network::Entity. Some things you will find interesting are

- serialize: This is the function that NetworkManager calls when it wants to send the data from your Entity to the remote peer. On entities that inherit from rdm::network::Player, the server will allow the remote peer to send entity data to itself.

- deserialize: This is the function that NetworkManager will call when it receives data from the server, or from a peer that is allowed to modify the current entity.

- tick: This is called every network tick. The rate may be variable, as the network tick rate is defined by CVar net_rate (by default it is 60.0, but check the tick rate using 

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
	
which will return a pointer to Entity*. You can then dynamic_cast it to your type of choice. instantiate is only useful when you are using the server.

#### Signals

Signals require that you use the ClosureId returned by `listen` to remove the closure after your Entity gets deconstructed.

In your MyEntity class definition:

	#include <signal.hpp>

	...
	
	private:
		ClosureId gfxJob;

In your MyEntity constructor:

	if (!getManager()->isBackend()) {
		gfxJob = getGfxEngine()->renderStepped.listen([this] {
			{
	  
In your ~MyEntity deconstructor:

	if (!getManager()->isBackend()) {
		getGfxEngine()->renderStepped.removeListener(gfxJob);

If the closure is not removed, it can lead to segmentation faults.

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

