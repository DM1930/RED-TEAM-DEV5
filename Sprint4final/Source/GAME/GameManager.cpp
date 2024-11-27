#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../DRAW/ModelManager.h"
#include "../CCL.h"
#include "../UTIL/Utilities.h"

namespace GAME
{
	using namespace GW::MATH;
	void BounceEnemy(GVECTORF enemyLocation, GVECTORF& enemyVelocity, GOBBF& obstacleBox)
	{
		GVECTORF point;
		GCollision::ClosestPointToOBBF(obstacleBox, enemyLocation, point);

		GVECTORF normal;
		GVector::SubtractVectorF(enemyLocation, point, normal);
		normal.y = 0.0f;
		normal.w = 0.0f;
		GVector::NormalizeF(normal, normal);

		float dot;
		GVector::DotF(enemyVelocity, normal, dot);
		dot *= 2.0f;
		GVector::ScaleF(normal, dot, normal);
		GVector::SubtractVectorF(enemyVelocity, normal, enemyVelocity);
	}

	void GameManager_Update(entt::registry& registry)
	{
		auto gameManagerEntity = registry.view<GameManager>().front();

		auto enemyView = registry.view<Enemy>();

		if (enemyView.empty()) {
			auto gameManagerEntity = registry.view<GameManager>().front();
			if (!registry.any_of<GameOver>(gameManagerEntity)) {
				registry.emplace<GameOver>(gameManagerEntity);
				std::cout << "You Win! Good Job!" << std::endl;
			}
		}

		if (!registry.any_of<GameOver>(gameManagerEntity))
		{
			std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
				registry.view<UTIL::Config>().front()).gameConfig;

			auto deltaTimeView = registry.view<UTIL::DeltaTime>();

			auto deltaTimeEntity = deltaTimeView.front();
			float deltaTime = registry.get<UTIL::DeltaTime>(deltaTimeEntity).dtSec;

			auto velocityView = registry.view<Transform, Velocity>();

			for (auto entity : velocityView) {
				auto& transform = velocityView.get<Transform>(entity);
				auto& velocity = velocityView.get<Velocity>(entity);

				float movementX = velocity.direction.x * deltaTime;
				float movementY = velocity.direction.y * deltaTime;
				float movementZ = velocity.direction.z * deltaTime;

				GW::MATH::GMatrix::TranslateGlobalF(transform.transform,
					GW::MATH::GVECTORF{ movementX, movementY, movementZ }, transform.transform);
			}

			auto view = registry.view<Transform, DRAW::MeshCollection>();
			registry.patch<Player>(registry.view<Player>().front());
			for (auto entity : view) {
				const auto& transform = registry.get<GAME::Transform>(entity);
				auto& meshCollection = registry.get<DRAW::MeshCollection>(entity);

				for (auto meshEntity : meshCollection.meshes) {
					auto& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
					gpuInstance.transform = transform.transform;
				}
			}
			//part 3c system setup
			auto& collisions = registry.view<Transform, DRAW::MeshCollection, Collidable>();
			for (auto i = collisions.begin(); i != collisions.end(); i++) {

				auto colliderA = registry.get<DRAW::MeshCollection>(*i).collider;
				auto& transformA = registry.get<Transform>(*i).transform;

				//scale the collider
				GVECTORF vecA;
				GMatrix::GetScaleF(transformA, vecA);
				colliderA.extent.x *= vecA.x;
				colliderA.extent.y *= vecA.y;
				colliderA.extent.z *= vecA.z;

				//transform the center of the collider into world space
				GMatrix::VectorXMatrixF(transformA, colliderA.center, colliderA.center);

				//rotate the collider
				GQUATERNIONF qA;
				GQuaternion::SetByMatrixF(transformA, qA);
				GQuaternion::MultiplyQuaternionF(colliderA.rotation, qA, colliderA.rotation);

				auto j = i;
				for (j++; j != collisions.end(); j++) {
					auto colliderB = registry.get<DRAW::MeshCollection>(*j).collider;
					auto& transformB = registry.get<Transform>(*j).transform;

					//scale the collider
					GVECTORF vecB;
					GMatrix::GetScaleF(transformB, vecB);
					colliderB.extent.x *= vecB.x;
					colliderB.extent.y *= vecB.y;
					colliderB.extent.z *= vecB.z;

					//transform the center of the collider into world space
					GMatrix::VectorXMatrixF(transformB, colliderB.center, colliderB.center);

					//rotate the collider
					GQUATERNIONF qB;
					GQuaternion::SetByMatrixF(transformB, qB);
					GQuaternion::MultiplyQuaternionF(colliderB.rotation, qB, colliderB.rotation);

					GCollision::GCollisionCheck result;
					GCollision::TestOBBToOBBF(colliderA, colliderB, result);
					if (result == GCollision::GCollisionCheck::COLLISION) {
						//objects have overlapped
						if (registry.all_of<Bullet>(*i) && registry.all_of<Obstacle>(*j)) {
							registry.emplace<toDestroy>(*i);
						}
						if (registry.all_of<Bullet>(*j) && registry.all_of<Obstacle>(*i)) {
							registry.emplace<toDestroy>(*j);
						}

						if (registry.all_of<Enemy>(*i) && registry.all_of<Obstacle>(*j)) {
							auto& vel = registry.get<Velocity>(*i).direction;
							BounceEnemy(transformA.row4, vel, colliderB);
						}

						if (registry.all_of<Enemy>(*j) && registry.all_of<Obstacle>(*i)) {
							auto& vel = registry.get<Velocity>(*j).direction;
							BounceEnemy(transformB.row4, vel, colliderA);
						}

						if (registry.all_of<Bullet>(*i) && registry.all_of<Enemy>(*j)) {
							auto& enemyHealth = registry.get<GAME::enemyHealth>(*j).hitPoints;
							enemyHealth--;
							registry.emplace<toDestroy>(*i);
							if (enemyHealth <= 0) {
								registry.emplace<toDestroy>(*j);
								entt::entity modelManagerEntity = registry.view<DRAW::ModelManager>().front();
								auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerEntity);

								if (registry.all_of<GAME::Shatters>(*j)) {
									auto shatters = registry.get<GAME::Shatters>(*j);
									auto& transform = registry.get<GAME::Transform>(*j);

									for (int i = 0; i < shatters.shatterAmount; ++i) {
										auto newEnemyEntity = registry.create();

										registry.emplace<GAME::Enemy>(newEnemyEntity);
										registry.emplace<GAME::Collidable>(newEnemyEntity);
										registry.emplace<GAME::enemyHealth>(newEnemyEntity, GAME::enemyHealth{ 3 });

										std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();
										auto newEnemyMeshEntities = modelManager.getRenderableEntities(registry, enemyModelName, modelManagerEntity);
										DRAW::MeshCollection newEnemyMeshes;

										for (auto meshEntity : newEnemyMeshEntities) {
											auto newMeshEntity = registry.create();
											auto gpuMesh = registry.get<DRAW::GPUInstance>(meshEntity);
											auto geoMesh = registry.get<DRAW::GeometryData>(meshEntity);
											registry.emplace<DRAW::GPUInstance>(newMeshEntity, gpuMesh);
											registry.emplace<DRAW::GeometryData>(newMeshEntity, geoMesh);

											newEnemyMeshes.meshes.push_back(newMeshEntity);
										}

										newEnemyMeshes.collider = modelManager.getCollider(registry, enemyModelName, modelManagerEntity);
										registry.emplace<DRAW::MeshCollection>(newEnemyEntity, newEnemyMeshes);


										auto& newTransform = registry.emplace<GAME::Transform>(newEnemyEntity);
										newTransform.transform = transform.transform;

										newTransform.transform.row1.x *= shatters.shatterScale;
										newTransform.transform.row2.y *= shatters.shatterScale;
										newTransform.transform.row3.z *= shatters.shatterScale;

										float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();
										GAME::Velocity velocity;
										velocity.direction = UTIL::GetRandomVelocityVector();
										velocity.direction.x *= enemySpeed;
										velocity.direction.y *= enemySpeed;
										velocity.direction.z *= enemySpeed;

										registry.emplace<GAME::Velocity>(newEnemyEntity, velocity);

										shatters.initialShatterCount--;
										if (shatters.initialShatterCount <= 0) {
											registry.remove<GAME::Shatters>(*j);
										}
										else
											registry.emplace<GAME::Shatters>(newEnemyEntity, GAME::Shatters{ shatters.initialShatterCount, shatters.shatterScale, shatters.shatterAmount });
									}
								}
							}
						}
						if (registry.all_of<Bullet>(*j) && registry.all_of<Enemy>(*i)) {
							auto& enemyHealth = registry.get<GAME::enemyHealth>(*i).hitPoints;
							enemyHealth--;
							registry.emplace<toDestroy>(*j);
							if (enemyHealth <= 0) {
								registry.emplace<toDestroy>(*i);
								entt::entity modelManagerEntity = registry.view<DRAW::ModelManager>().front();
								auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerEntity);

								if (registry.all_of<GAME::Shatters>(*i)) {
									auto& shatters = registry.get<GAME::Shatters>(*i);
									auto& transform = registry.get<GAME::Transform>(*i);

									for (int j = 0; j < shatters.shatterAmount; ++j) {
										auto newEnemyEntity = registry.create();

										registry.emplace<GAME::Enemy>(newEnemyEntity);
										registry.emplace<GAME::Collidable>(newEnemyEntity);
										registry.emplace<GAME::enemyHealth>(newEnemyEntity, GAME::enemyHealth{ 3 });

										std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();
										auto newEnemyMeshEntities = modelManager.getRenderableEntities(registry, enemyModelName, modelManagerEntity);
										DRAW::MeshCollection newEnemyMeshes;

										for (auto meshEntity : newEnemyMeshEntities) {
											auto newMeshEntity = registry.create();
											auto gpuMesh = registry.get<DRAW::GPUInstance>(meshEntity);
											auto geoMesh = registry.get<DRAW::GeometryData>(meshEntity);
											registry.emplace<DRAW::GPUInstance>(newMeshEntity, gpuMesh);
											registry.emplace<DRAW::GeometryData>(newMeshEntity, geoMesh);

											newEnemyMeshes.meshes.push_back(newMeshEntity);
										}

										newEnemyMeshes.collider = modelManager.getCollider(registry, enemyModelName, modelManagerEntity);
										registry.emplace<DRAW::MeshCollection>(newEnemyEntity, newEnemyMeshes);


										auto& newTransform = registry.emplace<GAME::Transform>(newEnemyEntity);
										newTransform.transform = transform.transform;

										newTransform.transform.row1.x *= shatters.shatterScale;
										newTransform.transform.row2.y *= shatters.shatterScale;
										newTransform.transform.row3.z *= shatters.shatterScale;

										float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();
										GAME::Velocity velocity;
										velocity.direction = UTIL::GetRandomVelocityVector();
										velocity.direction.x *= enemySpeed;
										velocity.direction.y *= enemySpeed;
										velocity.direction.z *= enemySpeed;

										registry.emplace<GAME::Velocity>(newEnemyEntity, velocity);

										shatters.initialShatterCount--;
										if (shatters.initialShatterCount <= 0) {
											registry.remove<GAME::Shatters>(*i);
										}
										else
											registry.emplace<GAME::Shatters>(newEnemyEntity, GAME::Shatters{ shatters.initialShatterCount, shatters.shatterScale, shatters.shatterAmount });
									}
								}
							}
						}
						if (registry.all_of<Player>(*i) && registry.all_of<Enemy>(*j)) {
							auto& PlayerHealth = registry.get<GAME::playerHealth>(*i);

							auto playerView = registry.view<Player, GAME::playerHealth>();

							for (auto entity : playerView) {
								auto& playerHealth = playerView.get<GAME::playerHealth>(entity);
								if (playerHealth.hitPoints > 0) {
									break;
								}
								else {
									registry.emplace<GameOver>(gameManagerEntity);
									std::cout << "You Lose! Game Over!" << std::endl;
								}
							}

							if (!registry.all_of<GAME::Invulnerability>(*i)) {
								PlayerHealth.hitPoints--;

								std::cout << "Player Health :" << PlayerHealth.hitPoints << std::endl;

								float invulnDuration = (*config).at("Player").at("invulnPeriod").as<float>();
								registry.emplace<GAME::Invulnerability>(*i, GAME::Invulnerability{ invulnDuration });
							}
						}
						if (registry.all_of<Player>(*j) && registry.all_of<Enemy>(*i)) {
							auto& PlayerHealth = registry.get<GAME::playerHealth>(*j);

							auto playerView = registry.view<Player, GAME::playerHealth>();

							for (auto entity : playerView) {
								auto& playerHealth = playerView.get<GAME::playerHealth>(entity);
								if (playerHealth.hitPoints > 0) {
									break;
								}
								else {
									registry.emplace<GameOver>(gameManagerEntity);
									std::cout << "You Lose! Game Over!" << std::endl;
								}
							}

							if (!registry.all_of<GAME::Invulnerability>(*j)) {
								PlayerHealth.hitPoints--;

								std::cout << "Player Health :" << PlayerHealth.hitPoints << std::endl;

								float invulnDuration = (*config).at("Player").at("invulnPeriod").as<float>();
								registry.emplace<GAME::Invulnerability>(*j, GAME::Invulnerability{ invulnDuration });
							}
						}
					}
				}
			}
			auto& destroySystem = registry.view<toDestroy>();
			for (auto ent : destroySystem) {
				if (registry.all_of<DRAW::MeshCollection>(ent)) {
					auto& meshCollection = registry.get<DRAW::MeshCollection>(ent);

					for (auto meshEntity : meshCollection.meshes) {
						registry.destroy(meshEntity);
					}
				}
				registry.destroy(ent);
			}
		}
	}



	CONNECT_COMPONENT_LOGIC() {
		registry.on_update<GameManager>().connect<GameManager_Update>();
	}
}
