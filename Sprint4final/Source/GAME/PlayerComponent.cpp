#include "../UTIL/Utilities.h"
#include "../CCL.h"
#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../DRAW/ModelManager.h"

namespace GAME
{
	void Player_Update(entt::registry& registry, entt::entity entity) {
		auto inputView = registry.view<UTIL::Input>();
		auto deltaTimeView = registry.view<UTIL::DeltaTime>();

		auto& playerTrans = registry.get<Transform>(entity).transform;

		std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
			registry.view<UTIL::Config>().front()).gameConfig;
		float playerModelSpeed = (*config).at("Player").at("speed").as<float>();

		if (!inputView.empty() && !deltaTimeView.empty()) {
			auto inputEntity = inputView.front();
			auto& input = registry.get<UTIL::Input>(inputEntity);

			auto deltaTimeEntity = deltaTimeView.front();
			auto& deltaTime = registry.get<UTIL::DeltaTime>(deltaTimeEntity).dtSec;


			float states[6] = { 0, 0, 0, 0 };

			input.immediateInput.GetState(G_KEY_W, states[0] = 0);
			input.immediateInput.GetState(G_KEY_S, states[1] = 0);
			input.immediateInput.GetState(G_KEY_D, states[2] = 0);
			input.immediateInput.GetState(G_KEY_A, states[3] = 0);

			double Z_Change = states[0] - states[1];
			double X_Change = states[2] - states[3];

			const float pfs = playerModelSpeed * deltaTime;

			GW::MATH::GMatrix::TranslateLocalF(playerTrans,
				GW::MATH::GVECTORF{ static_cast<float>(X_Change * pfs),
				0, static_cast<float>(Z_Change * pfs) }, playerTrans);

			bool firePressed = false;
			float fireStates[4] = { 0, 0, 0, 0 };

			input.immediateInput.GetState(G_KEY_UP, fireStates[0]);
			input.immediateInput.GetState(G_KEY_DOWN, fireStates[1]);
			input.immediateInput.GetState(G_KEY_LEFT, fireStates[2]);
			input.immediateInput.GetState(G_KEY_RIGHT, fireStates[3]);

			firePressed = fireStates[0] > 0 || fireStates[1] > 0 || fireStates[2] > 0 || fireStates[3] > 0;

			if (registry.all_of<GAME::Invulnerability>(entity)) {
				auto& invuln = registry.get<GAME::Invulnerability>(entity);
				invuln.cooldown -= deltaTime;

				if (invuln.cooldown <= 0.0f) {
					registry.remove<GAME::Invulnerability>(entity); 
				}
			}

			if (registry.all_of<isFiring>(entity)) {
				auto& firing = registry.get<isFiring>(entity);
				firing.cooldown -= deltaTime;

				if (firing.cooldown <= 0.0f) {
					registry.remove<isFiring>(entity);
				}
			}
			else if (firePressed) {
				auto bulletEntity = registry.create();

				auto& bulletTransform = registry.emplace<Transform>(bulletEntity, playerTrans);

				registry.emplace<Collidable>(bulletEntity);

				registry.emplace<Bullet>(bulletEntity);
				Velocity velocity;

				auto bulletDirection = velocity.direction;

				float dirX = fireStates[3] - fireStates[2];
				float dirZ = fireStates[0] - fireStates[1];

				if (dirX != 0 || dirZ != 0) {
					bulletDirection.x = dirX;
					bulletDirection.z = dirZ;


					float bulletSpeed = (*config).at("Bullet").at("speed").as<float>();
					float fireRate = (*config).at("Player").at("firerate").as<float>();

					bulletDirection.x *= bulletSpeed;
					bulletDirection.z *= bulletSpeed;

					registry.emplace<Velocity>(bulletEntity, Velocity{ bulletDirection });

					registry.emplace<isFiring>(entity, isFiring{ fireRate });

					std::string bulletModelName = (*config).at("Bullet").at("model").as<std::string>();

					entt::entity modelManagerEntity = registry.view<DRAW::ModelManager>().front();
					auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerEntity);


					auto bulletMeshEntities = modelManager.getRenderableEntities(registry, bulletModelName, modelManagerEntity);

					if (!bulletMeshEntities.empty()) {
						DRAW::MeshCollection bulletMeshes;
						if (firePressed) {
							for (auto meshEntity : bulletMeshEntities) {
								auto newMeshEntity = registry.create();

								auto gpuMesh = registry.get<DRAW::GPUInstance>(meshEntity);
								auto geoMesh = registry.get<DRAW::GeometryData>(meshEntity);
								registry.emplace<DRAW::GPUInstance>(newMeshEntity, gpuMesh);
								registry.emplace<DRAW::GeometryData>(newMeshEntity, geoMesh);

								

								bulletMeshes.meshes.push_back(newMeshEntity);
							}

							bulletMeshes.collider = modelManager.getCollider(registry, bulletModelName, modelManagerEntity);

							registry.emplace<DRAW::MeshCollection>(bulletEntity, bulletMeshes);

							bulletTransform.transform = playerTrans;
						}
					}
				}
			}
		}
	}
	CONNECT_COMPONENT_LOGIC() {
		registry.on_update<Player>().connect<Player_Update>();
	};
}