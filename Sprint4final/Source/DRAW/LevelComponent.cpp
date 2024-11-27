#include "DrawComponents.h"
#include "../CCL.h"
#include "ModelManager.h"
#include "../GAME/GameComponents.h"


namespace DRAW
{
	void CPULevel_Construct(entt::registry& registry, entt::entity entity)
	{
		auto& cpuLevel = registry.get<CPULevel>(entity);
		GW::SYSTEM::GLog log;
		log.Create("LevelLoadLog");
		log.EnableConsoleLogging(true);
		bool success = cpuLevel.levelData.LoadLevel(cpuLevel.levelPath.c_str(), cpuLevel.modelPath.c_str(), log);
		if (!success) {
			abort();
		}
	}

	void GPULevel_Construct(entt::registry& registry, entt::entity entity)
	{
		auto* gpuLevel = registry.try_get<CPULevel>(entity);

		if (gpuLevel == nullptr)
		{
			abort();
		}
		registry.emplace<VulkanVertexBuffer>(entity);

		registry.emplace<std::vector<H2B::VERTEX>>(entity, gpuLevel->levelData.levelVertices);

		registry.patch<VulkanVertexBuffer>(entity);

		registry.emplace<VulkanIndexBuffer>(entity);

		registry.emplace<std::vector<unsigned int>>(entity, gpuLevel->levelData.levelIndices);

		registry.patch<VulkanIndexBuffer>(entity);

		auto modelManagerEntity = registry.create();
		auto& modelManager = registry.emplace<ModelManager>(modelManagerEntity);

		for (const auto& blenderObject : gpuLevel->levelData.blenderObjects) {
			const Level_Data::LEVEL_MODEL& model = gpuLevel->levelData.levelModels[blenderObject.modelIndex];

			MeshCollection meshCollection;
			meshCollection.collider = gpuLevel->levelData.levelColliders[model.colliderIndex];


			for (unsigned int meshIndex = model.meshStart; meshIndex < model.meshStart + model.meshCount; ++meshIndex) {
				const H2B::MESH& mesh = gpuLevel->levelData.levelMeshes[meshIndex];

				auto newEntity = registry.create();

				if (model.isDynamic) {
					registry.emplace<DoNotRender>(newEntity);
					meshCollection.meshes.push_back(newEntity);
				}
				GeometryData geoData;
				geoData.indexStart = model.indexStart + mesh.drawInfo.indexOffset;
				geoData.indexCount = mesh.drawInfo.indexCount;
				geoData.vertexStart = model.vertexStart;
				registry.emplace<GeometryData>(newEntity, geoData);

				GPUInstance gpuInstance;
				gpuInstance.transform = gpuLevel->levelData.levelTransforms[blenderObject.transformIndex];
				gpuInstance.matData = gpuLevel->levelData.levelMaterials[model.materialStart + mesh.materialIndex].attrib;
				registry.emplace<GPUInstance>(newEntity, gpuInstance);
			}
			if (model.isDynamic) {
				modelManager.addCollection(blenderObject.blendername, meshCollection);
			}
			if (model.isCollidable) {
				auto collidableEntity = registry.create();

				registry.emplace<MeshCollection>(collidableEntity, meshCollection);

				auto& objectTransform = gpuLevel->levelData.levelTransforms[blenderObject.transformIndex];
				registry.emplace<GAME::Transform>(collidableEntity, objectTransform);

				registry.emplace<GAME::Collidable>(collidableEntity);
				registry.emplace<GAME::Obstacle>(collidableEntity); 
			}

		}
	}

	CONNECT_COMPONENT_LOGIC() {
		registry.on_construct<CPULevel>().connect<CPULevel_Construct>();
		registry.on_construct<GPULevel>().connect<GPULevel_Construct>();
	}
}