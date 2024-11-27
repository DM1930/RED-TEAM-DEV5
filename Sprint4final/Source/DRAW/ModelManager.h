#pragma once
#include "./Utility/load_data_oriented.h"
#include "../CCL.h"
#include <map>
#include <string>
#include <vector>

namespace DRAW
{
	struct MeshCollection {
		std::vector<entt::entity> meshes;
		GW::MATH::GOBBF collider;
	};

	struct ModelManager {
		std::map<std::string, MeshCollection> modelCollections;

		MeshCollection* getCollection(const std::string& name) {
			auto it = modelCollections.find(name);
			if (it != modelCollections.end()) {
				return &it->second;
			}
		}

		void addCollection(const std::string& name, const MeshCollection& collection) {
			modelCollections[name] = collection;
		}



		std::vector<entt::entity> getRenderableEntities(entt::registry& registry, const std::string& modelName, entt::entity entity) {
			auto* modelManager = registry.try_get<ModelManager>(entity);

			auto collection = modelManager->getCollection(modelName);
			if (collection) {
				return collection->meshes;
			}

			return {};
		}

		GW::MATH::GOBBF getCollider(entt::registry& registry, const std::string& modelName, entt::entity entity) {
			auto* modelManager = registry.try_get<ModelManager>(entity);

			auto collection = modelManager->getCollection(modelName);
			if (collection) {
				return collection->collider;
			}
		}
	};
}
