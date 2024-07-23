/*
==============================================================================
BARNEY
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

void()	barney_stand1	=[	$stand1,	barney_stand2	] {ai_stand();};
void()	barney_stand2	=[	$stand2,	barney_stand3	] {ai_stand();};
void()	barney_stand3	=[	$stand3,	barney_stand4	] {ai_stand();};
void()	barney_stand4	=[	$stand4,	barney_stand5	] {ai_stand();};
void()	barney_stand5	=[	$stand5,	barney_stand6	] {ai_stand();};
void()	barney_stand6	=[	$stand6,	barney_stand7	] {ai_stand();};
void()	barney_stand7	=[	$stand7,	barney_stand8	] {ai_stand();};
void()	barney_stand8	=[	$stand8,	barney_stand1	] {ai_stand();};

void()	barney_walk1	=[	$prowl_1,	barney_walk2	] {ai_walk(1);};
void()	barney_walk2	=[	$prowl_2,	barney_walk3	] {ai_walk(1);};
void()	barney_walk3	=[	$prowl_3,	barney_walk4	] {ai_walk(1);};
void()	barney_walk4	=[	$prowl_4,	barney_walk5	] {ai_walk(1);};
void()	barney_walk5	=[	$prowl_5,	barney_walk6	] {ai_walk(2);};
void()	barney_walk6	=[	$prowl_6,	barney_walk7	] {ai_walk(3);};
void()	barney_walk7	=[	$prowl_7,	barney_walk8	] {ai_walk(4);};
void()	barney_walk8	=[	$prowl_8,	barney_walk9	] {ai_walk(4);};
void()	barney_walk9	=[	$prowl_9,	barney_walk10	] {ai_walk(2);};
void()	barney_walk10	=[	$prowl_10,	barney_walk11	] {ai_walk(2);};
void()	barney_walk11	=[	$prowl_11,	barney_walk12	] {ai_walk(2);};
void()	barney_walk12	=[	$prowl_12,	barney_walk13	] {ai_walk(1);};
void()	barney_walk13	=[	$prowl_13,	barney_walk14	] {ai_walk(0);};
void()	barney_walk14	=[	$prowl_14,	barney_walk15	] {ai_walk(1);};
void()	barney_walk15	=[	$prowl_15,	barney_walk16	] {ai_walk(1);};
void()	barney_walk16	=[	$prowl_16,	barney_walk17	] {ai_walk(1);};
void()	barney_walk17	=[	$prowl_17,	barney_walk18	] {ai_walk(3);};
void()	barney_walk18	=[	$prowl_18,	barney_walk19	] {ai_walk(3);};
void()	barney_walk19	=[	$prowl_19,	barney_walk20	] {ai_walk(3);};
void()	barney_walk20	=[	$prowl_20,	barney_walk21	] {ai_walk(3);};
void()	barney_walk21	=[	$prowl_21,	barney_walk22	] {ai_walk(2);};
void()	barney_walk22	=[	$prowl_22,	barney_walk23	] {ai_walk(1);};
void()	barney_walk23	=[	$prowl_23,	barney_walk24	] {ai_walk(1);};
void()	barney_walk24	=[	$prowl_24,	barney_walk1	] {ai_walk(1);};

void()	barney_run1	=[	$run1,		barney_run2	] {ai_run(11);};
void()	barney_run2	=[	$run2,		barney_run3	] {ai_run(15);};
void()	barney_run3	=[	$run3,		barney_run4	] {ai_run(10);};
void()	barney_run4	=[	$run4,		barney_run5	] {ai_run(10);};
void()	barney_run5	=[	$run5,		barney_run6	] {ai_run(8);};
void()	barney_run6	=[	$run6,		barney_run7	] {ai_run(15);};
void()	barney_run7	=[	$run7,		barney_run8	] {ai_run(10);};
void()	barney_run8	=[	$run8,		barney_run1	] {ai_run(8);};

void()	barney_atk1	=[	$shoot1,	barney_atk2	] {ai_face();};
void()	barney_atk2	=[	$shoot2,	barney_atk3	] {ai_face();};
void()	barney_atk3	=[	$shoot3,	barney_atk4	] {ai_face();};
void()	barney_atk4	=[	$shoot4,	barney_atk5	] {ai_face();};
void()	barney_atk5	=[	$shoot5,	barney_atk6	] {ai_face();};
void()	barney_atk6	=[	$shoot6,	barney_atk7	] {ai_face();};
void()	barney_atk7	=[	$shoot7,	barney_atk8	] {ai_face();};
void()	barney_atk8	=[	$shoot8,	barney_atk9	] {ai_face();};
void()	barney_atk9	=[	$shoot9,	barney_run1	] {ai_face();};


void()	barney_pain1	=[	$pain1,		barney_pain2	] {};
void()	barney_pain2	=[	$pain2,		barney_pain3	] {};
void()	barney_pain3	=[	$pain3,		barney_pain4	] {};
void()	barney_pain4	=[	$pain4,		barney_pain5	] {};
void()	barney_pain5	=[	$pain5,		barney_pain6	] {};
void()	barney_pain6	=[	$pain6,		barney_run1	] {ai_pain(1);};

void()	barney_painb1	=[	$painb1,	barney_painb2	] {};
void()	barney_painb2	=[	$painb2,	barney_painb3	] {ai_painforward(13);};
void()	barney_painb3	=[	$painb3,	barney_painb4	] {ai_painforward(9);};
void()	barney_painb4	=[	$painb4,	barney_painb5	] {};
void()	barney_painb5	=[	$painb5,	barney_painb6	] {};
void()	barney_painb6	=[	$painb6,	barney_painb7	] {};
void()	barney_painb7	=[	$painb7,	barney_painb8	] {};
void()	barney_painb8	=[	$painb8,	barney_painb9	] {};
void()	barney_painb9	=[	$painb9,	barney_painb10] {};
void()	barney_painb10=[	$painb10,	barney_painb11] {};
void()	barney_painb11=[	$painb11,	barney_painb12] {};
void()	barney_painb12=[	$painb12,	barney_painb13] {ai_pain(2);};
void()	barney_painb13=[	$painb13,	barney_painb14] {};
void()	barney_painb14=[	$painb14,	barney_run1	] {};

void()	barney_painc1	=[	$painc1,	barney_painc2	] {};
void()	barney_painc2	=[	$painc2,	barney_painc3	] {ai_pain(1);};
void()	barney_painc3	=[	$painc3,	barney_painc4	] {};
void()	barney_painc4	=[	$painc4,	barney_painc5	] {};
void()	barney_painc5	=[	$painc5,	barney_painc6	] {ai_painforward(1);};
void()	barney_painc6	=[	$painc6,	barney_painc7	] {ai_painforward(1);};
void()	barney_painc7	=[	$painc7,	barney_painc8	] {};
void()	barney_painc8	=[	$painc8,	barney_painc9	] {ai_pain(1);};
void()	barney_painc9	=[	$painc9,	barney_painc10] {ai_painforward(4);};
void()	barney_painc10=[	$painc10,	barney_painc11] {ai_painforward(3);};
void()	barney_painc11=[	$painc11,	barney_painc12] {ai_painforward(6);};
void()	barney_painc12=[	$painc12,	barney_painc13] {ai_painforward(8);};
void()	barney_painc13=[	$painc13,	barney_run1] {};

void(entity attacker, float damage)	barney_pain =
{
	local float r;

	if (self.pain_finished > time)
		return;

	r = random();

	if (r < 0.1)
	{
		self.pain_finished = time + 0.6;
		barney_pain1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM);
	}
	else if (r < 0.2)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM);
	}
	else if (r < 0.3)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM);
	}
	else if (r < 0.4)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM);
	}
	else if (r < 0.5)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM);
	}
	else if (r < 0.6)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain6.wav", 1, ATTN_NORM);
	}
	else if (r < 0.7)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain7.wav", 1, ATTN_NORM);
	}
	else if (r < 0.8)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain8.wav", 1, ATTN_NORM);
	}
	else if (r < 0.9)
	{
		self.pain_finished = time + 1.1;
		barney_painb1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain9.wav", 1, ATTN_NORM);
	}
	else
	{
		self.pain_finished = time + 1.1;
		barney_painc1 ();
		sound (self, CHAN_VOICE, "scientist/sci_pain10.wav", 1, ATTN_NORM);
	}
};


void() barney_fire =
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

void()	barney_die1	=[	$death1,	barney_die2	] {};
void()	barney_die2	=[	$death2,	barney_die3	] {};
void()	barney_die3	=[	$death3,	barney_die4	]
{self.solid = SOLID_NOT;self.ammo_shells = 5;DropBackpack();};
void()	barney_die4	=[	$death4,	barney_die5	] {};
void()	barney_die5	=[	$death5,	barney_die6	] {};
void()	barney_die6	=[	$death6,	barney_die7	] {};
void()	barney_die7	=[	$death7,	barney_die8	] {};
void()	barney_die8	=[	$death8,	barney_die9	] {};
void()	barney_die9	=[	$death9,	barney_die10	] {};
void()	barney_die10	=[	$death10,	barney_die10	] {};

void()	barney_cdie1	=[	$deathc1,	barney_cdie2	] {};
void()	barney_cdie2	=[	$deathc2,	barney_cdie3	] {ai_back(5);};
void()	barney_cdie3	=[	$deathc3,	barney_cdie4	]
{self.solid = SOLID_NOT;self.ammo_shells = 5;DropBackpack();ai_back(4);};
void()	barney_cdie4	=[	$deathc4,	barney_cdie5	] {ai_back(13);};
void()	barney_cdie5	=[	$deathc5,	barney_cdie6	] {ai_back(3);};
void()	barney_cdie6	=[	$deathc6,	barney_cdie7	] {ai_back(4);};
void()	barney_cdie7	=[	$deathc7,	barney_cdie8	] {};
void()	barney_cdie8	=[	$deathc8,	barney_cdie9	] {};
void()	barney_cdie9	=[	$deathc9,	barney_cdie10	] {};
void()	barney_cdie10	=[	$deathc10,	barney_cdie11	] {};
void()	barney_cdie11	=[	$deathc11,	barney_cdie11	] {};


void() barney_die =
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
		barney_die1 ();
	else
		barney_cdie1 ();
};

void() monster_barney =
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

	self.th_stand = barney_stand1;
	self.th_walk = barney_walk1;
	self.th_run = barney_run1;
	self.th_missile = barney_atk1;
	self.th_pain = barney_pain;
	self.th_die = barney_die;

	walkmonster_start ();
};
