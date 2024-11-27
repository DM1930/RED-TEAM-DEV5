#ifndef UTILITIES_H_
#define UTILITIES_H_

#include "GameConfig.h"

namespace UTIL
{
	//GW::MATH::GMatrix matrixProxy;

	// TODO: Make this a new json loading gameConfig
	struct Config
	{
		std::shared_ptr<GameConfig> gameConfig = std::make_shared<GameConfig>();
	};

	struct DeltaTime
	{
		double dtSec;
	};

	struct Input
	{
		GW::INPUT::GController gamePads; // controller support
		GW::INPUT::GInput immediateInput; // twitch keybaord/mouse
		GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse
	};

	/// Method declarations
	void Construct_Input(entt::registry& registry, entt::entity entity);

	GW::MATH::GVECTORF GetRandomVelocityVector();

} // namespace UTIL
#endif // !UTILITIES_H_