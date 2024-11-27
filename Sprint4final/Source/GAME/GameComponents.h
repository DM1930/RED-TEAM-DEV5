#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	/// TODO: Create the tags and components for the game
	/// 
	///*** Tags ***///
	struct Player {}; 
	struct Enemy {};  
	struct Bullet {};
	struct Collidable {};
	struct Obstacle {};
	struct toDestroy {};
	struct GameOver {};

	///*** Components ***///
	struct Transform
	{
		GW::MATH::GMATRIXF transform;  
	};
	
	struct GameManager
	{
	
	};

	struct isFiring
	{
		float cooldown;
	};

	struct Velocity
	{
		GW::MATH::GVECTORF direction = { 0.0f, 0.0f, 0.0f };
	};

	struct enemyHealth
	{
		int hitPoints;
	};

	struct Shatters {
		int initialShatterCount; 
		float shatterScale;      
		int shatterAmount;       
	};

	struct playerHealth
	{
		int hitPoints;
	};

	struct Invulnerability
	{
		float cooldown;
	};

}// namespace GAME
#endif // !GAME_COMPONENTS_H_