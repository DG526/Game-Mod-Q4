
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterScientist : public idAI {

public:

	CLASS_PROTOTYPE( rvMonsterScientist );

	rvMonsterScientist ( void );
	
	void				Spawn				( void );
	
	virtual void		OnDeath				( void );
	
	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo		( debugInfoProc_t proc, void* userData );

	//David Begin//
	virtual void		Think(void);
	virtual char		GetMove(void);
	virtual char		Cheat(void);
	/*
	bool synced = false;
	typedef enum		{Monster = -1, Draw = 0, Player = 1} Winner;
	char				chosenMove = 'n';
	int					roundStartTime = 0, seed = 42069; //Yes, seed is a nice and high number.
	idPlayer*			player;
	*/
	virtual void		ForceKill(void);
	virtual void		LootDrop(void);
	virtual idAI::Winner GetWinner(void);
	// David End //

private:

	CLASS_STATES_PROTOTYPE ( rvMonsterScientist );
};

CLASS_DECLARATION( idAI, rvMonsterScientist )
END_CLASS

/*
================
rvMonsterScientist::rvMonsterScientist
================
*/
rvMonsterScientist::rvMonsterScientist ( void ) {
	//David begin//
	player = gameLocal.GetLocalPlayer();
	if(player)
		player->inMatch = true;
	// David end //
}

/*
================
rvMonsterScientist::Spawn
================
*/
void rvMonsterScientist::Spawn ( void ) {
	PlayEffect ( "fx_fly", animator.GetJointHandle ( "effects_bone" ), true );
} 

/*
================
rvMonsterScientist::OnDeath
================
*/
void rvMonsterScientist::OnDeath ( void ) {
	StopEffect ( "fx_fly" );
	
	idAI::OnDeath ( );
}

/*
================
rvMonsterScientist::GetDebugInfo
================
*/
void rvMonsterScientist::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterScientist )
END_CLASS_STATES


//David Begin//

void rvMonsterScientist::Think(void) {
	if (!CheckDormant()) {
		UpdateAnimation();
		Present();
	}
	if (CheckDormant() || health == 0) {
		return;
	}
	if (!player)
		player = gameLocal.GetLocalPlayer();
	if (roundStartTime == 0)
		roundStartTime = gameLocal.time;
	if (player->opponent && gameLocal.time >= roundStartTime + 3000)
		gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "Rock, Paper, Scissors, SHOOT!");
	else if (player->opponent && gameLocal.time >= roundStartTime + 2000)
		gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "Rock, Paper, Scissors,");
	else if (player->opponent && gameLocal.time >= roundStartTime + 1000)
		gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "Rock, Paper,");
	else if (player->opponent)
		gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "Rock,");
	if (player->moveChoice != 'n' && chosenMove == 'n' && gameLocal.time <= roundStartTime + 2500 && canCheat)
		chosenMove = Cheat();
	if (gameLocal.time >= roundStartTime + 3000 && gameLocal.time < roundStartTime + 3333) {
		/*
		switch (GetMove()) {
		case 'r':
			gameLocal.Printf("Grunt picks Rock!\n");
			break;
		case 'p':
			gameLocal.Printf("Grunt picks Paper!\n");
			break;
		case 's':
			gameLocal.Printf("Grunt picks Scissors!\n");
			break;
		}
		*/
		if (chosenMove == 'n') {
			chosenMove = GetMove();
			gameLocal.Printf("Scientist picked %s!\n", (chosenMove == 'r') ? "Rock" : ((chosenMove == 'p') ? "Paper" : "Scissors"));
		}
	}
	if (gameLocal.time >= roundStartTime + 3333 && chosenMove != 'n') {
		rvMonsterScientist::Winner winner = GetWinner();
		if (winner == Monster)
			player->AdjustHealthByDamage(10);
		if (player->health <= 0)
			player->CalcHighScore();
		chosenMove = 'n';
		if (winner == Player)
		{
			player->inMatch = false;
			player->IncrementScore();
			player->opponent = idPlayer::OP_NONE;
			LootDrop();
			AdjustHealthByDamage(health);
			Killed(player, player, health, vec3_zero, INVALID_JOINT);
			Gib(vec3_zero, "damage_hyperblaster");
			return;
		}
	}
	if (health > 0 && gameLocal.time >= roundStartTime + 4750) {
		roundStartTime = gameLocal.time;
		player->hud->SetStateString("outcomeStatus", "");
		player->moveChoice = 'n';
		gameLocal.GetLocalPlayer()->hud->SetStateString("movedisp", "");
		chosenMove = 'n';
	}

}

char rvMonsterScientist::GetMove(void) {
	idRandom2 rand;
	rand.SetSeed(gameLocal.time % 69420);
	int choicePool = rand.RandomInt(15);
	//gameLocal.Printf("Chose %i\n", choicePool);
	if (choicePool < 12)
		return 's';
	if (choicePool < 14)
		return 'p';
	return 'p';
}

char rvMonsterScientist::Cheat(void) {
	switch (player->moveChoice) {
	case 'r':
		return 'p';
	case 'p':
		return 's';
	case 's':
		return 'r';
	}
}

idAI::Winner rvMonsterScientist::GetWinner(void) {
	if (player->moveChoice == 'n') {
		gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "You didn't pick anything. Monster wins.");
		return Monster;
	}
	switch (chosenMove) {
	case 'r':
		switch (player->moveChoice) {
		case 'r':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Rock = Rock. It's a draw!");
			player->moveChoice = 'n';
			return Draw;
			break;
		case 'p':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Paper > Rock. Player wins!");
			player->moveChoice = 'n';
			return Player;
			break;
		case 's':
			if (synced)
				return GetWinner();
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Scissors < Rock. Monster wins!");
			player->moveChoice = 'n';
			return Monster;
			break;
		}
		break;
	case 'p':
		switch (player->moveChoice) {
		case 'r':
			if (synced)
				return GetWinner();
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Rock < Paper. Monster wins!");
			player->moveChoice = 'n';
			return Monster;
			break;
		case 'p':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Paper = Paper. It's a draw!");
			player->moveChoice = 'n';
			return Draw;
			break;
		case 's':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Scissors > Paper. Player wins!");
			player->moveChoice = 'n';
			return Player;
			break;
		}
		break;
	case 's':
		switch (player->moveChoice) {
		case 'r':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Rock > Scissors. Player wins!");
			player->moveChoice = 'n';
			return Player;
			break;
		case 'p':
			if (synced)
				return GetWinner();
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Paper < Scissors. Monster wins!");
			player->moveChoice = 'n';
			return Monster;
			break;
		case 's':
			gameLocal.GetLocalPlayer()->hud->SetStateString("outcomeStatus", "Rock = Rock. It's a draw!");
			player->moveChoice = 'n';
			return Draw;
			break;
		}
		break;
	}
}

void rvMonsterScientist::ForceKill(void) {
	player->inMatch = false;
	player->opponent = idPlayer::OP_NONE;
	AdjustHealthByDamage(health);
	Killed(player, player, health, vec3_zero, INVALID_JOINT);
	Gib(vec3_zero, "damage_hyperblaster");
	gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "");
}

void rvMonsterScientist::LootDrop(void) {
	idRandom2 rand;
	rand.SetSeed(gameLocal.time % 69420);
	int loot = rand.RandomInt(10);
	if (player->guaranteedLoot)
		loot %= 5;
	if (loot < 1) { //rare drop, 10% chance
		player->AddPowerup(4);
		player->AddPowerup(4);
	}
	else if (loot < 5) //common drop, 40% chance
		player->AddPowerup(2);
}
// David End //