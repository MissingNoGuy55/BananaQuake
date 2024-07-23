/*
==============================================================================

BRUSH ENTITIES

==============================================================================
*/

void() func_monsterclip =
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	setmodel(self, self.model);
};

void() func_tracktrain_find =

{
	local entity	targ;

	targ = find (world, targetname, self.target);
	self.target = targ.target;
	
	setorigin (self, targ.origin);
	adjusttrain(self, self.target);
	if (!self.targetname)
	{	// not triggered, so start immediately
		self.nextthink = self.ltime + 0.1;
		self.think = train_next;
	}
};

/*QUAKED func_tracktrain (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal

.float height;

*/
void() func_tracktrain =
{	
	if (!self.speed)
		self.speed = 100;
	if (!self.target)
		objerror ("func_tracktrain without a target");
	if (!self.dmg)
		self.dmg = 2;

	if (self.sounds == 0)
	{
		self.noise = ("misc/null.wav");
		precache_sound ("misc/null.wav");
		self.noise1 = ("misc/null.wav");
		precache_sound ("misc/null.wav");
	}

	if (self.sounds == 1)
	{
		self.noise = ("plats/train2.wav");
		precache_sound ("plats/train2.wav");
		self.noise1 = ("plats/train1.wav");
		precache_sound ("plats/train1.wav");
	}

	self.cnt = 1;
	self.solid = SOLID_BSP;
	self.movetype = MOVETYPE_PUSH;
	self.blocked = train_blocked;
	self.use = train_use;
	self.classname = "train";

	setmodel (self, self.model);
	setsize (self, self.mins , self.maxs);
	setorigin (self, self.origin);

// start trains on the second frame, to make sure their targets have had
// a chance to spawn
	self.nextthink = self.ltime + 0.1;
	self.think = func_tracktrain_find;
};

void() func_ladder =
{
	self.solid = SOLID_BSP;
	self.movetype = MOVETYPE_PUSH;
	
	precache_sound("player/pl_ladder1.wav");
	precache_sound("player/pl_ladder2.wav");
	precache_sound("player/pl_ladder3.wav");
	precache_sound("player/pl_ladder4.wav");
	
	setmodel(self, self.model);
	setcontents(self, CONTENT_LADDER);
};

void() func_rotating =
{

	self.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	
	setmodel(self, self.model);
	setorigin(self, self.origin);

};

void() func_water =
{	
	if (!self.t_length)
		self.t_length = 80;
	if (!self.t_width)
		self.t_width = 10;

	if (self.sounds == 0)
		self.sounds = 2;
// FIX THIS TO LOAD A GENERIC PLAT SOUND

	if (self.sounds == 1)
	{
		precache_sound ("plats/plat1.wav");
		precache_sound ("plats/plat2.wav");
		self.noise = "plats/plat1.wav";
		self.noise1 = "plats/plat2.wav";
	}

	if (self.sounds == 2)
	{
		precache_sound ("plats/medplat1.wav");
		precache_sound ("plats/medplat2.wav");
		self.noise = "plats/medplat1.wav";
		self.noise1 = "plats/medplat2.wav";
	}


	self.mangle = self.angles;
	self.angles = '0 0 0';

	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	setorigin (self, self.origin);	
	setmodel (self, self.model);
	setsize (self, self.mins , self.maxs);

	self.blocked = plat_crush;
	if (!self.speed)
		self.speed = 150;

// pos1 is the top position, pos2 is the bottom
	self.pos1 = self.origin;
	self.pos2 = self.origin;
	if (self.height)
		self.pos2_z = self.origin_z - self.height;
	else
		self.pos2_z = self.origin_z - self.size_z + 8;

	self.use = plat_trigger_use;

	// plat_spawn_inside_trigger ();	// the "start moving" trigger	

	if (self.targetname)
	{
		self.state = STATE_UP;
		self.use = plat_use;
	}
	else
	{
		setorigin (self, self.pos2);
		self.state = STATE_BOTTOM;
	}
	
	setmodel(self, self.model);
};

void() func_pushable =
{
	self.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	
	setmodel(self, self.model);
};

.float charge_amount_health;
.float charge_replenish_health_time;
.float charge_replenish_health_deny_time;
.float charge_replenish_health_per_tick_time;
.float charge_sound1_time_health;
.float charge_sound2_time_health;

.float charge_amount_armor;
.float charge_replenish_armor_time;
.float charge_replenish_armor_deny_time;
.float charge_replenish_armor_per_tick_time;
.float charge_sound1_time;
.float charge_sound2_time;

void() healthcharger_think =
{
	if ((time > self.charge_replenish_health_time) && self.charge_amount_health <= 0)
	{
		self.state = STATE_UP;
		self.charge_amount_health = 100;
		sound(self, CHAN_VOICE, "items/medshot5.wav", 1, ATTN_NORM);
	}
	
	self.think = healthcharger_think;
	self.nextthink = time + 0.1;
};

void() healthcharger_use =
{
	if (time > self.charge_replenish_health_per_tick_time)
	{
		if (self.charge_amount_health > 0)
		{
			if (time > self.charge_sound2_time_health)
			{
				sound(self, CHAN_VOICE, "items/medcharge4.wav", 1, ATTN_NORM);
				self.charge_sound2_time_health = time + 1;
			}
			if (time > self.charge_sound1_time_health)
			{
				sound(self, CHAN_VOICE, "items/medshot4.wav", 1, ATTN_NORM);
				self.charge_sound1_time_health = time + 999999;
				self.charge_sound2_time_health = time + 0.8;
			}
		}
		
		if (self.charge_amount_health <= 0)
		{
			self.state = STATE_BOTTOM;
			if (time > self.charge_replenish_health_deny_time)
			{
				sound(self, CHAN_VOICE, "items/medshotno1.wav", 1, ATTN_NORM);
				self.charge_replenish_health_deny_time = time + 2;
				self.frame = 1;
				self.charge_replenish_health_time = time + 30;
			}
			return;
		}
		if (activator.health >= 100)
		{
			if (time > self.charge_replenish_health_deny_time)
			{
				sound(self, CHAN_VOICE, "items/medshotno1.wav", 1, ATTN_NORM);
				self.charge_replenish_health_deny_time = time + 2;
			}
			return;
		}
			
		activator.health = activator.health + 1;
		self.charge_amount_health = self.charge_amount_health - 1;
		self.charge_replenish_health_per_tick_time = time + 0.1;
	}
};

void() func_healthcharger =
{

	precache_sound("items/medcharge4.wav");
	precache_sound("items/medshot4.wav");
	precache_sound("items/medshot5.wav");
	precache_sound("items/medshotno1.wav");
	precache_sound("common/null.wav");

	self.movetype = MOVETYPE_PUSH;
	self.use = healthcharger_use;
	self.solid = SOLID_BSP;
	self.charge_amount_health = 100;
	
	self.think = healthcharger_think;
	self.nextthink = time + 0.1;
	
	setmodel(self, self.model);

};

void() suitcharger_think =
{
	if (time > self.charge_replenish_armor_time && self.charge_amount_armor <= 0)
	{
		self.state = STATE_UP;
		self.charge_amount_armor = 50;
		sound(self, CHAN_VOICE, "items/suitchargeok1.wav", 1, ATTN_NORM);
		self.charge_replenish_armor_time = 999999;
		self.frame = 0;
	}
	
	self.think = suitcharger_think;
	self.nextthink = time + 0.1;
	
	if (!activator)
	{
		self.charge_sound1_time = 0;
		self.charge_sound2_time = 0;
		
		sound(self, CHAN_AUTO, "common/null.wav", 1, ATTN_NORM);
	}
};

void() suitcharger_use =
{
	if (time > self.charge_replenish_armor_per_tick_time)
	{
		if (self.charge_amount_armor > 0 && activator.armorvalue < 100)
		{
			if (time > self.charge_sound2_time)
			{
				sound(self, CHAN_VOICE, "items/suitcharge1.wav", 1, ATTN_NORM);
				self.charge_sound2_time = time + 1;
			}
			if (time > self.charge_sound1_time)
			{
				sound(self, CHAN_VOICE, "items/suitchargeok1.wav", 1, ATTN_NORM);
				self.charge_sound1_time = time + 999999;
				self.charge_sound2_time = time + 0.8;
			}
		}
		
		if (self.charge_amount_armor <= 0)
		{
			self.state = STATE_BOTTOM;
			if (time > self.charge_replenish_armor_deny_time)
			{
				sound(self, CHAN_VOICE, "items/suitchargeno1.wav", 1, ATTN_NORM);
				self.charge_replenish_armor_deny_time = time + 2;
				self.frame = 1;
				self.charge_replenish_armor_time = time + 30;
			}
			return;
		}
		if (activator.armorvalue >= 100)
		{
			if (time > self.charge_replenish_armor_deny_time)
			{
				sound(self, CHAN_VOICE, "items/suitchargeno1.wav", 1, ATTN_NORM);
				self.charge_replenish_armor_deny_time = time + 2;
			}
			return;
		}
			
		activator.items = activator.items | IT_ARMOR1;
		activator.armorvalue = activator.armorvalue + 1;
		self.charge_amount_armor = self.charge_amount_armor - 1;
		self.charge_replenish_armor_per_tick_time = time + 0.1;
	}
};

void() func_recharge =
{

	precache_sound("items/suitcharge1.wav");
	precache_sound("items/suitchargeno1.wav");
	precache_sound("items/suitchargeok1.wav");
	precache_sound("common/null.wav");

	self.movetype = MOVETYPE_PUSH;
	self.use = suitcharger_use;
	self.solid = SOLID_BSP;
	
	setmodel(self, self.model);
	
	self.charge_sound2_time = 999999;
	self.charge_replenish_armor_time = 999999;
	
	self.charge_amount_armor = 50;
	self.think = suitcharger_think;
	self.nextthink = time + 0.1;

};

void() breakable_use =
{

};

void() func_breakable =
{

	self.movetype = MOVETYPE_PUSH;
	self.use = breakable_use;
	self.solid = SOLID_BBOX;

};

/*
==============================================================================

POINT ENTITIES

==============================================================================
*/

void() env_glow =
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
};

.string netnamed;

void() path_track = 
{
	self.model = string_null;
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	
	if (!self.speed)
		self.speed = 100;
};

.float playing;

void() ambient_generic_use =
{
	if (self.playing == 0)
	{
		ambientsound (self.origin, self.message, self.health, ATTN_STATIC);
		self.playing = 1;
	}
	else
	{
		ambientsound (self.origin, "common/null.wav", self.health, ATTN_STATIC);
		self.playing = 0;
	}
	return;
};

void() ambient_generic =
{
	precache_sound (self.message);
	
	self.playing = 0;
	
	if (!(self.spawnflags & 16))
	{
		ambientsound (self.origin, self.message, self.health, ATTN_STATIC);
		self.playing = 1;
	}
	
	self.use = ambient_generic_use;
};

.entity nextnode;

void() info_node =
{
	self.model = string_null;
	self.nextnode = findradius(self.origin, 512.0);
};

void() info_target =
{
	self.model = string_null;
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
};

void() light_spot =
{
	if (!self.targetname)
	{	// inert light
		remove(self);
		return;
	}

	if (self.style >= 32)
	{
		self.use = light_use;
		if (self.spawnflags & START_OFF)
			lightstyle(self.style, "a");
		else
			lightstyle(self.style, "m");
	}
};

.string m_iszEntity, m_iszIdle, m_iszEntry, m_iszPlay, m_iszPostIdle, m_iszCustomMove, m_iszNextScript;
.float m_flRadius, m_flRepeat, onplayerdeath;

void() scripted_sequence_think;

void() scripted_sequence_use =
{
	self.think = scripted_sequence_think;
	self.nextthink = self.ltime + 0.1;
};

void() scripted_sequence =
{	
	self.use = scripted_sequence_use;
};

void() scripted_sequence_think =
{
	local entity test;
	
	test = findradius(self.origin, 512.0);
	
	if (test.targetname == self.m_iszEntity)
	{
		bprint("Scripted sequence found entity\n");
		return;
	}
	
	self.nextthink = self.ltime + 0.1;
	self.think = scripted_sequence_think;
};

void() multi_manager_cycle =
{
	multimanager(self);
	self.master_triggered = 1;
};

void() multi_manager =
{
	self.use = multi_manager_cycle;
};

void() multisource_use =
{
	self.master_triggered = 1;
};

void() multisource =
{
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	self.model = string_null;
	
	self.use = multisource_use;
};

.float triggerstate;

void() trigger_auto =
{
	self.use = SUB_UseTargets;
	self.triggerstate = self.triggerstate;
	
	bprint("Triggerstate is: ");
	bprint(ftos(self.triggerstate));
};

.string sentence;
.string entity;

void() scripted_sentence_talk =
{
	speak_sentence(self, self.sentence);
};

void() scripted_sentence =
{
	self.sentence = self.sentence;
	
	precache_sentence(self.sentence);
	
	self.use = scripted_sentence_talk;
	self.entity = self.entity;
};

/*
==============================================================================

ITEMS/AMMO

==============================================================================
*/

void() item_healthkit =
{
	item_health();
	droptofloor();
};

void() weapon_shotgun =
{
	self.classname = "weapon_supershotgun";

	weapon_supershotgun();
};

void() weapon_rpg =
{
	self.classname = "weapon_rocketlauncher";
	
	weapon_rocketlauncher();
};

void() weapon_9mmAR =
{
	self.classname = "weapon_nailgun";

	weapon_nailgun();
};

void() weapon_gauss =
{
	self.classname = "weapon_lightning";

	weapon_lightning();
};

void() weapon_handgrenade =
{
	self.classname = "weapon_grenadelauncher";
	weapon_grenadelauncher();
};

void() weapon_tripmine =
{
	item_rockets();
	droptofloor();
};

void() ammo_gaussclip =
{
	item_cells();
	droptofloor();
};

void() ammo_rpgclip =
{
	item_rockets();
	droptofloor();
};

void() ammo_buckshot =
{
	item_shells();
	droptofloor();
};

void() ammo_9mmAR =
{
	item_spikes();
	droptofloor();
};

void() ammo_ARgrenades =
{
	item_rockets();
	droptofloor();
};
