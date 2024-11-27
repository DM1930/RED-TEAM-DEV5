// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"
#include "./DRAW/ModelManager.h"



// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;	

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

	auto gameConfig = registry.create();
	registry.emplace<UTIL::Config>(gameConfig);

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	GameplayBehavior(registry); // create entities and components for gameplay
	
	MainLoopBehavior(registry); // update windows and input

	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear(); 

	return 0; // now destructors will be called for all components
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
		registry.view<UTIL::Config>().front()).gameConfig;

	auto display = registry.create();

	std::string levelpath = (*config).at("Level1").at("levelFile").as<std::string>();
	std::string modelpath = (*config).at("Level1").at("modelPath").as<std::string>();
	registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{ levelpath, modelpath });
	

	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	std::string title = (*config).at("Window").at("title").as<std::string>();

	// Add an entity to handle all the graphics data
	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, title});

	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
		DRAW::VulkanRendererInitialization{ 
			vertShader, pixelShader,
			{ {0.2f, 0.2f, 0.25f, 1} } , { 1.0f, 0u }, 75.f, 0.1f, 100.0f });
	registry.emplace<DRAW::VulkanRenderer>(display);
	
	// TODO : Load the Level then update the Vertex and Index Buffers
	registry.emplace<DRAW::GPULevel>(display);

	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent& e) {
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) {
			registry.clear<DRAW::VulkanRenderer>();
		}
		});
	//cannot get something with no member variables in it
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create a camera entity and emplace it
	auto camera = registry.create();
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = { 0.0f,  45.0f, -5.0f };
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(camera,
		DRAW::Camera{ initialCamera });
}

// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
		registry.view<UTIL::Config>().front()).gameConfig;

	// Create the input objects
	auto input = registry.create();
	registry.emplace<UTIL::Input>(input);

	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);

	std::string playerModelName = (*config).at("Player").at("model").as<std::string>();
	std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();
	int enemyHitPoints = (*config).at("Enemy1").at("hitpoints").as<int>();
	int playerHitPoints = (*config).at("Player").at("hitpoints").as<int>();

	entt::entity modelManagerEntity = registry.view<DRAW::ModelManager>().front();
	auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerEntity);
	auto playerEntity = registry.create();
	registry.emplace<GAME::Player>(playerEntity);
	registry.emplace<GAME::Collidable>(playerEntity);
	registry.emplace<GAME::playerHealth>(playerEntity, GAME::playerHealth{ playerHitPoints });


	auto playerMeshEntities = modelManager.getRenderableEntities(registry, playerModelName, modelManagerEntity);

	if (!playerMeshEntities.empty()) {
		DRAW::MeshCollection playerMeshes;

		for (auto meshEntity : playerMeshEntities) {
			
				auto newMeshEntity = registry.create();

				auto gpuMesh = registry.get<DRAW::GPUInstance>(meshEntity);
				auto geoMesh = registry.get<DRAW::GeometryData>(meshEntity);
				registry.emplace<DRAW::GPUInstance>(newMeshEntity, gpuMesh);
				registry.emplace<DRAW::GeometryData>(newMeshEntity, geoMesh);

				playerMeshes.meshes.push_back(newMeshEntity);
		}

		playerMeshes.collider = modelManager.getCollider(registry, playerModelName, modelManagerEntity);

		registry.emplace<DRAW::MeshCollection>(playerEntity, playerMeshes);

		auto& playerTransform = registry.emplace<GAME::Transform>(playerEntity);
		playerTransform.transform = registry.get<DRAW::GPUInstance>(playerMeshEntities[0]).transform;
	}

	auto enemyEntity = registry.create();
	registry.emplace<GAME::Enemy>(enemyEntity);
	registry.emplace<GAME::Collidable>(enemyEntity);
	registry.emplace<GAME::enemyHealth>(enemyEntity, GAME::enemyHealth{ enemyHitPoints });
	auto& velocity = registry.emplace<GAME::Velocity>(enemyEntity);

	int initialShatterCount = (*config).at("Enemy1").at("initialShatterCount").as<int>();
	int shatterAmount = (*config).at("Enemy1").at("shatterAmount").as<int>();
	float shatterScale = (*config).at("Enemy1").at("shatterScale").as<float>();
	registry.emplace<GAME::Shatters>(enemyEntity, GAME::Shatters{ initialShatterCount, shatterScale, shatterAmount });

	auto enemyMeshEntities = modelManager.getRenderableEntities(registry, enemyModelName, modelManagerEntity);

	if (!enemyMeshEntities.empty()) {
		DRAW::MeshCollection enemyMeshes;

		for (auto meshEntity : enemyMeshEntities) {
			auto newMeshEntity = registry.create();

			auto gpuMesh = registry.get<DRAW::GPUInstance>(meshEntity);
			auto geoMesh = registry.get<DRAW::GeometryData>(meshEntity);
			registry.emplace<DRAW::GPUInstance>(newMeshEntity, gpuMesh);
			registry.emplace<DRAW::GeometryData>(newMeshEntity, geoMesh);

			enemyMeshes.meshes.push_back(newMeshEntity);
		}

		enemyMeshes.collider = modelManager.getCollider(registry, enemyModelName, modelManagerEntity);

		registry.emplace<DRAW::MeshCollection>(enemyEntity, enemyMeshes);

		auto& enemyTransform = registry.emplace<GAME::Transform>(enemyEntity);
		enemyTransform.transform = registry.get<DRAW::GPUInstance>(enemyMeshEntities[0]).transform;

		float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();
		GAME::Velocity speed;
		speed.direction = UTIL::GetRandomVelocityVector();
		speed.direction.x = speed.direction.x * enemySpeed;
		speed.direction.y = speed.direction.y * enemySpeed;
		speed.direction.z = speed.direction.z * enemySpeed;
		velocity.direction = speed.direction;
	}

	auto gameManagerEntity = registry.create();
	registry.emplace<GAME::GameManager>(gameManagerEntity);
}

// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry& registry)
{	
	// main loop
	int closedCount; // count of closed windows
	auto winView = registry.view<APP::Window>(); // for updating all windows
	auto& deltaTime = registry.emplace<UTIL::DeltaTime>(registry.create()).dtSec;
	// for updating all windows
	do {
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		deltaTime = elapsed;

		// TODO : Update Game
		auto gameManagerView = registry.view<GAME::GameManager>();
		for (auto entity : gameManagerView) {
			registry.patch<GAME::GameManager>(entity);
		}


		closedCount = 0;
		// find all Windows that are not closed and call "patch" to update them
		for (auto entity : winView) {
			if (registry.any_of<APP::WindowClosed>(entity))
				++closedCount;
			else
				registry.patch<APP::Window>(entity); // calls on_update()
		}
	} while (winView.size() != closedCount); // exit when all windows are closed
}
