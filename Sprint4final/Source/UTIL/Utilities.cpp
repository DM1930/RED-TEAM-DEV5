#include "Utilities.h"
#include "../CCL.h"
namespace UTIL
{
	void Construct_Input(entt::registry& registry, entt::entity entity)
	{
		auto& input = registry.get<Input>(entity);
		// Should only be one window, so grab the front
		auto& window = registry.get<GW::SYSTEM::GWindow>(
			registry.view<GW::SYSTEM::GWindow>().front());

		input.bufferedInput.Create(window);
		input.immediateInput.Create(window);
		input.gamePads.Create();

		auto& pressEvents = registry.emplace<GW::CORE::GEventCache>(entity);
		pressEvents.Create(32);

		input.bufferedInput.Register(pressEvents);
		input.gamePads.Register(pressEvents);
	}

	GW::MATH::GVECTORF GetRandomVelocityVector()
	{
		GW::MATH::GVECTORF vel = { (rand() % 20) - 10, 0.0f, (rand() % 20) - 10 };
		if (vel.x <= 0.0f && vel.x > -1.0f)
			vel.x = -1.0f;
		else if (vel.x >= 0.0f && vel.x < 1.0f)
			vel.x = 1.0f;

		if (vel.z <= 0.0f && vel.z > -1.0f)
			vel.z = -1.0f;
		else if (vel.z >= 0.0f && vel.z < 1.0f)
			vel.z = 1.0f;

		GW::MATH::GVector::NormalizeF(vel, vel);

		return vel;
	}

	CONNECT_COMPONENT_LOGIC() {
		registry.on_construct<Input>().connect<Construct_Input>();
	}
} // namespace UTIL