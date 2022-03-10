
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterGrunt : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterGrunt );

	rvMonsterGrunt ( void );
	
	void				Spawn					( void );
	void				Save					( idSaveGame *savefile ) const;
	void				Restore					( idRestoreGame *savefile );
	
	virtual void		AdjustHealthByDamage	( int damage );

	//David Begin//
	virtual void		Think					(void);
	virtual char		GetMove					(void);
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

protected:

	rvAIAction			actionMeleeMoveAttack;
	rvAIAction			actionChaingunAttack;

	virtual bool		CheckActions		( void );

	virtual void		OnTacticalChange	( aiTactical_t oldTactical );
	virtual void		OnDeath				( void );

private:

	int					standingMeleeNoAttackTime;
	int					rageThreshold;

	void				RageStart			( void );
	void				RageStop			( void );
	
	// Torso States
	stateResult_t		State_Torso_Enrage		( const stateParms_t& parms );
	stateResult_t		State_Torso_Pain		( const stateParms_t& parms );
	stateResult_t		State_Torso_LeapAttack	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterGrunt );
};

CLASS_DECLARATION( idAI, rvMonsterGrunt )
END_CLASS

/*
================
rvMonsterGrunt::rvMonsterGrunt
================
*/
rvMonsterGrunt::rvMonsterGrunt ( void ) {
	standingMeleeNoAttackTime = 0;
	//David begin//
	player = gameLocal.GetLocalPlayer();
	if(player)
		player->inMatch = true;
	// David end //
}

/*
================
rvMonsterGrunt::Spawn
================
*/
void rvMonsterGrunt::Spawn ( void ) {
	rageThreshold = spawnArgs.GetInt ( "health_rageThreshold" );

	// Custom actions
	actionMeleeMoveAttack.Init	( spawnArgs, "action_meleeMoveAttack",	NULL,				AIACTIONF_ATTACK );
	actionChaingunAttack.Init	( spawnArgs, "action_chaingunAttack",	NULL,				AIACTIONF_ATTACK );
	actionLeapAttack.Init		( spawnArgs, "action_leapAttack",		"Torso_LeapAttack",	AIACTIONF_ATTACK );

	// Enraged to start?
	if ( spawnArgs.GetBool ( "preinject" ) ) {
		RageStart ( );
	}	

	//David begin//
	
	// David end //
}

/*
================
rvMonsterGrunt::Save
================
*/
void rvMonsterGrunt::Save ( idSaveGame *savefile ) const {
	actionMeleeMoveAttack.Save( savefile );
	actionChaingunAttack.Save( savefile );

	savefile->WriteInt( rageThreshold );
	savefile->WriteInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterGrunt::Restore
================
*/
void rvMonsterGrunt::Restore ( idRestoreGame *savefile ) {
	actionMeleeMoveAttack.Restore( savefile );
	actionChaingunAttack.Restore( savefile );

	savefile->ReadInt( rageThreshold );
	savefile->ReadInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterGrunt::RageStart
================
*/
void rvMonsterGrunt::RageStart ( void ) {
	SetShaderParm ( 6, 1 );

	// Disable non-rage actions
	actionEvadeLeft.fl.disabled = true;
	actionEvadeRight.fl.disabled = true;
	
	// Speed up animations
	animator.SetPlaybackRate ( 1.25f );

	// Disable pain
	pain.threshold = 0;

	// Start over with health when enraged
	health = spawnArgs.GetInt ( "health" );
	
	// No more going to rage
	rageThreshold = 0;
}

/*
================
rvMonsterGrunt::RageStop
================
*/
void rvMonsterGrunt::RageStop ( void ) {
	SetShaderParm ( 6, 0 );
}

/*
================
rvMonsterGrunt::CheckActions
================
*/
bool rvMonsterGrunt::CheckActions ( void ) {
	// If our health is below the rage threshold then enrage
	if ( health < rageThreshold ) { 
		PerformAction ( "Torso_Enrage", 4, true );
		return true;
	}

	// Moving melee attack?
	if ( PerformAction ( &actionMeleeMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL ) ) {
		return true;
	}
	
	// Default actions
	if ( CheckPainActions ( ) ) {
		return true;
	}

	if ( PerformAction ( &actionEvadeLeft,   (checkAction_t)&idAI::CheckAction_EvadeLeft, &actionTimerEvade )			 ||
			PerformAction ( &actionEvadeRight,  (checkAction_t)&idAI::CheckAction_EvadeRight, &actionTimerEvade )			 ||
			PerformAction ( &actionJumpBack,	 (checkAction_t)&idAI::CheckAction_JumpBack, &actionTimerEvade )			 ||
			PerformAction ( &actionLeapAttack,  (checkAction_t)&idAI::CheckAction_LeapAttack )	) {
		return true;
	} else if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack ) ) {
		standingMeleeNoAttackTime = 0;
		return true;
	} else {
		if ( actionMeleeAttack.status != rvAIAction::STATUS_FAIL_TIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_EXTERNALTIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_CHANCE )
		{//melee attack fail for any reason other than timer?
			if ( combat.tacticalCurrent == AITACTICAL_MELEE && !move.fl.moving )
			{//special case: we're in tactical melee and we're close enough to think we've reached the enemy, but he's just out of melee range!
				if ( !standingMeleeNoAttackTime )
				{
					standingMeleeNoAttackTime = gameLocal.GetTime();
				}
				else if ( standingMeleeNoAttackTime + 2500 < gameLocal.GetTime() )
				{//we've been standing still and not attacking for at least 2.5 seconds, fall back to ranged attack
					//allow ranged attack
					actionRangedAttack.fl.disabled = false;
				}
			}
		}
		if ( PerformAction ( &actionRangedAttack,(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	}
	return false;
}

/*
================
rvMonsterGrunt::OnDeath
================
*/
void rvMonsterGrunt::OnDeath ( void ) {
	RageStop ( );
	return idAI::OnDeath ( );
}

/*
================
rvMonsterGrunt::OnTacticalChange

Enable/Disable the ranged attack based on whether the grunt needs it
================
*/
void rvMonsterGrunt::OnTacticalChange ( aiTactical_t oldTactical ) {
	switch ( combat.tacticalCurrent ) {
		case AITACTICAL_MELEE:
			actionRangedAttack.fl.disabled = true;
			break;

		default:
			actionRangedAttack.fl.disabled = false;
			break;
	}
}

/*
=====================
rvMonsterGrunt::AdjustHealthByDamage
=====================
*/
void rvMonsterGrunt::AdjustHealthByDamage ( int damage ) {
	// Take less damage during enrage process 
	if ( rageThreshold && health < rageThreshold ) { 
		health -= (damage * 0.25f);
		return;
	}
	return idAI::AdjustHealthByDamage ( damage );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterGrunt )
	STATE ( "Torso_Enrage",		rvMonsterGrunt::State_Torso_Enrage )
	STATE ( "Torso_Pain",		rvMonsterGrunt::State_Torso_Pain )
	STATE ( "Torso_LeapAttack",	rvMonsterGrunt::State_Torso_LeapAttack )
END_CLASS_STATES

/*
================
rvMonsterGrunt::State_Torso_Pain
================
*/
stateResult_t rvMonsterGrunt::State_Torso_Pain ( const stateParms_t& parms ) {
	// Stop streaming pain if its time to get angry
	if ( pain.loopEndTime && health < rageThreshold ) {
		pain.loopEndTime = 0;
	}
	return idAI::State_Torso_Pain ( parms );
}

/*
================
rvMonsterGrunt::State_Torso_Enrage
================
*/
stateResult_t rvMonsterGrunt::State_Torso_Enrage ( const stateParms_t& parms ) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ANIM:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "anger", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_ANIM_WAIT );
		
		case STAGE_ANIM_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				RageStart ( );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterGrunt::State_Torso_LeapAttack
================
*/
stateResult_t rvMonsterGrunt::State_Torso_LeapAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ANIM:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			lastAttackTime = 0;
			// Play the action animation
			PlayAnim ( ANIMCHANNEL_TORSO, animator.GetAnim ( actionAnimNum )->FullName ( ), parms.blendFrames );
			return SRESULT_STAGE ( STAGE_ANIM_WAIT );
		
		case STAGE_ANIM_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				// If we missed our leap attack get angry
				if ( !lastAttackTime && rageThreshold ) {
					PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Enrage", parms.blendFrames );
				}
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

//David Begin//

void rvMonsterGrunt::Think(void) {
	if(!CheckDormant()){
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
			gameLocal.Printf("Grunt picked %s!\n", (chosenMove == 'r') ? "Rock" : ((chosenMove == 'p') ? "Paper" : "Scissors"));
		}
	}
	if (gameLocal.time >= roundStartTime + 3333 && chosenMove != 'n') {
		rvMonsterGrunt::Winner winner = GetWinner();
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

char rvMonsterGrunt::GetMove(void) {
	idRandom2 rand;
	rand.SetSeed(gameLocal.time % 69420);
	int choicePool = rand.RandomInt(10);
	//gameLocal.Printf("Chose %i\n", choicePool);
	if (choicePool < 7)
		return 'r';
	return 'p';
}

idAI::Winner rvMonsterGrunt::GetWinner(void) {
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

void rvMonsterGrunt::ForceKill(void) {
	player->inMatch = false;
	player->opponent = idPlayer::OP_NONE;
	AdjustHealthByDamage(health);
	Killed(player, player, health, vec3_zero, INVALID_JOINT);
	Gib(vec3_zero, "damage_hyperblaster");
	gameLocal.GetLocalPlayer()->hud->SetStateString("countdownStatus", "");
}

void rvMonsterGrunt::LootDrop(void) {
	idRandom2 rand;
	rand.SetSeed(gameLocal.time % 69420);
	int loot = rand.RandomInt(10);
	if (player->guaranteedLoot)
		loot %= 5;
	if (loot < 1) { //rare drop, 10% chance
		player->AddPowerup(1);
		player->AddPowerup(1);
	}
	else if (loot < 5) //common drop, 40% chance
		player->AddPowerup(2);
}
// David End //