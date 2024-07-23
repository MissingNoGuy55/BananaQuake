/*
==============================================================================
SCIENTIST
==============================================================================
*/

$cd /raid/quake/id1/models/soldier3
$origin 0 -6 24
$base base
$skin skin

$frame stand1 stand2 stand3 stand4 stand5 stand6 stand7 stand8

$frame death1 death2 death3 death4 death5 death6 death7 death8
$frame death9 death10

$frame deathc1 deathc2 deathc3 deathc4 deathc5 deathc6 deathc7 deathc8
$frame deathc9 deathc10 deathc11

$frame load1 load2 load3 load4 load5 load6 load7 load8 load9 load10 load11

$frame pain1 pain2 pain3 pain4 pain5 pain6

$frame painb1 painb2 painb3 painb4 painb5 painb6 painb7 painb8 painb9 painb10
$frame painb11 painb12 painb13 painb14

$frame painc1 painc2 painc3 painc4 painc5 painc6 painc7 painc8 painc9 painc10
$frame painc11 painc12 painc13

$frame run1 run2 run3 run4 run5 run6 run7 run8

$frame shoot1 shoot2 shoot3 shoot4 shoot5 shoot6 shoot7 shoot8 shoot9

$frame prowl_1 prowl_2 prowl_3 prowl_4 prowl_5 prowl_6 prowl_7 prowl_8
$frame prowl_9 prowl_10 prowl_11 prowl_12 prowl_13 prowl_14 prowl_15 prowl_16
$frame prowl_17 prowl_18 prowl_19 prowl_20 prowl_21 prowl_22 prowl_23 prowl_24

void()	scientist_stand1	=[	$stand1,	scientist_stand2	] {ai_stand();};
void()	scientist_stand2	=[	$stand2,	scientist_stand3	] {ai_stand();};
void()	scientist_stand3	=[	$stand3,	scientist_stand4	] {ai_stand();};
void()	scientist_stand4	=[	$stand4,	scientist_stand5	] {ai_stand();};
void()	scientist_stand5	=[	$stand5,	scientist_stand6	] {ai_stand();};
void()	scientist_stand6	=[	$stand6,	scientist_stand7	] {ai_stand();};
void()	scientist_stand7	=[	$stand7,	scientist_stand8	] {ai_stand();};
void()	scientist_stand8	=[	$stand8,	scientist_stand1	] {ai_stand();};

void()	scientist_walk1	=[	$prowl_1,	scientist_walk2	] {ai_walk(1);};
void()	scientist_walk2	=[	$prowl_2,	scientist_walk3	] {ai_walk(1);};
void()	scientist_walk3	=[	$prowl_3,	scientist_walk4	] {ai_walk(1);};
void()	scientist_walk4	=[	$prowl_4,	scientist_walk5	] {ai_walk(1);};
void()	scientist_walk5	=[	$prowl_5,	scientist_walk6	] {ai_walk(2);};
void()	scientist_walk6	=[	$prowl_6,	scientist_walk7	] {ai_walk(3);};
void()	scientist_walk7	=[	$prowl_7,	scientist_walk8	] {ai_walk(4);};
void()	scientist_walk8	=[	$prowl_8,	scientist_walk9	] {ai_walk(4);};
void()	scientist_walk9	=[	$prowl_9,	scientist_walk10	] {ai_walk(2);};
void()	scientist_walk10	=[	$prowl_10,	scientist_walk11	] {ai_walk(2);};
void()	scientist_walk11	=[	$prowl_11,	scientist_walk12	] {ai_walk(2);};
void()	scientist_walk12	=[	$prowl_12,	scientist_walk13	] {ai_walk(1);};
void()	scientist_walk13	=[	$prowl_13,	scientist_walk14	] {ai_walk(0);};
void()	scientist_walk14	=[	$prowl_14,	scientist_walk15	] {ai_walk(1);};
void()	scientist_walk15	=[	$prowl_15,	scientist_walk16	] {ai_walk(1);};
void()	scientist_walk16	=[	$prowl_16,	scientist_walk17	] {ai_walk(1);};
void()	scientist_walk17	=[	$prowl_17,	scientist_walk18	] {ai_walk(3);};
void()	scientist_walk18	=[	$prowl_18,	scientist_walk19	] {ai_walk(3);};
void()	scientist_walk19	=[	$prowl_19,	scientist_walk20	] {ai_walk(3);};
void()	scientist_walk20	=[	$prowl_20,	scientist_walk21	] {ai_walk(3);};
void()	scientist_walk21	=[	$prowl_21,	scientist_walk22	] {ai_walk(2);};
void()	scientist_walk22	=[	$prowl_22,	scientist_walk23	] {ai_walk(1);};
void()	scientist_walk23	=[	$prowl_23,	scientist_walk24	] {ai_walk(1);};
void()	scientist_walk24	=[	$prowl_24,	scientist_walk1	] {ai_walk(1);};

void()	scientist_run1	=[	$run1,		scientist_run2	] {ai_run(11);};
void()	scientist_run2	=[	$run2,		scientist_run3	] {ai_run(15);};
void()	scientist_run3	=[	$run3,		scientist_run4	] {ai_run(10);};
void()	scientist_run4	=[	$run4,		scientist_run5	] {ai_run(10);};
void()	scientist_run5	=[	$run5,		scientist_run6	] {ai_run(8);};
void()	scientist_run6	=[	$run6,		scientist_run7	] {ai_run(15);};
void()	scientist_run7	=[	$run7,		scientist_run8	] {ai_run(10);};
void()	scientist_run8	=[	$run8,		scientist_run1	] {ai_run(8);};

void()	scientist_atk1	=[	$shoot1,	scientist_atk2	] {ai_face();};
void()	scientist_atk2	=[	$shoot2,	scientist_atk3	] {ai_face();};
void()	scientist_atk3	=[	$shoot3,	scientist_atk4	] {ai_face();};
void()	scientist_atk4	=[	$shoot4,	scientist_atk5	] {ai_face();};
void()	scientist_atk5	=[	$shoot5,	scientist_atk6	] {ai_face();};
void()	scientist_atk6	=[	$shoot6,	scientist_atk7	] {ai_face();};
void()	scientist_atk7	=[	$shoot7,	scientist_atk8	] {ai_face();};
void()	scientist_atk8	=[	$shoot8,	scientist_atk9	] {ai_face();};
void()	scientist_atk9	=[	$shoot9,	scientist_run1	] {ai_face();};


void()	scientist_pain1	=[	$pain1,		scientist_pain2	] {};
void()	scientist_pain2	=[	$pain2,		scientist_pain3	] {};
void()	scientist_pain3	=[	$pain3,		scientist_pain4	] {};
void()	scientist_pain4	=[	$pain4,		scientist_pain5	] {};
void()	scientist_pain5	=[	$pain5,		scientist_pain6	] {};
void()	scientist_pain6	=[	$pain6,		scientist_run1	] {ai_pain(1);};

void()	scientist_painb1	=[	$painb1,	scientist_painb2	] {};
void()	scientist_painb2	=[	$painb2,	scientist_painb3	] {ai_painforward(13);};
void()	scientist_painb3	=[	$painb3,	scientist_painb4	] {ai_painforward(9);};
void()	scientist_painb4	=[	$painb4,	scientist_painb5	] {};
void()	scientist_painb5	=[	$painb5,	scientist_painb6	] {};
void()	scientist_painb6	=[	$painb6,	scientist_painb7	] {};
void()	scientist_painb7	=[	$painb7,	scientist_painb8	] {};
void()	scientist_painb8	=[	$painb8,	scientist_painb9	] {};
void()	scientist_painb9	=[	$painb9,	scientist_painb10] {};
void()	scientist_painb10=[	$painb10,	scientist_painb11] {};
void()	scientist_painb11=[	$painb11,	scientist_painb12] {};
void()	scientist_painb12=[	$painb12,	scientist_painb13] {ai_pain(2);};
void()	scientist_painb13=[	$painb13,	scientist_painb14] {};
void()	scientist_painb14=[	$painb14,	scientist_run1	] {};

void()	scientist_painc1	=[	$painc1,	scientist_painc2	] {};
void()	scientist_painc2	=[	$painc2,	scientist_painc3	] {ai_pain(1);};
void()	scientist_painc3	=[	$painc3,	scientist_painc4	] {};
void()	scientist_painc4	=[	$painc4,	scientist_painc5	] {};
void()	scientist_painc5	=[	$painc5,	scientist_painc6	] {ai_painforward(1);};
void()	scientist_painc6	=[	$painc6,	scientist_painc7	] {ai_painforward(1);};
void()	scientist_painc7	=[	$painc7,	scientist_painc8	] {};
void()	scientist_painc8	=[	$painc8,	scientist_painc9	] {ai_pain(1);};
void()	scientist_painc9	=[	$painc9,	scientist_painc10] {ai_painforward(4);};
void()	scientist_painc10=[	$painc10,	scientist_painc11] {ai_painforward(3);};
void()	scientist_painc11=[	$painc11,	scientist_painc12] {ai_painforward(6);};
void()	scientist_painc12=[	$painc12,	scientist_painc13] {ai_painforward(8);};
void()	scientist_painc13=[	$painc13,	scientist_run1] {};

void(entity attacker, float damage)	scientist_pain =
{
	local float r;

	if (self.pain_finished > time)
		return;

	r = random();

	if (r < 0.1)
	{
		self.pain_finished = time + 0.6;
		scientist_pain1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM);
	}
	else if (r < 0.2)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM);
	}
	else if (r < 0.3)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM);
	}
	else if (r < 0.4)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM);
	}
	else if (r < 0.5)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM);
	}
	else if (r < 0.6)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain6.wav", 1, ATTN_NORM);
	}
	else if (r < 0.7)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain7.wav", 1, ATTN_NORM);
	}
	else if (r < 0.8)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain8.wav", 1, ATTN_NORM);
	}
	else if (r < 0.9)
	{
		self.pain_finished = time + 1.1;
		scientist_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain9.wav", 1, ATTN_NORM);
	}
	else
	{
		self.pain_finished = time + 1.1;
		scientist_painc1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain10.wav", 1, ATTN_NORM);
	}
};


void() scientist_fire =
{
	local	vector	dir;
	local	entity	en;

	ai_face();

	sound (self, CHAN_WEAPON, "soldier/sattck1.wav", 1, ATTN_NORM);

// fire somewhat behind the player, so a dodging player is harder to hit
	en = self.enemy;

	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	FireBullets (4, dir, '0.1 0.1 0');
};

void()	scientist_die1	=[	$death1,	scientist_die2	] {};
void()	scientist_die2	=[	$death2,	scientist_die3	] {};
void()	scientist_die3	=[	$death3,	scientist_die4	]
{self.solid = SOLID_NOT;self.ammo_shells = 5;DropBackpack();};
void()	scientist_die4	=[	$death4,	scientist_die5	] {};
void()	scientist_die5	=[	$death5,	scientist_die6	] {};
void()	scientist_die6	=[	$death6,	scientist_die7	] {};
void()	scientist_die7	=[	$death7,	scientist_die8	] {};
void()	scientist_die8	=[	$death8,	scientist_die9	] {};
void()	scientist_die9	=[	$death9,	scientist_die10	] {};
void()	scientist_die10	=[	$death10,	scientist_die10	] {};

void()	scientist_cdie1	=[	$deathc1,	scientist_cdie2	] {};
void()	scientist_cdie2	=[	$deathc2,	scientist_cdie3	] {ai_back(5);};
void()	scientist_cdie3	=[	$deathc3,	scientist_cdie4	]
{self.solid = SOLID_NOT;self.ammo_shells = 5;DropBackpack();ai_back(4);};
void()	scientist_cdie4	=[	$deathc4,	scientist_cdie5	] {ai_back(13);};
void()	scientist_cdie5	=[	$deathc5,	scientist_cdie6	] {ai_back(3);};
void()	scientist_cdie6	=[	$deathc6,	scientist_cdie7	] {ai_back(4);};
void()	scientist_cdie7	=[	$deathc7,	scientist_cdie8	] {};
void()	scientist_cdie8	=[	$deathc8,	scientist_cdie9	] {};
void()	scientist_cdie9	=[	$deathc9,	scientist_cdie10	] {};
void()	scientist_cdie10	=[	$deathc10,	scientist_cdie11	] {};
void()	scientist_cdie11	=[	$deathc11,	scientist_cdie11	] {};


void() scientist_die =
{
// check for gib
	if (self.health < -35)
	{
		sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		ThrowHead ("progs/h_guard.mdl", self.health);
		ThrowGib ("progs/gib1.mdl", self.health);
		ThrowGib ("progs/gib2.mdl", self.health);
		ThrowGib ("progs/gib3.mdl", self.health);
		return;
	}

// regular death
	sound (self, CHAN_VOICE, "scientist/nogrant.wav", 1, ATTN_NORM);
	if (random() < 0.5)
		scientist_die1 ();
	else
		scientist_cdie1 ();
};

void() monster_scientist =
{
	precache_model ("progs/soldier.mdl");
	precache_model ("progs/h_guard.mdl");
	precache_model ("progs/gib1.mdl");
	precache_model ("progs/gib2.mdl");
	precache_model ("progs/gib3.mdl");

	precache_sound ("soldier/death1.wav");
	precache_sound ("soldier/idle.wav");
	precache_sound ("scientist/sci_pain1.wav");
	precache_sound ("scientist/sci_pain2.wav");
	precache_sound ("scientist/sci_pain3.wav");
	precache_sound ("scientist/sci_pain4.wav");
	precache_sound ("scientist/sci_pain5.wav");
	precache_sound ("scientist/sci_pain6.wav");
	precache_sound ("scientist/sci_pain7.wav");
	precache_sound ("scientist/sci_pain8.wav");
	precache_sound ("scientist/sci_pain9.wav");
	precache_sound ("scientist/sci_pain10.wav");
	precache_sound ("scientist/asexpected.wav");
	precache_sound ("scientist/ahfreeman.wav");
	precache_sound ("scientist/nogrant.wav");

	precache_sound ("player/udeath.wav");		// gib death

	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_STEP;

	setmodel(self, "progs/soldier.mdl");

	setsize (self, '-16 -16 -24', '16 16 40');
	self.health = 30;

	self.th_stand = scientist_stand1;
	self.th_walk = scientist_walk1;
	self.th_run = scientist_run1;
	self.th_missile = scientist_atk1;
	self.th_pain = scientist_pain;
	self.th_die = scientist_die;

	walkmonster_start ();
};
